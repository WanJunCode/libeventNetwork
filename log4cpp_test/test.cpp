#include <log4cpp/Category.hh>
#include <log4cpp/FileAppender.hh>
#include <log4cpp/BasicLayout.hh>
#include <log4cpp/PatternLayout.hh>
#include <log4cpp/OstreamAppender.hh>
#include <log4cpp/StringQueueAppender.hh>
#include <log4cpp/RollingFileAppender.hh>
#include <log4cpp/NDC.hh>
#include <log4cpp/PropertyConfigurator.hh>
#include <log4cpp/CategoryStream.hh>

#include <iostream>

void test()
{
    // Layout -> OstreamAppender -> Category
    //           Appender
    log4cpp::OstreamAppender* osAppender =new log4cpp::OstreamAppender("osAppender",&std::cout);
    osAppender->setLayout(new log4cpp::PatternLayout());
    log4cpp::Category& root =log4cpp::Category::getRoot();
    root.addAppender(osAppender);
    root.setPriority(log4cpp::Priority::DEBUG);
    
    root.error("Hello log4cpp in aError Message!");
    root.warn("Hello log4cpp in aWarning Message!");

    log4cpp::Category::shutdown();
}

void test1()
{
    // PatternLayout --> OstreamAppender --> Category
    log4cpp::OstreamAppender* osAppender = new log4cpp::OstreamAppender("osAppender",&std::cout);
    log4cpp::PatternLayout* pLayout = new log4cpp::PatternLayout();
    pLayout->setConversionPattern("%d: %p %c %x: %m%n");

// %c category
// %d 日期；日期可以进一步的设置格式，用花括号包围，例如%d{%H:%M:%S,%l} 或者 %d{%d %m %Y%H:%M:%S,%l}。如果不设置具体日期格式，则如下默认格式被使用“Wed Jan 02 02:03:55 1980”。日期的格式符号与ANSI C函数strftime中的一致。但增加了一个格式符号%l，表示毫秒，占三个十进制位
// %m 消息
// %n 换行符，会根据平台的不同而不同，但对于用户透明
// %p 优先级
// %r 自从layout被创建后的毫秒数
// %R 从1970年1月1日0时开始到目前为止的秒数
// %u 进程开始到目前为止的时钟周期数
// %x NDC

    osAppender->setLayout(pLayout);
    log4cpp::Category& root =log4cpp::Category::getRoot();
    log4cpp::Category& infoCategory =root.getInstance("infoCategory");
    infoCategory.addAppender(osAppender);
    infoCategory.setPriority(log4cpp::Priority::INFO);
    
    infoCategory.info("system isrunning");
    infoCategory.warn("system has awarning");
    infoCategory.error("system hasa error, can't find a file");
    infoCategory.fatal("system hasa fatal error,must be shutdown");
    infoCategory.info("systemshutdown,you can find some information in systemlog");
    
    log4cpp::Category::shutdown();
}

void test2()
{
    log4cpp::Layout* layout =  new log4cpp::BasicLayout();
    // 2. 初始化一个appender 对象
    log4cpp::Appender* appender = new log4cpp::FileAppender("FileAppender","./test_log4cpp1.log");
    // 3. 把layout对象附着在appender对象上
    appender->setLayout(layout);
    // 4. 实例化一个category对象
    log4cpp::Category& warn_log = log4cpp::Category::getInstance("mywarn");
    // 5. 设置additivity为false，替换已有的appender
    warn_log.setAdditivity(false);
    // 5. 把appender对象附到category上
    warn_log.setAppender(appender);
    // 6. 设置category的优先级，低于此优先级的日志不被记录
    warn_log.setPriority(log4cpp::Priority::WARN);
    // 记录一些日志
    warn_log.info("Program info which cannot be wirten");
    warn_log.debug("This debug message will fail to write");
    warn_log.alert("Alert info");
    // 其他记录日志方式
    warn_log.log(log4cpp::Priority::WARN, "This will be a logged warning");
    log4cpp::Priority::PriorityLevel priority;
    bool this_is_critical = true;
    if(this_is_critical)
        priority = log4cpp::Priority::CRIT;
    else
       priority = log4cpp::Priority::DEBUG;
    warn_log.log(priority,"Importance depends on context");
        

    warn_log.critStream() << "This will show up << as " << 1 << " critical message"<< log4cpp::Priority::ERROR;
  
    // clean up and flush all appenders
    log4cpp::Category::shutdown();
}

void test3()
{
    // BasicLayout --> StringQueueAppender --> Category
    using namespace std;

    log4cpp::StringQueueAppender* strQAppender = new log4cpp::StringQueueAppender("strQAppender");
    strQAppender->setLayout(new log4cpp::BasicLayout());    
    log4cpp::Category& root =log4cpp::Category::getRoot();
    root.addAppender(strQAppender);
    root.setPriority(log4cpp::Priority::DEBUG);

    root.error("Hello log4cpp in a Error Message!");
    root.warn("Hello log4cpp in a WarningMessage!");
    
    cout<<"Get message from MemoryQueue!"<<endl;
    cout<<"-------------------------------------------"<<endl;
    // 获得内存中的 string queue
    queue<string>& myStrQ =strQAppender->getQueue();
    while(!myStrQ.empty())
    {
        cout<<myStrQ.front();
        myStrQ.pop();
    }

    log4cpp::Category::shutdown();
}

void test4()
{
    // PatternLayout -->  Appender            -->  Category
    //                    RollingFileAppender

    using namespace std;
    log4cpp::PatternLayout* pLayout1 = new log4cpp::PatternLayout();
    pLayout1->setConversionPattern("%d: %p %c%x: %m%n");
    log4cpp::PatternLayout* pLayout2 = new log4cpp::PatternLayout();
    pLayout2->setConversionPattern("%d: %p %c%x: %m%n");

    log4cpp::Appender* fileAppender = new log4cpp::FileAppender("fileAppender","wxb.log");
    fileAppender->setLayout(pLayout1);
    log4cpp::RollingFileAppender* rollfileAppender = new log4cpp::RollingFileAppender("rollfileAppender","rollwxb.log",5*1024,3);
    rollfileAppender->setLayout(pLayout2);
    log4cpp::Category& root =log4cpp::Category::getRoot().getInstance("RootName");
    root.addAppender(fileAppender);
    root.addAppender(rollfileAppender);
    root.setPriority(log4cpp::Priority::DEBUG);
    
    for (int i = 0; i < 100; i++)
    {
        string strError;
        ostringstream oss;
        oss<<i<<":RootError Message!";
        strError = oss.str();
        root.error(strError);
    }
    log4cpp::Category::shutdown();
}

void test5()
{
    using namespace std;
    log4cpp::OstreamAppender*osAppender1 = new log4cpp::OstreamAppender("osAppender1",&cout);
    osAppender1->setLayout(new log4cpp::BasicLayout());
    log4cpp::OstreamAppender* osAppender2 = new log4cpp::OstreamAppender("osAppender2",&cout);
    osAppender2->setLayout(new log4cpp::BasicLayout());

    log4cpp::Category& root =log4cpp::Category::getRoot();
    root.setPriority(log4cpp::Priority::DEBUG);
    
    log4cpp::Category& sub1 =root.getInstance("sub1");
    sub1.addAppender(osAppender1);
    sub1.setPriority(log4cpp::Priority::DEBUG);
    sub1.error("suberror");
    
    log4cpp::Category& sub2 =root.getInstance("sub2");
    sub2.addAppender(osAppender2);
    sub2.setPriority(101);
    sub2.warn("sub2warning");
    sub2.fatal("sub2fatal");
    sub2.alert("sub2alert");
    sub2.crit("sub2crit");
    
    log4cpp::Category::shutdown();
}

void test6()
{
    using namespace log4cpp;

    std::cout<< "1.empty NDC: " <<NDC::get()<< std::endl;
    NDC::push("context1");  
    std::cout<< "2.push context1: " <<NDC::get()<< std::endl;
    NDC::push("context2");
    std::cout<< "3.push context2: " <<NDC::get()<< std::endl;
    NDC::push("context3");
    std::cout<< "4.push context3: " <<NDC::get()<< std::endl;

    std::cout<< "5.get depth: " <<NDC::getDepth() <<std::endl;
    std::cout<< "6.pop: " << NDC::pop()<< std::endl;
    std::cout<< "7.after pop:"<<NDC::get()<<std::endl;
    NDC::clear();
    std::cout<< "8.clear: " << NDC::get() <<std::endl;
}

int test7()
{
    try{
        log4cpp::PropertyConfigurator::configure("./log4cpp.conf");
    }
    catch(log4cpp::ConfigureFailure& f)
    {
        std::cout<< "Configure Problem "<< f.what()<< std::endl;
        return -1;
    }

    log4cpp::Category& root =log4cpp::Category::getRoot();
    log4cpp::Category& sub1 =log4cpp::Category::getInstance(std::string("sub1"));
    log4cpp::Category& sub3 =log4cpp::Category::getInstance(std::string("sub1.sub2"));
    sub1.info("This is someinfo");
    sub1.alert("Awarning");
    // sub3 only have A2 appender.

    sub3.debug("This debug messagewill fail to write");
    sub3.alert("All hands abandonship");
    sub3.critStream() <<"This will show up<< as "<< 1 <<" critical message";
    sub3<<log4cpp::Priority::ERROR<<"And this will be anerror";
    sub3.log(log4cpp::Priority::WARN, "This will be a logged warning");
}

// Layout -> Appender -> Category
int main()
{
    test7();
    return 0;
}
