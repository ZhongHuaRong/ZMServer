#include "TcpServer.h"
#include <string.h>
//#include <stdio.h>
#include<algorithm>
#include <iostream>
#include <stdlib.h>

std::list<TcpServer *> TcpServer::m_lthreadFd;
pthread_mutex_t TcpServer::m_sMutex;
int TcpServer::m_nlistenFd;
struct sockaddr_in TcpServer::m_slistenAddr;

TcpServer::TcpServer():
    m_bActivation(false)
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
    char buf[50];
    while(1)
    {
        memset(buf,'0',50);
        count=read(server->m_nSocketFd,buf,50);
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
    listRemove(server);
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
    return m_lthreadFd.size();
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
     char *head;
     char *arg[argCount];
     while(cmd&&n!=argCount+1)
      {
        key_point = strsep(&cmd,&ch);
        if(n==0)
        {
            head = key_point;
        }
        else
        {
             arg[n-1] = key_point;
        }
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
void TcpServer::sendCmd(TcpServer::CommandType cmd, bool result, const char *msg)
{
    int msgLength = strlen(msg);
    //长度:前四位命令+msg长度+分隔符'$'+结束符'#'+字符串结尾'\0'
    char data[msg!=nullptr?msgLength+1:0+4+1+1];
    data[0] = cmd;
    data[1] ='$';
    data[2] = result?'1':'0';
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
    char msg[10+60*pageRow];
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

    std::cout<<this->bActivation()<<std::endl;
    std::cout<<count<<std::endl;
    //用户未激活且CT不是登陆则不判断了,未激活只允许服务器发命令和只接受登陆,注册指令
    if(cmd!=CT_SIGNUP&&cmd!=CT_SIGNUPAUTO&&cmd!=CT_PARACHECKACCOUNTNUMBER&&cmd!=CT_REGISTERED)
        if(!this->bActivation())
            return;
    switch(cmd)
    {
    case CT_SIGNUP:
    case CT_SIGNUPAUTO:
    {
        DBManagement::SIGNUPERRORCODE code =m_dbMan.signup(arg[0],arg[1]);
        const char *str;
        bool result;
        switch(code)
        {
        case DBManagement::NOERROR:
            str="sign up success";
            result =true;
            setServerState(true);
            m_nUserID = m_dbMan.nUserID();
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
            TcpServer::userLogout(m_nUserID);

            setServerState(true);
            break;
        default:
            result =false;
            str="unknow error";
        }
        sendCmd(cmd,result,str);
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
    case CT_REGISTERED:
    {
        if(count<3)
        {
            sendCmd(CT_REGISTERED,false,"Too few parameters");
            break;
        }
        if(m_dbMan.registered(arg[0],arg[1],arg[2]))
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
    case CT_STATISTICS:
    case CT_CONTROL:
    {
        if(count<3)
            sendMsg(cmd,atoi(arg[0]),atoi(arg[1]));
        else
            sendMsg(cmd,atoi(arg[0]),atoi(arg[1]),atoi(arg[2]),arg[3][0]-'0',arg[4][0],arg[5][0],arg[6]);
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
   * @date 2018-1
   */
void TcpServer::listRemove(TcpServer* fd)
{
    pthread_mutex_lock(&TcpServer::m_sMutex);//上锁
    TcpServer::m_lthreadFd.remove(fd);
    pthread_mutex_unlock(&TcpServer::m_sMutex);//解锁
}

/**
   * @函数意义:从list中添加新建的线程标示符
   * @作者:ZM
   * @param [in] fd
   *                线程对象指针
   * @date 2018-1
   */
void TcpServer::listCreate(TcpServer* fd)
{
    pthread_mutex_lock(&TcpServer::m_sMutex);//上锁
    TcpServer::m_lthreadFd.push_back(fd);
    pthread_mutex_unlock(&TcpServer::m_sMutex);//解锁
}

/**
   * @函数意义:用户退出,退出原因时由于该用户在其他地方登陆
   * @作者:ZM
   * @param [in] userID
   *                        通过这个用户id找出server对象,通知其退出
   * @date 2018-1
   */
void TcpServer::userLogout(int userID)
{
    TcpServer *server;

    int size = m_lthreadFd.size();
    for(int n=0;n<size;n++)
    {
        server =findUserIDFromList(userID,&n);
        std::cout<<n<<std::endl;
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
        server->m_dbMan.userLogout();
}

/**
   * @函数意义:从给出的位置开始查找相应的位置,返回第一个找到的对象
   * @作者:ZM
   * @param [in] userID
   *                        用户的ID
   * @param [in] startNum
   *                        开始查找的位置,设为指针是为了让元素的位置得到记录
   * @date 2018-1
   */
TcpServer* TcpServer::findUserIDFromList(int userID,int *startNum)
{
    if(*startNum>=TcpServer::m_lthreadFd.size())
        return nullptr;
    pthread_mutex_lock(&TcpServer::m_sMutex);//上锁
    TcpServer *server;
    std::list<TcpServer*>::iterator it;
    int n=0;
    for(it = TcpServer::m_lthreadFd.begin();it!=TcpServer::m_lthreadFd.end();it++,n++)
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

bool TcpServer::bActivation() const
{
    return m_bActivation;
}
