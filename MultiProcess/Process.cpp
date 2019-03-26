#include "Process.h"

#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>


Process::Process(std::function<void(int)> function, int index) noexcept
    : pid_(0)
    , func_(function)
    , index_(index) {
    run();
}

Process::~Process() noexcept{
    kill(true);
    get_exit_status();
}

void Process::run() {
    // 调用 system fork 函数，从此时 进程一分为二
    pid_t pid =fork();
    if (pid < 0) { //error in fork!
        fprintf(stderr, "fork error!\n");
        // fprintf(stdout,"stdout printf test\n");
        printf("process[%d] fork error", getpid());
    } else if (pid == 0) { //child process
        printf("current child process<==%d==>\n", getpid());
        // 执行子进程的 执行函数
        if (func_)
            func_(index_);
        // 子进程执行完毕后 退出
        _exit(EXIT_SUCCESS);
    } else { //parent process
        // 父进程打印信息
        // printf("process[%d] fork child process[==%d==]\n", getpid(), pid);
        pid_ = pid;
    }
}

int Process::get_exit_status() noexcept {
    if (pid_ <= 0)
        return -1;

    int exit_status;
    // 等待进程结束
    waitpid(pid_, &exit_status, 0);

    if (exit_status >= 256)
        exit_status = exit_status >> 8;
    return exit_status;
}

// 尝试获得 子进程退出的 status
bool Process::try_get_exit_status(int &exit_status) noexcept {
    if (pid_ <= 0)
        return false;

    if (0 == waitpid(pid_, &exit_status, WNOHANG))
        return false;

    if (exit_status >= 256)
        exit_status = exit_status >> 8;

    return true;
}

void Process::kill() noexcept {
    Process::kill(pid_);
}

// static
void Process::kill(pid_t id) noexcept {
    if (id <= 0)
        return;

    ::kill(id, SIGKILL);
}
