#ifndef __CF_MULTI_PROCESS_H
#define __CF_MULTI_PROCESS_H

#include <map>
#include <memory>
#include "Process.h"

class MultiProcess {
    typedef std::map< pid_t, std::shared_ptr<Process> > args_t;
public:
    // 单例模式
    static MultiProcess& getInstance() {
        static MultiProcess instance;
        return instance;
    }
    ~MultiProcess();

public:
    virtual std::shared_ptr<Process> fork(std::function<void(int)> function, int index) noexcept;
public:
    void killChildren();
    void checkChildren();
    void chkServerRunWpid();
    void listPid() const;
    void killPid(pid_t pid);

    inline int getSize() const {
        return child_.size();
    }
    inline pid_t get_pid() const {
        return pid_;
    };

private:
    MultiProcess();
    MultiProcess(MultiProcess &) = delete;
    MultiProcess & operator=(const MultiProcess &) = delete;
private:
    pid_t pid_;
    std::map< pid_t, std::shared_ptr<Process> > child_;  // map
};

#endif

