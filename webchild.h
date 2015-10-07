#ifndef WEBCHILD_H_INCLUDED
#define WEBCHILD_H_INCLUDED
void web_child(int connfd);
int isUserAllowed(char *user);
void readUserFile(void);
#endif
