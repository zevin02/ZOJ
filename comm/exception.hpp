#pragma once
#include "log.hpp"
namespace ns_exception
{
    using namespace ns_log;
    class Exception
    {
    protected:
        string _header; // 日志的前置信息
        string _errmsg; // 错误码描述
        // int _id;        // 错误码描述编号
    public:
        Exception(const string &header, const string &errmsg)
            : _errmsg(errmsg), _header(header)
        {
        }
        ~Exception()
        {
        }
        virtual string what() const // 虚函数，需要被后面进行重写
        {
            string ret = _header + _errmsg;

            return ret;
        }
    };
    //添加
    class RedisException : public Exception
    {
        private:
        string _command;
        public:
        RedisException(const string &header, const string &errmsg,const string &command)
        :Exception(header,errmsg),_command(command)
        {
        }
        virtual string what() const //添加redis报错
        {
            string ret = _header + "RedisException->" + _errmsg + "command:" + _command;
            return ret;
            
        }
    };
    class SqlException : public Exception
    {
    private:
        string _sql;

    public:
        SqlException(const string header, const string &errmsg, const string &sql)
            : Exception(header, errmsg), _sql(sql)
        {
        }
        virtual string what() const
        {
            // sql类完成对what的重写
            string ret = _header + "SqlException->" + _errmsg + ",sql:" + _sql;
            return ret;
        }
    };
    class CompilerException:public Exception
    {
        private:
        public:
        CompilerException(const string& header,const string &errmsg)
        :Exception(header,errmsg)
        {
        }
        virtual string what()const 
        {
            string ret=_header+"CompileException->"+_errmsg;
            return ret;
        }
    };

};
