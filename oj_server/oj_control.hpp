#pragma once
// 最重要的就是进行逻辑控制
#include <iostream>
#include <string>
// #include "oj_model.hpp"
#include "oj_modelsql.hpp"
#include "../comm/util.hpp"
#include "../comm/log.hpp"
#include "../comm/httplib.h"
#include "oj_view.hpp"
#include <jsoncpp/json/json.h>
#include <mutex>
#include <algorithm>
#include <set>
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
            if (_mtx)
            {
                delete _mtx; // 这里因为之前的操作会被delete两次，所以才会导致死锁
                _mtx = nullptr;
            }
        }
        // 移动构造
        void swap(Machine &m)
        {
            ::swap(ip, m.ip);
            ::swap(port, m.port);
            ::swap(load, m.load);
            ::swap(_mtx, m._mtx);
        }

        Machine(Machine &&m) // 移动构造
            : ip(""), port(0), load(0), _mtx(nullptr)
        {
            swap(m);
        }

        Machine &operator=(Machine &&m) // 移动赋值
        {
            ip = "";
            port = 0;
            load = 0;
            _mtx = nullptr;
            swap(m);
            return *this;
        }
        Machine &operator=(const Machine &m) // 拷贝赋值
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
                load--;
                _mtx->unlock();
            }
        }
        uint64_t GetLoad() const // 没有意义
        {
            if (_mtx)
            {
                _mtx->lock();
                return load;
                _mtx->unlock();
            }
            return 0;
        }
        void ResetLoad() // 重置负载
        {
            if (_mtx)
            {
                _mtx->lock();
                load = 0;
                _mtx->unlock();
            }
        }
    };
    // 负载均衡的模块,根据配置文件，把所有预先配置的文件全部都加载进来
    class LoadBalance
    {
    private:
        map<int, Machine> machine; // 里面的所有的机器,我们使用端口号来充当主机编号,只要是预先添加到了系统中，就需要加载进来，里面就是我们预先设置了所有会运用的后端机器
        set<int> online;           // 代表的是所有在线的主机,使用端口号来用作唯一的标识
        set<int> offline;          // 使用布隆过滤器来判断是否存在
        vector<int> hash;          // 用来处理一致性hash，
        // 保证在选择主机的时候，需要保证数据的安全
        mutex mtx;
        Mysql m;

    public:
        LoadBalance() // 构造函数，创建这个对象的时候，就把后端在线的机器加载进去
            : m(host, port, db, user, passwd)
        {
            string sql = "select * from machine_list";
            assert(LoadConf(sql) == true); // 启动的时候就加载进来了
            // LoadConf(machinelist,sql); // 启动的时候就加载进来了
            if (LogStatus::GetInstance().isDebugEnable())
            {
                ShowMachine();

                for (auto &num : hash)
                {
                    LOG(DEBUG) << num << " ";
                }
            }
            if (!online.empty()) // 在线不为空才能算接入成功
                LOG(INFO) << "加载后端服务器载入成功" << endl;
        }
        ~LoadBalance()
        {
        }

        void AskCompile(const string &ip, const int &port) // 发起后端请求判断是否在线
        {
            httplib::Client cli("127.0.0.1", port);
            auto res = cli.Get("/oj_judgeonline");
            if (res == nullptr) // 如果访问没有结果为空
            {
                LOG(INFO) << "machine:" << port << "接入失败" << endl;

                offline.insert(port);
                // 请求成功就要添加到对应的
            }
            else
            {
                // 请求成功,说明该机器在线
                LOG(INFO) << "machine:" << port << "接入成功" << endl;
                hash.push_back(port);
                online.insert(port);
                if (LogStatus::GetInstance().isDebugEnable())
                {
                    ShowMachine();
                }
            }
        }

        bool LoadConf(const string &sql) // 使用包装的数据库类来操作sql中加载机器,修改成，这个加载后面都不会被使用了，所以不需要添加到redis里面
        {

            vector<vector<string>> data;
            if (m.Select(sql, data))
            {
                // 成功,所有的数据都在data里面
                for (int i = 0; i < data.size(); i++)
                {
                    Machine ma;
                    ma.ip = data[i][0];
                    ma.port = stoi(data[i][1]);
                    ma.load = 0;
                    ma._mtx = new mutex();
                    // online.push_back(machine.size()); // 放进去对应的id
                    // 需要对每个服务器发起http请求，如果在线的话，就添加到online服务器中,否则就添加到offline中
                    AskCompile(ma.ip, ma.port);

                    machine[ma.port] = move(ma);
                }
                return true;
            }
            else
                throw SqlException(LogHeader(ERROR), "加载机器失败", sql);

            return false;
        }

        // 智能的在在线机器中选择
        bool SmartChoice(int *id, Machine **m)
        {
            // 智能选择
            // 使用选择好的主机(更新主机上的负载情况)
            // 后续我们可能会需要离线该主机

            // 负载均衡的算法
            // 1.随机数+hash，保证了每台主机都有一定的概率可以被选择到
            // 2.轮询+hash
            // unique_lock<mutex> lock(mtx);
            int online_num = online.size();
            if (online_num == 0)
            {
                // 没有在线的主机了
                LOG(FATAL) << "所有后端的编译主机已经全部离线了，需要尽快修复" << endl;
                return false;
            }
            //通过哈希算法来计算一个一个哈希值，再和现在总共的机器数取模得到一个特定的值
            long key = stol(TimeUtil::GetTimeMs()); // 使用毫秒级时间戳计算一个key
            int randon = key % hash.size();         // 和当前在线的机器数量进行取模

            int port = hash[randon];
            *id = port;
            *m = &machine[port];

            if (LogStatus::GetInstance().isDebugEnable())
            {
                LOG(DEBUG) << "key=" << key << ",port=" << (*m)->port << endl;
            }
            // lock.unlock();
            return true; // 找到了对应的主机
        }
        // 请求不成功就去选择离线
        void Offlinemachine(int which)
        {
            // 把在线主机的id放到offline中
            unique_lock<mutex> lock(mtx);

            machine[which].ResetLoad();
            // 要离线的主机找到了
            // 就要把这个元素给删除掉
            online.erase(which); // 删除这个元素对应的数据

            offline.insert(which);
            for (auto iter = hash.begin(); iter != hash.end(); iter++) // 删除对应的hash值
            {
                if (*iter == which)
                {
                    hash.erase(iter);

                    break;
                }
            }
        }
        void ShowMachine() // 打印机器状态
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

        void OnlineMachine() // 就需要发送过去http请求
        {
            // 上线,所有主机离线的时候进行统一上线
            unique_lock<mutex> lock(mtx);
            // 把offline的东西插入到online中
            for (auto &ma : machine)
            {
                AskCompile(ma.second.ip, ma.second.port);
            }
            LOG(INFO) << "所有的主机上线了" << endl;
        }
        void OnlineAdd(const string &ip, const int port) // 后端机器上线，添加到在线列表中
        {
            online.insert(port);
            hash.push_back(port);
            // 从离线列表中去除
            offline.erase(port);
        }
    };

    class Control
    {
    private:
    //在control中控制数据和视图
        Model _model; // 数据
        // 还需要有构建成网页的view
        View _view; // 提供网页绚烂的功能
        LoadBalance load_balance;//负载均衡器
        //连接上redis服务器,对mysql进行数据的缓存      

    public:
        Control()
        {

        }
        ~Control()
        {
        }

        bool Access(const string &injson, string &out) // 其他后端机器请求连接
        {
            Json::Reader reader;
            Json::Value in_value;
            reader.parse(injson, in_value);
            string ip = in_value["ip"].asString();
            int port = in_value["port"].asInt();

            load_balance.OnlineAdd(ip, port);
            out = "机器" + to_string(port) + "连接成功";
            LOG(INFO) << out << endl;
            return true;
        }
        bool GetAllQuestions(string *html) // 根据题目数据构建网页
        {
            vector<Question> all;
            if (_model.GetAllQuestion(all))
            {
                // 获得成功,将所有的题目数据构成一个网页
                // 将所有获得的数据按照序号从小到大进行排序
                sort(all.begin(), all.end(), [](const Question &s1, const Question &s2)
                     { return stoi(s1.number) < stoi(s2.number); });//使用Lambda表达式
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
                Machine *m;
                if (!load_balance.SmartChoice(&id, &m))
                {
                    break;
                }
                // 发起http请求
                // 充当一个客户端的角色
                httplib::Client cli(m->ip, m->port); // 绑定主机和端口
                // 发起请求
                m->Increasement();
                LOG(INFO) << "选择主机成功,主机ID:" << id << "详情:" << m->ip << ":" << m->port << "该主机的负载情况:" << m->load << endl;

                if (auto res = cli.Post("/compile_run", compile_string, "application/json;charset=utf-8"))
                {
                    // 成功了,完成了对应的请求
                    // 状态码为200的时候才是完成成功的
                    if (res->status == 200)
                    {
                        outjson = res->body; // outjson里面就是对应的返回回来的数据
                        m->Decreasement();
                        LOG(INFO) << "请求编译运行服务成功" << endl;
                        break;
                    }

                    // 不等于200,访问到目标主机但是结果是不对的
                    //  请求成功了就要减少负载
                    LOG(ERROR) << "请求服务失败status=" << res->status << endl;
                    m->Decreasement();
                }
                else
                {
                    // 请求失败
                    // 没有得到任何响应
                    LOG(ERROR) << "当前请求的主机无响应,主机ID:" << m->ip << ":" << m->port << ",当前主机可能已经离线" << endl;

                    load_balance.Offlinemachine(id); // 把某台主机下线
                    // load_balance.ShowMachine();      // 只是用来调试
                }
            }

            // 在service.conf可以查看哪个主机上线了
        }
        bool Load(const string &injson, string *out) // 登陆
        {
            return _model.Load(injson, out);
        }
        bool TopicAdd(const string &injson, string *out)
        {
            return _model.TopicAdd(injson, out);
        }
        bool Register(const string &injson, string *out) // 注册
        {
            return _model.Register(injson, out);
        }
    };
};