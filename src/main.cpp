#include <Arduino.h>
// #include <WiFiManager.h>

#include "./util/TaskScheduler.h"
#include "./conn/WebSocketClient.h"
#include "util/Logger.h"

TaskScheduler heartbeatScheduler;
TaskScheduler killSwitchScheduler;
WebSocketClient socketClient;
// WiFiManager wiFiManager;
bool resetConn = false;

void waitForWifi() {
    Logger::log("Waiting for connection to Wi-Fi");
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
    Logger::log("Connected to WiFi network.");
}

void setup() {

    if (!LittleFS.begin()) {
        Serial.println("LittleFS Mount Failed");
        return;
    }

    //TODO: receive the pin info remotely
    pinMode(0, OUTPUT);
    Serial.begin(115200);
    // wiFiManager.autoConnect("WaterTankPot", "12345677");
    WiFi.begin("AC6", "BigAssBird1");

    heartbeatScheduler.init(4000, true, []() {
        resetConn = !socketClient.isHeartBeatReceived();
        socketClient.resetHeartBeat();
        if (resetConn) {
            Logger::log("Heartbeat message was not received in time, resetting the connection!");
            return;
        }

        String ping = "|ping|";
        socketClient.sendMessage(ping);
    });

    String server = "connector.cyecize.fun";
    String sockUrl = "/public/v1/device/connect";
    constexpr int port = 80;
    constexpr bool isSecure = false;

    // String server = "cyecize.com";
    // String sockUrl = "/public/v1/device/connect";
    // constexpr int port = 8010;
    // constexpr bool isSecure = false;

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
                    socketClient.sendMessage("Turning on");

                    killSwitchScheduler.init(300000, false, []() {
                        digitalWrite(0, LOW);
                        Serial.println("Kill Switch activated!");
                        socketClient.sendMessage("Kill Switch turning off");
                    });

                } else {
                    digitalWrite(0, LOW);
                    killSwitchScheduler.stop();
                    socketClient.sendMessage("Turning off");
                }

                if (str.equals("force")) {
                    Logger::log("Forcing conn!");
                    socketClient.forceConnect();
                    Logger::log("Forced a conn!");
                }

                socketClient.acknowledge(offset);
            }
    );

    if (socketClient.tick())
    {
        Logger::readAndSendLogs(socketClient);
    }
}

void loop() {
    killSwitchScheduler.tick();

    if (WiFi.status() != WL_CONNECTED) {
        Logger::log("loop -> Something happened with Wi-Fi!");
        waitForWifi();
    }

    heartbeatScheduler.tick();
    if (resetConn) {
        resetConn = false;
        Logger::log("loop -> Resetting connection!");
        waitForWifi();
        if (socketClient.forceReconnect())
        {
            Logger::readAndSendLogs(socketClient);
        }
    }

    if (!socketClient.tick()) {
        Logger::log("loop -> Connection to server dropped!");
        waitForWifi();

        Logger::log("loop -> Retrying server connection!");
        if (socketClient.forceReconnect())
        {
            Logger::readAndSendLogs(socketClient);
        }
    }
}
