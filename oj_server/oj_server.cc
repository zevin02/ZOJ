#include "../comm/httplib.h"
#include <mysql/mysql.h>
#include "oj_control.hpp"
#include <signal.h>
using namespace ns_control;

int main()
{
    // DebugEnable(); // 把调试开关打开

    // 用户请求的服务器路由功能

    httplib::Server svr;
    Control ctrl; // ctrl实现了对客户端需要功能的路由
    // 用户要获取所有的题目列表
    // mysql版本
    try
    {

        svr.set_base_dir("./www_root");                                                         // 设置根目录
        svr.Get("/all_question", [&ctrl](const httplib::Request &req, httplib::Response &res) { // 返回的一张包含所有题目的网页
            string html;
            if (ctrl.GetAllQuestions(&html))
                res.set_content(html, "text/html;charset=utf8");
        });
        // 用户要更具题目编号，获得题目内容
        /// question/100正则匹配
        svr.Get(R"(/question/(\d+))", [&ctrl](const httplib::Request &req, httplib::Response &res)
                {
        std::string num=req.matches[1];//获得题号码,可以进行正则表达式的匹配
        string html;
        if(ctrl.GetAQuestions(num,html)) 
        res.set_content(html,"text/html;charset=utf8"); });
        // 用户提交代码，使用我们的判题功能
        //(1.每道题的测试用力2.compile_run)

        svr.Post(R"(/judge/(\d+))", [&ctrl](const httplib::Request &req, httplib::Response &res)
                 {
        std::string num=req.matches[1];//获得题号码,可以进行正则表达式的匹配
        string ret;
        ctrl.Judge(num,req.body,ret);//在正文中就存在请求的json串，使用rpc来对后端请求进行一个响应
        res.set_content(ret,"application/json;charset=utf8"); });

        svr.Post("/online", [&ctrl](const httplib::Request &req, httplib::Response &res)
                 {
        string out;
        ctrl.Access(req.body, out);
        res.set_content(out, "text/plain;cahrset=utf-8"); });
        // User u;
        svr.Post("/register", [&ctrl](const httplib::Request &req, httplib::Response &res)
                 {
        string out;
        ctrl.Register(req.body,&out);
        res.set_content(out, "text/plain;cahrset=utf-8"); });
        svr.Post("/clicklogin", [&ctrl](const httplib::Request &req, httplib::Response &res)
                 {
        string out;
        ctrl.Login(req.body,&out);
        res.set_content(out, "text/plain;cahrset=utf-8"); });

        svr.Get("/login", [&ctrl](const httplib::Request &req, httplib::Response &res) // 获得页面
                {
        string html;
        FileUtil::ReadFile("./www_root/load.html",html,true);
        res.set_content(html,"text/html;charset=utf8"); });

        svr.Get("/registerpage", [&ctrl](const httplib::Request &req, httplib::Response &res) // 获得注册页面
                {
        string html;
        FileUtil::ReadFile("./www_root/register.html",html,true);
        res.set_content(html,"text/html;charset=utf8"); });

        svr.listen("0.0.0.0", 8080);
    }
    catch (const Exception &e)
    {
        cout << e.what() << endl;
    }
    return 0;
}