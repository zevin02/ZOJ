#pragma once
// 只处理代码的编译功能
#include <iostream>
#include<unistd.h>
#include<sys/types.h>
#include"../comm/util.hpp"
#include<sys/wait.h>
#include<fcntl.h>
#include"../comm/log.hpp"

using namespace std;

//远端会提交一份代码，一定要形成一个临时文件(假设已经有了)
//编译：1.编译通过，2.编译出错（stderr）（需要形成一个临时文件，保存编译出错的结果）
//使用fork（）-》parent一直执行
//-》child来完成编译服务,同时把错误结果重定向到文件中




namespace ns_compiler
{
    using namespace ns_util;
    using namespace ns_log;
    class Compiler
    {
    private:
    public:
        Compiler()
        {
        }
        ~Compiler()
        {
        }
        //filename:1234
        //需要构建出->  ./temp/1234.cpp
        //需要构建出->  ./temp/1234.out
        //需要构建出->  ./temp/1234.stderr


        static bool Compile(const string& code_filename)//传入的是传进来代码的所在文件，不要路径，不要后缀
        {
            //执行编译功能
            pid_t pid=fork();
            if(pid<0)
            {
                //失败,
                LOG(ERROR)<<"fork() error"<<endl;//启动等级的开关
                return false;
            }
            else if(pid==0)
            {
                //child
                //子进程就要执行编译的功能
                //g++ src -o target -std=c++11
                //同时需要把g++出错的结果打印到文件中
                umask(0);//把掩码清零

                int _stderr=open(PathUtil::Compile_Error(code_filename).c_str(),O_CREAT|O_WRONLY,0644);//创建一个错误文件
                if(_stderr<0)
                {
                    LOG(WARNING)<<"haven't create stderr file"<<endl;//
                    exit(2);//打开文件失败了
                }
                //打开文件成功了
                //重定向标准错误到文件中
                dup2(_stderr,2);

                execlp("g++","g++",ns_util::PathUtil::Src(code_filename).c_str(),"-o",ns_util::PathUtil::Extension(code_filename).c_str(),"-std=c++11","-D","COMPLER_ONLINE",nullptr);
                LOG(ERROR)<<"executing g++ fails,maybe parameter has wrong"<<endl;
                exit(1);//退出

            }
            else 
            {
                //父进程
                //等待子进程
                waitpid(pid,nullptr,0);
                //判断编译是否完成
                if(FileUtil::Exists(PathUtil::Extension(code_filename)))//就看是否有形成可执行文件
                {
                    LOG(INFO)<<PathUtil::Src(code_filename)<<" compilation success"<<endl;
                    return true;
                }
                else
                {
                    LOG(ERROR)<<"Compilation failed,didn't create executable file"<<endl;   
                    return false;
                }

            }   

            
        }
    };
};