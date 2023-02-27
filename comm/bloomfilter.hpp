#include <iostream>
using namespace std;
#include <bitset>
#include <string>
#include <vector>



//使用了仿函数
struct BKDRHash //特化，全特化,这个是最强的
{
    size_t operator()(const string &key)
    {
        size_t ret = 0;
        for (auto e : key)
        {
            ret *= 31;
            ret += e;
        }
        return ret;
    }
};

struct APHash
{
    size_t operator()(const string &s)
    {
        size_t hash = 0;
        size_t ch;
        for (long i = 0; i < s.size(); i++)
        {
            if ((i & 1) == 0)
            {
                hash ^= ((hash << 7)) ^ s[i] ^ (hash >> 3);
            }
            else
            {
                hash ^= (~(hash << 11)) ^ s[i] ^ (hash >> 5);
            }
        }
        return hash;
    }
};

struct DJBHash
{
    size_t operator()(const string &s)
    {
        size_t hash = 5381;
        for (auto ch : s)
        {
            hash += (hash << 5) + ch;
        }
        return hash;
    }
};

template <size_t N, class K = string, class HashFunc1 = BKDRHash, class HashFunc2 = APHash, class HashFunc3 = DJBHash> //给个默认类型,一个值我们让他映射3个位置
class BloomFilter
{
private:
    bitset<N * 4> _bs; //给个模板参数，要开多少bit位
public:
    void set(const K &key)
    {
        //标记成存在
        HashFunc1 h1;
        HashFunc2 h2;
        HashFunc3 h3;

        size_t len = 4 * N;
        size_t index1 = h1(key) % len;
        size_t index2 = h2(key) % len;
        size_t index3 = h3(key) % len;
        // cout << index1 << endl;
        // cout << index2 << endl;
        // cout << index3 << endl;
        _bs.set(index1);
        _bs.set(index2);
        _bs.set(index3);
    }

    bool test(const K &key)
    {
        //因为我们映射到了3个位置，所以一个在不能说明它就在，但是如果一个不在，他就不在
        HashFunc1 h1;
        HashFunc2 h2;
        HashFunc3 h3;

        size_t len = 4 * N;
        size_t index1 = h1(key) % len;
        size_t index2 = h2(key) % len;
        size_t index3 = h3(key) % len;
        // cout << index1 << endl;
        // cout << index2 << endl;
        // cout << index3 << endl;
        if (_bs.test(index1) == false)
            return false;
        if (_bs.test(index2) == false)
            return false;
        if (_bs.test(index3) == false)
            return false;

        //前面只要有一个位不在，就是不在
        //走到这里，就是三个位都存在，但是也可能会出现误判
        return true;
    }
    void reset(const K& key)
    {
        //把对应的位删除,删除可能会把冲突的删掉了,所以不支持删除
        //可能会影响其他值
        //那么如何扩展一下，使得布隆过滤器能够支持删除
        //每个位置存储多个bit位，存储引用计数，（有多少个值映射到了当前的位置）
        //支持删除，整体消耗空间变多了，但是效率也就下降了
        
        
    }
};