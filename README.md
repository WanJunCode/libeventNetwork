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