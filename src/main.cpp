#include <Arduino.h>
#include <WiFiManager.h>
#include "./util/TaskScheduler.h"
#include "./conn/WebSocketClient.h"

TaskScheduler heartbeatScheduler;
WebSocketClient socketClient;
WiFiManager wiFiManager;

void waitForWifi() {
    // waiting for connection to Wi-Fi network
    Serial.println("Waiting for connection to Wi-Fi");
    int c = 0;
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
    wiFiManager.autoConnect("WaterTankPot", "12345677");

    heartbeatScheduler.init(4000, true, []() {
        String ping = "|ping|";
        socketClient.sendMessage(ping);
    });

    String server = "connector.cyeserv.fun";
    String sockUrl = "/public/v1/device/connect";
    constexpr int port = 443;
    constexpr bool isSecure = true;

    waitForWifi();

    socketClient.init(
            isSecure,
            server,
            port,
            sockUrl,
            false,
            [](String str, String &offset) {
                Serial.println("Got: " + str);

                String res = "We just got " + str;

                socketClient.sendMessageAndAcknowledge(str, offset);
            }
    );
}

void loop() {
    heartbeatScheduler.tick();
    if (!socketClient.tick()) {
        socketClient.forceConnect();
    }
}
