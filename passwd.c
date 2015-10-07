#include "header.h"
#include "passwd.h"

#define USER_NOT_FOUND          1
#define PW_MAX_LEN              14
#define PASSWD_NOT_MATCH        2


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
char *rand_string(char *str, size_t size) {
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

