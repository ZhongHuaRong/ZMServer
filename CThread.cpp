#include "CThread.h"
#include <stdio.h>
#include <algorithm>

CThread::CThread() :
    m_pthreadFd(0),
    runFunction(nullptr),
    m_prunArg(nullptr)
{
}

CThread::CThread(void *(*func)(void *), void *arg):
    m_pthreadFd(0),
    runFunction(func),
    m_prunArg(arg)
{
    create();
}

CThread::~CThread()
{

}

/**
   * @函数意义:创建子线程并返回线程的文件标示符,可以通过setRunFunction设置调用的函数
   * @作者:ZM
   * @date 2018-1
   */
pthread_t CThread::create()
{
    if(-1==this->create(this->runFunction,this->m_prunArg))
    {
        perror("Sub-thread fails to open");
        return -1;
    }
    else
        return m_pthreadFd;
}

/**
   * @函数意义: create()的重载函数
   * @作者:ZM
   * @param [in] func
   *                线程运行的函数,由父类传入,传入空指针则调用runFunction
   * @param [in] arg
   *                func函数的参数
   * @date 2018-1
   */
pthread_t CThread::create(void *(*func)(void *), void *arg)
{
    if(nullptr == func)
    {
        if(this->runFunction==nullptr)
        {
            perror("Function pointer is empty, thread generation failed");
            return -1;
        }
        else
            func = this->runFunction;
    }

    pthread_attr_init(&m_pattr);
    pthread_attr_setdetachstate(&m_pattr,PTHREAD_CREATE_DETACHED);
    int err = pthread_create(&this->m_pthreadFd,&m_pattr,func,arg);
    if(err!=0)
        return -1;
    else
        return m_pthreadFd;
}

pthread_t CThread::getThreadFd() const
{
    return m_pthreadFd;
}


