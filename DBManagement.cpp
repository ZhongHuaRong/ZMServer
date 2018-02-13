#include "DBManagement.h"
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <math.h>
#include <stdlib.h>

#define DBROOT "wbh"
#define DBIP "119.29.243.183"
#define DBPW "577253577253"
#define DBNAME "boat"

DBManagement::DBManagement():
    m_nUserID(-1)
{
    initMysql();
}

DBManagement::~DBManagement()
{
    destroy();
}

/**
   * @函数意义:初始化数据库,一般在类初始化的时候已经初始化
   * @作者:ZM
   * @date 2018-1
   */
void DBManagement::initMysql()
{
    if(mysql_init(&m_mysql)==NULL)
    {
            perror("Cannot init mysql!\n");
            return ;
    }
    if(mysql_real_connect(&m_mysql,DBIP,DBROOT,DBPW,DBNAME,0,NULL,0)==NULL)
    {
        std::cout<<mysql_error(&m_mysql)<<std::endl;
        return ;
    }
    setColumnName();
    std::cout<<"Connect successfuly!\n";
}

/**
   * @函数意义:释放数据库的资源,一般在类析构的时候已经初始化
   * @作者:ZM
   * @date 2018-1
   */
void DBManagement::destroy()
{
    mysql_close(&m_mysql);
}

/**
   * @函数意义:用户登录验证,返回成功或者失败
   *                      加点有意思的东西,先查找用户名,如果存在则对比密码,返回对应的错误码
   * @作者:ZM
   * @param [in] accountNumber
   *                        账号
   * @param [in] pw
   *                        密码
   * @date 2018-1
   */
DBManagement::SIGNUPERRORCODE DBManagement::signup(const char *accountNumber, const char *pw)
{
    char query[100];
    sprintf(query,"select * from User where accountNumber = '%s';",accountNumber);
    //std::cout<<query<<std::endl;
    mysql_real_query(&m_mysql,query,(unsigned int) strlen(query));
    m_pRes = mysql_store_result(&m_mysql);
    int rowCount;
    if(m_pRes)
     {
        rowCount = mysql_num_rows(m_pRes);//行

        if(rowCount!=0)
        {
            m_row = mysql_fetch_row(m_pRes);
            char * r=m_row[indexOfColumnName("password")];
            bool loggedin=*m_row[indexOfColumnName("signup")];
            //printf("loggedin:%d\n",*m_row[indexOfColumnName("signup")]);
            m_nUserID = string2int(m_row[indexOfColumnName("id")]);
            mysql_free_result(m_pRes);
            if(0==strcmp(r,pw))
            {
                if(loggedin)
                    return USERHASLOGGEDIN;
                else
                {
                    updateUserLoginState(true);
                    return NOERROR;
                }
            }
            else
            {
                return WRONGPASSWORD;
            }
        }
        else
        {
            mysql_free_result(m_pRes);
            return USERNOTFOUND;
        }
     }
    else
        return USERNOTFOUND;
}

/**
   * @函数意义:注册新用户
   * @作者:ZM
   * @param [in]    accountNumber
   *                        帐号名
   * @param [in] pw
   *                        密码
   * @param [in] userName
   *                        用户名
   * @date 2018-1
   */
bool DBManagement::registered(const char *accountNumber, const char *pw, const char *userName)
{
    char * sql = "set names \'utf8\'";
    mysql_query(&m_mysql, sql);

    char query[100];
    sprintf(query,"insert into User(accountNumber,password,userName) values('%s','%s','%s');",accountNumber,pw,userName);
    std::cout<<query<<std::endl;
    int ret=mysql_real_query(&m_mysql,query,(unsigned int) strlen(query));
    if(ret)
     {
        return false;
     }
    else
        return true;
}

/**
   * @函数意义:查找相应的数据
   * @作者:ZM
   * @param [in] returnData
   *                        保存查找的数据
   * @param [in] isControl
   *                        查找的数据是否是控制数据,是的话则添加两个状态位
   * @param [in] pageNum
   *                        页数
   * @param [in] pageRow
   *                        每页行数
   * @date 2018-1
   */
void DBManagement::findData(char *returnData, bool isControl, int pageNum, int pageRow)
{
    if(isControl)
    {
        char query[] = "select count(*) from control_data";
        char query2[] = "select * from control_data order by id desc limit %d,%d";
        pagingQuery(returnData,query,query2,pageNum,pageRow,true);
    }
    else
    {
        char query[] = "select count(*) from boat_data";
        char query2[] = "select * from boat_data order by id desc limit %d,%d";
        pagingQuery(returnData,query,query2,pageNum,pageRow);
    }
}

/**
   * @函数意义:查找相应的数据(附带查找功能)
   * @作者:ZM
   * @param [in] returnData
   *                        保存查找的数据
   * @param [in] tableName
   *                        需要查找的表
   * @param [in] pageNum
   *                        页数
   * @param [in] pageRow
   *                        每页行数
   * @param [in] type
   *                        需要查找的数据类型
   * @param [n] compare
   *                        比较符
   * @param [n] data
   *                        比较值
   * @date 2018-1
   */
void DBManagement::findDataAndCheck(char *returnData, int pageNum, int pageRow,
                                    DBManagement::DATATYPE type, DBManagement::DATACOMPARE compare, char * data)
{
    char query[150];

    char oper[5];
    switch(compare)
    {
    case MoreThan:
    {
        strcpy(oper,">");
        break;
    }
    case Equal:
    {
        strcpy(oper,"=");
        break;
    }
    case LessThan:
    {
        strcpy(oper,"<");
        break;
    }
    }

    char where[100];
    switch(type)
    {
    case AllData:
    {
        sprintf(where,"t_data %s '%s' and ph_data %s '%s' and tds_data %s '%s'",oper,data,oper,data,oper,data);
        break;
    }
    case Temperature:
    {
        sprintf(where,"t_data %s '%s'",oper,data);
        break;
    }
    case PH:
    {
        sprintf(where,"ph_data %s '%s'",oper,data);
        break;
    }
    case Turbidity:
    {
        sprintf(where,"tds_data %s '%s'",oper,data);
        break;
    }
    }
    sprintf(query,"select count(*) from boat_data where %s",where);
    std::cout<<type<<std::endl;
    std::cout<<compare<<std::endl;

    char query2[150];
    sprintf(query2,"select * from boat_data where %s order by id desc limit ",where);
    strcat(query2,"%d,%d");
    pagingQuery(returnData,query,query2,pageNum,pageRow);
}

/**
   * @函数意义:查找测试用数据
   * @作者:ZM
   * @param [in]    returnData
   *                        存储返回的数据
   * @param [in] testFlag
   *                        跳跃行数
   * @date 2018-2
   */
void DBManagement::findTestData(char *returnData, int testFlag)
{
    int size=0;
    char query[150];

    strcpy(returnData,"14");
    size += 2;
    returnData[size++] = '$';

    sprintf(query,"select * from boat_data order by id desc limit %d,%d",testFlag,14);
    std::cout<<query<<std::endl;
    mysql_real_query(&m_mysql,query,(unsigned int) strlen(query));
    m_pRes = mysql_store_result(&m_mysql);
    if(m_pRes)
     {
        int  iTableRow = mysql_num_rows(m_pRes);//行
        int  iTableCol = mysql_num_fields(m_pRes);//列
         for(int i=0; i<iTableRow; i++)
         {
           m_row = mysql_fetch_row(m_pRes);
           char *r;
           for(int j=0; j<iTableCol; j++)
           {
                r=m_row[j];
                strcpy(returnData+size,r);
                size += strlen(r);
                returnData[size++] = '^';
           }
           returnData[size++] = '$';
         }
        mysql_free_result(m_pRes);
     }

    //std::cout<<"row:"<<returnData<<std::endl;
    returnData[size++] = '#';
    returnData[size++] = '\0';
}

/**
   * @函数意义:更新用户登陆状态信息,主要用在用户登陆成功时和用户退出时
   * @作者:ZM
   * @param [in] state
   *                用户状态
   * @date 2018-1
   */
void DBManagement::updateUserLoginState(bool state)
{
    char query[100];
    sprintf(query,"update User set signup = %d where id = %d;",state,m_nUserID);
    mysql_real_query(&m_mysql,query,(unsigned int) strlen(query));
}

/**
   * @函数意义:获取数据库的字段名
   * @作者:ZM
   * @date 2018-1
   */
void DBManagement::setColumnName()
{
    m_lColumnName.clear();
    char query[] ="SHOW FULL COLUMNS FROM User;";
    //std::cout<<query<<std::endl;
    mysql_real_query(&m_mysql,query,(unsigned int) strlen(query));
    m_pRes = mysql_store_result(&m_mysql);
    int rowCount;
    if(m_pRes)
     {
        rowCount = mysql_num_rows(m_pRes);//行

        for(int n=0;n<rowCount;n++)
        {
            m_row = mysql_fetch_row(m_pRes);
            m_lColumnName.push_back(std::string(m_row[0]));
        }
        mysql_free_result(m_pRes);
    }
}

/**
   * @函数意义:通过参数给出的两个sql查询语句,找出相应数据存储在returnData中
   * @作者:ZM
   * @param [in] returnData
   *                        外部提供存储数据的数组
   * @param [in] sqlSelectCount
   *                        用于查询数据所有行数的sql语句
   * @param [in] sqlSelectPage
   *                        用于查询相应页面以及行数的sql语句
   * @param [in] pageNum
   *                        用于异常判断
   * @param [in] pageRow
   *                        用于异常判断
   * @param [in] isControl
   *                        用于判断是否是控制数据,默认为false
   * @date 2018-1
   */
void DBManagement::pagingQuery(char *returnData, char *sqlSelectCount, char *sqlSelectPage,int pageNum,int pageRow,bool isControl)
{
    int size=0;
    char query[150];
    int count;
    std::cout<<sqlSelectCount<<std::endl;
    mysql_real_query(&m_mysql,sqlSelectCount,(unsigned int) strlen(sqlSelectCount));
    m_pRes = mysql_store_result(&m_mysql);
    if(m_pRes)
     {
        int rowCount = mysql_num_rows(m_pRes);//行

        if(rowCount!=0)
        {
            m_row = mysql_fetch_row(m_pRes);
            strcpy(returnData,m_row[0]);
            size += strlen(m_row[0]);
            returnData[size++] = '$';
            count = string2int(m_row[0]);
        }
        mysql_free_result(m_pRes);
     }
    else
    {
        returnData[size++] = '#';
        returnData[size++] = '\0';
        return;
    }

    std::cout<<"count:"<<count<<std::endl;

    //如果control为真则添加标志位
    if(isControl)
        controlFlag(returnData,size);

    if(pageRow==0)
    {
        returnData[size++] = '#';
        returnData[size++] = '\0';
        return;
    }
    int startRow;
    //防止出现各种特殊情况
    if(pageRow>count)
    {
        startRow=0;
    }
    else if(pageNum<0)
    {
        startRow =0;
    }
    else if(pageNum*pageRow+pageRow>count)
    {
        startRow = pageNum*pageRow;
    }
    else
    {
        startRow = pageNum*pageRow;
    }
    sprintf(query,sqlSelectPage,startRow,pageRow);
    std::cout<<query<<std::endl;
    mysql_real_query(&m_mysql,query,(unsigned int) strlen(query));
    m_pRes = mysql_store_result(&m_mysql);
    if(m_pRes)
     {
        int  iTableRow = mysql_num_rows(m_pRes);//行
        int  iTableCol = mysql_num_fields(m_pRes);//列
         for(int i=0; i<iTableRow; i++)
         {
           m_row = mysql_fetch_row(m_pRes);
           char *r;
           for(int j=0; j<iTableCol; j++)
           {
                r=m_row[j];
                strcpy(returnData+size,r);
                size += strlen(r);
                returnData[size++] = '^';
           }
           returnData[size++] = '$';
         }
        mysql_free_result(m_pRes);
     }

    //std::cout<<"row:"<<returnData<<std::endl;
    returnData[size++] = '#';
    returnData[size++] = '\0';
}

/**
   * @函数意义:为returnData添加标识信息
   * @作者:ZM
   * @param [in] returnData
   *                        存储返回的信息
   * @param [in] size
   *                        当前位置
   * @date 2018-2
   */
void DBManagement::controlFlag(char *returnData, int &size)
{
    char query[100];
    int count[4];

    for(int a=0;a<4;a++)
    {
        sprintf(query,"select * from control_data where content ='%d' order by id desc",a+1);
        mysql_real_query(&m_mysql,query,(unsigned int) strlen(query));
        m_pRes = mysql_store_result(&m_mysql);
        if(m_pRes)
         {
            int rowCount = mysql_num_rows(m_pRes);//行
            if(rowCount!=0)
            {
                m_row = mysql_fetch_row(m_pRes);
                count[a] = string2int(m_row[0]);
            }
            mysql_free_result(m_pRes);
         }
        else
        {
            count[a] = -1;
        }
    }

    char oxygen,water;
    if(count[0]>count[2])
        water = '1';
    else if(count[0]<count[2])
        water = '3';
    else
        water = '0';

    if(count[1]>count[3])
        oxygen = '2';
    else if(count[1]<count[3])
        oxygen = '4';
    else
        oxygen = '0';

    returnData[size++] = water;
    returnData[size++] = '$';
    returnData[size++] = oxygen;
    returnData[size++] = '$';
}

int DBManagement::nUserID() const
{
    return m_nUserID;
}

/**
   * @函数意义: 用户退出,将会修改数据库状态为false
   * @作者:ZM
   * @date 2018-1
   */
void DBManagement::userLogout()
{
    updateUserLoginState(false);
}

/**
   * @函数意义:检查账号是否存在
   * @作者:ZM
   * @param [in] account
   *                        账号
   * @date 2018-1
   */
bool DBManagement::checkAccountNumber(char *account)
{
    char query[100];
    sprintf(query,"select * from User where accountNumber = '%s';",account);
    std::cout<<query<<std::endl;
    mysql_real_query(&m_mysql,query,(unsigned int) strlen(query));
    m_pRes = mysql_store_result(&m_mysql);
    int rowCount;
    if(m_pRes)
     {
        rowCount = mysql_num_rows(m_pRes);//行
        mysql_free_result(m_pRes);
        if(rowCount!=0)
        {
            return false;
        }
    }
    return true;
}

/**
   * @函数意义:数字字符串转成int,因为mysql Int型得到char*型
   * @作者:ZM
   * @param [in] str
   *                        将要转换的字符串
   * @date 2018-1
   */
int DBManagement::string2int(char * str)
{
    int n = strlen(str);
    int num=0;
    for(int m=0;m<n;m++)
    {
        num+=(str[m]-'0')*pow(10,n-m-1);
    }
    return num;
}

/**
   * @函数意义:给出字段名返回所在位置
   * @作者:ZM
   * @param [in] name
   *                        字段名
   * @date 2018-1
   */
int DBManagement::indexOfColumnName(std::string name)
{
    std::list<std::string>::iterator it;
    int n=0;
    for(it = m_lColumnName.begin();it!=m_lColumnName.end();it++,n++)
    {
        if(name.compare(*it)==0)
            return n;
    }
    return -1;
}

