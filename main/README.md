# libeventNetwork


一. 各个类的简述
MainServer
作为最主要的服务器类，中间组合了 MyTransport (作为监听器) IOThread (多线程运行libevent)

MyTransport
监听器类，负责 new client 的连接

IOThread
io线程，使用多线程减轻主线程的负担 

TSocket
对tcp底层api的封装

TConnection
对 TSocket & bufferevent 的封装，更好的处理 client 的 数据读取发送




二. 业务流程
具体流程
MyTransport 在 MainServer 中启动，开启一个监听器绑定在 MainServer 的 eventBase 上。

// 新的客户端连接
MyTransport  ==>  TSocket  ==>  MainServer ==> TConnection  =(使用管道)=>  IOThread  ==>  libevent 监听事件

// 客户端断开连接
TConnection  ==>  MainServer  ==>  MyTransport 回收 TSocket
                              ==>  回收 TConnection
                              ==>  TConnection  设置内部 socket 为 null

// 客户端数据处理


三. logcpp4cpp 的使用
g++ 编译添加 -llogcpp4cpp
参考链接
https://www.ibm.com/developerworks/cn/linux/l-logcpp4cpp/


四. 着手协议的定制
目标: 实现一个聊天室的功能

数据库设计

table user{
    id
    passwd
    name
    sex
    age
    address
    email
}

table group{
    id
    
}


每个　客户端client　可以　单人聊天　和　群聊

package 解析

head:
    headflag
    data length
    cmd         // 发送数据　　enum 
    u2u         // user to user
    u2g         // user to group
// json 格式
data:
    sendid
        [userid]            // 表明自己身份
    receiveid
        [userid]            // 目标 userid
        [groupid]           // 目标 group id
    text
        不定长
tail:
    crc32 校验
    tailflag