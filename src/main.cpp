#include <Arduino.h>
#include "./util/TaskScheduler.h"

TaskScheduler testScheduler;
bool isOn;

void sw() {
    isOn = !isOn;
    digitalWrite(2, isOn);
    Serial.println("Changed!");
}

void setup() {
    Serial.begin(115200);
    pinMode(2, OUTPUT);

    testScheduler.init(500, true, []() {
        sw();
    });
}

void loop() {
    testScheduler.tick();

}

