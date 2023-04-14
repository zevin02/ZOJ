#include <iostream>
#include <cstdio>
#include <string>
using namespace std;
#include <hiredis/hiredis.h>
#include <vector>
#include <map>
class MyRedis
{
private:
    redisContext *c;
    redisReply *reply_;

private:
    void connect() // 连接
    {
        c = redisConnect((char *)"127.0.0.1", 6379);
        if (c == NULL || c->err)
        {
            if (c->err)
                printf("Error:%s\n", c->errstr);
            else
                printf("cant allocate redis context\n");
        }
    }
    void disConnect() // 断开连接
    {
        redisFree(c);
        freeReplyObject(reply_);
    }

    redisReply *executeCommand(string command) // 执行命令
    {

        reply_ = (redisReply *)redisCommand(c, command.c_str());

        if (reply_ == NULL)
        {
            fprintf(stderr, "Can't execute redis command: %s\n", command.c_str());
            redisFree(c);
            exit(1);
        }
        if (reply_->type == REDIS_REPLY_ERROR)
        {
            fprintf(stderr, "Redis command failed: %s\n", reply_->str);
            freeReplyObject(c);
            exit(1);
        }

        return reply_;
    }

public:
    MyRedis()
    {
        connect();
    }
    ~MyRedis()
    {
        disConnect();
    }

    bool addData(const string &command) // 所有的向redis里面添加数据的都调用这个
    {
        reply_ = executeCommand(command);
        return reply_ ? true : false;
    }
    bool exists(const string& command) // 判断某个key是否在数据库里面
    {
        reply_ = (redisReply *)redisCommand(c, command.c_str());
        return reply_->integer; //
    }
    vector<string> getMultipleData(const string& command) // 获得多个字符串
    {
        reply_ = (redisReply *)executeCommand(command);

        // string s = reply_->str;
        size_t size = reply_->elements;
        vector<string> ret;
        for (int i = 0; i < size; i++)
        {
            string s = (reply_->element[i])->str;
            ret.push_back(move(s));
        }
        return ret;
    }
    string getSingleData(const string& command) // 这个地方只获得一个字符串
    {
        reply_ = (redisReply *)executeCommand(command);
        return reply_->str;
    }

    void del(const string& command) // 删除数据库中的一个key
    {
        executeCommand(command);
    }

};