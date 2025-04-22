#include "WebSocketClient.h"
#include <util/Logger.h>


#define WS_FIN            0x80
#define WS_OPCODE_TEXT    0x01

#define WS_MASK           0x80
#define WS_SIZE16         126

const int MAX_RETRY_CONN_ATTEMPTS = 5;
const int RETRY_CONN_INTERVAL_MS = 2000;
const long MAX_MESSAGE_SIZE_BYTES = 512;
const int KEEP_ALIVE_IDLE_SEC = 120;
const int KEEP_ALIVE_INTRV_SEC = 10;
const int KEEP_ALIVE_MAX_FAIL_COUNT = 9;
char MSG_RESP_BUFFER[MAX_MESSAGE_SIZE_BYTES];

WebSocketClient::~WebSocketClient() {
    delete this->client;
}

void WebSocketClient::init(bool isSecure,
                           String &srv,
                           int srvPort,
                           String &socketUrl,
                           bool autoAcknowledge,
                           const CallbackFunction &callbackFunc) {
    this->client = this->getClient(isSecure);
    this->server = srv;
    this->port = srvPort;
    this->url = socketUrl;
    this->callback = callbackFunc;
    this->autoAck = autoAcknowledge;
    this->heartBeatReceived = true;

    this->forceConnect();
}

bool WebSocketClient::tick() {
    if (!client->connected()) {
        client->stop();
        Serial.println("Connection with server ended unexpectedly.");
        return false;
    }

    if (client->available()) {
        readWebSocketData();
    }

    return true;
}

bool WebSocketClient::forceConnect() {
    int count = 1;
    Logger::log("WebSocketClient::forceConnect -> Connecting to server socket!");
    while (!this->connect() && count <= MAX_RETRY_CONN_ATTEMPTS) {
        count++;
        Logger::log(
            "WebSocketClient::forceConnect -> Retrying server socket connection after " + String(RETRY_CONN_INTERVAL_MS)
            + " ms!");
        delay(RETRY_CONN_INTERVAL_MS);
    }

    if (count > MAX_RETRY_CONN_ATTEMPTS) {
        Logger::log(
            "WebSocketClient::forceConnect -> Could not connect to server socket after " + String(
                MAX_RETRY_CONN_ATTEMPTS) + " attempts.");
        return false;
    }

    return true;
}

bool WebSocketClient::forceReconnect() {
    Logger::log("WebSocketClient::forceReconnect -> Stopping client");
    this->client->stop(1);
    return this->forceConnect();
}

bool WebSocketClient::connect() {
    timeoutTicker.once_ms(5000, [this]() {
        Logger::log("WebSocketClient::connect -> ESP Is stuck on conn, stopping!");
        client->stop(1);

        Logger::log("WebSocketClient::connect -> ESP Is stuck on conn, aborting!");
        client->abort();
    });

    if (!client->connect(server, this->port)) {
        Logger::log("WebSocketClient::connect -> Connection to " + this->server + " failed!");
        return false;
    }

    Logger::log("WebSocketClient::connect -> Connection to " + this->server + " succeeded!");

    timeoutTicker.detach();

    // client->keepAlive(KEEP_ALIVE_IDLE_SEC, KEEP_ALIVE_INTRV_SEC, KEEP_ALIVE_MAX_FAIL_COUNT);

    // Line 1
    client->print("GET ");
    client->print(this->url);
    client->println(" HTTP/1.1");

    // Host header
    client->print("Host: ");
    client->print(this->server);
    if (this->port != 80 && this->port != 443) {
        client->print(":" + String(this->port));
    }
    client->println();

    client->println("Connection: Upgrade");
    client->println("Upgrade: websocket");

    // Websocket Key header
    client->print("Sec-WebSocket-Key: ");
    client->println("4KiZwqw3OeyTS2D1lv8nJQ==");

    // // TODO: Add logic for registering device here
    // client->print("Device-Id: ");
    // client->println("cf4d185b-ae8b-4b75-8b8b-4de5ac74e396");
    //
    // client->print("Device-Secret: ");
    // client->println("1aa5107c-546c-411d-a4d9-c80a66738dd6");

    // Gordan (balcony system)
    // client->print("Device-Id: ");
    // client->println("f88c08bc-f3aa-4cc3-8bea-fcb6279ef794");
    // client->print("Device-Secret: ");
    // client->println("0f01f84d-7270-4561-978d-2a6a925e91ea");

    // // Pot
    client->print("Device-Id: ");
    client->println("63b331d7-8148-45f7-bb3c-c6a3f2102540");

    client->print("Device-Secret: ");
    client->println("f7c5f53c-674b-4766-a98b-ba99cc79c716");

    client->println("Sec-WebSocket-Version: 13");
    client->println();
    client->flush();

    readHTTPResponseHeaders();
    this->heartBeatReceived = true;
    return true;
}

String WebSocketClient::generateWebSocketKey() {
    String key = "";
    for (int i = 0; i < 22; ++i) {
        int r = random(0, 3);
        if (r == 0)
            key += (char) random(48, 57);
        else if (r == 1)
            key += (char) random(65, 90);
        else if (r == 2)
            key += (char) random(97, 122);
    }
    return key;
}

void WebSocketClient::readHTTPResponseHeaders() {
    int currentChar;
    int prevChar = -1;
    int newLineCount = 0;

    while (true) {
        while (client->available() > 0) {
            currentChar = client->read();

            if (currentChar == -1) {
                // End of stream
                break;
            }

            Serial.print((char) currentChar);

            if (prevChar == '\r' && currentChar == '\n') {
                newLineCount++;
                if (newLineCount == 2) {
                    // End of HTTP response headers
                    break;
                }
            } else if (currentChar != '\n' && currentChar != '\r') {
                // Reset newLineCount if the character is not a newline
                newLineCount = 0;
            }

            prevChar = currentChar;
        }

        if (newLineCount == 2 || currentChar == -1) {
            // End of HTTP response headers or end of stream, break out of the loop
            break;
        }
    }

    Serial.println("\nReceived HTTP Response Headers\n");
}

void WebSocketClient::readWebSocketData() {
    byte opcode, mask;
    long payloadLength;
    byte maskingKey[4];

    // Read the first two bytes (FIN, RSV1-3, Opcode, MASK, Payload Length)
    while (client->available() < 2); // Wait until at least 2 bytes are available
    byte b1 = client->read();
    byte b2 = client->read();

    bool finFlag = (b1 & 0b10000000) != 0;
    opcode = b2 & 0b00001111;
    mask = (opcode & 0b10000000) != 0;
    opcode &= 0b01111111;
    payloadLength = b2 & 0b01111111;

    if (payloadLength == 126) {
        byte lengthBytes[2];
        while (client->available() < 2); // Wait until at least 2 bytes are available
        client->readBytes(lengthBytes, 2);
        payloadLength = ((lengthBytes[0] & 0xFF) << 8) | (lengthBytes[1] & 0xFF);
    } else if (payloadLength == 127) {
        byte lengthBytes[8];
        while (client->available() < 8); // Wait until at least 8 bytes are available
        client->readBytes(lengthBytes, 8);
        unsigned long extendedLength = 0;
        for (int i = 0; i < 8; i++) {
            extendedLength |= (unsigned long) (lengthBytes[i] & 0xFF) << ((7 - i) * 8);
        }
        payloadLength = (long) (extendedLength & 0x7FFFFFFFFFFFFFFF);
    }

    // Read the optional masking key
    if (mask) {
        while (client->available() < 4); // Wait until at least 4 bytes are available
        client->readBytes(maskingKey, 4);
    }

    // Read the payload data
    // while (wificlient->available() < payloadLength); // Wait until payloadLength bytes are available
    client->readBytes(MSG_RESP_BUFFER, min(MAX_MESSAGE_SIZE_BYTES, payloadLength));
    if (payloadLength > MAX_MESSAGE_SIZE_BYTES) {
        Serial.println(
                "Socket message larger than " + String(MAX_MESSAGE_SIZE_BYTES) + " bytes, trimming the rest!");
        size_t diff = payloadLength - MAX_MESSAGE_SIZE_BYTES;
        for (size_t i = 0; i < diff; i++) {
            client->read();
        }

        payloadLength = MAX_MESSAGE_SIZE_BYTES;
    }

    // Unmask payload data if necessary
    if (mask) {
        for (unsigned int i = 0; i < payloadLength; i++) {
            MSG_RESP_BUFFER[i] = MSG_RESP_BUFFER[i] ^ maskingKey[i % 4];
        }
    }

    String msg = String(MSG_RESP_BUFFER);

    if (msg.startsWith("^|")) {
        int firstIndex = msg.indexOf('|');
        int lastIndex = msg.lastIndexOf('|');

        String offset = msg.substring(firstIndex + 1, lastIndex);

        msg = msg.substring(lastIndex + 2);

        this->callback(msg, offset);
        if (this->autoAck) {
            this->acknowledge(offset);
        }
    } else if (msg.equals("|pong|")) {
        this->heartBeatReceived = true;
    } else {
        String offset = "";
        this->callback(String(MSG_RESP_BUFFER), offset);
    }

    // Reset the buffer.
    memset(MSG_RESP_BUFFER, 0, MAX_MESSAGE_SIZE_BYTES);
}

WiFiClient *WebSocketClient::getClient(bool isSecure) {
    if (isSecure) {
        auto client = new WiFiClientSecure();
        client->setInsecure();
        return client;
    }

    return new WiFiClient();
}

void WebSocketClient::sendMessage(const String &msg) {
    if (!client->connected()) {
        Serial.println("Could not send msg " + msg + " because client is not connected!");
        return;
    }

    // 1. send fin and type text
    this->client->write(WS_FIN | WS_OPCODE_TEXT);

    // 2. send length
    int size = msg.length();
    if (size > 125) {
        this->client->write(WS_MASK | WS_SIZE16);
        this->client->write((uint8_t) (size >> 8));
        this->client->write((uint8_t) (size & 0xFF));
    } else {
        this->client->write(WS_MASK | (uint8_t) size);
    }

    // 3. send mask
    uint8_t mask[4];
    mask[0] = random(0, 256);
    mask[1] = random(0, 256);
    mask[2] = random(0, 256);
    mask[3] = random(0, 256);

    this->client->write(mask[0]);
    this->client->write(mask[1]);
    this->client->write(mask[2]);
    this->client->write(mask[3]);

    //4. send masked data
    for (int i = 0; i < size; ++i) {
        this->client->write(msg[i] ^ mask[i % 4]);
    }
}

void WebSocketClient::sendMessageAndAcknowledge(String &msg, String &offset) {
    String ackString = "ack=|" + offset + "|" + msg;
    Serial.println("Sending ack: " + ackString);
    this->sendMessage(ackString);
}

void WebSocketClient::acknowledge(String &offset) {
    String ackString = "ack=|" + offset + "|";
    Serial.println("Sending ack: " + ackString);
    this->sendMessage(ackString);
}

void WebSocketClient::resetHeartBeat() {
    this->heartBeatReceived = false;
}

bool WebSocketClient::isHeartBeatReceived() {
    return !this->client->connected() || this->heartBeatReceived;
}
