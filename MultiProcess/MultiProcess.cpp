#include "MultiProcess.h"

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>

#define PROC_DIR "wanjun"

MultiProcess::MultiProcess()
    : pid_(getpid()) {
}

MultiProcess::~MultiProcess() {
    if (pid_ == getpid()) {
        // 只有主进程在析构函数中执行
        for (args_t::iterator iter = child_.begin(); iter != child_.end(); ++iter) {
            iter->second->get_exit_status();
        }
    }
}

// 父进程 新建 Process ， 插入 child_
std::shared_ptr<Process> MultiProcess::fork(std::function<void(int)> function, int index) noexcept {
    if (pid_ != getpid()) {
        // 非父进程不可以使用 fork
        printf("process[%d] can't use this method [fork] parent[%d]", getpid(), pid_);
    } else {
        // Process 构造函数中调用 run , 完成 fork() 调用
        std::shared_ptr<Process> proc(new Process(function, index));
        if (pid_ == getpid()) {
            child_.insert(std::pair<pid_t, std::shared_ptr<Process> >(proc->get_id(), proc));
            return proc;
        }
    }
    return nullptr;
}

// 检查 子进程
void MultiProcess::checkChildren() {
    if (pid_ != getpid()) {
        printf("process[%d] can't use this method [killChildren] parent[%d]\n", getpid(), pid_);
    } else {
        for (args_t::iterator iter = child_.begin(); iter != child_.end(); ++iter) {
            int exitCode = 0;
            // 检查子进程是否有结束的
            if (iter->second->try_get_exit_status(exitCode)) {
                // 重启子进程
                if (!fork(iter->second->func_, iter->second->index_)) {
                    printf("process[%d] fork failded\n", getpid());
                    exit(0);
                }
                // 删除失败的进程 pid，新建的进程拥有 新的 pid
                printf("Child Program[%d] encountered an unrecoverable error, crashed...", iter->first);
                child_.erase(iter);
            }
        }
    }
}

// 杀死所有的 子进程
void MultiProcess::killChildren() {
    if (pid_ != getpid()) {
        printf("process[%d] can't use this method [killChildren] parent[%d]\n", getpid(), pid_);
    } else {
        for (args_t::iterator iter = child_.begin(); iter != child_.end(); ++iter) {
            iter->second->kill();
            iter->second->get_exit_status();
            printf("process[%d] child process[%d] exited...", getpid(), iter->first);
        }
    }
}

void MultiProcess::chkServerRunWpid() {
    //check  dir
    if (access(PROC_DIR, F_OK) < 0) {
        if (mkdir(PROC_DIR, 0755) < 0) {
            printf("mkdir proc err");
            return;
        }
    }

    const char fileName[] = PROC_DIR"/wj_server.pid";

    int fileFd = open(fileName, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fileFd < 0) {
        printf("can't open %s: %m\n", fileName);
        exit(1);
    }

    struct flock fl;
    memset(&fl, 0x00, sizeof(flock));
    fl.l_type = F_WRLCK;  /* write lock */
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;  //lock the whole file


    /*  */
    if (fcntl(fileFd, F_SETLK, &fl) == -1) {
        if (errno == EACCES || errno == EAGAIN) {
            printf("file: %s already locked..\nServer has already running...\n", fileName);
            close(fileFd);
            exit(-1);
        }
        printf("can't lock %s: %m\n", fileName);
        exit(-1);
    }
    /*  */
    char buf[16] = { 0x00 };
    ftruncate(fileFd, 0);
    sprintf(buf, "%ld", (long)getpid());
    write(fileFd, buf, strlen(buf) + 1);
}

void MultiProcess::listPid() const{
    printf("===========[child pid]============\n");
    for(auto iter=child_.begin();iter!=child_.end();++iter){
        printf("[%d]\n",iter->first);
    }
}

void MultiProcess::killPid(pid_t pid){
    auto process = child_.at(pid);
    if(process){
        process->kill();
        process->get_exit_status();
        child_.erase(child_.find(pid));
    }else{
        printf("no process find by pid [%d]\n",pid);
    }
}