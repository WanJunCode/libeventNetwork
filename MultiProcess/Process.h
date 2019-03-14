#ifndef __WJ_PROCESS_H
#define __WJ_PROCESS_H

#include <functional>

// 进程类
class Process {
    friend class MultiProcess;
public:
    Process(std::function<void(int)> function, int index) noexcept;
    virtual ~Process() noexcept;
public:
    ///Get the process id of the started process.
    inline pid_t get_id() const noexcept {
        return pid_;
    };
    ///Wait until process is finished, and return exit status.
    int get_exit_status() noexcept;
    ///If process is finished, returns true and sets the exit status. Returns false otherwise.
    bool try_get_exit_status(int &exit_status) noexcept;
    ///Kill the process
    void kill() noexcept;
    ///Kill a given process id
    static void kill(pid_t id) noexcept;
protected:
    void run();
protected:
    pid_t pid_;
    std::function<void(int)> func_;
    int index_;
};

#endif
