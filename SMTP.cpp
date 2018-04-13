#include "SMTP.h"

#define qqUser "18814125877"
#define qqPW "zhr18814125877"
#define qqSendMailBox "m18814125877@163.com"
#define serverName "smtp.163.com"


char SMTP::ch64[] = {
    'A','B','C','D','E','F','G','H','I','J','K','L','M','N',
    'O','P','Q','R','S','T','U','V','W','X','Y','Z',
    'a','b','c','d','e','f','g','h','i','j','k','l','m','n',
    'o','p','q','r','s','t','u','v','w','x','y','z',
    '0','1','2','3','4','5','6','7','8','9','+','/','='
};

char SMTP::buf[];

SMTP::SMTP()
{
}

char* SMTP::toBase64(const char* buffer,int buflen)
{
    strcpy(buf,"");
    int nLeft =  3 - buflen%3 ;
    //根据BASE64算法，总长度会变成原来的4/3倍
    //所以内存分配＝length*4/3并加1位作为结束符号(0)
    //allocMem(( buflen + nLeft )*4/3+1);
    //临时变量，
    char ch[4];
    int i ,j;
    for ( i = 0 ,j = 0; i < ( buflen - buflen%3 );  i += 3,j+= 4 )
    {
            ch[0] = (char)((buffer[i]&0xFC) >> 2 );
            ch[1] = (char)((buffer[i]&0x03) << 4 | (buffer[i+1]&0xF0) >> 4 );
            ch[2] = (char)((buffer[i+1]&0x0F) << 2 | (buffer[i+2]&0xC0) >> 6 );
            ch[3] = (char)((buffer[i+2]&0x3F));
            //查询编码数组获取编码后的字符
            buf[j] = ch64[ch[0]];
            buf[j+1] = ch64[ch[1]];
            buf[j+2] = ch64[ch[2]];
            buf[j+3] = ch64[ch[3]];
    }

    if ( nLeft == 2 )
    {
            ch[0] = (char)((buffer[i]&0xFC) >> 2);
            ch[1] = (char)((buffer[i]&0x3) << 4 );
            ch[2] = 64;
            ch[3] = 64;
            //查询编码数组获取编码后的字符
            buf[j] = ch64[ch[0]];
            buf[j+1] = ch64[ch[1]];
            buf[j+2] = ch64[ch[2]];
            buf[j+3] = ch64[ch[3]];
    }
    else if ( nLeft == 1 )
    {
            ch[0] = (char)((buffer[i]&0xFC) >> 2 );
            ch[1] = (char)((buffer[i]&0x03) << 4 | (buffer[i+1]&0xF0) >> 4 );
            ch[2] = (char)((buffer[i+1]&0x0F) << 2 );
            ch[3] = 64;
            //查询编码数组获取编码后的字符
            buf[j] = ch64[ch[0]];
            buf[j+1] = ch64[ch[1]];
            buf[j+2] = ch64[ch[2]];
            buf[j+3] = ch64[ch[3]];
    }
    return buf;
}

void SMTP::sendEmail(const char *recvMailbox, const char *szMessage,const char *subject)
{
    const char szServer[] = serverName;
    const short nPort = 25;
    int sockfd;
    struct sockaddr_in server_addr;
    struct hostent *host;

    if((host=gethostbyname(szServer))==NULL)/*取得主机IP地址*/
    {
        fprintf(stderr,"Gethostname error, %s\n", strerror(errno));
        exit(1);
    }

    if((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1)/*建立SOCKET连接*/
    {
        fprintf(stderr,"Socket Error:%s\a\n",strerror(errno));
        exit(1);
    }

    /* 客户程序填充服务端的资料 */
    bzero(&server_addr,sizeof(server_addr));
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(nPort);
    server_addr.sin_addr=*((struct in_addr *)host->h_addr);

    /* 客户程序发起连接请求 */
    if(connect(sockfd,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr))==-1)/*连接网站*/
    {
         fprintf(stderr,"Connect Error:%s\a\n",strerror(errno));
         exit(1);
    }

    printf("\nStream Client connecting to server: %s on port: %d\n",szServer, nPort);

    char szBuf[2048];
    memset(szBuf,0,2048);
    int nRet;
    // strcpy(szBuf, "From the Client");
    char buf[350]="0";
    char server[250];
    gethostname(server,250);
    strcpy(buf, "HELO ");

    strcat(buf, szServer);
    strcat(buf, " \r\n");
    printf("%s:::%d\n",buf,strlen(buf));

    //
    // Wait for a reply
    //
    nRet = recv(sockfd,szBuf,sizeof(szBuf),0);
    printf("\nData received OVER DATA: %s", szBuf);

    nRet = send(sockfd, buf, strlen(buf), 0);
    printf("\nsend %s",buf);
    nRet = recv(sockfd, szBuf, sizeof(szBuf), 0);
    printf("\nData received2: %s", szBuf);

    //发送准备登陆信息
    nRet = send(sockfd, "AUTH LOGIN\r\n", strlen("AUTH LOGIN\r\n"), 0);
    nRet = recv(sockfd, szBuf, sizeof(szBuf), 0);
    printf("\nData received LOGIN: %s", szBuf);

    //发送用户名和密码,这里的用户名和密码必须用base64进行转码,发送转码以后的字符串，对于126邮箱来说用户名是@前面的字符串
    char *userName = SMTP::toBase64(qqUser,strlen(qqUser));
    strcat(userName,"\r\n");
    nRet = send(sockfd,userName, strlen(userName),0);
    printf("\nData Send USERNAME");
    nRet = recv(sockfd, szBuf, sizeof(szBuf),0);
    printf("\nData received USERNAME: %s", szBuf);
    printf("\nData Send PASS");

    //发送用户密码
    char *userPW = SMTP::toBase64(qqPW,strlen(qqPW));
    strcat(userPW,"\r\n");
    nRet = send(sockfd,userPW, strlen(userPW), 0);
    //printf("\nData Send PASS");
    nRet = recv(sockfd, szBuf, sizeof(szBuf),0);
    printf("\nData received USERPASSWORD: %s", szBuf);

    //发送[发送邮件]的信箱(改成你的邮箱!），该邮箱要与用户名一致，否则发送不成功
    char fromMailBox[512];
    sprintf(fromMailBox,"MAIL FROM: <%s>\r\n",qqSendMailBox);
    send(sockfd,fromMailBox,strlen(fromMailBox),0);
    printf("\nsend MAIL FROM\n");
    nRet = recv(sockfd, szBuf, sizeof(szBuf), 0);printf("\nData received MAILFROM: %s", szBuf);

    //发送[接收邮件]的邮箱
    char toMailBox[512];
    sprintf(toMailBox,"RCPT TO: <%s>\r\n",recvMailbox);
    nRet= send(sockfd,toMailBox,strlen(toMailBox),0);
    printf("\nsend RCPT TO\r\n");
    nRet = recv(sockfd, szBuf, sizeof(szBuf), 0);
    printf("\nData received TO MAIL: %s", szBuf);
    /*
    nRet= send(sockfd,"RCPT TO: <server_ip_alert@hotmail.com>\r\n",strlen("RCPT TO: <server_ip_alert@hotmail.com>\r\n"),0);printf("\nsend RCPT TO\r\n");
    nRet = recv(sockfd, szBuf, sizeof(szBuf), 0);   printf("\nData received TO MAIL: %s", szBuf);
    */
    char MailData[512];
    sprintf(MailData,"From: \"%s\"<%s>\r\n"
                                    "To: %s\r\n"
                                    "Subject: %s\r\n\r\n",
                                    "LWMAMSDeveloper",
                                    qqSendMailBox,
                                    recvMailbox,
                                    subject); //主题可以改成别的
    //各诉邮件服务器，准备发送邮件内容

    send(sockfd,"DATA\r\n", strlen("DATA\r\n"),0);
    printf("\nsend DATA\n");
    //  nRet = recv(sockfd, szBuf, sizeof(szBuf)+1, 0); printf("\nData receivedSEND DATA: %s", szBuf);
    //发送邮件标题
    send(sockfd,MailData, strlen(MailData),0);
    //发送邮件内容
    char msg[strlen(szMessage)+2];
    strcpy(msg,szMessage);
    strcat(msg,"\r\n");
    send(sockfd,msg, strlen(msg),0);//发送信息
    //发送邮件结束
    send(sockfd,"\r\n.\r\n", strlen("\r\n.\r\n"),0);
    //接收邮件服务器返回信息
    nRet = recv(sockfd,szBuf,sizeof(szBuf),0);
    printf("\nData received OVER DATA: %s", szBuf);
    send(sockfd,"QUIT\r\n", strlen("QUIT\r\n"),0);
    nRet = recv(sockfd,szBuf,sizeof(szBuf),0);
    printf("\nData received QUIT: %s", szBuf);

    //
    // Display the received data
    //
    //printf("\nData received3: %s", szBuf);

    close(sockfd);
    return;
}

