#pragma once

#if defined(ESP32)
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include "headtracker.h"
#include "button_manager.h"

#define MAX_WS_CLIENTS 4

class WebConfigManager
{
public:
    WebConfigManager();

    // Starts SoftAP (ELRS-HeadTracker-AP on 192.168.4.1), Captive Portal DNS, mDNS, HTTP & WebSocket Server
    void begin(HeadTracker* ht, ButtonManager* buttons, uint8_t* uid, String* bindingPhrase, int8_t* txPower);

    // Non-blocking loop handler for DNS, HTTP, WebSocket 30Hz telemetry, and AP LED breathing
    void update();

    bool isActive() const { return _active; }

private:
    HeadTracker*   _ht;
    ButtonManager* _buttons;
    uint8_t*       _uid;
    String*        _bindingPhrase;
    int8_t*        _txPower;

    bool           _active;
    WebServer      _server;
    DNSServer      _dnsServer;
    WiFiClient     _wsClients[MAX_WS_CLIENTS];
    uint32_t       _lastWsBroadcastMs;
    uint32_t       _lastStatusLogMs;

    // HTTP Route Handlers
    void handleRoot();
    void handleWebSocketUpgrade();
    void handleGetTelemetry();
    void handleGetConfig();
    void handlePostCenter();
    void handlePostTare();
    void handlePostLive();
    void handlePostSave();
    void handleOtaComplete();
    void handleOtaUpload();

    // JSON & WebSocket Helpers
    void applyJsonSettings(const String& body, bool saveNetwork);
    void pollWebSockets();
    void broadcastTelemetry();
    void sendWsText(WiFiClient& client, const char* payload, size_t len);

    // Minimal JSON field extractors (zero external library dependency)
    static bool extractJsonString(const String& json, const char* key, String& outVal);
    static bool extractJsonFloat(const String& json, const char* key, float& outVal);
    static bool extractJsonInt(const String& json, const char* key, int& outVal);
    static bool extractJsonBool(const String& json, const char* key, bool& outVal);
    static bool extractJsonUidArray(const String& json, uint8_t outUid[6]);
};
#endif
