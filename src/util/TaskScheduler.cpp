#include "TaskScheduler.h"

void TaskScheduler::init(long intervalMillis, bool isLoop, const ScheduleCallback &callbackFunc) {
    this->interval = intervalMillis;
    this->callback = callbackFunc;
    this->running = true;
    this->loop = isLoop;
}

void TaskScheduler::reset() {
    this->startMillis = millis();
}

void TaskScheduler::stop() {
    this->running = false;
}

void TaskScheduler::tick() {
    if (!this->running) {
        return;
    }

    unsigned long now = millis();

    // In case millis has overflown after 49 days, prevent endless cycle with that.
    if (this->startMillis > now) {
        this->startMillis = now;
    }

    if (now - this->startMillis > this->interval) {
        this->callback();
        this->startMillis = now;

        if (!this->loop) {
            this->running = false;
        }
    }
}
