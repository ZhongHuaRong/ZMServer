#ifndef SMTP_H
#define SMTP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <limits.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ctype.h>

using namespace std;

typedef int SOCKET;

class SMTP
{
public:
    SMTP();

    void sendEmail(const char *recvMailbox,const char *szMessage,const char *subject = "LWMAMS 异常通知");
     static char* toBase64(const char* buffer,int buflen);

     static char ch64[];
     static  char buf[1024];
};

#endif // SMTP_H
