#include <Arduino.h>
#include <WiFiManager.h>
#include "./util/TaskScheduler.h"
#include "./conn/WebSocketClient.h"

TaskScheduler testScheduler;
WebSocketClient socketClient;
WiFiManager wiFiManager;

bool isOn;

void sw() {
    isOn = !isOn;
    digitalWrite(4, isOn);
    digitalWrite(2, !isOn);
//    digitalWrite(12, isOn);
//   digitalWrite(16, !isOn);
    Serial.println("Changed!");
}

void waitForWifi() {
    // waiting for connection to Wi-Fi network
    Serial.println("Waiting for connection to Wi-Fi");
    int c = 0;
//    WiFi.begin("msi", "12345677");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print('.');
        c++;

        if (c >= 30) {
            Serial.println();
            c = 0;
        }
    }

    Serial.println();
    Serial.println("Connected to WiFi network.");
}

void setup() {
    Serial.begin(115200);
    pinMode(4, OUTPUT);
    pinMode(2, OUTPUT);
//    pinMode(12, OUTPUT);
//    pinMode(16, OUTPUT);
    wiFiManager.autoConnect("Genadi", "12345677");

    testScheduler.init(500, true, []() {
        sw();
    });

    String server = "cyecize.com";
    String sockUrl = "/socket";

    waitForWifi();

    socketClient.init(
            false,
            server,
            4200,
            sockUrl,
            [](String str) {
                Serial.println("Got: " + str);

                String res = "We just got" + str;
                socketClient.sendMessage(res);
            }
    );
}

void loop() {
    testScheduler.tick();
    socketClient.tick();

}

