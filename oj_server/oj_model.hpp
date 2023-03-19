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
#include "../comm/exception.hpp"
#include "../comm/redis.hpp"
#include <nlohmann/json.hpp>
using json = nlohmann::json;
// 将所有的题目加载到内存中
namespace ns_model
{
    using namespace ns_util;
    using namespace ns_log;
    using namespace ns_exception;
    string host = "127.0.0.1";
    int port = 3306;
    string db = "oj"; // 选择数据库
    string user = "oj_client";
    string passwd = "@123456!!";
    // 在这里一开始就把所有的已经注册过的用户的用户名，全部都导入到布隆过滤器中
    class Model
    {
    private:
        Mysql m;
        BloomFilter<1000> registered_users; // 已经注册过的用户
        Myredis redis;                      // 使用自己封装的redis，不需要使用redis的构造函数
    public:
        Model()
            : m(host, port, db, user, passwd)

        {
            // 在构造函数的时候就要加载
            // 把users中的所有数据都加载进去
            load_mysql_to_redis();
        }
        ~Model()
        {
        }

        bool exportUserToRedis_Bloom() // 把数据导入到redis和bloom中
        {
            string sql = "select username,passwd from users";
            vector<vector<string>> users;
            try
            {
                if (m.Select(sql, users))
                {
                    // 获得了数据
                    for (int i = 0; i < users.size(); i++)
                    {
                        string username = users[i][0];
                        string password = users[i][1];

                        registered_users.set(username); // 将所有的用户名都添加到布隆过滤器中
                        // 添加到一个hash里面
                        // 使用的key 是user：username
                        // map<string, string> fields;
                        // fields["username"] = username;
                        // fields["password"] = password;
                        // string key = "user:" + username;
                        string command = "hmset user:" + username + " username " + username + " password " + password;

                        // redis.hmset(key, fields);

                        redis.adddata(command);
                    }
                }
                else
                {
                    throw SqlException(LogHeader(ERROR), "加载用户名失败", sql);
                }
                return true;
            }
            catch (const Exception &e)
            {
                cout << e.what() << endl;
            }
            return false;
        }
        void load_mysql_to_redis() // 把mysql的user数据导入到redis里面
        {
            try
            {
                exportUserToRedis_Bloom(); // 把mysql中的redis
                // exportQuestionToRedis();//把题目导入到redis中，大文本文件使用redis很不好用，之后使用mongodb来进行操作修改
            }
            catch (const Exception &e)
            {
                cout << e.what() << endl;
            }
        }
        // 加载所有的题目
        // bool exportQuestionToRedis()
        // {
        //     const string sql = "select * from oj_question;";

        //     vector<vector<string>> data;
        //     if (m.Select(sql, data))
        //     {
        //         for (int i = 0; i < data.size(); i++)
        //         {
        //             vector<pair<string, string>> fields;
        //             fields.emplace_back(make_pair("number", data[i][0]));   // 获取第一列的数据
        //             fields.emplace_back(make_pair("title", data[i][1]));    // 获取第一列的数据
        //             fields.emplace_back(make_pair("star", data[i][2]));     // 获取第一列的数据
        //             fields.emplace_back(make_pair("desc", data[i][3]));     // 获取第一列的数据
        //             fields.emplace_back(make_pair("headr", data[i][4]));    // 获取第一列的数据
        //             fields.emplace_back(make_pair("tail", data[i][5]));     // 获取第一列的数据
        //             fields.emplace_back(make_pair("cpulimit", data[i][6])); // 获取第一列的数据
        //             fields.emplace_back(make_pair("memlimit", data[i][7])); // 获取第一列的数据
        //             // fields["title"] = data[i][1];
        //             // fields["star"] = data[i][2];
        //             // fields["desc"] = data[i][3];
        //             // fields["header"] = data[i][4];

        //             // fields["tail"] = data[i][5];
        //             // fields["cpulimit"] = data[i][6];

        //             // fields["memlimit"] = data[i][7];

        //             string key = "question:" + data[i][0];
        //             redis.hmsetforquestion(key, fields);
        //         }
        //         return true;
        //     }

        //     return false;
        // }
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

                    out.push_back(move(q)); // 移动构造，直接把q的值放进去不弄一个新的对象出来
                }
                return true;
            }

            return false;
        }
        bool GetAllQuestion(vector<Question> &out) // 获得所有题目再根据登陆人的身份查看自己的题库
        {
            const string sql = "select * from oj_question;";
            // const string sql="select * from some_question";
            if (QueryQuestion(sql, out))
            {
                return true;
            }
            else
            {
                throw SqlException(LogHeader(ERROR), "获得题库失败", sql);
                return false;
            }
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
            else
            {
                throw SqlException(LogHeader(ERROR), "获得题：" + number + "失败", sql);
            }
            return res;
        }
        bool Query(const string &sql, vector<UserInfo> &userlist) // 在数据库中加载机器
        {
            if (m.Query(sql))
            {
                // LOG(ERROR) << sql << endl;
                return true;
            }
            return false;
        }

        bool Register(const string &injson, string *out) // 注册，在布隆过滤器和redis里面进行两次对数据进行过滤
        {
            auto body = json::parse(injson);
            string username = body["username"];
            string password = body["password"];

            UserInfo u(username, password); // 把发送过来的json串进行反序列化
            // insert into users (username,passwd) values (11,'123');
            if (registered_users.test(u.username))
            {
                // 别人要注册的用户名已经被人给注册了所以，就注册失败
                *out = "注册失败，用户名已经存在";
                return false;
            }
            else
            {
                // 这地方先使用redis进行缓存判断一下
                // 如果缓存中得到了，就不需要再使用mysql
                // string hkey = "user:" + u.username; // 先得到需要查询的key
                string command = "exists user:" + u.username; // 获得用户的密码
                if (redis.exists(command))
                {
                    // redis缓存里面如果找到了,说明该用户已经被注册过了
                    *out = "注册失败，用户名已经存在";
                    return false;
                }

                else
                {
                    // redis里面也没找到这个用户，所以就可以执行添加
                    //  要注册的用户名并没有被人给注册过
                    //  所以可以执行sql语句
                    string sql = "insert into users (username,passwd) values (" + u.username + ","
                                                                                               "'" +
                                 u.passwd + "'"
                                            ");";

                    // string sql = "insert into users (username,passwd) values ("+u.username+","+u.passwd+");";
                    vector<UserInfo> userlist;
                    if (Query(sql, userlist))
                    {
                        *out = "注册成功";
                        // 如果注册成功，就要把新加进来的这个用户名添加到布隆过滤器中
                        registered_users.set(u.username);
                        // 这个地方也需要往redis里面缓存添加新的数据
                        map<string, string> fields;
                        fields["username"] = u.username;
                        fields["password"] = u.passwd;
                        string command = "hmset user:" + u.username + " username " + u.username + " password " + u.passwd;
                        redis.adddata(command);
                        return true;
                    }
                    else
                    {
                        *out = "注册失败";
                        throw SqlException(LogHeader(ERROR), "注册失败", sql);

                        return false;
                    }
                }
            }
        }
        bool Load(const string &injson, string *out) // 登陆
        {

            // UserInfo u = JsonUtil::UserInfoDeSerialize(injson);
            auto body = json::parse(injson);
            string username = body["username"];
            string password = body["password"];

            UserInfo u(username, password); // 把发送过来的json串进行反序列化
            if (!registered_users.test(u.username))
            {
                // 别人要注册的用户名已经被人给注册了所以，就注册失败
                *out = "登陆失败，用户名不存在";
                return false;
            }
            else
            {
                // 再使用redis进行一个过滤
                string command = "hget user:" + u.username + " password";
                string password = redis.singledata(command);
                if (!password.empty() || u.passwd == password)
                {
                    // load failed
                    *out = "登陆成功";
                    return true;
                }
                else
                {
                    // 登陆失败。使用mysql进行操作
                    //  用户名存在，看看密码是否正确
                    string sql = "select username,passwd from users where username=";
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
                            *out = "登陆失败,密码错误";
                            LOG(WARNING) << sql << endl;
                            return false;
                        }

                        *out = "登陆成功";
                        return true;
                    }
                    else
                    {
                        // sql语句失败
                        LOG(ERROR) << sql << endl;
                        *out = "登陆失败";
                        throw SqlException(LogHeader(ERROR), "登陆失败", sql);

                        return false;
                    }
                }
            }
        }
        bool TopicAdd(const string &injson, string *out) // 添加一个题目
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