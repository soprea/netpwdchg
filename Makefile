all: 

	gcc -std=c99 netpwdchg.c header.h -lcrypt -o netpwdchg
