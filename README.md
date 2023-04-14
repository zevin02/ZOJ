- [综述](#综述)
- [依赖](#依赖)
- [安装](#安装)
- [文件架构](#文件架构)
- [效果](#效果)
- [编译服务器](#编译服务器)
- [在线服务器](#在线服务器)
- [数据库](#数据库)
  - [MySQL图表](#mysql图表)
  - [Redis](#redis)
- [RPC](#rpc)

# 综述

这个项目可以让用户可以在平台上进行做题和判题。用户可以通过登录和注册功能方便地使用平台。平台提供了多道题目供用户选择，也可以自己导入，并且可以直接在网页上进行代码的编写和编辑。当用户完成编写后，平台可以对代码进行编译和运行，并实时返回运行结果，让用户可以直接查看和分析自己的代码运行情况。

![在这里插入图片描述](https://img-blog.csdnimg.cn/39e08568ff414bfa86f0901e20192420.png)
#  依赖
1. MySQL  
2. Redis
3. cpp-http 
4. Redis
5. ZeroMQ
6. jsoncpp
7. boost标准库
8. ctemplate

# 安装
~~~c
git clone git@github.com:zevin02/OnlineJudge.git
cd OnlineJudge
make
~~~
# 文件架构
* comm
存放的就是公共模块
![在这里插入图片描述](https://img-blog.csdnimg.cn/d416492bef1946fba81897b738b01a19.png)
* oj_server
存放oj_server端代码
	* www_root 
![在这里插入图片描述](https://img-blog.csdnimg.cn/e2e13cc49327421aa5bf28c775f9bfe1.png)
	*  src 
	![在这里插入图片描述](https://img-blog.csdnimg.cn/8976496a27084a648bed2f9ef25555cb.png)
* compile_server
存放compile_server端代码
![在这里插入图片描述](https://img-blog.csdnimg.cn/b77dddfffe524531a4e07e8b93caa7f2.png)
# 效果
![在这里插入图片描述](https://img-blog.csdnimg.cn/351277984473464488864b6aec8cab67.png)
![在这里插入图片描述](https://img-blog.csdnimg.cn/2dfb141411f949d2afc80e326c203b76.png)![在这里插入图片描述](https://img-blog.csdnimg.cn/2510edba06c84f569635190f991118a3.png)


![请添加图片描述](https://img-blog.csdnimg.cn/da2f1f62fe7c4323856bae04b77fba5a.png)

![请添加图片描述](https://img-blog.csdnimg.cn/02838932a1434c7b98aae81a5b45bb79.png)


![请添加图片描述](https://img-blog.csdnimg.cn/610c495c5dd345cab62a77b5f02b425a.png)


# 编译服务器
![请添加图片描述](https://img-blog.csdnimg.cn/0529b9fe58c242f98900e5d746285a6c.png)


* 在线服务器将用户需要进行判断的代码通过`RPC`交给后端的判题服务器
* 判题服务器将获得的代码交给 `compiler_runner`类，来执行代码的编译和运行的功能
* `compile_runner `分别将代码先交给 `compile`类，进行代码的编译，生成一个可执行文件
* `compile_runner`再将生成的可执行文件交给 `runne`r类，来进行代码的运行，并将结果通过 `重定向`生成 `临时文件`的方式层层递进，同时服务器也会生成相应的 `日志`，反馈给用户

使用层状结构，实现代码之间的 `解耦`，`低耦合，高类聚`

> 判题服务器一经上线，就会发出 `http请求`申请加入到一致性hash类的机器中，供在线服务器进行选择，实现 `负载均衡`

# 在线服务器

1. 获得首页的首页
2. 获得网页的题目列表
3. 用户的登陆，注册，题库的增删改查
4. 题目的编译运行

在线服务器使用了 `MVC`架构
![请添加图片描述](https://img-blog.csdnimg.cn/6df3413712ac4844af3e6e1b288d27c9.png)


请求服务的编译运行功能就需要用到 `负载均衡`模块

![请添加图片描述](https://img-blog.csdnimg.cn/a58d238732a9422cbac6e2baf2f5178b.png)
使用一致性hash算法来实现负载均衡的选择（添加虚拟节点，解决请求分布不均匀的问题）
# 数据库
## MySQL图表
提前加载进可供选择的后端编译机器

| ip | port |
| -- | ---- |

题目信息

| 题号 | 标题 | 难度 | 题目表述 | 显示给用户的函数 | 测试样例 | cpu的时间限制 | 内存限制 |
| ---- | ---- | ---- | -------- | ---------------- | -------- | ------------- | -------- |

用户

| 用户名 | 密码 |
| ------ | ---- |

## Redis
使用HASH来缓存注册用户信息
`key:user:username`
|  filed| value |
|--|--|
| username |  
| password| 

同时使用`布隆过滤器`将注册过的**用户数据缓存**起来，避免**Redis**的**缓存穿透**
# RPC
使用`C++14`基于`ZeroMQ`实现的一个RPC框架
`Buffer类`：字符数组，存储网络发送有效的数据
`Serializer类`：将用户的数据进行一个序列化，符合计算机的字节序列
`rpc类`：实现客户端和服务器的通信功能


使用`map`来将用户需要执行的`函数名(string)`和对应的`函数(function)`进行绑定

![在这里插入图片描述](https://img-blog.csdnimg.cn/f5e0894f0c6c4143b10101d112d56e56.png)
