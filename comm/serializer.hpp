#pragma once
#include <assert.h>
#include <string.h>
#include <vector>
#include <algorithm>
#include <memory>
#include <initializer_list>
#include <string>
// 存储数据流的容器
using namespace std;
class Buffer
{
public:
    typedef std::shared_ptr<Buffer> ptr;
    Buffer() : curpos_(0) {}
    Buffer(const char *s, size_t len) : curpos_(0)
    {
        buf.insert(buf.begin(), s, s + len);
    }
    //返回对应的字节数
    const char *data() { return &buf[0]; }          //获得buf的地址
    const char *curdata() { return &buf[curpos_]; } //获得当前的地址
    size_t cursize() const { return buf.size() - curpos_; }
    void offset(int k) { curpos_ += k; }
    void append(const char *s, size_t len) { buf.insert(buf.end(), s, s + len); } //数据插入到buf中,因为时存储char，每个位置都是一个char，所以使用insert插入
    void reset()
    {
        curpos_ = 0;
        buf.clear();
    }

private:
    size_t curpos_;   //当前数据的游标
    vector<char> buf; //用来存储数据,但是如果游标往后进行延伸的话，能不能把前面数据删除
};

// 主机字节序是否小端字节序
static bool isLittleEndian()
{
    static uint16_t flag = 1;
    static bool little_end_flag = *((uint8_t *)&flag) == 1;
    return little_end_flag;
}

class Serializer
{
public:
    typedef std::shared_ptr<Serializer> ptr; //这个可以通过Serializer::ptr；来访问
    Serializer()
    {
        buffer_ = std::make_shared<Buffer>(); //
    }
    Serializer(const char *s, size_t len)
    {
        buffer_ = std::make_shared<Buffer>();
        input(s, len);
    }
    Serializer(Buffer::ptr buffer)
        : buffer_(buffer)
    {
    }

    template <typename T>
    void input_type(T t);//在类里面进行声明，在类外面实现，并且添加偏特化

    template <typename T>
    void output_type(T &t);

    void reset() { buffer_->reset(); }
    void clear() { reset(); }
    void input(const char *data, int len) { buffer_->append(data, len); }//把数据添加到buffer当中

    template <typename Tuple, std::size_t Id>
    void getv(Serializer &ds, Tuple &t)
    {
        ds >> std::get<Id>(t); //获得tuple对应位置的元素
    }

    template <typename Tuple, std::size_t... I>
    Tuple get_tuple(std::index_sequence<I...>)
    {
        Tuple t;
        //使用逗号表达式，如果左边成功的话，就往tuple里面添加对应的数据，否则往tuple里面添加0
        std::initializer_list<int>{(getv<Tuple, I>(*this, t), 0)...};//使用initialzer对tuple进行一个处理实现
        return t;
    }

    template <typename T>
    Serializer &operator>>(T &i)
    {
        output_type(i);
        return *this;
    }

    template <typename T>
    Serializer &operator<<(T i)
    {
        input_type(i);
        return *this;
    }

    const char *data() { return buffer_->curdata(); }//这里的data永远是当前的data
    size_t size() const { return buffer_->cursize(); }
    std::string toString()
    {
        return std::string(data(), size());
    }

private:
    static void byteOrder(char *s, int len)//安字节序来处理字符
    {
        if (littleEndian_)
            std::reverse(s, s + len);
    }

private:
    static bool littleEndian_; //判断当前是否为小端字节序,这个变量属于整个类
    Buffer::ptr buffer_;       //这是一个智能指针类型，并且是符合计算机字节序的进行保存
};
//这个需要在类外初始化
bool Serializer::littleEndian_ = isLittleEndian();

template <typename T>
inline void Serializer::input_type(T v)//处理非字符串类型
{
    size_t len = sizeof(v);//把对应数据的字节大小存入
    char *p = reinterpret_cast<char *>(&v);//将其转化成char*进行处理
    byteOrder(p, len);//按照字节序
    input(p, len);//输入到buffer中
}

// 偏特化
template <>
inline void Serializer::input_type(std::string v)
{
    // 先存入字符串长度
    uint16_t len = v.size();
    input_type(len);
    byteOrder(const_cast<char *>(v.c_str()), len);
    // 再存入字符串
    input(v.c_str(), len);
}

template <>
inline void Serializer::input_type(const char *v)
{
    input_type<std::string>(std::string(v));
}

template <typename T>
inline void Serializer::output_type(T &v)
{
    size_t len = sizeof(v);
    assert(size() >= len);
    ::memcpy(&v, data(), len);//从内存里面进行提取数据
    buffer_->offset(len);
    byteOrder(reinterpret_cast<char *>(&v), len);
}

// 偏特化
template <>
inline void Serializer::output_type(std::string &v)
{
    uint16_t strLen = 0;
    output_type(strLen);
    v = std::string(data(), strLen);
    buffer_->offset(strLen);
    byteOrder(const_cast<char *>(v.c_str()), strLen);
}
