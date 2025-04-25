#ifndef ARDUINO_WIFI_REALTIME_SWITCH_WEBSOCKETCLIENT_H
#define ARDUINO_WIFI_REALTIME_SWITCH_WEBSOCKETCLIENT_H

#include <Ticker.h>
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
    Ticker timeoutTicker;

public:
    ~WebSocketClient();

    bool init(bool isSecure,
              String &srv,
              int srvPort,
              String &socketUrl,
              bool autoAcknowledge,
              const CallbackFunction &callbackFunc);

    bool tick();

    bool forceConnect();

    bool forceReconnect();

    bool sendMessage(const String &msg);

    void sendMessageAndAcknowledge(String &msg, String &offset);

    void acknowledge(String &offset);

    void resetHeartBeat();

    bool isHeartBeatReceived();

    void disconnect();

private:
    volatile bool timeoutTriggered = false;
    volatile bool sendHeartbeat = false;
    volatile bool closeConn = false;

    bool connect();

    static String generateWebSocketKey();

    bool readHTTPResponseHeaders();

    void readWebSocketData();

    void scheduleHeartbeat();

    void setHeartBeatReceived(bool hbr);

    static WiFiClient *getClient(bool isSecure);
};

#endif // ARDUINO_WIFI_REALTIME_SWITCH_WEBSOCKETCLIENT_H
