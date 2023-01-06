#pragma once
#include <string>
#include <iostream>
using namespace std;
#include<sys/stat.h>
#include<unistd.h>
#include<sys/types.h>

namespace ns_util
{
    string temp = "./temp/";

    class PathUtil
    {
    public:
        // 需要自动拼接路径+后缀,形成完整的原文件名字
        // 1234   ------->  ./temp/1234.cpp
        static string AddSuffix(const string & filename,const string & suffix)
        {
            string path=temp;
            path+=filename;
            path+=suffix;
            return path;
        }
        static string Src(const string &file_name)
        {
            return AddSuffix(file_name,".cpp");
        }
        // 形成完整的可执行程序路径
        static string Extension(const string &file_name)
        {
            return AddSuffix(file_name,".out");
        }
        //构建出错信息
        
        static string Error(const string &file_name)
        {
            return AddSuffix(file_name,".stderr");
        }
    };

    class FileUtil
    {
        public:
        static bool Exists(const string & path)
        {
            //stat来判断文件是否存在
            struct stat st;
            int ret=stat(path.c_str(),&st);
            if(ret==0)
            {
                return true;//获取属性成功，文件存在
            }
            else
            {
                return false;//获取属性失败，文件不存在
            }

        }
    };
};