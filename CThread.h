#ifndef CTHREAD_H
#define CTHREAD_H

#include <thread>

class CThread
{
public:
    CThread();
    explicit CThread(void *(*func)(void *),void *arg = nullptr);
    ~CThread();

    pthread_t create();
    pthread_t create(void *(*func)(void *),void *arg);

    void setRunFunction(void *(*func)(void *));

    pthread_t getThreadFd() const;
private:
    pthread_t m_pthreadFd;
    void *(*runFunction)(void *);
    void *m_prunArg;

    pthread_attr_t m_pattr;
};

#endif // CTHREAD_H
