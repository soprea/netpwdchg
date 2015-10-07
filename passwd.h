#ifndef PASSWD_H_INCLUDED
#define PASSWD_H_INCLUDED
int changePassword(char *username, char *password);
int checkPassword(char *user, char *password);
char *rand_string(char *str, size_t size);
char* rand_string_alloc(size_t size);
int getFileCreationTime(char *filePath);
int timeCompare();
void chkpwloop();
#endif
