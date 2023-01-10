#pragma once
// 最重要的就是进行逻辑控制
#include <iostream>
#include <string>
#include "oj_model.hpp"
#include "../comm/util.hpp"
#include "../comm/log.hpp"
#include"oj_view.hpp"
using namespace std;
namespace ns_control
{
    using namespace ns_view;
    using namespace ns_log;
    using namespace ns_util;
    using namespace ns_model;

    class Control
    {
    private:
        Model _model;//数据
        //还需要有构建成网页的view
        View _view;//提供网页绚烂的功能
    public:
        Control()
        {}
        ~Control()
        {}
        bool GetAllQuestions(string* html)//根据题目数据构建网页
        {
            vector<Question> all;
            if(_model.GetAllQuestion(all))
            {
                //获得成功,将所有的题目数据构成一个网页
                //把所有题目转化成html
                _view.AllExpand(all,html);
                return true;
            }
            else
            {
                *html="获得题目失败，形成题目列表失败";
                return false;
            }
        }
        bool GetAQuestions(string& number,string& html)//根据题目数据构建网页
        {
            Question q;
            if(_model.GetAQuestion(number,q))
            {
                //获得了一个题目的具体信息
                 _view.AExpand(q,html);
                 return true;
            }
            else
            {
                //获取失败了
                html="指定题目"+number+"不存在";
                return false;
            }
        }
    };
};