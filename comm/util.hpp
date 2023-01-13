#pragma once
#include <string>
#include <iostream>
using namespace std;
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <atomic>
#include <vector>
#include <mysql/mysql.h>
namespace ns_util
{
    class TimeUtil
    {
    public:
        static string GetTime() // 获得时间
        {
            struct timeval _time;
            gettimeofday(&_time, nullptr);
            struct tm tm = *localtime(&_time.tv_sec);
            static char time_str[32]{0};
            snprintf(time_str, sizeof(time_str),
                     "%04d-%02d-%02d %02d:%02d:%02d",
                     tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
            return string(time_str);
        }
        // 获得毫秒级时间戳
        static string GetTimeMs()
        {
            struct timeval _time;
            gettimeofday(&_time, nullptr);
            // 秒-》1000ms
            return to_string(_time.tv_sec * 1000 + _time.tv_usec / 1000); //
        }
    };
    string temp = "./temp/";

    class PathUtil
    {
    public:
        // 需要自动拼接路径+后缀,形成完整的原文件名字
        // 1234   ------->  ./temp/1234.cpp
        static string AddSuffix(const string &filename, const string &suffix)
        {
            string path = temp;
            path += filename;
            path += suffix;
            return path;
        }
        //
        static string Src(const string &file_name)
        {
            return AddSuffix(file_name, ".cpp");
        }
        // 形成完整的可执行程序路径
        static string Extension(const string &file_name)
        {
            return AddSuffix(file_name, ".out");
        }
        // 构建出错信息

        static string Compile_Error(const string &file_name) // 编译时报错
        {
            return AddSuffix(file_name, ".compiler_error");
        }

        // 运行时报错
        static string StdError(const string &file_name) // 运行时报错
        {
            return AddSuffix(file_name, ".stderr");
        }
        static string Stdout(const string &file_name) // 编译时报错
        {
            return AddSuffix(file_name, ".stdout");
        }

        static string Stdin(const string &file_name) // 编译时报错
        {
            return AddSuffix(file_name, ".stdin");
        }
    };

    class FileUtil
    {
    public:
        static bool Exists(const string &path)
        {
            // stat来判断文件是否存在
            struct stat st;
            int ret = stat(path.c_str(), &st); // 如果能够获得文件的属性，就成功
            if (ret == 0)
            {
                return true; // 获取属性成功，文件存在
            }
            else
            {
                return false; // 获取属性失败，文件不存在
            }
        }
        static string UniqueFilename() // 这样就形成了一个唯一的文件名,毫秒即时间戳+uid原子性自增
        {
            string ms = TimeUtil::GetTimeMs();
            static atomic<uint64_t> id(0);
            string uid = to_string(id);
            id++; // 这个就是一个原子性的自增
            return ms + "_" + uid;
        }
        static bool WriteFile(const string &filename, const string &source) // 写文件
        {
            ofstream ofs(filename, ios::out | ios::binary);
            if (!ofs.is_open())
            {
                // 没有被打开成功
                ofs.close();
                return false;
            }
            ofs.write(source.c_str(), source.size());

            ofs.close();
            return true;
        }

        static bool ReadFile(const string &filename, string &content, bool keep = false)//读取文件数据
        {
            ifstream ifs(filename, ios::in | ios::binary);
            if (!ifs.is_open())
            {
                ifs.close();
                return false;
            }
            string line;
            while (getline(ifs, line))
            {
                // getline不会保留换行所以如果使用这个的话，所有的换行我们都不会保留
                content += line;
                content += (keep ? "\n" : ""); // 如果上层需要这个换行我们就需要保留
            }
            ifs.close();

            return true;
        }
    };

    class StringUtil
    {
    public:
        static void CutString(const string &target, vector<string> *out, string sep) // 字符串切分
        {
            boost::split(*out, target, boost::is_any_of(sep), boost::token_compress_on); // 压缩中间的分隔符,把所有的压缩成一个\3
        }
    };

    class Mysql
    {
    private:
        string _host;
        int _port;
        string _db;
        string _user;
        string _passwd;

    private:
        MYSQL *my;
        MYSQL_RES *res;

    public:
        Mysql(string host, int port, string db, string user, string passwd)
            : _host(host), _port(port), _db(db), _user(user), _passwd(passwd)
        {
            my = mysql_init(nullptr);                                                                                      // 创建一个mysql句柄
            assert(mysql_real_connect(my, _host.c_str(), _user.c_str(), _passwd.c_str(), _db.c_str(), _port, nullptr, 0)); // 连接数据库
        }
        ~Mysql()
        {
            mysql_close(my); // 关闭mysql链接
        }
        bool Store(const string &sql)
        {
            res = mysql_store_result(my);

            return true;
        }
    };


};