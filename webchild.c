#include "header.h"
#include "webchild.h"
#include "passwd.h"

#define MAXLINE                 3000
#define CONF_PATH               "."
#define AUTH_USER_FILE          "authuser"


int maxUserAllowed = 0;
char **userAllowed; //data structure as an array with the latest item at zero

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
void web_child(int connfd) {
    char line[MAXLINE];
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

