#ifndef __WJ_POSIX_SIGNAL_H
#define __WJ_POSIX_SIGNAL_H

class PosixSignal {
public:
    static void register_signals();
    static int sigignore(int sig);
private:
    static void signalHandler(int sig);
    static void recordCrashInfo();
};
#endif
