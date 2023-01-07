#pragma once
#include"compiler.hpp"
#include"runner.hpp"
using namespace ns_compiler;
using namespace ns_runner;

//编译并运行的功能
//适配用户需求
#include<jsoncpp/json/json.h>

namespace ns_compile_run
{
    class CompileRun
    {
        public:
        //injson
        //code：用户给自己提交的代码
        //input：用户给自己提交的代码对应的输入
        //cpulimit:时间限制
        //memlimit：空间限制

        //outjson
        //status:状态码
        //reason:请求结果，原因
        //选填写
        //stdout:输出结果
        //stderr：程序运行完的错误结果
        static void Start(const string& injson,string& outjson)
        {
            Json::Value in_value;
            Json::Reader reader;
            reader.parse(injson,in_value);
            string code=in_value["code"].asString();//用户提交的代码
            string input=in_value["input"].asString();//用户的输入
            int cpulimit=in_value["cpulimit"].asInt();
            int memlimit=in_value["memlimit"].asInt();

            if(code.size()==0)
            {
                //差错处理
            }
            //形成一个唯一的文件名

            string file_name=FileUtil::UniqueFilename();
            FileUtil::WriteFile(PathUtil::Src(file_name),code);//往唯一文件中写入code代码
            Compiler::Compile(file_name);
            Runner::Run(file_name,cpulimit,memlimit);
        }
    };
};

