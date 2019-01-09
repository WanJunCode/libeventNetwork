#include <log4cpp/Category.hh>
#include <log4cpp/FileAppender.hh>
#include <log4cpp/BasicLayout.hh>
#include <log4cpp/PatternLayout.hh>
#include <log4cpp/OstreamAppender.hh>

#include <iostream>

// Layout -> OstreamAppender -> Category
//           Appender
void test()
{
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

// Layout -> Appender -> Category
int main()
{
    test1();
    return 0;
}
