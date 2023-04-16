#pragma once
// 最重要的就是进行逻辑控制
#include <iostream>
#include <string>
// #include "oj_model.hpp"
#include "oj_model.hpp"
#include "../comm/util.hpp"
#include "../comm/log.hpp"
#include "../comm/httplib.h"
#include "oj_view.hpp"
#include <jsoncpp/json/json.h>
#include <mutex>
#include <algorithm>
#include <set>
#include "../comm/consistent_hash.hpp"
#include "../comm/rpc.hpp"
using namespace std;
namespace ns_control
{
    using namespace ns_view;
    using namespace ns_log;
    using namespace ns_util;
    using namespace ns_model;
    rpc client;//这里放一个全局的请求后端响应的客户端

    // 提供服务的主机，一台机器对应一个后端编译运行服务
    // 一旦对方机器上线，就会向我们发起http请求，所以不应该我们向对方发起http请求来添加
    class Machine
    {
    public:
        string ip_;
        int port_;
        uint64_t load_; // 负载情况
        mutex *_mtx;   // mutex禁止拷贝，使用指针
    public:
        Machine()
            : ip_(""), port_(0), _mtx(nullptr),load_(0)
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
            ::swap(ip_, m.ip_);
            ::swap(port_, m.port_);
            ::swap(load_, m.load_);
            ::swap(_mtx, m._mtx);
        }

        Machine(Machine &&m) // 移动构造
            : ip_(""), port_(0), load_(0), _mtx(nullptr)
        {
            swap(m);
        }

        Machine &operator=(Machine &&m) // 移动赋值
        {
            ip_ = "";
            port_ = 0;
            load_ = 0;
            _mtx = nullptr;
            swap(m);
            return *this;
        }
        Machine &operator=(const Machine &m) // 拷贝赋值
        {
            ip_ = m.ip_;
            port_ = m.port_;
            load_ = m.load_;
            _mtx = m._mtx;
            return *this;
        }
        // 更新负载
        void Increasement()
        {
            if (_mtx)
            {
                _mtx->lock();
                load_++;
                _mtx->unlock();
            }
        }
        void Decreasement() // 减少负载
        {
            if (_mtx)
            {
                _mtx->lock();
                load_--;
                _mtx->unlock();
            }
        }
        uint64_t GetLoad() const // 获得负载数
        {
            if (_mtx)
            {
                unique_lock<mutex>(*_mtx);//避免return不解锁,使用RAII
                return load_;
            }
            return 0;
        }
        void ResetLoad() // 重置负载
        {
            if (_mtx)
            {
                _mtx->lock();
                load_ = 0;
                _mtx->unlock();
            }
        }
    };
    // 负载均衡的模块,根据配置文件，把所有预先配置的文件全部都加载进来
    class LoadBalance
    {
    private:
        map<int, Machine> machine; // 里面的所有的机器,我们使用端口号来充当主机编号,只要是预先添加到了系统中，就需要加载进来，里面就是我们预先设置了所有会运用的后端机器
        // 保证在选择主机的时候，需要保证数据的安全
        mutex mtx;
        unique_ptr<MySQL> mysql;
        ConsistentHash ch; //添加一致性hash进来

    public:
        LoadBalance()                                 // 构造函数，创建这个对象的时候，就把后端在线的机器加载进去
            : ch(32) // 我们给每个机器配上32个虚拟节点
        {
            mysql=unique_ptr<MySQL>(new MySQL(host, port, db, user, passwd));
            constexpr const char* sql = "select * from machine_list";
            assert(LoadConf(sql) == true); // 启动的时候就加载进来了
            if (!ch.empty())               // 在线不为空才能算接入成功
            {
                LOG(INFO) << "加载后端服务器载入成功" << endl;
            }
        }
        ~LoadBalance()
        {
        }


        bool LoadConf(const char* sql) // 使用包装的数据库类来操作sql中加载机器,修改成，这个加载后面都不会被使用了，所以不需要添加到redis里面
        {

            vector<vector<string>> data;
            if (mysql->Select(sql, data))
            {
                // 成功,所有的数据都在data里面
                for (int i = 0; i < data.size(); i++)
                {
                    Machine ma;
                    ma.ip_ = data[i][0];
                    ma.port_ = stoi(data[i][1]);
                    ma.load_ = 0;
                    ma._mtx = new mutex();
                    // 需要对每个服务器发起http请求，如果在线的话，就添加到online服务器中,否则就添加到offline中

                    machine[ma.port_] = move(ma);
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
            // 这里使用一致性hash算法(使用虚拟节点)来实现负载均衡
            unique_lock<mutex> lock(mtx);
            int online_num = ch.size();
            if (online_num == 0)
            {
                // 没有在线的主机了
                LOG(FATAL) << "所有后端的编译主机已经全部离线了，需要尽快修复" << endl;
                return false;
            }
            // 通过哈希算法来计算一个一个哈希值，再和现在总共的机器数取模得到一个特定的值

            // long key = stol(TimeUtil::GetTimeMs()); // 使用毫秒级时间戳计算一个key
            string key = TimeUtil::GetTimeMs();   // 使用毫秒级时间戳计算一个key
            string port = ch.GetserverIndex(key); // 根据该毫秒级的时间戳获得他对应的机器的端口号

            *id = stoi(port);
            *m = &machine[stoi(port)]; // 因为machine里面包含的就是所有的机器
            lock.unlock();

            if (LogStatus::GetInstance().isDebugEnable())
            {
                LOG(DEBUG) << "key=" << key << ",port=" << (*m)->port_ << endl;
            }
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
            ch.DeletePhysicalNode(to_string(which));
        }

        void OnlineAdd(const string &ip, const int port) // 后端机器上线，添加到在线列表中
        {
            ch.AddNewPhysicalNode(to_string(port)); // 后端机器上线值后，就把他添加到一致性hash里面的机器上
        }
    };

    class Control
    {
    private:
        // 在control中控制数据和视图
        Model _model; // 数据
        // 还需要有构建成网页的view
        View _view;               // 提供网页绚烂的功能
        unique_ptr<LoadBalance> _loadBalance; // 负载均衡器
        // 连接上redis服务器,对mysql进行数据的缓存

    public:
        Control()
        {
            _loadBalance=unique_ptr<LoadBalance>(new LoadBalance);
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

            _loadBalance->OnlineAdd(ip, port);
            out = "机器" + to_string(port) + "连接成功";
            LOG(INFO) << out << endl;
            return true;
        }
        bool GetAllQuestions(string *html) // 根据题目数据构建网页
        {
            vector<tuple<string,string,string>> all;
            if (_model.getQuestionList(all))
            {
                // 获得成功,将所有的题目数据构成一个网页
                // 将所有获得的数据按照序号从小到大进行排序
                // sort(all.begin(), all.end(), [](const tuple<string,string,string> &s1, const tuple<string,string,string> &s2)
                //      { return stoi(get<0>(s1)) < stoi(get<0>(s2)); }); // 使用Lambda表达式
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
            if (_model.getSpecifyQuestion(number, q))
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
            _model.getSpecifyQuestion(number, q);
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
                if (!_loadBalance->SmartChoice(&id, &m))
                {
                    break;
                }
                // 发起http请求
                // 充当一个客户端的角色
                httplib::Client cli(m->ip_, m->port_); // 绑定主机和端口
                // 发起请求
                m->Increasement();
                LOG(INFO) << "选择主机成功,主机ID:" << id << "详情:" << m->ip_ << ":" << m->port_ << "该主机的负载情况:" << m->load_ << endl;
                // 这里根据机器的的端口号如果请求成功，就载入
                client.as_client(m->ip_, m->port_);
                outjson = client.call<string>("Start", compile_string).value();

                if (!outjson.empty()) //如果当前有数据
                {
                    //说明服务运行成功
                    m->Decreasement();
                    cout << "outjson: " << outjson << endl;
                    LOG(INFO) << "请求编译运行服务成功" << endl;
                    break;
                }
                else
                {
                    //服务运行失败
                    LOG(ERROR) << "当前请求的主机无响应,主机ID:" << m->ip_ << ":" << m->port_ << ",当前主机可能已经离线" << endl;
                    _loadBalance->Offlinemachine(id); // 把某台主机下线，并且重新进行请求
                }
                
            }

            // 在service.conf可以查看哪个主机上线了
        }
        bool Login(const string &injson, string *out) // 登陆
        {
            return _model.login(injson, out);
        }
        bool TopicAdd(const string &injson, string *out)
        {
            return _model.topicAdd(injson, out);
        }
        bool Register(const string &injson, string *out) // 注册
        {
            return _model.Register(injson, out);
        }
    };
};