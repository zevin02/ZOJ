#pragma once
#include"bloomfilter.hpp"
#include <string>
using namespace std;

struct UserInfo
{
    string username;
    string passwd;
    UserInfo()
    {
    }
    UserInfo(const string &n, const string &ps)
        : username(n), passwd(ps)
    {
    }

    UserInfo(const UserInfo &ui)
        : username(ui.username), passwd(ui.passwd)
    {
    }
    UserInfo &operator=(const UserInfo &ui)
    {
        username = ui.username;
        passwd = ui.passwd;
        return *this;
    }
};

struct Question
{
    string number; // 题目的编号
    string title;  // 题目的标题
    string star;
    int cpulimit;
    int memlimit;
    string desc; //
    string header;
    string tail; // 题目的测试用例
    Question()
    {
    }
    Question(const string &n, const string &_title, const string &s, const int &c, const int &m, const string d, const string &h, string &_tail)
        : number(n), title(_title), star(s), cpulimit(c), memlimit(m), desc(d), header(h), tail(_tail)
    {
    }
    Question &operator=(const Question &q)
    {
        number = q.number;
        title = q.title;
        star = q.star;
        cpulimit = q.cpulimit;
        memlimit = q.memlimit;
        desc = q.desc;
        header = q.header;
        tail = q.tail;
        return *this;
    }
    Question(const Question &q)
    {
        number = q.number;
        title = q.title;
        star = q.star;
        cpulimit = q.cpulimit;
        memlimit = q.memlimit;
        desc = q.desc;
        header = q.header;
        tail = q.tail;
    }
};

