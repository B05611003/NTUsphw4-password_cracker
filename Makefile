all:
	gcc main.c -o main -Wall -lpthread -lcrypto -O3 -L/usr/local/opt/openssl/lib
