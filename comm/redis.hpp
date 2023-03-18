#include <iostream>
#include <cstdio>
#include <string>
using namespace std;
#include <hiredis/hiredis.h>
#include <vector>
#include <map>
class Myredis
{
private:
    redisContext *c;
    redisReply *pm_rr;

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
    void disconnect() // 断开连接
    {
        redisFree(c);
        freeReplyObject(pm_rr);
    }

    redisReply *execute_command(string command) // 执行命令
    {

        pm_rr = (redisReply *)redisCommand(c, command.c_str());

        if (pm_rr == NULL)
        {
            fprintf(stderr, "Can't execute redis command: %s\n", command.c_str());
            redisFree(c);
            exit(1);
        }
        if (pm_rr->type == REDIS_REPLY_ERROR)
        {
            fprintf(stderr, "Redis command failed: %s\n", pm_rr->str);
            freeReplyObject(c);
            exit(1);
        }

        return pm_rr;
    }

public:
    Myredis()
    {
        connect();
    }
    ~Myredis()
    {
        disconnect();
    }

    bool adddata(string command) // 所有的向redis里面添加数据的都调用这个
    {
        pm_rr = execute_command(command);
        return pm_rr ? true : false;
    }
    bool exists(string command) // 判断某个key是否在数据库里面
    {
        pm_rr = (redisReply *)redisCommand(c, command.c_str());
        return pm_rr->integer; //
    }
    vector<string> multipledata(string command) // 获得多个字符串
    {
        pm_rr = (redisReply *)execute_command(command);

        // string s = pm_rr->str;
        size_t size = pm_rr->elements;
        vector<string> ret;
        for (int i = 0; i < size; i++)
        {
            string s = (pm_rr->element[i])->str;
            ret.push_back(move(s));
        }
        return ret;
    }
    string singledata(string command) // 这个地方只获得一个字符串
    {
        pm_rr = (redisReply *)execute_command(command);
        return pm_rr->str;
    }

    void del(string command) // 删除数据库中的一个key
    {
        execute_command(command);
    }

};