#include <Arduino.h>
#include <WiFiManager.h>
#include <sys/signal.h>

#include "./util/TaskScheduler.h"
#include "./conn/WebSocketClient.h"

TaskScheduler heartbeatScheduler;
TaskScheduler killSwitchScheduler;
WebSocketClient socketClient;
WiFiManager wiFiManager;
bool resetConn = false;

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
    //TODO: receive the pin info remotely
    pinMode(0, OUTPUT);
    Serial.begin(115200);
    wiFiManager.autoConnect("WaterTankPot", "12345677");

    heartbeatScheduler.init(4000, true, []() {
        resetConn = !socketClient.isHeartBeatReceived();
        socketClient.resetHeartBeat();
        if (resetConn) {
            Serial.println("Heartbeat message was not received in time, resetting the connection!");
            return;
        }

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
                Serial.println("Received : " + str);
                if (str.equals("on")) {
                    digitalWrite(0, HIGH);

                    killSwitchScheduler.init(300000, false, []() {
                        digitalWrite(0, LOW);
                        Serial.println("Kill Switch activated!");
                    });

                } else {
                    digitalWrite(0, LOW);
                    killSwitchScheduler.stop();
                }

                socketClient.acknowledge(offset);
            }
    );
}

void loop() {
    killSwitchScheduler.tick();

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Something happen with Wi-Fi!");
        waitForWifi();
    }

    heartbeatScheduler.tick();
    if (resetConn) {
        resetConn = false;
        waitForWifi();
        socketClient.forceReconnect();
    }

    if (!socketClient.tick()) {
        waitForWifi();
        socketClient.forceConnect();
    }
}
