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
    //这些常量在编译的时候就应该生成
    constexpr const char *host = "127.0.0.1";
    constexpr int port = 3306;
    constexpr const char *db = "oj"; // 选择数据库
    constexpr const char *user = "oj_client";
    constexpr const char *passwd = "@123456!!";
    // 在这里一开始就把所有的已经注册过的用户的用户名，全部都导入到布隆过滤器中
    class Model
    {
    private:
        //使用智能指针来维护数据库
        unique_ptr<MySQL> mysql;
        BloomFilter<1000> registeredUser; // 已经注册过的用户
        unique_ptr<MyRedis> redis;        // 使用自己封装的redis，不需要使用redis的构造函数
    public:
        Model()
        {
            // 在构造函数的时候就要加载
            // 把users中的所有数据都加载进去
            redis = unique_ptr<MyRedis>(new MyRedis);
            mysql = unique_ptr<MySQL>(new MySQL(host, port, db, user, passwd));
            loadMysql2Redis();
        }
        ~Model()
        {
            //修改
            //执行完之后，把相应的tasklist这个key给销毁掉,修改
            string command="del taskList";
            redis->del(command);
        }

        bool exportUser2Redis_Bloom() // 把用户数据导入到redis和bloom中
        {
            constexpr const char *sql = "select username,passwd from users"; //编译的时候就生成这个字符串
            vector<vector<string>> users;
            // 在redis里面判断一下我们这边
            try
            {
                if (mysql->Select(sql, users))
                {
                    // 获得了数据
                    for (int i = 0; i < users.size(); i++)
                    {
                        string username = users[i][0];
                        string password = users[i][1];

                        registeredUser.set(username); // 将所有的用户名都添加到布隆过滤器中
                        // 添加到一个hash里面
                        // 使用的key 是user：username
                        string command = "exists user:" + username;
                        if (!redis->exists(command))
                        {
                            //如果该机器不存在，就添加
                            command = "hmset user:" + username + " username " + username + " password " + password;
                            redis->addData(command);
                        }
                    }
                }
                else
                {
                    throw SqlException(LogHeader(ERROR), "Failed to load username", sql);
                }
                return true;
            }
            catch (const Exception &e)
            {
                cout << e.what() << endl;
            }
            return false;
        }
        bool exportTaskList2Redis()
        {
            //把mysql中的题目栏都导入到redis中
            //使用一个json(number,title,star)，tasklist,
            tuple<int, string, string> q;
            const string sql = "select number,title,star from oj_question;";
            vector<tuple<string, string, string>> out;
            //把这个结果序列化
            //会出现mysql的相同数据导入多次
            if (getQuestion(sql, out))
            {
                string command = "rpush taskList ";
                for (auto t : out)
                {
                    string value = JsonUtil::QuestionSerialize(t);
                    command += value;
                    command += " ";
                }
                //添加到redis中
                redis->addData(command);
                return true;
            }
            return false;
        }

        void loadMysql2Redis() // 把mysql的user数据导入到redis里面
        {
            try
            {
                exportUser2Redis_Bloom(); // 把mysql中的redis
                exportTaskList2Redis();   //把题目导入到redis中，大文本文件使用redis很不好用，之后使用mongodb来进行操作修改
            }
            catch (const Exception &e)
            {
                cout << e.what() << endl;
            }
        }
        // getAllField说明是需要所有的参数
        bool getQuestion(const string &sql, vector<tuple<string, string, string>> &out)
        {
            vector<vector<string>> data;
            if (mysql->Select(sql.c_str(), data))
            {
                for (int i = 0; i < data.size(); i++)
                {
                    string number = data[i][0];
                    string title = data[i][1];
                    string star = data[i][2];

                    tuple<string, string, string> t(number, title, star);
                    out.push_back(move(t));
                }
                return true;
            }
            return false;
        }
        //从数据库中获得一道题目具体的信息
        bool getTaskDetail(const string &sql, Question &out) //从数据库里面获得题目信息
        {

            vector<vector<string>> data;
            if (mysql->Select(sql.c_str(), data))
            {
                Question q;
                q.number = data[0][0]; // 获取第一列的数据
                q.title = data[0][1];
                q.star = data[0][2];

                q.desc = data[0][3];
                q.header = data[0][4];
                q.tail = data[0][5];
                q.cpulimit = stoi(data[0][6]);
                q.memlimit = stoi(data[0][7]);
                out = move(q);

                return true;
            }
            return false;
        }
        //查看题目列表
        bool getQuestionList(vector<tuple<string,string,string>> &out) // 获得所有题目再根据登陆人的身份查看自己的题库
        {
            // const string sql = "select number,title,star from oj_question;"; //我们只需要查询3个field即可
            // 从redis中读取缓存数据
            string command="lrange taskList 0 -1";
            //从redis中读取缓存的题目，读出来的都是一个json串
            vector<string> taskList=redis->getMultipleData(command);
            for(auto& json:taskList)
            {
                tuple<string,string,string> t=JsonUtil::TaskDeSerialize(json);
                out.emplace_back(t);
            }
            return true;
        }
        //根据题目名获得指定的题目
        bool getSpecifyQuestion(string number, Question &out) // 获得一个题目
        {

            string sql = "select * from oj_question where number=" + number + ";";
            if (getTaskDetail(sql, out))
            {
                LOG(INFO) << "get task:" << number << " success" << endl; //输出打印用户成功的日志消息
                return true;
            }
            else
            {
                throw SqlException(LogHeader(ERROR), "get task:" + number + "fail", sql);
            }
        }

        bool Register(const string &injson, string *out) // 注册，在布隆过滤器和redis里面进行两次对数据进行过滤
        {
            auto body = json::parse(injson);
            string username = body["username"];
            string password = body["password"];

            UserInfo u(username, password); // 把发送过来的json串进行反序列化
            // insert into users (username,passwd) values (11,'123');
            if (registeredUser.test(u.username))
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
                if (redis->exists(command))
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

                    string sql = "INSERT INTO users (username, passwd) VALUES ('" + username + "', '" + password + "')";

                    //添加到mysql中
                    if (mysql->Query(sql.c_str()))
                    {
                        *out = "注册成功";
                        // 如果注册成功，就要把新加进来的这个用户名添加到布隆过滤器中
                        registeredUser.set(u.username);
                        // 这个地方也需要往redis里面缓存添加新的数据
                        string command = "hmset user:" + u.username + " username " + u.username + " password " + u.passwd;
                        redis->addData(command);
                        LOG(INFO) << "user:" << u.username << " register success" << endl; //输出打印用户成功的日志消息

                        return true;
                    }
                    else
                    {
                        // sql语句错误
                        *out = "注册失败";
                        throw SqlException(LogHeader(ERROR), "注册失败", sql);

                        return false;
                    }
                }
            }
        }
        bool login(const string &injson, string *out) // 登陆
        {

            // UserInfo u = JsonUtil::UserInfoDeSerialize(injson);
            auto body = json::parse(injson);
            string username = body["username"];
            string password = body["password"];

            if (!registeredUser.test(username))
            {
                // 别人要注册的用户名已经被人给注册了所以，就注册失败
                *out = "Login failed, user does not exist";
                return false;
            }
            else
            {
                // 再使用redis进行一个过滤
                string command = "hget user:" + username + " password";
                string password = redis->getSingleData(command);
                if (!password.empty() || passwd == password)
                {
                    // load failed
                    *out = "login success";
                    LOG(INFO) << "user:" << username << " login success" << endl; //输出打印用户成功的日志消息
                    return true;
                }
                else
                {
                    // 登陆失败。使用mysql进行操作
                    //  用户名存在，看看密码是否正确
                    //这句sql应该使用防止sql注入的操作

                    string sql = "SELECT username,passwd FROM users  WHERE username = '" + username + "' AND passwd = '" + password + "'";
                    vector<vector<string>> data;

                    if (mysql->Select(sql.c_str(), data))
                    {
                        if (data.empty())
                        {
                            // 没有数据，说明登陆失败
                            *out = "登陆失败,密码错误";
                            LOG(WARNING) << "user:" << username << " login failed,password error" << endl;
                            return false;
                        }

                        *out = "登陆成功";
                        LOG(INFO) << "user:" << username << " login success" << endl; //输出打印用户成功的日志消息

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
        bool topicAdd(const string &injson, string *out) // 添加一个题目
        {
            // 添加题目
            Question q = JsonUtil::QuestionDeSerialize(injson); // 获得序列化的题目
            // insert into oj_question ()
            // string sql="insert into "
            return true;
        }
    };
};