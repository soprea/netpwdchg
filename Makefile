all: 

	gcc -std=c99 netpwdchg.c config.c passwd.c webchild.c -lcrypt -o netpwdchg -Wall
