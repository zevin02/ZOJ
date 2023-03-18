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
// 将所有的题目加载到内存中
namespace ns_model
{
    using namespace ns_util;
    using namespace ns_log;

    string questionlist = "./questions/question.list"; // 题目列表
    class Model
    {
    private:
        // 基于题号映射到题目细节
        unordered_map<string, Question> questions;

    public:
        Model()
        {
            assert(LoadQuestionList(questionlist));
        }
        ~Model()
        {
        }
        // 加载所有的题目
        bool LoadQuestionList(const string &quesionlist)
        {
            // 加载配置文件：question/questions.list+题目的编号文件
            ifstream ifs(quesionlist, ios::in | ios::binary);
            if (!ifs.is_open())
            {
                LOG(FATAL)<<"Loading topics fails,please examin if the topics exists"<<endl;
                return false;
            }
            string line;
            while (getline(ifs, line))
            {
                // 按字符串切分
                vector<string> out;                     // 切分的字符串放进去
                StringUtil::CutString(line, &out, " "); // 按照空格进行了切分
                // 1 判断回文数 简单 1 10000
                if(out.size()!=5)
                {
                    //如果不等于5,
                    LOG(WARNING)<<"load some topics fail,please check the format of the file"<<endl;
                    continue;
                }
                // 填充question?
                Question q;
                q.number=out[0];
                q.title=out[1];
                q.star=out[2];
                q.cpulimit=stoi(out[3]);
                q.memlimit=stoi(out[4]);
                string question_desc="./questions/"+q.number+"/desc.txt";
                //打开这个文件
                FileUtil::ReadFile(question_desc,q.desc,true);
                string question_tail="./questions/"+q.number+"/tail.cpp";
                FileUtil::ReadFile(question_tail,q.tail,true);
                string question_head="./questions/"+q.number+"/header.cpp";
                FileUtil::ReadFile(question_head,q.header,true);
                questions.insert(make_pair(q.number,q));
            }

            LOG(INFO)<<"load topics success"<<endl;
            ifs.close();
            return true;
        }

        bool GetAllQuestion(vector<Question> &out) // 获得所有题目
        {
            if (!questions.size()){
                LOG(ERROR)<<"user getallquestions fail"<<endl;
                return false;
            }
            // 把里面的数据都放进去
            for (auto &pair : questions)
            {
                out.push_back(pair.second);
            }
            return true;
        }
        bool GetAQuestion(string number, Question &q) // 获得一个题目
        {
            auto iter = questions.find(number);
            if (iter == questions.end())
            {
                LOG(ERROR)<<"user get a question fail,number:"<<number<<endl;

                return false;
            }
            q = iter->second;
            return true;
        }
    };

};