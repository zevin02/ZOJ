#pragma once
// 最重要的就是进行逻辑控制
#include <iostream>
#include <string>
// #include "oj_model.hpp"
#include"oj_modelsql.hpp"
#include "../comm/util.hpp"
#include "../comm/log.hpp"
#include "oj_view.hpp"
#include <jsoncpp/json/json.h>
#include <mutex>
#include <algorithm>
using namespace std;
namespace ns_control
{
    using namespace ns_view;
    using namespace ns_log;
    using namespace ns_util;
    using namespace ns_model;

    // 提供服务的主机，一台机器对应一个后端编译运行服务
    class Machine
    {
    public:
        string ip;
        int port;
        uint64_t load; // 负载情况
        mutex *_mtx;   // mutex禁止拷贝，使用指针

    public:
        Machine()
            : ip(""), port(0), _mtx(nullptr)
        {
        }
        ~Machine()
        {
        }
        Machine &operator=(const Machine &m)
        {
            ip = m.ip;
            port = m.port;
            load = m.load;
            _mtx = m._mtx;
            return *this;
        }
        // 更新负载
        void Increasement()
        {
            if (_mtx)
            {
                _mtx->lock();
                load++;
                _mtx->unlock();
            }
        }
        void Decreasement() // 减少负载
        {
            if (_mtx)
            {
                _mtx->lock();
                load++;
                _mtx->unlock();
            }
        }
        uint64_t GetLoad() // 没有意义
        {
            if (_mtx)
            {
                _mtx->lock();
                return load;
                _mtx->unlock();
            }
            return 0;
        }
        void ResetLoad()
        {
            _mtx->lock();
            load=0;
            _mtx->unlock();
        }
    };
    // 负载均衡的模块,根据配置文件，把所有预先配置的文件全部都加载进来
    const string machinelist = "./conf/service_machine.conf";
    class LoadBalance
    {
    private:
        vector<Machine> machine; // 里面的所有的机器,我们使用下标来充当主机编号
        vector<int> online;      // 代表的是所有在线的主机
        vector<int> offline;     // 离线的主机
        // 保证在选择主机的时候，需要保证数据的安全
        mutex mtx;

    public:
        LoadBalance()
        {
            assert(LoadConf(machinelist)); // 启动的时候就加载进来了
            LOG(INFO) << "加载后端服务器配置文件成功" << endl;
        }
        ~LoadBalance()
        {
        }
        bool LoadConf(const string &machinelist)
        {
            // 把主机加载进来
            ifstream ifs(machinelist, ios::in | ios::binary);
            if (!ifs.is_open())
            {
                LOG(FATAL) << "加载" << machinelist << "配置文件失败" << endl;
                return false;
            }
            string line;
            while (getline(ifs, line))
            {
                vector<string> out;
                StringUtil::CutString(line, &out, ":");
                if (out.size() != 2)
                {
                    LOG(WARNING) << "切分" << line << "失败" << endl;
                    continue;
                }
                Machine ma;
                ma._mtx = new mutex();
                ma.ip = out[0];
                ma.port = stoi(out[1]);
                ma.load = 0;
                online.push_back(machine.size()); // 放进去对应的id
                machine.push_back(move(ma));
            }
            ifs.close();
            return true;
        }
        // 智能的在在线机器中选择
        bool SmartChoice(int *id, Machine &m)
        {
            // 智能选择
            // 使用选择好的主机(更新主机上的负载情况)
            // 后续我们可能会需要离线该主机

            // 负载均衡的算法
            // 1.随机数+hash，保证了每台主机都有一定的概率可以被选择到
            // 2.轮询+hash
            unique_lock<mutex> lock(mtx);
            int online_num = online.size();
            if (online_num == 0)
            {
                // 没有在线的主机了
                LOG(FATAL) << "所有后端的编译主机已经全部离线了，需要尽快修复" << endl;
                return false;
            }
            // 选择这个时刻负载最小的机器
            // 通过遍历的方式
            uint64_t min_load = machine[online[0]].load; //
            *id = online[0];
            m = machine[online[0]];
            for (int i = 1; i < online.size(); i++)
            {
                if (min_load > machine[online[i]].load)
                {
                    min_load = machine[online[i]].load; // 更新最小负载
                    *id = online[i];
                    m = machine[online[i]];
                }
            }
            return true; // 找到了对应的主机
        }
        // 请求不成功就去选择离线
        void Offlinemachine(int which)
        {
            // 把在线主机的id放到offline中
            unique_lock<mutex> lock(mtx);
            for (auto iter = online.begin(); iter != online.end(); iter++)
            {
                if (*iter == which)
                {
                    machine[which].ResetLoad();
                    // 要离线的主机找到了
                    // 就要把这个元素给删除掉
                    online.erase(iter);
                    offline.push_back(which);
                    break; // 这样就不要考虑迭代器失效的问题
                }
            }
        }
        void ShowOnline()
        {
            unique_lock<mutex> lock(mtx);
            cout << "当前在线主机列表:" << endl;
            for (auto &id : online)
            {
                cout << id << " " << endl;
            }
            cout << endl;
            cout << "当前离线主机列表:" << endl;
            for (auto &id : offline)
            {
                cout << id << " " << endl;
            }
            cout << endl;
        }

        void OnlineMachine()
        {
            // 上线,所有主机离线的时候进行统一上线
            unique_lock<mutex> lock(mtx);
            // 把offline的东西插入到online中
            online.insert(online.end(), offline.begin(), offline.end());
            offline.erase(offline.begin(), offline.end());
            LOG(INFO) << "所有的主机上线了" << endl;
        }
    };

    class Control
    {
    private:
        Model _model; // 数据
        // 还需要有构建成网页的view
        View _view; // 提供网页绚烂的功能
        LoadBalance load_balance;

    public:
        Control()
        {
        }
        ~Control()
        {
        }
        bool GetAllQuestions(string *html) // 根据题目数据构建网页
        {
            vector<Question> all;
            if (_model.GetAllQuestion(all))
            {
                // 获得成功,将所有的题目数据构成一个网页
                // 把所有题目转化成html
                sort(all.begin(), all.end(), [](const Question &s1, const Question &s2)
                     { return stoi(s1.number) < stoi(s2.number); });
                _view.AllExpand(all, html);
                return true;
            }
            else
            {
                *html = "获得题目失败，形成题目列表失败";
                return false;
            }
        }
        bool GetAQuestions(string &number, string &html) // 根据题目数据构建网页
        {
            Question q;
            if (_model.GetAQuestion(number, q))
            {
                // 获得了一个题目的具体信息
                _view.AExpand(q, html);
                return true;
            }
            else
            {
                // 获取失败了
                html = "指定题目" + number + "不存在";
                return false;
            }
        }
        void Recoverymachine()
        {
            load_balance.OnlineMachine();
        }
        void Judge(const string &number, const string &injson, string &outjson) // 判题
        {
            // 根据题目可以直接拿到对应的题目细节
            Question q;
            _model.GetAQuestion(number, q);
            // 获得了题目的具体细节

            // 反序列化题目的id，code，input
            Json::Reader reader;
            Json::Value in_value;
            reader.parse(injson, in_value);
            string code = in_value["code"].asString();
            // code+tail.cpp这个代码拼接起来，传送给后端去判断题目
            Json::Value compile_value;                             // 这个需要发送到远端
            compile_value["input"] = in_value["input"].asString(); // 将用户提交的代码进行重新组装序列化到对应的远端
            compile_value["code"] = code + q.tail;                 // 将题目进行拼接
            compile_value["cpulimit"] = q.cpulimit;
            compile_value["memlimit"] = q.memlimit;
            Json::FastWriter writer;
            string compile_string = writer.write(compile_value); // 就这个发送给远端的服务器
            // 序列化成功

            // 负载均衡的选择主机，然后发起http请求，得到结果
            // 一直选择，直到主机可以用，否则就是全部挂掉
            while (true)
            {
                int id = 0;
                Machine m;
                if (!load_balance.SmartChoice(&id, m))
                {
                    break;
                }
                // 发起http请求
                // 充当一个客户端的角色
                httplib::Client cli(m.ip, m.port); // 绑定主机和端口
                // 发起请求
                m.Increasement();
                LOG(INFO) << "选择主机成功,主机ID:" << id << "详情:" << m.ip << ":" << m.port << "该主机的负载情况:" << m.load << endl;

                if (auto res = cli.Post("/compile_run", compile_string, "application/json;charset=utf-8"))
                {
                    // 成功了,完成了对应的请求
                    // 状态码为200的时候才是完成成功的
                    if (res->status == 200)
                    {
                        outjson = res->body; // outjson里面就是对应的返回回来的数据
                        m.Decreasement();
                        LOG(INFO) << "请求编译运行服务成功" << endl;
                        break;
                    }

                    // 不等于200,访问到目标主机但是结果是不对的
                    //  请求成功了就要减少负载
                    LOG(ERROR) << "请求服务失败status=" << res->status << endl;
                    m.Decreasement();
                }
                else
                {
                    // 请求失败
                    // 没有得到任何响应
                    LOG(ERROR) << "当前请求的主机无响应,主机ID:" << id << "详情:" << m.ip << ":" << m.port << ",当前主机可能已经离线" << endl;

                    load_balance.Offlinemachine(id); // 把某台主机下线
                    load_balance.ShowOnline();       // 只是用来调试
                }
            }

            // 将结果给到outjson
            // 在service.conf可以查看哪个主机上线了
        }
    };
};