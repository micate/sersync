#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include<iostream>

#ifndef _DAEMON_H
#define _DAEMON_H

void Daemon_Stop(int nSignal) {
    pid_t _pid_t;
    int nState;
    while ((_pid_t = waitpid(-1, &nState, WNOHANG)) > 0);
    signal(SIGCLD, Daemon_Stop);
}

int Daemon_Start() {
    std::cout << "daemon start，sersync run behind the console " << std::endl;
    pid_t pid1;
    assert((pid1 = fork()) >= 0);
    if (pid1 != 0) {
        sleep(1);
        exit(0);
    }
    assert(setsid() >= 0);
    umask(0);
    signal(SIGINT, SIG_IGN);
    signal(SIGCLD, Daemon_Stop);
    return 0;
}
#endif
