#include <Arduino.h>
#include <wl_definitions.h>
// #include <WiFiManager.h>

#include "./util/TaskScheduler.h"
#include "util/Logger.h"
#include "util/Restarter.h"
#include "conn/WebSocketClient.h"

TaskScheduler killSwitchScheduler;
WebSocketClient socketClient;
// WiFiManager wiFiManager;
bool resetConn = false;

int failCount = 0;
int retryCount = 0;
long killSwitchMs = 0;

void waitForWifi()
{
    Logger::log("Waiting for connection to Wi-Fi");
    int c = 0;

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print('.');
        c++;

        if (c >= 30)
        {
            Logger::log("WiFi failed after 30 attempts");
            Restarter::restart();
            Serial.println();
            c = 0;
        }
    }

    Serial.println();
    Logger::log("Connected to WiFi network.");
}

void save(const char* path, String& value) {
    File file = LittleFS.open(path, "w");
    if (!file) {
        Serial.println("Failed to open file for writing");
        return;
    }
    file.print(value);
    file.close();
}


String read(const char* path) {
    File file = LittleFS.open(path, "r");
    if (!file) {
        Serial.println("Failed to open file for reading");
        return "";
    }
    String value = file.readStringUntil('\n');  // Read the integer string
    file.close();
    return value;
}

void saveInt(const char* path, long value) {
    String val = String(value);
    save(path, val);
}

long readInt(const char* path) {
    String val = read(path);
    if (val.isEmpty()) {
        return 0;
    }
    return val.toInt();  // Convert the string to integer
}

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("Starting up!");

    if (!LittleFS.begin()) {
        Serial.println("LittleFS Mount Failed");
        return;
    }

    //TODO: receive the pin info remotely
    pinMode(0, OUTPUT);
    Serial.begin(115200);
    // wiFiManager.autoConnect("WaterTankPot", "12345677");
//    WiFi.begin("AC6", "BigAssBird1");
    WiFi.begin("pegi61_mon", "123456789");

    String server = "connector.cyecize.fun";
    String sockUrl = "/public/v1/device/connect";
    constexpr int port = 80;
    constexpr bool isSecure = false;

    // String server = "cyecize.com";
    // String sockUrl = "/public/v1/device/connect";
    // constexpr int port = 8010;
    // constexpr bool isSecure = false;


    killSwitchMs = readInt("/killswitch.txt");
    if (killSwitchMs <= 0) {
        killSwitchMs = 60000; // 1 Min
    }
    Serial.print("Current killswitch is ");
    Serial.println(killSwitchMs);

    waitForWifi();

    bool connected = socketClient.init(
            isSecure,
            server,
            port,
            sockUrl,
            false,
            [](String str, String& offset)
            {
                Serial.println("Received : " + str);
                if (str.equals("on")) {
                    digitalWrite(0, HIGH);
                    socketClient.sendMessage("Turning on");

                    killSwitchScheduler.init(killSwitchMs, false, []() {
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

//                socketClient.acknowledge(offset);

                if (str.equalsIgnoreCase("fk"))
                {
                    socketClient.sendMessage(String(failCount));
                }

                if (str.equalsIgnoreCase("rst")) {
                    socketClient.sendMessage(String(readInt("/resets.txt")));
                }

                if (str.equalsIgnoreCase("rstrst")) {
                    saveInt("/resets.txt", 0);
                }

                if (str.startsWith("ks_")) {
                    str.replace("ks_", "");
                    killSwitchMs = str.toInt();
                    saveInt("/killswitch.txt", killSwitchMs);
                    socketClient.sendMessage("Kill switch set to: " + String(killSwitchMs));
                }

                if (str.equalsIgnoreCase("ks")) {
                    socketClient.sendMessage(String(killSwitchMs));
                }
            }
    );

    if (connected) {
        Logger::readAndSendLogs(socketClient);
    }
}

void loop() {
    killSwitchScheduler.tick();
    if (!socketClient.tick())
    {
        if (retryCount == 0)
        {
            failCount++;
            Logger::log("Lost connection! Checking WiFi");
        }

        retryCount++;

        Serial.println(retryCount);
        if (retryCount > 10)
        {
            Logger::log("10 Ticks and still no connection...");
            Restarter::restart();
        }

        if (WiFi.status() != WL_CONNECTED)
        {
            waitForWifi();
        }

        Logger::log("Trying to reconnect to server!");
        if (socketClient.forceReconnect())
        {
            retryCount = 0;
            Logger::readAndSendLogs(socketClient);
        }
    }
}
