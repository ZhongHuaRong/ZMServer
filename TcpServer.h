#ifndef TCPSERVER_H
#define TCPSERVER_H

#include "CThread.h"
#include "DBManagement.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <list>

//继承TcpThread,继承类型时protected,因为create接口不想暴漏在外面
//通讯协议: 命令格式 + 页数 + 页的行数 + test标识(一般为0,从第几行开始获取数据,仅用于测试)
//                  + check标识(一般为false,check为false后面不用填写) +用于比较的数据类型+用于比较的符号+比较值
class TcpServer : protected CThread
{
public:
    enum CommandType{
        CT_SIGNUP = 0x10,
        CT_SIGNUPAUTO,
        CT_USERSLOGINELSEWHERE,
        CT_PARACHECKACCOUNTNUMBER,
        CT_PARACHECKAPPID,
        CT_REGISTERED,
        CT_DATASHOW,
        CT_ROUTE,
        CT_CONTROL,
        CT_STATISTICS
    };

public:
    TcpServer();

    static void serverStartRunning();
    static void *run(void *arg);
    static void threadDestroy(TcpServer * server);
    static void threadCreate(TcpServer *server);

    static void initPara();
    static void initListenAddr();
    static int getClientCount();

    char const * getClientIP() const;

    int nSocketFd() const;
    void setNSocketFd(int nSocketFd);

    sockaddr_in sclientAddr() const;
    void setSclientAddr(const sockaddr_in &sclientAddr);

    bool bActivation() const;

private:
    void setClientIP();
    void closeStocket();
    void printfIP();
    void printfMsg(const char  *msg);
    void recognizeCmd(char *cmd);
    int getCmdArgCount(char *cmd);
    void updateUserActivation(bool updateDB);
    void translation(CommandType cmd,char **arg,int count);
    void sendCmd(CommandType cmd,bool result);
    void sendCmd(CommandType cmd,bool result,const char *msg = nullptr);
    void sendMsg(CommandType cmd,int pageNum,int pageRow,int testFlag = 0,
                 bool isCheck =false,char dataType = 1,char compare = 1,char * checkData = nullptr);
    void sendTest(int testFlag);
    void setServerState(bool state);

    static void listRemove(TcpServer* fd);
    static void listCreate(TcpServer* fd);
    static void userLogout(int userID);
    static void userLogout(TcpServer* server);
    static TcpServer* findUserIDFromList(int userID,int* startNum);

private:
    //为了实现挤用户功能,后面登陆的把前者挤下去
    static std::list<TcpServer *> m_lthreadFd;
    //声明互斥量
    static pthread_mutex_t m_sMutex;

    static int m_nlistenFd;
    static struct sockaddr_in m_slistenAddr;
    static const int m_cnPost = 48428;

    //判断是否激活,默认false,非激活状态服务器不发送任何指令
    bool m_bActivation;
    int m_nSocketFd;
    struct sockaddr_in m_sclientAddr;
    char m_cipstr[16];
    DBManagement m_dbMan;
    char *m_pUserName;
    int m_nUserID;
};

#endif // TCPSERVER_H
