#pragma once
#include <string>
#include <iostream>
#include "util.hpp"
#include "unordered_map"
using namespace std;
namespace ns_log
{
    using namespace ns_util;
    // 日志等级

    enum
    {
        DEBUG,
        INFO,
        WARNING,
        ERROR, // 当前用户出错了
        FATAL, // 系统出错，直接服务器都不能用,直接返回
    };

    int option = INFO;
    // LOG(WARNING)<<"message",这样的使用，写到缓冲区里面,开放式的接口
    // 这个地方设置成内连函数，就不用频繁的进行函数调用，而是直接进行宏替换即可
    unordered_map<string, int> LogLevel = {{"DEBUG", 0}, {"INFO", 1}, {"WARNING", 2}, {"ERROR", 3}, {"FATAL", 4}};//设置对应

    inline ostream &log(const string &level, const string &filename, const int &line)
    {

        if (LogLevel[level] >= option)
        {
            // 添加日志等级
            string message = "[";
            message += level;
            message += "] ";

            // 添加日志出错的文件名
            message += "[";
            message += filename;
            message += ":";
            // 添加日志的出错行号码
            message += to_string(line);
            message += "] ";
            // 添加日志的时间
            message += "[";
            message += TimeUtil::GetTime();
            message += "] ";

            // cout本质是包含缓存区的
            cout << message; // 这样就会暂时存在cout缓存区中
        }
        return std::cout; // 形成一个这样的开放式的输入输出形式
    }

// LOG(INFO)<<"hello"
#define LOG(level) log(#level, __FILE__, __LINE__)
    // 如果debug打开才能使用
    void DebugEnable() // 启动debug
    {
        option = DEBUG;
    }
    void DebugUnEnable() // 关闭debug
    {
        option = INFO;
    }
    bool isDebugEnable() // 判断debug是否启动
    {
        return option == DEBUG;
    }

};