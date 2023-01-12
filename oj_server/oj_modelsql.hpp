#pragma once
// 文件版本
// 1.题目的编号，2.题目的标题，3,题目的难度，4.题目的时间要求 5.题目的时间和空间要求 6.题目的题面
// 需要有两批文件构成
// 1.题目列表（不需要题目的内容）
// 2.一个题的具体描述，预设置的代码,测试用例代码

// 这两个内容是通过文件的编号来产生关联的
// oj从用户获得的代码中，将提交上来的代码和对应的测试用户进行拼接，之后再交给compile_run
#include <iostream>
#include <string>
#include <unordered_map>
using namespace std;
#include <cassert>
#include "../comm/log.hpp"
#include <vector>
#include "../comm/util.hpp"
#include <mysql/mysql.h>
// 将所有的题目加载到内存中
namespace ns_model
{
    using namespace ns_util;
    using namespace ns_log;
    struct Question
    {
        string number; // 题目的编号
        string title;  // 题目的标题
        string star;
        int cpulimit;
        int memlimit;
        string desc; //
        string header;
        string tail; // 题目的测试用例
    };

    string host = "127.0.0.1";
    int port = 3306;
    string db = "oj"; // 选择数据库
    string user = "oj_client";
    string passwd = "@123456!!";

    class Model
    {

    public:
        Model()
        {
        }
        ~Model()
        {
        }
        // 加载所有的题目
        bool QueryMysql(const string sql, vector<Question> &out)
        {
            MYSQL *my = mysql_init(nullptr); // 创建一个mysql句柄
            // 连接数据库成功
            if (mysql_real_connect(my, host.c_str(), user.c_str(), passwd.c_str(), db.c_str(), port, nullptr, 0) == nullptr) // 连接数据库
            {
                LOG(FATAL) << "连接数据库失败" << endl;
                return false;
            }
            LOG(INFO) << "连接数据库成功" << endl;
            // 执行sql语句
            if (mysql_query(my, sql.c_str()) != 0)
            {
                LOG(WARNING) << sql << " sql语句执行失败 " << endl;
                return false;
            }
            // 提取结果
            MYSQL_RES *res = mysql_store_result(my);

            int row = mysql_num_rows(res);   // 获得行树
            int col = mysql_num_fields(res); // 获得列数
            for (int i = 0; i < row; i++)
            {
                // 获取一行的数据
                MYSQL_ROW row = mysql_fetch_row(res);
                Question q;
                q.number = row[0]; // 获取第一列的数据
                q.title = row[1];
                q.star = row[2];
                q.desc = row[3];
                q.header = row[4];

                q.tail = row[5];

                q.cpulimit = atoi(row[6]);

                q.memlimit = atoi(row[7]);

                out.push_back(q);
            }
            // 分析结果

            mysql_free_result(res);

            mysql_close(my); // 关闭mysql链接
            LOG(INFO) << "数据库关闭成功" << endl;
            return true;
        }
        bool GetAllQuestion(vector<Question> &out) // 获得所有题目
        {
            const string sql = "select * from oj_question;";
            return QueryMysql(sql, out);
        }
        bool GetAQuestion(string number, Question &q) // 获得一个题目
        {
            bool res = false;
            string sql = "select * from oj_question where number=";
            sql += number;
            sql += ";";
            vector<Question> out;
            if (QueryMysql(sql, out))
            {
                if (out.size() == 1)
                {
                    q = out[0];
                    res = true;
                }
            }
            return res;
        }
    };

};