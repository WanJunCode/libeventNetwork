2019-7-5 准备增加一个http模块
程序运行在　12345　端口
浏览器访问　http://127.0.0.1:12345
就可以得到一个　界面用来观察系统的各种信息

使用　c++ 自己写一个　http 模块？
结合　python django 完成？　python 和　C++ 的　tcp 通信．

.
├── base
│   ├── Atomic.h
│   ├── copyable.h
│   ├── CountDownLatch.cpp
│   ├── CountDownLatch.h
│   ├── CurrentThread.cpp
│   ├── CurrentThread.h
│   ├── Mutex.cpp
│   ├── Mutex.h
│   ├── noncopyable.h
│   ├── Thread.cpp
│   ├── Thread.h
│   ├── ThreadPool.cpp
│   ├── ThreadPool.h
│   ├── Timer.cpp
│   ├── Timer.h
│   ├── TimerManager.cpp
│   └── TimerManager.h
├── build.sh
├── Cedis
│   ├── Command.h
│   ├── Queue_s.h
│   ├── RedisConfig.cpp
│   ├── RedisConfig.h
│   ├── Redis.cpp
│   ├── Redis.h
│   ├── RedisPool.cpp
│   ├── RedisPool.h
│   ├── Reply.cpp
│   └── Reply.h
├── Config
│   └── Log4cpp.conf
├── etc
│   └── config.json
├── main
│   ├── IOThread.cpp
│   ├── IOThread.h
│   ├── logcpp.cpp
│   ├── logcpp.h
│   ├── MainConfig.cpp
│   ├── MainConfig.h
│   ├── main.cpp
│   ├── MainServer.cpp
│   ├── MainServer.h
│   ├── MyTransport.cpp
│   ├── MyTransport.h
│   ├── README.md
│   ├── TConnection.cpp
│   ├── TConnection.h
│   ├── ThreadPool.h
│   ├── Tool.cpp
│   ├── Tool.h
│   ├── TSocket.cpp
│   └── TSocket.h
├── Makefile
├── Mysql
├── Package
│   ├── Adapter
│   │   ├── Adapter.cpp
│   │   ├── Adapter.h
│   │   ├── AdapterMap.cpp
│   │   ├── AdapterMap.h
│   │   ├── ChatAdapter.cpp
│   │   ├── ChatAdapter.h
│   │   ├── ChatProcessor
│   │   │   ├── ChatMessage.cpp
│   │   │   ├── ChatMessage.h
│   │   │   ├── chatPackage.protocal
│   │   │   ├── ChatSignIn.cpp
│   │   │   └── ChatSignIn.h
│   │   ├── EchoAdapter.cpp
│   │   ├── EchoAdapter.h
│   │   └── Process.h
│   ├── ChatPackage.cpp
│   ├── ChatPackage.h
│   ├── CRC.cpp
│   ├── CRC.h
│   ├── EchoPackage.cpp
│   ├── EchoPackage.h
│   ├── MultipleProtocol.cpp
│   ├── MultipleProtocol.h
│   ├── Package.h
│   └── package_test.cpp
├── README.md
├── Signal
│   ├── PosixSignal.cpp
│   └── PosixSignal.h
├── System
│   └── config.h
└── Util
    ├── Timestamp.cpp
    ├── Timestamp.h
    ├── utime.cpp
    └── utime.h

12 directories, 82 files
