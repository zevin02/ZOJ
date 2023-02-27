#pragma once
#include "compiler.hpp"
#include "runner.hpp"
using namespace ns_compiler;
using namespace ns_runner;
// 编译并运行的功能
// 适配用户需求
#include <signal.h>
#include <jsoncpp/json/json.h>

namespace ns_compile_run
{
    class CompileRun
    {
    public:
        // injson
        // code：用户给自己提交的代码
        // input：用户给自己提交的代码对应的输入
        // cpulimit:时间限制
        // memlimit：空间限制

        // outjson
        // status:状态码
        // reason:请求结果，原因
        // 选填写
        // stdout:输出结果
        // stderr：程序运行完的错误结果

        static string CodeToDesc(int code, const string &filename)//从返回的值获得对应的描述
        {
            // code>0信号导致崩溃
            // code<0编译运行时出现的内部错误
            // code=0整个过程完成
            string desc;
            switch (code)
            {
            case 0:
                desc = "compile and run success";
                break;
            case -1:
                desc = "user commits empty code";
                break;
            case -2:
                desc = "Unknown error";
                break;
            case -3:
                //
                FileUtil::ReadFile(PathUtil::Compile_Error(filename), desc, true);//-3就是编译错误,所以就需要读取编译错误的文件
                break;
            case SIGABRT: // 6
                desc = "memory out of bounds";
                break;
            case SIGXCPU: // 24
                desc = "CPU usage time out";
                break;
            case SIGFPE: // 8
                desc = "divide by zero";
                break;
            case SIGSEGV: // 11段错误
                desc = "Segmentation Fault";
                break;
            default:
                desc = "unknow signal,for debug status_code=" + to_string(code);
                break;
            }
            return desc;
        }
        static void Start(const string &injson, string &outjson)//由oj_server通过http发过来的网络请求,我们需要进行执行
        {

            Json::Value in_value;
            Json::Reader reader;
            reader.parse(injson, in_value);
            string code = in_value["code"].asString();   // 用户提交的代码
            string input = in_value["input"].asString(); // 用户的输入

            int cpulimit = in_value["cpulimit"].asInt();
            int memlimit = in_value["memlimit"].asInt();

            Json::Value out_value;
            int status_code = 0;
            int ret = 0;
            // 错误码为负数说明这个就是由位置错误导致的
            string file_name; // 需要内部形成的唯一文件名

            if (code.size() == 0)
            {
                status_code = -1;
                goto end;
                // 差错处理
            }

            // 形成一个唯一的文件名
            // 使用毫秒级时间戳+原子性递增的方式来形成一个唯一值

            file_name = FileUtil::UniqueFilename();
            if (!FileUtil::WriteFile(PathUtil::Src(file_name), code))
            {
                // 往唯一文件中写入code代码
                status_code = -2;
                goto end;
            }

            if (!Compiler::Compile(file_name))
            {
                status_code = -3;
                goto end;
                // 编译失败的信息就需要进行返回
            }
            ret = Runner::Run(file_name, cpulimit, memlimit);
            if (ret < 0)
            {
                status_code = -2;
                goto end;
            }
            // 错误码为正数代表是由于信号导致的出错
            else if (ret > 0)
            {
                status_code = ret;
                goto end;
            }
            // 错误码为0代表就是正常退出
            else
            {
                // 运行成功
                status_code = 0;
                goto end;
            }

        end:
            //构建返回的json串
            out_value["status"] = status_code;                        // 未知错误
            out_value["reason"] = CodeToDesc(status_code, file_name); // 获得描述原因
            if (status_code == 0)
            {
                // 整个过程全部成功
                string string_stdout;
                FileUtil::ReadFile(PathUtil::Stdout(file_name), string_stdout, true);//把运行的输出结果获得并返回给上层
                out_value["stdout"] = string_stdout;
                string string_stderr;
                FileUtil::ReadFile(PathUtil::StdError(file_name), string_stderr, true); // 把运行的错误结果返回给上层
                out_value["stderr"] = string_stderr;                                    //
            }

            Json::FastWriter writer;
            outjson = writer.write(out_value);

            // 处理完之后，就要把所有生成的临时文件去除掉
            RemoveTempFile(file_name);
        }

        static void RemoveTempFile(const string &file_name)//移除临时文件
        {
            
            // 临时文件的个数是不确定的
            //但是由多少文件是确定的，所以就可以判断，如果存在就删除
            
            string src = PathUtil::Src(file_name);
            // 如果存在就删除
            if (FileUtil::Exists(src))
                unlink(src.c_str()); // 存在就删除,unlink就是把该文件的inode减少，如果减少到0说明就直接删除了
            string cpe = PathUtil::Compile_Error(file_name);
            // 如果存在就删除
            if (FileUtil::Exists(cpe))
                unlink(cpe.c_str()); // 存在就删除
            string ext = PathUtil::Extension(file_name);
            // 如果存在就删除
            if (FileUtil::Exists(ext))
                unlink(ext.c_str()); // 存在就删除
            string ster = PathUtil::StdError(file_name);
            // 如果存在就删除
            if (FileUtil::Exists(ster))
                unlink(ster.c_str()); // 存在就删除
            string stin = PathUtil::Stdin(file_name);
            // 如果存在就删除
            if (FileUtil::Exists(stin))
                unlink(stin.c_str()); // 存在就删除
            string stou = PathUtil::Stdout(file_name);
            // 如果存在就删除
            if (FileUtil::Exists(stou))
                unlink(stou.c_str()); // 存在就删除
        }
    };
};
