# libeventNetwork


一. 各个类的简述
MainServer
作为最主要的服务器类，中间组合了 PortListener (作为监听器) IOThread (多线程运行libevent)

PortListener
监听器类，负责 new client 的连接

IOThread
io线程，使用多线程减轻主线程的负担 

TSocket
对tcp底层api的封装

TConnection
对 TSocket & bufferevent 的封装，更好的处理 client 的 数据读取发送, 会在绑定的IOThread上的eventbase创建事件处理


二. 业务流程
具体流程
PortListener 在 MainServer 中启动，开启一个监听器绑定在 MainServer 的 eventBase 上。

// 新的客户端连接
PortListener  ==>  TSocket  ==>  MainServer ==> TConnection  =(使用管道)=>  IOThread  ==>  libevent 监听事件
通过管道传输的实际是TConnection的指针，在64位操作系统上指针大小为8字节

TConnection创建分为两个阶段
1.初始化阶段在IOThread的event_base上创建bufferevent、设置bufferevent回调函数、设置bufferevent的水位、暂时关bufferevent。
2.在主线程上使用conn->notify()将指针通过管道发送给IOThread，在IOThread中完成transition()函数：开启bufferevent，设置水位

// 客户端断开连接
TConnection  ==>  MainServer  ==>  PortListener 回收 TSocket
                              ==>  回收 TConnection
                              ==>  TConnection  设置内部 socket 为 null

// 客户端数据处理
bufferevent设置的read_cb()   ==>  workSocket()  ==>  recv_framing()
// 客户端关闭处理
bufferevent设置的error_cb()  ==>  TConnection::close()  ==>  MainServer::returnTConnection()  ==>  PortListener::returnTSocket()

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