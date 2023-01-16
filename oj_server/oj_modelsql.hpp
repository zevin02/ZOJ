#pragma once

// 这两个内容是通过文件的编号来产生关联的
// oj从用户获得的代码中，将提交上来的代码和对应的测试用户进行拼接，之后再交给compile_run
#include <iostream>
#include <string>
#include <unordered_map>
using namespace std;
#include <cassert>
#include "../comm/log.hpp"
#include <vector>
#include "../comm/util.hpp"
#include <mysql/mysql.h>
// 将所有的题目加载到内存中
namespace ns_model
{
    using namespace ns_util;
    using namespace ns_log;

    string host = "127.0.0.1";
    int port = 3306;
    string db = "oj"; // 选择数据库
    string user = "oj_client";
    string passwd = "@123456!!";

    class Model
    {
    private:
        Mysql m;

    public:
        Model()
            : m(host, port, db, user, passwd)

        {
        }
        ~Model()
        {
        }
        // 加载所有的题目
        bool QueryQuestion(const string sql, vector<Question> &out)
        {

            vector<vector<string>> data;
            if (m.Select(sql, data))
            {
                for (int i = 0; i < data.size(); i++)
                {
                    Question q;
                    q.number = data[i][0]; // 获取第一列的数据
                    q.title = data[i][1];
                    q.star = data[i][2];
                    q.desc = data[i][3];
                    q.header = data[i][4];

                    q.tail = data[i][5];

                    q.cpulimit = stoi(data[i][6]);

                    q.memlimit = stoi(data[i][7]);

                    out.push_back(move(q));//移动构造，直接把q的值放进去不弄一个新的对象出来

                }
                return true;
            }

     
            return false;
        }
        bool GetAllQuestion(vector<Question> &out) // 获得所有题目再根据登陆人的身份查看自己的题库
        {
            const string sql = "select * from oj_question;";
            // const string sql="select * from some_question";
            return QueryQuestion(sql, out);
        }
        bool GetAQuestion(string number, Question &q) // 获得一个题目
        {
            bool res = false;
            string sql = "select * from oj_question where number=";
            sql += number;
            sql += ";";
            vector<Question> out;
            if (QueryQuestion(sql, out))
            {
                if (out.size() == 1)
                {
                    q = out[0];
                    res = true;
                }
            }
            return res;
        }
        bool Query(const string &sql, vector<UserInfo> &userlist) // 在数据库中加载机器
        {
            if (!m.Query(sql))
            {
                LOG(ERROR) << sql << endl;
                return false;
            }
            return true;
        }

        bool Register(const string &injson, string *out) // 注册
        {

            UserInfo u = JsonUtil::UserInfoDeSerialize(injson);
            // insert into users (username,passwd) values (11,'123');

            string sql = "insert into users (username,passwd) values (" + u.username + ","
                                                                                       "'" +
                         u.passwd + "'"
                                    ");";
            // string sql = "insert into users (username,passwd) values ("+u.username+","+u.passwd+");";
            vector<UserInfo> userlist;
            if (Query(sql, userlist))
            {
                *out = "注册成功";
                return true;
            }
            else
            {
                *out = "注册失败";
                return false;
            }
        }
        bool Load(const string &injson, string *out) // 登陆
        {
            UserInfo u = JsonUtil::UserInfoDeSerialize(injson);

            string sql = "select * from users where username=";
            sql += u.username;
            sql += " and passwd=";
            sql += "'";
            sql += u.passwd;
            sql += "'";
            sql += ";";
            vector<vector<string>> data;

            if (m.Select(sql, data))
            {
                if (data.empty())
                {
                    // 没有数据，说明登陆失败
                    *out = "登陆失败";
                    LOG(ERROR) << sql << endl;
                    return true;
                }
                *out = "登陆成功";
                return true;
            }
            else
            {
                LOG(ERROR) << sql << endl;
                *out = "登陆失败";
                return false;
            }
        }
        bool TopicAdd(const string &injson, string *out)
        {
            // 添加题目
            Question q = JsonUtil::QuestionDeSerialize(injson); // 获得序列化的题目
            // insert into oj_question ()
            // string sql="insert into "
            return true;
        }

        bool GetInfo(const string &sql)
        {
            vector<vector<string>> data;
            if (m.Select(sql, data))
            {
                for (int i = 0; i < data.size(); i++)
                {
                    for (int j = 0; j < data[0].size(); j++)
                    {
                        cout << data[i][j] << " ";
                    }
                    cout << endl;
                }
                return true;
            }
            else
            {
                LOG(ERROR) << sql << endl;
                return false;
            }
        }
    };

};