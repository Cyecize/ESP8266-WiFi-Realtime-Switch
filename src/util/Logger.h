#ifndef ESP8266_WIFI_REALTIME_FEEDER_LOGGER_H
#define ESP8266_WIFI_REALTIME_FEEDER_LOGGER_H

#include "WString.h"
#include "HardwareSerial.h"
#include "LittleFS.h"
#include "../conn/WebSocketClient.h"

class Logger
{
public:
    static void log(const String& val)
    {
        Serial.println(val);
        String time = String(millis()) + " ";

        File file = LittleFS.open("/logs.txt", "a");
        if (!file)
        {
            Serial.println("Failed to open file for writing");
            return;
        }
        file.println(time + val);
        file.close();
    }

    static void readAndSendLogs(WebSocketClient& socketClient)
    {
        File file = LittleFS.open("/logs.txt", "r");
        if (!file)
        {
            Serial.println("Failed to open file for reading");
        }

        int c = 0;
        int retryC = 0;

        while (file.available() > 0)
        {
            String value = file.readStringUntil('\n');
            Serial.print("sending: ");
            Serial.println(value);
            c++;

            if (c > 15) {
                socketClient.sendMessage("|ping|");
                c = 0;
            }

            if (!socketClient.sendMessage(value)) {
                retryC++;
                if (retryC < 10) {
                    if (socketClient.forceReconnect()) {
                        socketClient.sendMessage(value);
                    }
                }
            }

            delay(50);
        }

        file.close();

        delay(500);

        // Clear the file
        File f2 = LittleFS.open("/logs.txt", "w");
        if (!f2)
        {
            Serial.println("Failed to open file for writing");
        }
        f2.print("");
        f2.close();
    }
};

#endif //ESP8266_WIFI_REALTIME_FEEDER_LOGGER_H
