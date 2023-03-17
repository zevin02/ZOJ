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
public:
    vector<string> multipledata(string command)//获得多种的数据
    {
        pm_rr = (redisReply *)redisCommand(c, command.c_str());

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

public:
    Myredis()
    {
        connect();
    }
    ~Myredis()
    {
        disconnect();
    }

    void set(string command) // 增加数据
    {
        pm_rr = (redisReply *)redisCommand(c, command.c_str());
    }

    bool exists(string command) // 判断某个key是否在数据库里面
    {
        pm_rr = (redisReply *)redisCommand(c, command.c_str());
        return pm_rr->integer; //
    }
    string get(string command) // 获得数据库里面的数据
    {
        pm_rr = (redisReply *)redisCommand(c, command.c_str());
        return pm_rr->str;
    }

    vector<string> smembers(string command) // 获得set里面的数据
    {
        return multipledata(command);
    }
    void del(string command) // 删除数据库中的一个key
    {
        pm_rr = (redisReply *)redisCommand(c, command.c_str());
    }
    vector<string> hmget(string command) // 在hash表里面获得字符串
    {
        return multipledata(command);
    }
    vector<string> lrange(string command) // 获得list里面的所有值
    {
        return multipledata(command);
    }
    void SetData(string command)
    {
        // 设置数据库里面的数据
        pm_rr = (redisReply *)redisCommand(c, command.c_str());
    }
    map<string, string> GetGroupApplyInfo(string command) // 获得申请加入群的名单，包含群+人
    {
        map<string, string> ret;
        string key, value;
        pm_rr = (redisReply *)redisCommand(c, command.c_str());
        for (int i = 0; i < pm_rr->elements; i++)
        {
            string ll = pm_rr->element[i]->str;
            size_t pos = ll.find("|");
            value = ll.substr(0, pos);
            key = ll.substr(pos + 1);
            ret[key] = value;
        }
        return ret;
    }
};