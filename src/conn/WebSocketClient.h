#ifndef ARDUINO_WIFI_REALTIME_SWITCH_WEBSOCKETCLIENT_H
#define ARDUINO_WIFI_REALTIME_SWITCH_WEBSOCKETCLIENT_H

#include <ESP8266WiFi.h>

typedef void (*CallbackFunction)(String str);

class WebSocketClient {
private:
    WiFiClient *client;
    String server;
    int port;
    String url;
    CallbackFunction callback;

public:
    ~WebSocketClient();

    void init(bool isSecure,
              String &srv,
              int srvPort,
              String &socketUrl,
              const CallbackFunction &callbackFunc);

    bool tick();

    void forceConnect();

private:
    bool connect();

    static String generateWebSocketKey();

    void readHTTPResponseHeaders();

    void readWebSocketData();

    static WiFiClient *getClient(bool isSecure);
};

#endif // ARDUINO_WIFI_REALTIME_SWITCH_WEBSOCKETCLIENT_H
