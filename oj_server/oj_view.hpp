#pragma once
// 充当渲染功能
#include <iostream>
#include <ctemplate/template.h>
// #include "oj_model.hpp"
#include"oj_model.hpp"

using namespace ns_model;

namespace ns_view
{
    const string template_path="./template/";

    class View
    {
    public:
        View()
        {
        }
        ~View()
        {
        }
        void AllExpand(const vector<Question> &all,string*& html)
        {
            
            //题目的编号，标题，难度，使用表格显示
            //形成路径
            string src_html=template_path+"allquestion.html";
            //形成数据字典
            ctemplate::TemplateDictionary root("all_question");
            for(const auto& q:all)
            {
                //形成root的子字典
                ctemplate::TemplateDictionary* sub=root.AddSectionDictionary("question_list");//往root里面添加一个子字典
                //形成一个子字典，往里面添加数据,通过循环遍历，把数据放到里面去
                sub->SetValue("number",q.number);
                sub->SetValue("title",q.title);
                sub->SetValue("star",q.star);

            }   
            //获取被渲染的网页
            ctemplate::Template* tpl=ctemplate::Template::GetTemplate(src_html,ctemplate::DO_NOT_STRIP);
            //开始完成渲染功能
            
            tpl->Expand(html,&root);
            // cout<<*html<<endl;
            //html里面就是渲染成功之后的html网页

        }
        void AExpand(const Question &q,string& html)
        {
            //渲染一张网页
            
            string src_html=template_path+"onequestion.html";
            ctemplate::TemplateDictionary root("one_question");
            root.SetValue("number",q.number);
            root.SetValue("title",q.title);
            root.SetValue("star",q.star);
            root.SetValue("header",q.header);
            root.SetValue("desc",q.desc);

            ctemplate::Template* tpl=ctemplate::Template::GetTemplate(src_html,ctemplate::DO_NOT_STRIP);
            tpl->Expand(&html,&root);
            
        }
    };
};