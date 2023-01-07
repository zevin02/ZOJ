#pragma once
#include <iostream>
#include <string>
using namespace std;
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "../comm/log.hpp"
#include "../comm/util.hpp"
#include <fcntl.h>
#include <sys/time.h>
#include <sys/resource.h>
namespace ns_runner
{
    using namespace ns_util;
    using namespace ns_log;
    class Runner
    {
    public:
        Runner()
        {
        }
        ~Runner()
        {
        }

        // 只需要指明文件名，不需要带路径和后缀
        static int Run(const string &filename)
        {
            /*
                返回值 >0:程序异常了，返回值就是对应的信号
                返回值 =0：程序就是正常运行完成，结果保存在临时文件中
                返回值 <0:程序在运行的时候内部出错
            
            
            */
            // 程序运行
            // 1.代码跑完，结果错误
            // 2,代码跑完，结果不正确
            // 3.代码跑完，异常
            // 我们不用考虑结果正确与否
            // 一个程序在默认启动的时候
            // 1.标准输入：不处理
            // 2.标准输出：程序运行完成，输出结果是什么
            // 3.标准错误：运行时错误信息
            string exe_file = PathUtil::Extension(filename);
            string _stdin = PathUtil::Stdin(filename);
            string _stdout = PathUtil::Stdout(filename);
            string _stderr = PathUtil::StdError(filename);
            umask(0);
            int infd = open(_stdin.c_str(), O_CREAT | O_RDONLY, 0644);
            int outfd = open(_stdout.c_str(), O_CREAT | O_WRONLY, 0644);
            int errfd = open(_stderr.c_str(), O_CREAT | O_WRONLY, 0644);

            if (infd < 0 || outfd < 0 || errfd < 0)
            {
                LOG(ERROR) << "Open standard file error at runtime"<<endl;
                return -1; // 代表打开文件失败
            }
            pid_t pid = fork();
            if (pid < 0)
            {
                LOG(ERROR)<<"Fork() error at runtime"<<endl;
                close(infd);
                close(outfd);
                close(errfd);
                return -2; // 创建子进程失败
            }
            else if (pid == 0)
            {
                //现在我们要进行重定向流
                dup2(infd,0);
                dup2(outfd,1);
                dup2(errfd,2);
                //这个是带路径的，所以需要我们进行来执行
                //前面是要执行谁，后面就是在命令行中怎么执行
                execl(exe_file.c_str(),exe_file.c_str(),nullptr);
                exit(1);
            }
            else
            {
                close(infd);
                close(outfd);
                close(errfd);
                int status = 0;
                waitpid(pid, &status, 0); // 我们不关心执行结果，只关心执行是否出现了异常
                //程序运行收到异常，都要通过程序的退出型号来判断
                LOG(INFO)<<"Run OK,info:"<<(status&0x7f)<<endl;

                return status&0x7F;//按位上信号的标志，如果没有异常的话，返回的就是0,如果有异常的话，返回的就是对应的1-31
            }
        }
    };
};