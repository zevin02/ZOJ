#pragma once
// 最重要的就是进行逻辑控制
#include <iostream>
#include <string>
#include "oj_model.hpp"
#include "../comm/util.hpp"
#include "../comm/log.hpp"
#include "oj_view.hpp"
#include <jsoncpp/json/json.h>
#include <mutex>

using namespace std;
namespace ns_control
{
    using namespace ns_view;
    using namespace ns_log;
    using namespace ns_util;
    using namespace ns_model;
    // 提供服务的主机
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
    };
    // 负载均衡的模块
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
                online.push_back(ma.size()); // 放进去对应的id
                machine.push_back(move(ma));
            }
            ifs.close();
        }
        // 智能的在在线机器中选择
        bool SmartChoice(int *id, Machine *&m)
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
            *m = machine[online[0]];
            for (int i = 0; i < online.size(); i++)
            {
                if (min_load > machine[online[i]].load)
                {
                    min_load = machine[online[i]].load; // 更新最小负载
                    *id = online[i];
                    *m = machine[online[i]];
                }
            }
            return true; // 找到了对应的主机
        }
        // 请求不成功就去选择离线
        void Offlinemachine()
        {
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

        void Judge(const string injson, string &outjson) // 判题
        {
            // 反序列化题目的id，code，input
            // code+tail.cpp这个代码拼接起来，传送给后端去判断题目
            // 负载均衡的选择主机，然后发起http请求，得到结果
            // 将结果给到outjson
            // 在service.conf可以查看哪个主机上线了
            
        }
    };
};