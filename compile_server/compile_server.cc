
#include "compile_run.hpp"
#include "../comm/httplib.h"
// 编译服务随时可能被多个用户请求，我们要保证链接上来的代码，具有唯一性，不然，不同的用户之间就会互相影响
using namespace ns_compile_run;
int main(int argc,char* argv[])
{
  if(argc!=2)
  {
    cerr<<"usage: proc+port"<<endl;
    return 1;
  }
  // 要将提供的服务编译服务，转化成网络服务
  httplib::Server svr;
  // 当用户请求这个hello的时候，就会响应这个东西
  svr.Post("/compile_run", [](const httplib::Request &req, httplib::Response &res)
           {
      string injson=req.body;
        string outjson;
      if(!injson.empty())
      {
        //传入的数据不为空
        CompileRun::Start(injson, outjson);
        res.set_content(outjson,"application/json;charset=utf-8");
        
      } });
  
  svr.listen("0.0.0.0",atoi(argv[1])); // 启动http服务

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