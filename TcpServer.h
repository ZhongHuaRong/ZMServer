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
        CT_AUTHORIZEDSIGNUP,
        CT_AUTHORIZEDSIGNUPRESULT,
        CT_USERSLOGINELSEWHERE,
        CT_PARACHECKACCOUNTNUMBER,
        CT_EMAILCODE,
        CT_CHANGEPASSWORD,
        CT_REGISTERED,
        CT_DATASHOW,
        CT_ROUTE,
        CT_CONTROL,
        CT_STATISTICS,
        CT_ANDROIDDATASHOW,
        CT_SMSEMAILPUSH
    };

    enum Platform{
        PC = 0x01,
        Android,
        NoSet
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

    Platform ePlat() const;
    void setEPlat(const Platform &ePlat);

    bool bAuthorized() const;
    void setBAuthorized(bool bAuthorized);

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
    void sendCmd(CommandType cmd,int result,const char *msg = nullptr);
    void sendMsg(CommandType cmd,int pageNum,int pageRow,int testFlag = 0,
                 bool isCheck =false,char dataType = 1,char compare = 1,char * checkData = nullptr);
    void sendTest(int testFlag);
    void setServerState(bool state);
    void setPlatform(Platform plat);
    void setLoginSuccessMsg();
    void sendSMSAndEmail(bool pushTemp, bool pushPH, bool pushTur,
                         bool pushSMS, bool pushEmail,
                         const char *phone, const  char *email);
    void sendSMS(bool pushTemp, bool pushPH, bool pushTur,const char *phone);
    void sendEmail(bool pushTemp, bool pushPH, bool pushTur,const char *email);

    static void listRemove(TcpServer* fd,Platform plat = NoSet);
    static void listCreate(TcpServer* fd,Platform plat = NoSet);
    static void threadListChanged(TcpServer *fd,Platform plat);
    static void userLogout(int userID,Platform plat);
    static void userLogout(TcpServer* server);
    static TcpServer* findUserIDFromList(int userID,int* startNum,Platform plat);

private:
    //未登录已连接的用户放在这里
    static std::list<TcpServer *> m_lthreadFd;
    //为了实现双平台登陆
    static std::list<TcpServer *> m_lPCThreadFd;
    static std::list<TcpServer *> m_lAndThreadFd;

    //声明互斥量
    static pthread_mutex_t m_sMutex;

    static int m_nlistenFd;
    static struct sockaddr_in m_slistenAddr;
    static const int m_cnPost = 48428;

    //判断是否激活,默认false,非激活状态服务器不发送任何指令
    bool m_bActivation;
    //判断是否是PC端,不同平台会有不同处理
    Platform m_ePlat;
    //标示该客户端是否在等待授权
    bool m_bAuthorized;

    int m_nSocketFd;
    struct sockaddr_in m_sclientAddr;
    char m_cipstr[16];
    DBManagement m_dbMan;
    char *m_pUserName;
    int m_nUserID;
};

#endif // TCPSERVER_H
