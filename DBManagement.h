#ifndef DBMANAGEMENT_H
#define DBMANAGEMENT_H

#include <mysql/mysql.h>
#include <list>
#include <string>

class DBManagement
{
public:
    enum SIGNUPERRORCODE{
        NOERROR = 0x00,
        USERNOTFOUND =0x01,
        WRONGPASSWORD,
        USERHASLOGGEDIN
    };

    enum DATATYPE{
        AllData = 1,
        Temperature,
        PH,
        Turbidity
    };
    enum DATACOMPARE{
        MoreThan = 1,
        Equal,
        LessThan
    };

public:
    DBManagement();
    ~DBManagement();

    void initMysql();

    void destroy();

    SIGNUPERRORCODE signup(const bool plat,const char *accountNumber,const char *pw);
    const char * getNewUserName(const char *accountNumber);
    const char * getEmail(const char *accountNumber);
    char *getUserName();
    void authorizedStateChanged(const bool plat,const bool state,int userID);
    bool registered(const char *accountNumber,const char *pw,const char *userName,const char *email);
    bool changePW(const char *accountNumber,const char *pw);
    void findData(char *returnData,bool isControl,int pageNum,int pageRow);
    void findDataAndCheck(char *returnData,int pageNum,int pageRow,DATATYPE type, DATACOMPARE compare,char * data);
    void findTestData(char *returnData,int testFlag);
    void findStatisticsDataAndSend(const char *datatype,
                                                            const char *dateType,
                                                            char *dateTime,
                                                            const char *showType,
                                                            const float &tempMax,
                                                            const float &tempMin,
                                                            const float &phMax,
                                                            const float &phMin,
                                                            const float &turMax,
                                                            const float &turMin,
                                                            char * msg);
    int getCountFromSql(const char* sql);
    int nUserID() const;
    void setUserID(int id);

    void userLogout(const bool plat);

    bool checkAccountNumber(char *account);
    int checkAnotherPlatformExits(const char *account,bool isPC);
    bool checkAnotherPlatformExits(int id,bool isPC);

    int string2int(char * str);
    const char * int2str(const int &int_temp,char *string_temp);
    char * myStrsep( char *str,char ch);

    int indexOfColumnName(std::string name);
private:
    void updateUserLoginState(const bool plat,bool state);
    void setColumnName();

    void pagingQuery(char *returnData,char *sqlSelectCount,char *sqlSelectPage,int pageNum,int pageRow,bool isControl = false);
    void controlFlag(char *returnData,int &size);
private:
     MYSQL m_mysql;
     MYSQL_RES *m_pRes;
     MYSQL_ROW m_row;

     //在signup函数中返回的用户id,不过只能返回一个参数,所以这个用来临时保存用户的id,通过getUserID获取
     int m_nUserID;

     //用户名
     char m_sUserName[50];
     //临时保存将要返回的邮箱
     char m_sEmail[30];

    std::list<std::string> m_lColumnName;
};

#endif // DBMANAGEMENT_H
