#include "TcpServer.h"
#include <string.h>
//#include <stdio.h>
#include<algorithm>
#include <iostream>
#include <stdlib.h>
#include "HttpReq.h"
#include "SMTP.h"

std::list<TcpServer *> TcpServer::m_lthreadFd;
std::list<TcpServer *> TcpServer::m_lPCThreadFd;
std::list<TcpServer *> TcpServer::m_lAndThreadFd;
pthread_mutex_t TcpServer::m_sMutex;
int TcpServer::m_nlistenFd;
struct sockaddr_in TcpServer::m_slistenAddr;

TcpServer::TcpServer():
    m_bActivation(false),
    m_ePlat(NoSet),
    m_bAuthorized(false)
{
}

/**
   * @函数意义:打开serverSocket开始侦听,服务器程序启动,该函数不会返回
   * @作者:ZM
   * @date 2018-1
   */
void TcpServer::serverStartRunning()
{
    TcpServer *server;
    socklen_t socksize=sizeof(struct sockaddr);
    struct sockaddr_in client_addr;
    int comm_fd;

    pthread_mutex_init(&TcpServer::m_sMutex,NULL);//初始化
    TcpServer::initPara();
    while(true)
    {
        server = new TcpServer();
        if((comm_fd=accept(TcpServer::m_nlistenFd,(struct sockaddr*)&client_addr,&socksize))>=0)
        {
            server->setNSocketFd(comm_fd);
            server->setSclientAddr(client_addr);
            TcpServer::threadCreate(server);
        }
    }
    pthread_mutex_destroy(&TcpServer::m_sMutex);//删除
    close(TcpServer::m_nlistenFd);
}

/**
   * @函数意义:线程运行的函数
   * @作者:ZM
   * @param [in] arg
   *                      传入线程的文件标示符
   * @date 2018-1
   */
void* TcpServer::run(void *arg)
{
    TcpServer *server = static_cast<TcpServer*>(arg);

    std::cout<<"server["<<server->getThreadFd()<<"] is running\n";
    std::cout<<"client count:"<<TcpServer::getClientCount()<<std::endl;
    int count;
    char buf[100];
    while(1)
    {
        memset(buf,'0',100);
        count=read(server->m_nSocketFd,buf,100);
        if(count<=0)
        {
            perror("internet disconnected.\n");
            break;
        }
        server->printfMsg(buf);
        server->recognizeCmd(buf);
        //server->findData("boat_data");
    }

    TcpServer::threadDestroy(server);
    return server;
}

/**
   * @函数意义:在线程退出时调用的函数,从list中移除该标示符
   * @作者:ZM
   * @param [in] thread_fd
   *                        将要移除的文件标示符
   * @date 2018-1
   */
void TcpServer::threadDestroy(TcpServer * server)
{
    std::cout<<"server["<<server->getThreadFd()<<"] is destroy\n";
    //移除队列
    listRemove(server,server->m_ePlat);
    //修改数据库状态
    server->userLogout(server);
    //关闭stocket
    server->closeStocket();
    //删除对象
    delete server;
}

/**
   * @函数意义:当Socket连接时,调用该函数新建线程
   * @作者:ZM
   * @date 2018-1
   */
void TcpServer::threadCreate(TcpServer *server)
{
    server->setClientIP();
    server->printfIP();
    if(server->create((&TcpServer::run),server)==-1)
    {
        perror("Failed to create thread");
    }
    else
    {
        listCreate(server);
    }
}

/**
   * @函数意义:初始化所有和socket有关的参数
   * @作者:ZM
   * @date 2018-1
   */
void TcpServer::initPara()
{
    int socksize,ret;
    int i=1;
    socksize=sizeof(struct sockaddr);
    bzero(&TcpServer::m_slistenAddr,sizeof(TcpServer::m_slistenAddr));
    initListenAddr();
    TcpServer::m_nlistenFd=socket(AF_INET,SOCK_STREAM,0);
    setsockopt( TcpServer::m_nlistenFd,SOL_SOCKET,SO_REUSEADDR,&i,sizeof(i));
    ret=bind(TcpServer::m_nlistenFd,(struct sockaddr*)&TcpServer::m_slistenAddr,socksize);
    if(ret==0)
        std::cout<<"Bind successfully:"<<TcpServer::m_cnPost<<std::endl;
    ret=listen( TcpServer::m_nlistenFd,8);
    if(ret==0)
        std::cout<<"Listen successfully!\n";
}

/**
   * @函数意义:初始化ListenAddr
   * @作者:ZM
   * @date 2018-1
   */
void TcpServer::initListenAddr()
{
    m_slistenAddr.sin_family=AF_INET;
    m_slistenAddr.sin_port=htons(TcpServer::m_cnPost);
    m_slistenAddr.sin_addr.s_addr=INADDR_ANY;
}

char const * TcpServer::getClientIP() const
{
    return m_cipstr;
}

int TcpServer::getClientCount()
{
    return m_lthreadFd.size() + m_lPCThreadFd.size() + m_lAndThreadFd.size();
}

int TcpServer::nSocketFd() const
{
    return m_nSocketFd;
}

void TcpServer::setNSocketFd(int nSocketFd)
{
    m_nSocketFd = nSocketFd;
}
sockaddr_in TcpServer::sclientAddr() const
{
    return m_sclientAddr;
}

void TcpServer::setSclientAddr(const sockaddr_in &sclientAddr)
{
    m_sclientAddr = sclientAddr;
}

/**
   * @函数意义:输出客户端IP
   * @作者:ZM
   * @date 2018-1
   */
void TcpServer::printfIP()
{
    std::cout<<"A new connection come on:"<<m_cipstr<<std::endl;
}

/**
   * @函数意义:输出Socket接受到的信息
   * @作者:ZM
   * @param [in] msg
   *                字符数组,发送的信息
   * @date 2018-1
   */
void TcpServer::printfMsg(const char *msg)
{
    std::cout<<"Information from ip["<<this->getClientIP()<<
            "]:"<<msg<<std::endl;
}

/**
   * @函数意义:解析命令,获取有效字符
   * @作者:ZM
   * @param [in] cmd
   *                socket中提取的字符数组
   * @date 2018-1
   */
void TcpServer::recognizeCmd(char *cmd)
{
     char ch('$');
     char *key_point;
     int n=0;
     int argCount = getCmdArgCount(cmd);
     char *head = nullptr;
     char *arg[argCount];
//     while(cmd&&n!=argCount+1)
//      {
//        key_point = strsep(&cmd,&ch);
//        if(n==0)
//        {
//            head = key_point;
//        }
//        else
//        {
//             arg[n-1] = key_point;
//        }
//        n++;
//      }
     key_point = cmd;
     head = cmd;
     while(key_point[0] !='\0')
     {
         arg[n] = m_dbMan.myStrsep(key_point,ch);
         key_point = arg[n];
         n++;
     }
     translation(static_cast<CommandType>(*head),arg,argCount);
}

/**
   * @函数意义:返回命令行的参数个数
   * @作者:ZM
   * @param [in] cmd
   *                    命令
   * @date 2018-1
   */
int TcpServer::getCmdArgCount( char *cmd)
{
    char ch=*cmd;
    int n;
    for(n=0;ch!='#';cmd++)
    {
        if(*cmd=='$')
            n++;
        else if(*cmd=='#')
            return n-1;
        else if(*cmd =='\0')
            return n-4;
    }
    return n-1;
}

/**
   * @函数意义:给客户端发送命令
   * @作者:ZM
   * @param [in] cmd
   *                        命令类型
   * @param [in] result
   *                        执行结果
   * @param [in] msg
   *                        执行失败附带的错误信息
   * @date 2018-1
   */
void TcpServer::sendCmd(TcpServer::CommandType cmd, int result, const char *msg)
{
    int msgLength = strlen(msg);
    //长度:前四位命令+msg长度+分隔符'$'+结束符'#'+字符串结尾'\0'
    char data[msg!=nullptr?msgLength+1:0+4+1+1];
    data[0] = cmd;
    data[1] ='$';
    data[2] = result + '0';
    data[3] ='$';
    if(msg==nullptr)
    {
        data[4]='#';
        data[5]='\0';
    }
    else
    {
        strcpy(data+4,msg);
        data[4+msgLength] = '$';
        data[5+msgLength] = '#';
        data[6+msgLength] = '\0';
    }
    int num=write(m_nSocketFd, data, strlen(data));
    std::cout<<"写入成功字节:"<<num<<std::endl;
    std::cout<<"写入成功字符:"<<data<<std::endl;
}

/**
   * @函数意义:发送相应的数据
   * @作者:ZM
   * @param [in] cmd
   *                    指令类型
   * @param [in] pageNum
   *                    页数
   * @param [in] pageRow
   *                    页的行数
   * @param [in] testFlag
   *                        从第几行开始获取数据(仅用于测试)
   * @param [in] isCheck
   *                    数据是否时查找类型
   * @param [in] dataType
   *                    需要查找的数据类型
   * @param [in] compare
   *                    比较符号的类型
   * @param [in] checkData
   *                       比较的数值
   * @date 2018-1
   */
void TcpServer::sendMsg(TcpServer::CommandType cmd, int pageNum, int pageRow,int testFlag,
                        bool isCheck, char dataType, char compare, char * checkData)
{
//    std::cout<<"cmd:"<<cmd<<std::endl;
//    std::cout<<"pageNum:"<<pageNum<<std::endl;
//    std::cout<<"pageRow:"<<pageRow<<std::endl;
//    std::cout<<"isCheck:"<<isCheck<<std::endl;
//    std::cout<<"dataType:"<<dataType<<std::endl;
//    std::cout<<"compare:"<<compare<<std::endl;
//    std::cout<<"checkData:"<<checkData<<std::endl;
    if(testFlag!=0)
    {
        sendTest(testFlag);
        std::cout<<"测试用"<<std::endl;
        return;
    }
    char msg[15+60*(pageRow+1)];
    msg[0]=static_cast<char>(cmd);
    msg[1]='$';
    if(isCheck)
    {
        m_dbMan.findDataAndCheck(msg+2,pageNum,pageRow,
                                 static_cast<DBManagement::DATATYPE>(dataType-'0'),
                                 static_cast<DBManagement::DATACOMPARE>(compare-'0'),
                                 checkData);
    }
    else
    {
        if(cmd==CT_CONTROL)
            m_dbMan.findData(msg+2,true,pageNum,pageRow);
        else
        {
            m_dbMan.findData(msg+2,false,pageNum,pageRow);
        }
    }
    int num=write(m_nSocketFd, msg, strlen(msg));
    std::cout<<"写入成功字节:"<<num<<std::endl;
    //std::cout<<"写入成功字符:"<<msg<<std::endl;
}

/**
   * @函数意义:查找测试用的数据
   * @作者:ZM
   * @param [in] testFlag
   *                        第几行开始搜索
   * @date 2018-2
   */
void TcpServer::sendTest(int testFlag)
{
    char msg[10+70*14];
    msg[0]=static_cast<char>(TcpServer::CT_DATASHOW);
    msg[1]='$';

    m_dbMan.findTestData(msg+2,testFlag);

    int num=write(m_nSocketFd, msg, strlen(msg));
    std::cout<<"写入成功字节:"<<num<<std::endl;
}

/**
   * @函数意义:设置服务器的响应状态标识m_bActivation,设置为false相当于该对象所连接的客户端下线
   * @作者:ZM
   * @param [in] state
   *                        将要设置的状态
   * @date 2018-1
   */
void TcpServer::setServerState(bool state)
{
    this->m_bActivation = state;
}

void TcpServer::setPlatform(TcpServer::Platform plat)
{
    this->m_ePlat = plat;
}

/**
   * @函数意义:登陆成功后需要的操作
   * @作者:ZM
   * @date 2018-2
   */
void TcpServer::setLoginSuccessMsg()
{
    setServerState(true);
    m_nUserID = m_dbMan.nUserID();
    threadListChanged(this,ePlat());
}

/**
  * @函数意义:发送邮件和短信
  * @作者:ZM
  * @param [in] pushTemp
  *             温度是否需要警告
  * @param [in] pushPH
  *             PH是否需要警告
  * @param [in] pushTur
  *             浑浊度是否需要警告
  * @param [in] pushSMS
  *             是否在手机端开启时发送短信
  * @param [in] pushEmail
  *             是否在手机端开启时发送邮件
  * @param [in] phone
  *             将要发送短信的手机号
  * @param [in] email
  *             将要发送邮件的邮箱
  * @date 2018-3
  */
void TcpServer::sendSMSAndEmail(bool pushTemp, bool pushPH, bool pushTur,
                                bool pushSMS, bool pushEmail,
                                const char *phone, const char *email)
{
    std::cout<<"pushTemp:"<<pushTemp<<std::endl;
    std::cout<<"pushPH:"<<pushPH<<std::endl;
    std::cout<<"pushTur:"<<pushTur<<std::endl;
    std::cout<<"pushSMS:"<<pushSMS<<std::endl;
    std::cout<<"pushEmail:"<<pushEmail<<std::endl;
    std::cout<<"phone:"<<phone<<std::endl;
    std::cout<<"email:"<<email<<std::endl;

    bool android = m_dbMan.checkAnotherPlatformExits(m_nUserID,true);
    std::cout<<"signup_Android:"<<android<<std::endl;

    if(android)
    {
        //安卓端登陆了
        if(pushSMS)
            sendSMS(pushTemp,pushPH,pushTur,phone);
        if(pushEmail)
            sendEmail(pushTemp,pushPH,pushTur,email);
    }
    else
    {
        sendSMS(pushTemp,pushPH,pushTur,phone);
        sendEmail(pushTemp,pushPH,pushTur,email);
    }
}

/**
   * @函数意义:发送短信
   * @作者:ZM
  * @param [in] pushTemp
  *             温度是否需要警告
  * @param [in] pushPH
  *             PH是否需要警告
  * @param [in] pushTur
  *             浑浊度是否需要警告
  * @param [in] phone
  *             将要发送短信的手机号
   * @date 2018-3
   */
void TcpServer::sendSMS(bool pushTemp, bool pushPH, bool pushTur, const char *phone)
{
    if( !pushTemp && !pushPH && !pushTur)
        return;

    HttpRequest *http;
    char http_return[4096];
    char http_msg[4096];

    sprintf(http_msg,"http://api.sms.cn/sms/?ac=send&"
            "uid=zhr18814125877&pwd=524651da822cd7930d87b8df505264b7&"
             "mobile=%s&content=您好！你所管理的鱼塘出现异常数据，请尽快去查看【LWMAMS】",phone);

    //短信需要收费,所以再需要的时候才取消注释演示一波
//    if(http->HttpGet(http_msg,http_return))
//    {
//        std::cout << http_return << std::endl;
//    }
}

/**
   * @函数意义:发送邮件
   * @作者:ZM
  * @param [in] pushTemp
  *             温度是否需要警告
  * @param [in] pushPH
  *             PH是否需要警告
  * @param [in] pushTur
  *             浑浊度是否需要警告
  * @param [in] email
  *             将要发送邮件的邮箱
   * @date 2018-3
   */
void TcpServer::sendEmail(bool pushTemp, bool pushPH, bool pushTur, const char *email)
{
    if( !pushTemp && !pushPH && !pushTur)
        return;

    char msg[50];
    strcpy(msg,"");
    bool isStart = true;
    if(pushTemp)
    {
        strcat(msg,"温度");
        isStart = false;
    }
    if(pushPH)
    {
        strcat(msg,isStart?"酸碱度":",酸碱度");
        isStart = false;
    }
    if(pushTur)
    {
        strcat(msg,isStart?"浑浊度":",浑浊度");
    }
    char data[300];
    sprintf(data,"亲爱的用户:\n"
                 "您好!\n"
                 "你所管理的鱼塘出现异常数据(%s),请您有空就去PC端或者安卓端查看!!!\n"
                 "想进入网页端查看请点击(http://119.29.243.183/111/index6.php)\n"
                 "LWMAMS开发人员",msg);

    SMTP smtp;
    //smtp.sendEmail(email,data);
}

/**
   * @函数意义:翻译命令,运行相应的操作
   * @作者:ZM
   * @param [in] cmd
   *               CommandType类型,枚举然后执行相应的动作
   * @param [in[ arg
   *                参数
   * @param [in] count
   *                参数的数量
   * @date 2018-1
   */
void TcpServer::translation(CommandType cmd, char **arg,int count)
{
    std::cout<<cmd<<std::endl;
    for(int a=0;a<count;a++)
        std::cout<<*(arg+a)<<std::endl;

    std::cout<<"bActivation:"<<this->bActivation()<<std::endl;
    //std::cout<<count<<std::endl;
    //用户未激活且CT不是登陆则不判断了,未激活只允许服务器发命令和只接受登陆,注册指令
    if(cmd!=CT_SIGNUP&&cmd!=CT_SIGNUPAUTO&&cmd!=CT_AUTHORIZEDSIGNUP&&
            cmd!=CT_AUTHORIZEDSIGNUPRESULT&&cmd!=CT_PARACHECKACCOUNTNUMBER&&cmd!=CT_REGISTERED
            &&cmd!=CT_EMAILCODE&&cmd!=CT_CHANGEPASSWORD)
        if(!this->bActivation())
            return;

//    std::cout<<"NOSET:"<<m_lthreadFd.size()<<std::endl;
//    std::cout<<"A:"<<m_lAndThreadFd.size()<<std::endl;
//    std::cout<<"P:"<<m_lPCThreadFd.size()<<std::endl;

    switch(cmd)
    {
    case CT_SIGNUP:
    case CT_SIGNUPAUTO:
    {
        setEPlat(static_cast<Platform>(atoi(arg[0])));
        DBManagement::SIGNUPERRORCODE code =m_dbMan.signup(ePlat() ==PC?true:false,arg[1],arg[2]);
        const char *str;
        bool result;
        switch(code)
        {
        case DBManagement::NOERROR:
            str=m_dbMan.getUserName();
            result =true;
            setLoginSuccessMsg();
            break;
        case DBManagement::USERNOTFOUND:
            str="user not found";
            result =false;
            break;
        case DBManagement::WRONGPASSWORD:
            str="password is wrong";
            result =false;
            break;
        case DBManagement::USERHASLOGGEDIN:
            str="sign up success";
            result =true;
            m_nUserID = m_dbMan.nUserID();
            //先让存在的登陆者退出
            TcpServer::userLogout(m_nUserID,ePlat());

            setLoginSuccessMsg();
            break;
        default:
            result =false;
            str="unknow error";
        }
        sendCmd(cmd,result,str);
        break;
    }
    case CT_AUTHORIZEDSIGNUP:
    {
        setEPlat(static_cast<Platform>(atoi(arg[0])));
        int id = m_dbMan.checkAnotherPlatformExits(arg[1],ePlat() ==PC?true:false);
        m_nUserID = id;

        if(id)
        {
            int size;
            TcpServer *server;
            Platform plat = ePlat()==PC?Android:PC;
            switch(plat)
            {
            case PC:
                size = m_lPCThreadFd.size();
                break;
            case Android:
                size = m_lAndThreadFd.size();
                break;
            case NoSet:
                size = m_lthreadFd.size();
                break;
            }

            for(int n=0;n<size;n++)
            {
                server =findUserIDFromList(id,&n,plat);
//                std::cout<<"id:"<<id<<std::endl;
//                std::cout<<"bActivation:"<<server->bActivation()<<std::endl;
                if(server!=nullptr&&server->bActivation()==true)
                {
                    server->sendCmd(CT_AUTHORIZEDSIGNUP,1,"其他设备要求登陆该账号,是否同意");
                    this->setBAuthorized(true);
                    return;
                }
            }
        }
        else
        {
            sendCmd(CT_AUTHORIZEDSIGNUP,0,"用户未登录");
        }
        break;
    }
    case CT_AUTHORIZEDSIGNUPRESULT:
    {
        int size;
        TcpServer *server;
        size = m_lthreadFd.size();

        for(int n=0;n<size;n++)
        {
            server =findUserIDFromList(this->m_dbMan.nUserID(),&n,NoSet);
            //std::cout<<"id:"<<this->m_dbMan.nUserID()<<std::endl;
            //std::cout<<"authorized:"<<server->bAuthorized()<<std::endl;
            if(server!=nullptr&&server->bAuthorized()==true)
            {
                server->setBAuthorized(false);

                server->m_dbMan.setUserID(this->m_dbMan.nUserID());
                bool result = atoi(arg[0]);
                if(result)
                {
                    server->setLoginSuccessMsg();
                    server->m_dbMan.authorizedStateChanged(server->ePlat()==PC?true:false,true,server->m_nUserID);
                }
                server->sendCmd(CT_AUTHORIZEDSIGNUPRESULT,result,result?"登陆成功":"拒绝授权");

                return;
            }
        }
        break;
    }
    case CT_USERSLOGINELSEWHERE:
    {
        break;
    }
    case CT_PARACHECKACCOUNTNUMBER:
    {
        if(m_dbMan.checkAccountNumber(arg[0]))
        {
            sendCmd(CT_PARACHECKACCOUNTNUMBER,true,"not error");
        }
        else
        {
            sendCmd(CT_PARACHECKACCOUNTNUMBER,false,"User already exists");
        }
        break;
    }
    case CT_EMAILCODE:
    {
        const char *email = m_dbMan.getEmail(arg[0]);
        if(strlen(email)==0)
        {
            sendCmd(CT_EMAILCODE,false,"user not found");
            return;
        }

        SMTP smtp;
        char data[400];
        sprintf(data,"亲爱的用户:\n"
                     "您好!\n"
                     "感谢你使用LWMAMS服务.您正在进行邮箱验证,请在验证码输入框输入此验证码:\n"
                     "%s 以完成验证\n"
                     "为了账号安全请不要泄露该验证码\n"
                     "如非本人操作,请忽略此邮件,由此给您带来的不便请谅解!\n"
                     "LWMAMS开发人员",arg[1]);
        smtp.sendEmail(email,data,"LWMAMS邮箱身份验证");
        break;
    }
    case CT_CHANGEPASSWORD:
    {
        if(m_dbMan.changePW(arg[0],arg[1]))
        {
            sendCmd(CT_CHANGEPASSWORD,true,"not error");
        }
        else
        {
            sendCmd(CT_CHANGEPASSWORD,false,"registration failed");
        }
        break;
    }
    case CT_REGISTERED:
    {
        if(count<4)
        {
            sendCmd(CT_REGISTERED,false,"Too few parameters");
            break;
        }
        if(m_dbMan.registered(arg[0],arg[1],arg[2],arg[3]))
        {
            sendCmd(CT_REGISTERED,true,"not error");
        }
        else
        {
            sendCmd(CT_REGISTERED,false,"registration failed");
        }
        break;
    }
    case CT_DATASHOW:
    case CT_ROUTE:
    case CT_CONTROL:
    {
        if(count<3)
            sendMsg(cmd,atoi(arg[0]),atoi(arg[1]));
        else
            sendMsg(cmd,atoi(arg[0]),atoi(arg[1]),atoi(arg[2]),arg[3][0]-'0',arg[4][0],arg[5][0],arg[6]);
        break;
    }
    case CT_STATISTICS:
    {
        if(count<10)
            return;
        char msg[512];
        msg[0]=static_cast<char>(CT_STATISTICS);
        msg[1]='$';
        m_dbMan.findStatisticsDataAndSend(arg[0],arg[1],arg[2],arg[3],
                                                                        atof(arg[4]),atof(arg[5]),
                                                                        atof(arg[6]),atof(arg[7]),
                                                                        atof(arg[8]),atof(arg[9]),
                                                                        msg+2);

        int num=write(m_nSocketFd, msg, strlen(msg));
        std::cout<<"写入成功字节:"<<num<<std::endl;
        break;
    }
    case CT_ANDROIDDATASHOW:
    {
        sendMsg(cmd,atoi(arg[0]),atoi(arg[1]));
        break;
    }
    case CT_SMSEMAILPUSH:
    {
        sendSMSAndEmail(arg[0][0]-'0',arg[1][0]-'0',arg[2][0]-'0',arg[3][0]-'0',arg[4][0]-'0',arg[5],arg[6]);
        break;
    }
    default:
        break;
    }
}

/**
   * @函数意义:连接成功时调用此函数设置客户端地址
   * @作者:ZM
   * @date 2018-1
   */
void TcpServer::setClientIP()
{
    inet_ntop(AF_INET,&m_sclientAddr.sin_addr.s_addr,m_cipstr,16);
}

/**
   * @函数意义:关闭Socket通道,这是服务器自主断开连接,被动断开则在读取数据那里判断
   * @作者:ZM
   * @date 2018-1
   */
void TcpServer::closeStocket()
{
    close(this->m_nSocketFd);
}

/**
   * @函数意义:从list中移除已关闭的线程标示符
   * @作者:ZM
   * @param [in] fd
   *                线程对象指针
   * @param [in] plat
   *                        相应的平台
   * @date 2018-1
   */
void TcpServer::listRemove(TcpServer* fd,Platform plat)
{
    pthread_mutex_lock(&TcpServer::m_sMutex);//上锁
    switch(plat)
    {
    case PC:
        TcpServer::m_lPCThreadFd.remove(fd);
        break;
    case Android:
        TcpServer::m_lAndThreadFd.remove(fd);
        break;
    case NoSet:
        TcpServer::m_lthreadFd.remove(fd);
        break;
    }
    pthread_mutex_unlock(&TcpServer::m_sMutex);//解锁
}

/**
   * @函数意义:从list中添加新建的线程标示符
   * @作者:ZM
   * @param [in] fd
   *                线程对象指针
   * @param [in] plat
   *                        相应的平台
   * @date 2018-1
   */
void TcpServer::listCreate(TcpServer* fd,Platform plat)
{
    pthread_mutex_lock(&TcpServer::m_sMutex);//上锁
    switch(plat)
    {
    case PC:
        TcpServer::m_lPCThreadFd.push_back(fd);
        break;
    case Android:
        TcpServer::m_lAndThreadFd.push_back(fd);
        break;
    case NoSet:
        TcpServer::m_lthreadFd.push_back(fd);
        break;
    }
    pthread_mutex_unlock(&TcpServer::m_sMutex);//解锁
}

/**
   * @函数意义:用户登陆后,将fd移到相应的list,到时候授权登陆方便
   * @作者:ZM
   * @param [in] fd
   *                        线程对象指针
   * @param [in] plat
   *                        相应的平台
   * @date 2018-2
   */
void TcpServer::threadListChanged(TcpServer *fd,Platform plat)
{
    //移除自己
    listRemove(fd);
    //添加自己
    listCreate(fd,plat);
}

/**
   * @函数意义:用户退出,退出原因时由于该用户在其他地方登陆
   * @作者:ZM
   * @param [in] userID
   *                        通过这个用户id找出server对象,通知其退出
   * @param [in] plat
   *                        平台,不同的平台操作不同的list
   * @date 2018-1
   */
void TcpServer::userLogout(int userID,Platform plat)
{
    TcpServer *server;

    int size = 0;
    switch(plat)
    {
    case PC:
        size = m_lPCThreadFd.size();
        break;
    case Android:
        size = m_lAndThreadFd.size();
        break;
    case NoSet:
        size = m_lthreadFd.size();
        break;
    }

    for(int n=0;n<size;n++)
    {
        server =findUserIDFromList(userID,&n,plat);
        //std::cout<<n<<std::endl;
        //std::cout<<"bActivation:"<<server->bActivation()<<std::endl;
        if(server!=nullptr&&server->bActivation()==true)
        {
            server->setServerState(false);
            server->sendCmd(CT_USERSLOGINELSEWHERE,false,"The user has logged in somewhere else");
        }
    }
}

/**
   * @函数意义:用户退出,退出原因时由于该用户在客户端退出,
   * @作者:ZM
   * @param [in] server
   *                       退出的对象
   * @date 2018-1
   */
void TcpServer::userLogout(TcpServer *server)
{
    //未登录用户和被挤下用户退出也会调用,所以需要判断是否时正常登陆用户
    if(server->bActivation())
        server->m_dbMan.userLogout(server->m_ePlat==PC?true:false);
}

/**
   * @函数意义:从给出的位置开始查找相应的位置,返回第一个找到的对象
   * @作者:ZM
   * @param [in] userID
   *                        用户的ID
   * @param [in] startNum
   *                        开始查找的位置,设为指针是为了让元素的位置得到记录
   * @param [in] plat
   *                        所在的平台
   * @date 2018-1
   */
TcpServer* TcpServer::findUserIDFromList(int userID,int *startNum,Platform plat)
{
//    if(*startNum>=TcpServer::m_lthreadFd.size())
//        return nullptr;
    pthread_mutex_lock(&TcpServer::m_sMutex);//上锁
    TcpServer *server;
    std::list<TcpServer*>::iterator it;
    std::list<TcpServer*>::iterator end;

    switch(plat)
    {
    case PC:
        it = TcpServer::m_lPCThreadFd.begin();
        end = TcpServer::m_lPCThreadFd.end();
        break;
    case Android:
        it = TcpServer::m_lAndThreadFd.begin();
        end = TcpServer::m_lAndThreadFd.end();
        break;
    case NoSet:
        it = TcpServer::m_lthreadFd.begin();
        end = TcpServer::m_lthreadFd.end();
        break;
    }

    int n=0;
    for(;it!=end;it++,n++)
    {
        if(n<*startNum)
            continue;
        server = *it;
        if(server->m_nUserID == userID)
        {
            pthread_mutex_unlock(&TcpServer::m_sMutex);//解锁
            return server;
        }
    }
    pthread_mutex_unlock(&TcpServer::m_sMutex);//解锁
    return nullptr;
}

bool TcpServer::bAuthorized() const
{
    return m_bAuthorized;
}

void TcpServer::setBAuthorized(bool bAuthorized)
{
    m_bAuthorized = bAuthorized;
}


TcpServer::Platform TcpServer::ePlat() const
{
    return m_ePlat;
}

void TcpServer::setEPlat(const Platform &ePlat)
{
    m_ePlat = ePlat;
}


bool TcpServer::bActivation() const
{
    return m_bActivation;
}
