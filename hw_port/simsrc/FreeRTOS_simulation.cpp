#include "FreeRTOS_simulation.h"

#include <csignal>
#include <map>
#include <string>
#include <iostream>

struct TmrInfo {
    std::string name;
    uint32_t timeout;
    bool singleShot;
    void *timerID;
    tmrcb cb;
};

static std::map<timer_t, TmrInfo> tmrLut;

extern "C" {

TimerHandle_t xTimerCreate(const char *name, TickType_t timeout, bool singleShot, void *timerID, tmrcb cb) {
    // create timer
    timer_t *pTmr = new timer_t;
    sigevent sev;
    //sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_signo = SIGRTMIN;
    sev.sigev_value.sival_ptr = pTmr;
    sev.sigev_notify_function = (void(*)(union sigval))cb;
    sev.sigev_notify_attributes = nullptr;

    if (timer_create(CLOCK_REALTIME, &sev, pTmr) != 0) {
        delete pTmr;
        return nullptr;
    }

    // put callback function into the lookup-table
    TmrInfo info = {name, timeout, singleShot, timerID, cb};

    tmrLut[*pTmr] = info;

    return pTmr;
}

BaseType_t xTimerDelete(TimerHandle_t xTimer, TickType_t xBlockTime) {
    // delete timer
    timer_delete(*xTimer);

    // remove callback from lut
    tmrLut.erase(*xTimer);

    // release timer ID resource
    delete xTimer;

    return 0;
}

BaseType_t xTimerStart(TimerHandle_t xTimer, TickType_t xBlockTime) {
    const TmrInfo &info = tmrLut.at(*xTimer);

    itimerspec its;
    its.it_value.tv_sec = info.timeout / 1000;
    its.it_value.tv_nsec = info.timeout % 1000;

    if (!info.singleShot) {
        its.it_interval = its.it_value;
    } else {
        its.it_interval.tv_sec = 0;
        its.it_interval.tv_nsec = 0;
    }

    timer_settime(*xTimer, 0, &its, nullptr);

    return 0;
}

BaseType_t xTimerStop(TimerHandle_t xTimer, TickType_t xBlockTime) {
    itimerspec its;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;
    its.it_value = its.it_interval;

    timer_settime(*xTimer, 0, &its, nullptr);

    return 0;
}

static void tmr_handler(int sig, siginfo_t *si, void *uc) {
    std::cout << "Timer!" << std::endl;
    signal(sig, SIG_IGN);
}

//void FreeRTOS_simulation_init() {
//    struct sigaction sa;
//    sa.sa_flags = SA_SIGINFO;
//    sa.sa_sigaction = tmr_handler;
//    sigemptyset(&sa.sa_mask);
//    sigaction(SIGRTMIN, &sa, nullptr);
//}

}