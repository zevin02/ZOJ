
#include "compile_run.hpp"
#include "../comm/httplib.h"
#include "../comm/rpc.hpp"

// 编译服务随时可能被多个用户请求，我们要保证链接上来的代码，具有唯一性，不然，不同的用户之间就会互相影响
using namespace ns_compile_run;

string Start(const string &injson) //远程rpc调用这个函数，再去调用真正处理的函数
{
  string outjson;
  CompileRun::Start(injson, outjson);
  return outjson; //返回输出的结果
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    cerr << "usage: proc+port" << endl;
    return 1;
  }
  // 运行服务启动的话，就要给客户端发送一个请求表示我需要添加到在线列表,所以在里面需要有一个去重的功能
  // 现在就固定了8080就是我们oj_server的端口号
  Json::Value upload_value;    // 这个需要发送到远端
  upload_value["ip"] = "8080"; // 将用户提交的代码进行重新组装序列化到对应的远端
  upload_value["port"] = atoi(argv[1]);
  Json::FastWriter writer;
  string upload_string = writer.write(upload_value); // 就这个发送给远端的服务器
  httplib::Client cli("127.0.0.1", 8080);            // 绑定主机和端口
                                                     //
                                                     // 我需要把自己的ip和端口号传送给服务器

  try
  {
    if (auto res = cli.Post("/online", upload_string, "application/json;charset=utf-8")) // 发送http请求
    {
      // 成功了,完成了对应的请求
      // 状态码为200的时候才是完成成功的
      if (res->status == 200)
      {
        LOG(INFO) << "接入oj服务器" << endl;
      }
      LOG(INFO) << res->status << endl;

      // 不等于200,访问到目标主机但是结果是不对的
      //  请求成功了就要减少负载
    }
    else
    {
      // 请求失败
      // 没有得到任何响应
      LOG(ERROR) << "未接入oj服务器,当前oj服务器可能未上线" << endl;
    }

    // 要将提供的服务编译服务，转化成网络服务
    // 这个就是提前注册响应服务，如果请求该服务，就会去调用这个回调方法

    //这个地方我们使用一个rpc来进行操作
    rpc server;
    server.as_server(atoi(argv[1])); //绑定上端口号
    //执行响应的函数
    server.regist("Start", Start);
    server.run();
  }
  catch (const Exception &e)
  {
    cout << e.what() << endl;
  }
  // string code="code";
  // Compiler::Compile(code);
  // Runner::Run(code,1,60);
  // 通过http让client给我们上传一个json string
  // Json::Value in_value;
  // // C++11中添加一个新特性
  // // R“（）” ,raw string;（）里面的字符串都保持原样，特殊字符都保存不要处理

  // in_value["code"] = R"(#include<iostream>
  //    using namespace std;
  //     int main(){
  //       cout<<"hello"<<endl;
  //       int a=10;
  //       int* p=nullptr;
  //       *p=2;
  //       return 0;
  //        })";
  // in_value["input"] = "";
  // in_value["cpulimit"] = 1;
  // in_value["memlimit"] = 10240 * 3;

  // Json::StyledWriter writer;
  // string in_json = writer.write(in_value);
  // cout << in_json << endl;
  // string outjson; // 这个是将来返回给用户看的输出
  // CompileRun::Start(in_json, outjson);
  // cout << outjson << endl;
  return 0;
}