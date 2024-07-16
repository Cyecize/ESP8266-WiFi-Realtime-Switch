#ifndef ARDUINO_WIFI_REALTIME_SWITCH_WEBSOCKETCLIENT_H
#define ARDUINO_WIFI_REALTIME_SWITCH_WEBSOCKETCLIENT_H

#include <ESP8266WiFi.h>

typedef void (*CallbackFunction)(String str, String &offset);

class WebSocketClient {
private:
    WiFiClient *client;
    String server;
    int port;
    String url;
    CallbackFunction callback;
    bool autoAck;
    bool heartBeatReceived;

public:
    ~WebSocketClient();

    void init(bool isSecure,
              String &srv,
              int srvPort,
              String &socketUrl,
              bool autoAcknowledge,
              const CallbackFunction &callbackFunc);

    bool tick();

    void forceConnect();

    void forceReconnect();

    void sendMessage(String &msg);

    void sendMessageAndAcknowledge(String &msg, String &offset);

    void acknowledge(String &offset);

    void resetHeartBeat();

    bool isHeartBeatReceived();

private:
    bool connect();

    static String generateWebSocketKey();

    void readHTTPResponseHeaders();

    void readWebSocketData();

    static WiFiClient *getClient(bool isSecure);
};

#endif // ARDUINO_WIFI_REALTIME_SWITCH_WEBSOCKETCLIENT_H
