#include "PosixSignal.h"
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include "../main/logcpp.h"

#define array_item_number(array)    (sizeof(array)/sizeof(*(array)))

void PosixSignal::register_signals() {
    /* Install signal handlers */
    int signal_catch_array[] = { SIGQUIT, SIGILL, SIGABRT, SIGBUS, SIGSEGV, SIGTERM, SIGUSR1, SIGCHLD };
    for (size_t i = 0; i < array_item_number(signal_catch_array); ++i) {
        if (SIG_ERR == signal(signal_catch_array[i], &PosixSignal::signalHandler)) {
            fprintf(stderr, "register signal[%d] err:%s", signal_catch_array[i], strerror(errno));
        }
    }

    int signal_ignore_array[] = { SIGINT, SIGHUP, SIGPIPE, SIGXFSZ, SIGCONT, SIGSTOP, SIGPWR, SIGWINCH};
    for (size_t i = 0; i < array_item_number(signal_ignore_array); ++i)
        sigignore(signal_ignore_array[i]);
}

int PosixSignal::sigignore(int sig) {
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;

    if (sigemptyset(&sa.sa_mask) == -1 || sigaction(sig, &sa, 0) == -1)
        return -1;

    return 0;
}

void PosixSignal::signalHandler(int sig) {
    LOG_DEBUG("process[%d] received signal:%d", getpid(), sig);

    switch (sig) {
    case SIGTERM: //�������(terminate)�ź�, ��SIGKILL��ͬ���Ǹ��źſ��Ա������ʹ���ͨ������Ҫ������Լ������˳���shell����killȱʡ��������źš����������ֹ���ˣ����ǲŻ᳢��SIGKILL
        LOG_DEBUG("Process[%d] signal[%d]: Illegal address, including memory address alignment (alignment) error", getpid(), sig);
        break;
    case SIGILL: //ִ���˷Ƿ�ָ��. ͨ������Ϊ��ִ���ļ�������ִ���, ������ͼִ�����ݶ�. ��ջ���ʱҲ�п��ܲ�������ź�
        LOG_DEBUG("Process[%d] signal[%d]: Illegal directives were executed. Usually because of an error in the executable itself", getpid(), sig);
        break;
    case SIGBUS: //�Ƿ���ַ, �����ڴ��ַ����(alignment)�����������һ���ĸ��ֳ�������, �����ַ����4�ı���������SIGSEGV���������ں��������ڶԺϷ��洢��ַ�ķǷ����ʴ�����(����ʲ������Լ��洢�ռ��ֻ���洢�ռ�)
        LOG_DEBUG("Process[%d] signal[%d]: Illegal address, including memory address alignment (alignment) error. For example, a four-word integer is accessed, but its address is not a multiple of 4. It differs from SIGSEGV in that the latter is triggered by illegal access to legitimate storage addresses (such as access that is not part of its own storage space or read-only storage).", getpid(), sig);
        break;
    case SIGABRT: //����abort�������ɵ��ź�
        LOG_DEBUG("Process[%d] signal[%d]: Exit instructions issued by abort (3)", getpid(), sig);
        break;
    case SIGSYS: //�Ƿ���ϵͳ���� (SVID)
        LOG_DEBUG("Process[%d] signal[%d]: Invalid system call (Svid)", getpid(), sig);
        break;
    case SIGFPE: //�ڷ��������������������ʱ����. �������������������, ���������������Ϊ0���������е������Ĵ���
        LOG_DEBUG("Process[%d] signal[%d]: Emitted when a fatal arithmetic operation error occurs. Includes not only floating-point arithmetic errors, but also all other arithmetic errors such as overflow and divisor 0.", getpid(), sig);
        break;
    case SIGSTOP: //ֹͣ(stopped)���̵�ִ��. ע������terminate�Լ�interrupt������:�ý��̻�δ����, ֻ����ִͣ��. ���źŲ��ܱ�����, ��������
        LOG_DEBUG("Process[%d] signal[%d]: Stop (stopped) the execution of the process. Notice the difference between it and terminate and interrupt: The process is not finished, it's just suspending execution. This signal cannot be blocked, processed or ignored.", getpid(), sig);
        break;
    case SIGSEGV: //��ͼ����δ������Լ����ڴ�, ����ͼ��û��дȨ�޵��ڴ��ַд����.
        LOG_DEBUG("Process[%d] signal[%d]: An attempt was made to access memory that was not assigned to itself, or to write data to a memory address that did not have write access. ", getpid(), sig);
        break;
    case SIGQUIT: //��SIGINT����, ����QUIT�ַ�(ͨ����Ctrl-\)������. ���������յ�SIGQUIT�˳�ʱ�����core�ļ�, �����������������һ����������ź�
        LOG_DEBUG("Process[%d] signal[%d]: A process produces a core file when it exits Sigquit, in the sense that it is similar to a program error signal", getpid(), sig);
        break;
    case SIGUSR1:
        PosixSignal::sigignore(SIGCHLD);
        // TMultiProcess::getInstance().killChildren();
        break;
    case SIGCHLD:
        // TMultiProcess::getInstance().checkChildren();
        return;
    default:
        signal(sig, signalHandler);
        return;
    }
#ifndef SUPPORT_MULTI_PROCESS
    recordCrashInfo();
#else
    if (TMultiProcess::getInstance().get_pid() != getpid()) { //�ӽ����쳣���ռ�������Ϣ
        recordCrashInfo();
    }
#endif

    LOG_DEBUG("=========Process[%d] Exit============", getpid());
    _exit(0);
}

void PosixSignal::recordCrashInfo() {
    LOG_DEBUG("Record crash info...\n");
}