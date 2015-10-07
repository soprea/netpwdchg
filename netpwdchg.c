#include "header.h"
#include "config.h"
#include "passwd.h"
#include "webchild.h"

#define CONFIG_FILE             "netpwdchg.conf"
#define AUTH_USER_FILE          "authuser"
#define LISTENQ                 5
#define PROGRAM_NAME            "netpwdchg"
#define MAXLINE                 3000

uid_t initial_r_uid; /* real uid */
uid_t initial_e_uid; /* effective uid */
gid_t initial_r_gid;
gid_t initial_e_gid;

//char **userAllowed; //data structure as an array with the latest item at zero
//int maxUserAllowed = 0;

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
int main(int argc, char **argv) {
    int listenfd;
    int connfd;
    socklen_t clilen;
    socklen_t addrlen;
    struct sockaddr_in servaddr;
    struct sockaddr cliaddr;
    pid_t childpid;
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
