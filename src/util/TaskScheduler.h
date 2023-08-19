#ifndef TASKSCHEDULER_H
#define TASKSCHEDULER_H

#include <Arduino.h>

typedef void (*ScheduleCallback)();

class TaskScheduler {
private:
    unsigned long startMillis;
    unsigned long interval;
    ScheduleCallback callback;
    bool running = false;
    bool loop = false;

public:
    void init(long intervalMillis, bool isLoop, const ScheduleCallback &callbackFunc);
    void reset();
    void stop();
    void tick();
};

#endif // TASKSCHEDULER_H
