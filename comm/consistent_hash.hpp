/*
    step1：插入存储节点，也就是虚拟节点（上图中A1,A2，B1,B2）这里需要记录两个信息
        1. 每个虚拟节点对应的哪个真实节点
        2. 哈希环上每个哈希值对应哪个虚拟节点
    step2:给每个数据分配存储的节点，先通过哈希函数计算出hash值，然后可以找到对应的虚拟节点，再映射到正式的节点上

*/

#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <set>
using namespace std;

class ConsistentHash
{
private:
    map<uint32_t, string> serverNodes; // 虚拟节点，key是虚拟节点的哈希值，value就是机器的真实ip
    set<string> physicalNodes;         // 真实的节点IP
    int virtualNOdeNum;                // 每个机器关联的虚拟节点个数
public:
    ConsistentHash(const int _virtualNOdeNum)
        : virtualNOdeNum(_virtualNOdeNum)
    {
        physicalNodes.insert("192.168.1.101"); // 给他初始化一些物理机器
        physicalNodes.insert("192.168.1.102");
        physicalNodes.insert("192.168.1.103");
        physicalNodes.insert("192.168.1.104");
    }
    ~ConsistentHash()
    {
        serverNodes.clear();
    }
    // 32位的Flower-Noll-Vo哈希算法,根据key获得一个hash值
    static uint32_t FNVHash(string key)
    {
        const int p = 16777619;
        uint32_t hash = 2166136261;
        for (int idx = 0; idx < key.size(); idx++)
        {
            hash = (hash ^ key[idx]) * p;
        }
        hash += hash << 13;
        hash ^= hash >> 7;
        hash += hash << 3;
        hash ^= hash >> 17;
        hash += hash << 5;
        if (hash < 0)
            hash = -hash;
        return hash;
    }
    void initialize() // 对这里的一致性hash函数进行一个初始化
    {
        for (auto &ip : physicalNodes)
        {
            for (int j = 0; j < virtualNOdeNum; j++) // 这里每个物理机关联上virtualnode个虚拟节点
            {
                // stringstream是C++的一个流对象，方便的读写字符串和数字等各个数据类型<<就是流输入，>>就是流输出
                stringstream nodekey;
                nodekey << ip << "#" << j;
                uint32_t partition = FNVHash(nodekey.str());
                serverNodes[partition] = ip; // 这里我们先绑定好
            }
        }
    }
    // 插入一个物理节点时，同时插入其关联的所有虚拟节点
    void AddNewPhysicalNode(const string nodeip)
    {
        // 如果该物理机器不存在就可以添加
        if (physicalNodes.find(nodeip) == physicalNodes.end())
        {
            physicalNodes.insert(nodeip);

            for (int j = 0; j < virtualNOdeNum; j++)
            {
                stringstream nodekey;
                nodekey << nodeip << "#" << j;
                uint32_t partition = FNVHash(nodekey.str());
                serverNodes[partition] = nodeip; // 这里把新的物理节点也要生成相应的虚拟节点，进行映射
            }
        }
        // 如果该物理机器存在，就需要进行添加
    }
    // 删除一个物理节点,同时他对应的虚拟节点也要删除
    void DeletePhysicalNode(const string nodeip)
    {
        // 如果该物理机器存在，才能进行删除
        if (physicalNodes.find(nodeip) != physicalNodes.end())
        {
            for (int j = 0; j < virtualNOdeNum; j++)
            {
                stringstream nodekey;
                nodekey << nodeip << "#" << j;
                uint32_t partition = FNVHash(nodekey.str());
                auto it = serverNodes.find(partition); // 找到了对应的虚拟节点的hash值的key
                if (it != serverNodes.end())
                {
                    serverNodes.erase(it);
                }
            }
            physicalNodes.erase(nodeip); // 这里直接再实际物理机上删除对应的机器
        }
        //如果该机器不存在就不能够进行删除
    }

    // 根据某个数据key，获得对应的虚拟节点的ip再获得对应的物理节点的ip
    string GetserverIndex(const string key)
    {
        uint32_t partition = FNVHash(key);
        // lower_bound时一个获得>=target的第一个元素
        // vector<int> v{ 1, 3, 5, 7, 9 };
        // int target = 6;
        // auto it = std::lower_bound(v.begin(), v.end(), target);，这个地方返回的就是7,7是>=6的第一个元素

        // 所以这个地方因为顺时针往下走，所以就能够获得这个key对应的最接近的一个虚拟节点

        auto it = serverNodes.lower_bound(partition);
        // 言环顺时针找到第一个大于等于partition的虚拟节点
        if (it == serverNodes.end()) // 这个地方发现没有找到一个比该节点大的值
        {
            if (serverNodes.empty()) // 一个节点都没有
            {
                cout << "no available servers node" << endl;
            }
            return serverNodes.begin()->second; // 所以就把第一个节点给行了，因为这是一个环
        }
        return it->second;
    }

    // 给区间[objmin,objmax] 上的数据寻找合适的存储节点
    void statisticpref(string label, int objmin, int objmax)
    {
        map<string, int> cnt;
        for (int i = objmin; i <= objmax; i++)
        {
            string nodeip = GetserverIndex(to_string(i));
            cnt[nodeip]++;
        }
        int total = objmax - objmin + 1;
        cout << "----" << label << "-----" << endl;
        for (auto &p : cnt)
        {
            cout << "nodeip:" << p.first << " rate: " << 100 * p.second / (total * 1.0) << "%" << endl;
        }
    }
    //获得在线机器的个数
    size_t size()
    {
        return physicalNodes.size();
    }
    //判断在线机器是否有机器
    bool empty()
    {
        return physicalNodes.empty();
    }
};