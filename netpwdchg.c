#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <shadow.h>
#include <sys/types.h>
#include <pwd.h>
#define _XOPEN_SOURCE
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#define __USE_XOPEN
#include <arpa/inet.h>
#include <time.h>
#include <syslog.h>
#include <signal.h>
#include <sys/stat.h>
#include <math.h>
#include <crypt.h>
#include <ctype.h>


#define PW_MAX_LEN              14
#define USER_NOT_FOUND          1
#define PASSWD_NOT_MATCH        2
#define PWD_FILE                "/etc/shadow"
#define OLD_PWD_FILE            "/etc/shadow~"
#define CONFIG_FILE             "netpwdchg.conf"
#define DEFAULT_USER            "nobody"
#define DEFAULT_GROUP           "nobody"
#define AUTH_USER_FILE          "authuser"
#define DEF_SERVER_IP           "127.0.0.1"
#define CONF_PATH               "."
#define MAXLINE                 3000
#define DEF_SERVER_PORT         "3000"
#define LISTENQ                 5
#define PROGRAM_NAME            "netpwdchg"
#define CONF_STRING_LEN         100

static int srand_called = 0;
uid_t initial_r_uid; /* real uid */
uid_t initial_e_uid; /* effective uid */
gid_t initial_r_gid;
gid_t initial_e_gid;

/********************************************************/
 /* BEGIN configuration file processing method */
/********************************************************/

struct conf {
    char ListenIP[CONF_STRING_LEN];
    char ListenPort[CONF_STRING_LEN];
    char group[CONF_STRING_LEN];
    char user[CONF_STRING_LEN];
    char groupThatCanConnect[CONF_STRING_LEN];
} conf;

char **userAllowed; //data structure as an array with the latest item at zero
int maxUserAllowed = 0;

/* initialize data to default values */
void init_parameters (struct conf * parms) {
  strncpy (parms->ListenIP, DEF_SERVER_IP, CONF_STRING_LEN);
  strncpy (parms->ListenPort, DEF_SERVER_PORT, CONF_STRING_LEN);
}

/* trim: get rid of trailing and leading whitespace */
char *trim (char * s) {
  /* Initialize start, end pointers */
  char *s1 = s, *s2 = &s[strlen (s) - 1];
  /* Trim and delimit right side */
  while ( (isspace (*s2)) && (s2 >= s1) )
    s2--;
  *(s2+1) = '\0';
  /* Trim left side */
  while ( (isspace (*s1)) && (s1 < s2) )
    s1++;
  /* Copy finished string */
  strcpy (s, s1);
  return s;
}

/* parse external parameters file */
void parse_config (struct conf * parms) {
  char *s, buff[256];
  FILE *fp = fopen (CONFIG_FILE, "r");
  if (fp == NULL) { return; }

  /* Read next line */
  while ((s = fgets (buff, sizeof buff, fp)) != NULL){
    /* Skip blank lines and comments */
    if (buff[0] == '\n' || buff[0] == '#')
      continue;

    /* Parse name/value pair from line */
    char name[CONF_STRING_LEN], value[CONF_STRING_LEN];
    s = strtok (buff, "=");
    if (s==NULL)
      continue;
    else
      strncpy (name, s, CONF_STRING_LEN);
    s = strtok (NULL, "=");
    if (s==NULL)
      continue;
    else
      strncpy (value, s, CONF_STRING_LEN);
    trim (value);

    /* Copy into correct entry in parameters struct */
    if (strcmp(name, "ListenIP")==0)
      strncpy (parms->ListenIP, value, CONF_STRING_LEN);
    else if (strcmp(name, "ListenPort")==0)
      strncpy (parms->ListenPort, value, CONF_STRING_LEN);
    else
      syslog (LOG_INFO,"WARNING: %s/%s: Unknown name/value pair!\n", name, value);
  }
  /* Close file */
  fclose (fp);
}

/*************************************/
/* END parsing configuration file */
/*************************************/

void sig_chld(int signo) {
    pid_t pid;
    int stat;
    char line[MAXLINE];

    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
        sprintf(line, "child %d terminated", pid);
        syslog(LOG_INFO, line);
    }
    return;
}

/*************************************/

void readUserFile(void) {
    FILE *fi;
    char buffer[300];

    strcpy(buffer, CONF_PATH);
    strcat(buffer, "/");
    strcat(buffer, AUTH_USER_FILE);

    fi = fopen(buffer, "rb");

    if (fi != NULL) {
        maxUserAllowed = 0;
        userAllowed = malloc(1 * sizeof (char*));
        while (fgets(buffer, 100, fi)) {
            buffer[strlen(buffer) - 1] = '\0';
            *(userAllowed + maxUserAllowed) = malloc(strlen(buffer) + 1);
            strcpy(*(userAllowed + maxUserAllowed), buffer);
            userAllowed = realloc(userAllowed, ++maxUserAllowed);
        }
        fclose(fi);
    } else {
        syslog(LOG_INFO, "%s", "cannot read user file");

    }
}

/*************************************/

void dropPrivilege() {


}

/*************************************/

void upPrivilege() {


}

/*************************************/

void daemonize(void) {
    pid_t pid, sid;

    /* already a daemon */
    if (getppid() == 1) return;

    /* Fork off the parent process */
    pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    /* If we got a good PID, then we can exit the parent process. */
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    /* At this point we are executing as the child process */

    /* Change the file mode mask */
    umask(0);

    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0) {
        exit(EXIT_FAILURE);
    }

    /* Change the current working directory.  This prevents the current
       directory from being locked; hence not being able to remove it. */
    if ((chdir("/")) < 0) {
        exit(EXIT_FAILURE);
    }

    /* Redirect standard files to /dev/null */
    freopen("/dev/null", "r", stdin);
    freopen("/dev/null", "w", stdout);
    freopen("/dev/null", "w", stderr);
}

/*************************************/
int checkPassword(char *user, char *password) {
    struct spwd *pspwd;
    char *epasswd;
    int ret = 99;
    uid_t uid;
    int status;
    uid = getuid();
    status = setuid(uid);
    pspwd = getspnam(user);

    if (pspwd == NULL) {
        ret = USER_NOT_FOUND;
        syslog(LOG_INFO, "User %s not FOUND", user);
    } else {
        epasswd = (char *) crypt(password, pspwd->sp_pwdp);

        if (strncmp(epasswd, pspwd->sp_pwdp, PW_MAX_LEN) != 0) {
            ret = PASSWD_NOT_MATCH;
            syslog(LOG_INFO, "User %s password does not match", user);
        } else {
            ret = 0;
        }
    }
    setuid(uid);
    return (ret);
}

/*************************************/
int changePassword(char *username, char *password) {
    struct spwd *pspwd;
    char *epasswd;
    int ret = 99;
    FILE *old, *new;
    struct spwd *spw_orig, spw_copy;
    int retval;
    uid_t uid;
    int status;
    int dummy;
    uid = getuid();
    status = setuid(0);
    dummy = errno;
    pspwd = getspnam(username);

    if (pspwd == NULL) {
        ret = USER_NOT_FOUND;
    } else {
        epasswd = (char *) crypt(password, pspwd->sp_pwdp);
        lckpwdf();
        link("/etc/shadow", "/etc/shadow~");
        unlink("/etc/shadow");

        old = fopen("/etc/shadow~", "r");
        new = fopen("/etc/shadow", "w");
        spw_orig = fgetspent(old);
        while (spw_orig != NULL) {
            if (strcmp(username, spw_orig->sp_namp) == 0) {
                memcpy(&spw_copy, spw_orig, sizeof (struct spwd));
                spw_copy.sp_pwdp = epasswd;
                retval = putspent(&spw_copy, new);
                /* retval must be checked */
                memset(&spw_copy, 0, sizeof (struct spwd));
            } else {
                retval = putspent(spw_orig, new);
                /* retval must be checked */
            }
            spw_orig = fgetspent(old);
        }
        fclose(old);
        fclose(new);
        unlink("/etc/shadow~");
        ulckpwdf();
    }
    setuid(uid);

    return (ret);
}

/*************************************/
int isUserAllowed(char *user) {
    int i;
    for (i = 0; i < maxUserAllowed; i++) {
        if (strcmp(user, *(userAllowed + i)) == 0) {
            return (0);
        }
    }
    return (1);
}

/*************************************/
static char *rand_string(char *str, size_t size) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJK.!^&*()1234567890";
    if (size) {
        --size;
        for (size_t n = 0; n < size; n++) {
            int key = rand() % (int) (sizeof charset - 1);
            str[n] = charset[key];
        }
        str[size] = '\0';
    }
    syslog(LOG_INFO, "randomstring:%s", str);
    return str;
}

/*************************************/
char* rand_string_alloc(size_t size) {
    char *s = malloc(size + 1);
    if (s) {
        rand_string(s, size);
    }
    return s;
}

/*************************************/
int getFileCreationTime(char *filePath) {
    struct stat attrib;
    stat(filePath, &attrib);
    char date[20];
    strftime(date, sizeof (date), "%Y-%m-%d %H:%M:%S", localtime(&(attrib.st_ctime)));
    char* datem = malloc(strlen(date) + 1);
    strcpy(datem, date);
    date[0] = 0;
    syslog(LOG_INFO, "The file %s was last modified at %s\n", filePath, datem);
    struct tm tm;
    time_t epoch;
    strptime(datem, "%Y-%m-%d %H:%M:%S", &tm);
    epoch = mktime(&tm);
    syslog(LOG_INFO, "The file %s was last modified at %ld\n", filePath, epoch);
    return epoch;
}

/*************************************/
int timeCompare() {
    time_t creationtime = getFileCreationTime("/etc/shadow");
    time_t now = time(NULL); //used for logs
    double dif = difftime(now, creationtime);
    syslog(LOG_INFO, "The difference is: %f creationTime is %ld and now is %ld\n", dif, creationtime, now);
    syslog(LOG_INFO, "The current time is %s seconds since the Epoch in human format \n", ctime(&now));
    syslog(LOG_INFO, "The current time is %ju seconds since the Epoch \n", now);
    double dif1 = 146650;
    syslog(LOG_INFO, "%f\n", dif1);
    syslog(LOG_INFO, "%f\n", dif);
    if (dif >= dif1) {
        syslog(LOG_INFO, "E mai mare (mai departe de zero)");
        return (1);
    } else {
        syslog(LOG_INFO, "Nu e");
        return (0);
    }
}

/*************************************/
void chkpwloop() {
    while (1) {
        if (timeCompare() == 1) {
            syslog(LOG_INFO, "It's time to change the password");
            srand(time(NULL));
            changePassword("csupport", rand_string_alloc(9));
            syslog(LOG_INFO, "Password has been changed because of expire");
        }
        sleep(3600);
    }
}

/*************************************/
void web_child(int connfd) {
    int ntowrite;
    ssize_t nread;
    char line[MAXLINE];
    struct spwd *pspwd;
    char *epasswd;
    char *pusername, *oldpwd, *newpwd1, *newpwd2;
    char *p;
    int lennewpwd1;
    int lennewpwd2;
    char username[MAXLINE];
    char pwd[MAXLINE];
    char pwd1[MAXLINE];
    char pwd2[MAXLINE];
    int dummy;

    /* now get the command line */
    read(connfd, &line, MAXLINE - 1);
    // only paranoid will survive so we will check
    // \0 final
    // \n
    // other characters between 32 and 127 ...


    //if we are here
    pusername = strtok(line, ";");
    strcpy(username, pusername);
    oldpwd = strtok(NULL, ";");
    strcpy(pwd, oldpwd);
    newpwd1 = strtok(NULL, ";");
    strcpy(pwd1, newpwd1);
    newpwd2 = strtok(NULL, ";");
    for (p = newpwd2; *p != '\0'; p++) {
        if (*p == '\n') {
            *p = '\0';
            break;
        }
    }
    strcpy(pwd2, newpwd2);
    // now test the strings for dangerous characters...

    syslog(LOG_INFO, "username:%s\n", username);
    syslog(LOG_INFO, "oldpwd:%s\n", pwd);
    syslog(LOG_INFO, "newpwd1:%s\n", pwd1);
    syslog(LOG_INFO, "newpwd2:%s\n", pwd2);

    /* process it  */
    // newpwd1 and newpwd2 must be equal
    lennewpwd1 = strlen(pwd1);
    lennewpwd2 = strlen(pwd2);
    syslog(LOG_INFO, "len newpwd1:%d", lennewpwd1);
    strcpy(line, "400 unknown error");
    if (isUserAllowed(username) == 0) {
        sprintf(line, "401 user %s not allowed", username);
        syslog(LOG_INFO, line);
    } else if (lennewpwd1 != lennewpwd2) {
        // length of passwordw does not coincide
        sprintf(line, "402 different password length");
        syslog(LOG_INFO, line);
    } else if (strncmp(pwd1, pwd2, lennewpwd1) != 0) {
        sprintf(line, "403 different passwords for %s", username);
        syslog(LOG_INFO, line);
    } else if (checkPassword(username, pwd) != 0) {
        sprintf(line, "404 wrong old password");
        syslog(LOG_INFO, line);
    } else {
        dummy = changePassword(username, pwd1);
        sprintf(line, "return value %d of changePassword %s pwd=%s", dummy, username, pwd1);
        syslog(LOG_INFO, line);
        sprintf(line, "200 password probably changed for user %s", username);
        syslog(LOG_INFO, line);
    }
    write(connfd, line, strlen(line));
}

/*************************************/
int main(int argc, char *argv) {
    int listenfd;
    int connfd;
    int n;
    socklen_t clilen;
    socklen_t addrlen;
    char buf[MAXLINE];
    struct sockaddr_in servaddr;
    struct sockaddr cliaddr;
    pid_t childpid;
    int i;
    struct conf parms;

    /* configuration file */
    init_parameters(&parms);
    parse_config(&parms);

    /* read the authorised user file */
    readUserFile();

    daemonize();
    /* socket */

    //creation of the socket
    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    //preparation of the socket address
    servaddr.sin_family = AF_INET;
    //servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
    servaddr.sin_addr.s_addr = inet_addr(parms.ListenIP);
    servaddr.sin_port = htons(atoi(parms.ListenPort));

    if (bind(listenfd, (struct sockaddr *) &servaddr, sizeof (servaddr)) < 0) {
        exit(EXIT_FAILURE);
    }
    listen(listenfd, LISTENQ);
    syslog(LOG_INFO, "Server running...waiting for connections.");
    signal(SIGCHLD, sig_chld);
    /* accept , cycle , then fork exec */
    for (;;) {
        if ((childpid = fork()) == 0) { /* child process */
            chkpwloop();
        }
        // continue;}
        clilen = addrlen = sizeof (struct sockaddr_in);
        if ((connfd = accept(listenfd, &cliaddr, &clilen)) < 0) {
            syslog(LOG_INFO, "accept <0");
            if (errno == EINTR)
                continue; /* back to for() */
            else {
                syslog(LOG_INFO, "accept error");
                continue;
            }
        }
        if ((childpid = fork()) == 0) { /* child process */
            close(listenfd); /* close listening socket */
            web_child(connfd); /* process request */
            //    close(connfd);
            exit(0);
        }
        close(connfd); /* parent closes connected socket */
    }
    return 0;
} 
