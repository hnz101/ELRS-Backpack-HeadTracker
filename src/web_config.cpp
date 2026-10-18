#if defined(ESP32)
#include "web_config.h"
#include "web_assets.h"
#include "logging.h"

#include <esp_wifi.h>
#include <esp_now.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <mbedtls/sha1.h>
#include <mbedtls/base64.h>

static const char* AP_SSID = "ELRS-HeadTracker-AP";
static const IPAddress AP_IP(192, 168, 4, 1);
static const IPAddress AP_NETMASK(255, 255, 255, 0);

WebConfigManager::WebConfigManager()
    : _ht(nullptr),
      _buttons(nullptr),
      _uid(nullptr),
      _bindingPhrase(nullptr),
      _txPower(nullptr),
      _active(false),
      _server(80),
      _lastWsBroadcastMs(0),
      _lastStatusLogMs(0)
{
}

void WebConfigManager::begin(HeadTracker* ht, ButtonManager* buttons, uint8_t* uid, String* bindingPhrase, int8_t* txPower)
{
    _ht = ht;
    _buttons = buttons;
    _uid = uid;
    _bindingPhrase = bindingPhrase;
    _txPower = txPower;
    _active = true;

    LOG_INFO("==========================================================");
    LOG_INFO("   ENTERING WEB CONFIGURATION & 3D TELEMETRY MODE         ");
    LOG_INFO("==========================================================");

    // 1. Ensure ESP-NOW is paused and configure Open SoftAP on 192.168.4.1
    if (WiFi.getMode() != WIFI_MODE_NULL) {
        esp_now_deinit();
        WiFi.disconnect(false);
    }
    WiFi.persistent(false);
    WiFi.mode(WIFI_AP);
    if (_txPower) {
        esp_wifi_set_max_tx_power(*_txPower);
    }
    WiFi.softAPConfig(AP_IP, AP_IP, AP_NETMASK);
    WiFi.softAP(AP_SSID, NULL, 1, 0, 4); // Open AP, Channel 1, Max 4 connections
    esp_wifi_set_ps(WIFI_PS_NONE);       // Zero power-save latency for 30Hz WebSockets
    if (_txPower) {
        esp_wifi_set_max_tx_power(*_txPower);
    }

    LOG_INFO("[WEB-IF] SoftAP Started  : SSID = '%s' (OPEN)", AP_SSID);
    LOG_INFO("[WEB-IF] Web Portal IP   : http://%s", WiFi.softAPIP().toString().c_str());

    // 2. Start Captive Portal DNS (* -> 192.168.4.1)
    _dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    _dnsServer.start(53, "*", AP_IP);
    LOG_INFO("[WEB-IF] Captive DNS     : Active (* -> 192.168.4.1)");

    // 3. Start mDNS (http://headtracker.local)
    if (MDNS.begin("headtracker")) {
        MDNS.addService("http", "tcp", 80);
        LOG_INFO("[WEB-IF] mDNS Responder  : http://headtracker.local");
    }

    // 4. Collect headers required for RFC 6455 WebSocket upgrade on port 80
    static const char* headerKeys[] = {"Upgrade", "Sec-WebSocket-Key"};
    _server.collectHeaders(headerKeys, 2);

    // 5. Register HTTP & WebSocket Routes
    _server.on("/", HTTP_GET, [this]() { handleRoot(); });
    _server.on("/index.html", HTTP_GET, [this]() { handleRoot(); });
    _server.on("/ws", HTTP_GET, [this]() { handleWebSocketUpgrade(); });

    // REST API Routes
    _server.on("/api/telemetry", HTTP_GET, [this]() { handleGetTelemetry(); });
    _server.on("/api/config", HTTP_GET, [this]() { handleGetConfig(); });
    _server.on("/api/center", HTTP_POST, [this]() { handlePostCenter(); });
    _server.on("/api/tare", HTTP_POST, [this]() { handlePostTare(); });
    _server.on("/api/live", HTTP_POST, [this]() { handlePostLive(); });
    _server.on("/api/save", HTTP_POST, [this]() { handlePostSave(); });

    // Wireless OTA Firmware Update Route
    _server.on("/update", HTTP_POST,
        [this]() { handleOtaComplete(); },
        [this]() { handleOtaUpload(); }
    );

    // Captive Portal OS Probe Endpoints -> Redirect or serve portal directly
    auto captiveRedirect = [this]() {
        _server.sendHeader("Location", "http://192.168.4.1/", true);
        _server.send(302, "text/plain", "");
    };
    _server.on("/generate_204", HTTP_GET, captiveRedirect);
    _server.on("/gen_204", HTTP_GET, captiveRedirect);
    _server.on("/hotspot-detect.html", HTTP_GET, [this]() { handleRoot(); });
    _server.on("/library/test/success.html", HTTP_GET, [this]() { handleRoot(); });
    _server.on("/ncsi.txt", HTTP_GET, captiveRedirect);
    _server.on("/connecttest.txt", HTTP_GET, captiveRedirect);
    _server.on("/redirect", HTTP_GET, captiveRedirect);
    _server.onNotFound([this]() { handleRoot(); });

    _server.begin();
    LOG_INFO("[WEB-IF] HTTP/WS Server  : Listening on Port 80");
    LOG_INFO("[WEB-IF] LED Status      : Breathing Magenta (#7A00B8)");
    LOG_INFO("==========================================================");
}

void WebConfigManager::handleRoot()
{
    // If someone connected to / with WebSocket upgrade header, upgrade it
    if (_server.hasHeader("Upgrade") && _server.header("Upgrade").equalsIgnoreCase("websocket")) {
        handleWebSocketUpgrade();
        return;
    }

    _server.sendHeader("Content-Encoding", "gzip");
    _server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    _server.send_P(200, "text/html", (const char*)index_html_gz, index_html_gz_len);
}

void WebConfigManager::handleWebSocketUpgrade()
{
    String wsKey = _server.header("Sec-WebSocket-Key");
    if (wsKey.length() == 0) {
        _server.send(400, "text/plain", "Missing Sec-WebSocket-Key");
        return;
    }

    // RFC 6455 Handshake: Base64(SHA1(Sec-WebSocket-Key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"))
    String combined = wsKey + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    uint8_t sha1Out[20];
    mbedtls_sha1_context ctx;
    mbedtls_sha1_init(&ctx);
#if defined(MBEDTLS_VERSION_NUMBER) && (MBEDTLS_VERSION_NUMBER < 0x03000000)
    mbedtls_sha1_starts_ret(&ctx);
    mbedtls_sha1_update_ret(&ctx, (const unsigned char*)combined.c_str(), combined.length());
    mbedtls_sha1_finish_ret(&ctx, sha1Out);
#else
    mbedtls_sha1_starts(&ctx);
    mbedtls_sha1_update(&ctx, (const unsigned char*)combined.c_str(), combined.length());
    mbedtls_sha1_finish(&ctx, sha1Out);
#endif
    mbedtls_sha1_free(&ctx);

    unsigned char acceptB64[40] = {0};
    size_t olen = 0;
    mbedtls_base64_encode(acceptB64, sizeof(acceptB64), &olen, sha1Out, sizeof(sha1Out));

    WiFiClient client = _server.client();
    if (!client) return;

    client.setNoDelay(true);
    String handshake =
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: " + String((const char*)acceptB64) + "\r\n\r\n";
    client.write((const uint8_t*)handshake.c_str(), handshake.length());

    // Store upgraded client in slot
    int slot = -1;
    for (int i = 0; i < MAX_WS_CLIENTS; i++) {
        if (!_wsClients[i] || !_wsClients[i].connected()) {
            slot = i;
            break;
        }
    }
    if (slot < 0) {
        slot = 0;
        _wsClients[0].stop();
    }
    _wsClients[slot] = client;
    LOG_INFO("[WEB-IF] WebSocket client connected (slot %d)", slot);
}

void WebConfigManager::handleGetTelemetry()
{
    char buf[256];
    snprintf(buf, sizeof(buf),
             "{\"pan\":%.2f,\"tilt\":%.2f,\"roll\":%.2f,"
             "\"cPan\":%u,\"cTilt\":%u,\"cRoll\":%u,"
             "\"sensor\":\"%s\",\"connected\":%s}",
             _ht ? _ht->getPan() : 0.0f,
             _ht ? _ht->getTilt() : 0.0f,
             _ht ? _ht->getRoll() : 0.0f,
             _ht ? _ht->getCrsfPan() : 992,
             _ht ? _ht->getCrsfTilt() : 992,
             _ht ? _ht->getCrsfRoll() : 992,
             _ht ? _ht->getSensorName() : "NONE",
             (_ht && _ht->isSensorConnected()) ? "true" : "false");
    _server.sendHeader("Cache-Control", "no-store");
    _server.send(200, "application/json", buf);
}

void WebConfigManager::handleGetConfig()
{
#if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(BOARD_WAVESHARE_ESP32C3) || defined(ARDUINO_ESP32C3_DEV)
    const char* boardName = "Waveshare ESP32-C3-Zero";
#else
    const char* boardName = "WEMOS LOLIN S3 Mini Pro";
#endif

    // Escape phrase safely
    String safePhrase = _bindingPhrase ? *_bindingPhrase : "";
    safePhrase.replace("\\", "\\\\");
    safePhrase.replace("\"", "\\\"");

    char buf[512];
    snprintf(buf, sizeof(buf),
             "{\"phrase\":\"%s\","
             "\"uid\":[%u,%u,%u,%u,%u,%u],"
             "\"fusionMode\":%u,"
             "\"maxPan\":%.1f,\"maxTilt\":%.1f,\"maxRoll\":%.1f,"
             "\"invPan\":%s,\"invTilt\":%s,\"invRoll\":%s,\"swapPitchRoll\":%s,"
             "\"chPan\":%u,\"chTilt\":%u,\"chRoll\":%u,"
             "\"txPower\":%d,"
             "\"sensor\":\"%s\","
             "\"board\":\"%s\"}",
             safePhrase.c_str(),
             _uid ? _uid[0] : 0, _uid ? _uid[1] : 0, _uid ? _uid[2] : 0,
             _uid ? _uid[3] : 0, _uid ? _uid[4] : 0, _uid ? _uid[5] : 0,
             _ht ? _ht->getFusionMode() : 0,
             _ht ? _ht->getMaxAnglePan() : 90.0f,
             _ht ? _ht->getMaxAngleTilt() : 60.0f,
             _ht ? _ht->getMaxAngleRoll() : 45.0f,
             (_ht && _ht->getInvertPan()) ? "true" : "false",
             (_ht && _ht->getInvertTilt()) ? "true" : "false",
             (_ht && _ht->getInvertRoll()) ? "true" : "false",
             (_ht && _ht->getSwapPitchRoll()) ? "true" : "false",
             _ht ? _ht->getChPan() : 0,
             _ht ? _ht->getChTilt() : 1,
             _ht ? _ht->getChRoll() : 2,
             _txPower ? (int)(*_txPower) : (int)WIFI_TX_POWER_DEFAULT,
             _ht ? _ht->getSensorName() : "NONE",
             boardName);
    _server.sendHeader("Cache-Control", "no-store");
    _server.send(200, "application/json", buf);
}

void WebConfigManager::handlePostCenter()
{
    if (_ht) _ht->center();
    if (_buttons) _buttons->flashLed(0, 80, 80, 500); // Cyan flash
    LOG_INFO("[WEB-IF] HeadTracker Centered / Zeroed (Cyan LED)");
    _server.send(200, "application/json", "{\"ok\":true}");
}

void WebConfigManager::handlePostTare()
{
    if (_buttons) _buttons->flashLed(80, 80, 0, 800); // Yellow flash
    if (_ht) _ht->calibrate();
    LOG_INFO("[WEB-IF] HeadTracker Tare / Calibration Triggered (Yellow LED)");
    _server.send(200, "application/json", "{\"ok\":true}");
}

void WebConfigManager::handlePostLive()
{
    String body = _server.arg("plain");
    applyJsonSettings(body, false);
    _server.send(200, "application/json", "{\"ok\":true}");
}

void WebConfigManager::handlePostSave()
{
    String body = _server.arg("plain");
    applyJsonSettings(body, true);

    bool doReboot = false;
    extractJsonBool(body, "reboot", doReboot);

    char resp[160];
    snprintf(resp, sizeof(resp),
             "{\"ok\":true,\"rebooting\":%s,\"uid\":[%u,%u,%u,%u,%u,%u]}",
             doReboot ? "true" : "false",
             _uid[0], _uid[1], _uid[2], _uid[3], _uid[4], _uid[5]);
    _server.send(200, "application/json", resp);

    if (_buttons) {
        _buttons->flashLed(0, 80, 80, 500);
    }

    if (doReboot) {
        LOG_INFO("[WEB-IF] Configuration saved to NVS. Rebooting into Normal Mode...");
        delay(450);
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_OFF);
        delay(100);
        ESP.restart();
    }
}

void WebConfigManager::applyJsonSettings(const String& body, bool saveNetwork)
{
    if (body.length() == 0 || !_ht) return;

    int fusionMode = _ht->getFusionMode();
    if (extractJsonInt(body, "fusionMode", fusionMode)) {
        if ((uint8_t)(fusionMode % 3) != _ht->getFusionMode()) {
            _ht->setFusionMode((uint8_t)(fusionMode % 3));
        }
    }

    float maxPan = _ht->getMaxAnglePan();
    float maxTilt = _ht->getMaxAngleTilt();
    float maxRoll = _ht->getMaxAngleRoll();
    bool hasAngle = false;
    if (extractJsonFloat(body, "maxPan", maxPan))   hasAngle = true;
    if (extractJsonFloat(body, "maxTilt", maxTilt)) hasAngle = true;
    if (extractJsonFloat(body, "maxRoll", maxRoll)) hasAngle = true;
    if (hasAngle) {
        _ht->setMaxAngles(maxPan, maxTilt, maxRoll);
    }

    bool invPan = _ht->getInvertPan();
    if (extractJsonBool(body, "invPan", invPan) && invPan != _ht->getInvertPan()) {
        _ht->setInvertPan(invPan);
    }
    bool invTilt = _ht->getInvertTilt();
    if (extractJsonBool(body, "invTilt", invTilt) && invTilt != _ht->getInvertTilt()) {
        _ht->setInvertTilt(invTilt);
    }
    bool invRoll = _ht->getInvertRoll();
    if (extractJsonBool(body, "invRoll", invRoll) && invRoll != _ht->getInvertRoll()) {
        _ht->setInvertRoll(invRoll);
    }
    bool swapPR = _ht->getSwapPitchRoll();
    if (extractJsonBool(body, "swapPitchRoll", swapPR) && swapPR != _ht->getSwapPitchRoll()) {
        _ht->setSwapPitchRoll(swapPR);
    }

    int chPan = _ht->getChPan();
    int chTilt = _ht->getChTilt();
    int chRoll = _ht->getChRoll();
    bool hasCh = false;
    if (extractJsonInt(body, "chPan", chPan))   hasCh = true;
    if (extractJsonInt(body, "chTilt", chTilt)) hasCh = true;
    if (extractJsonInt(body, "chRoll", chRoll)) hasCh = true;
    if (hasCh) {
        _ht->setChannelMapping((uint8_t)chPan, (uint8_t)chTilt, (uint8_t)chRoll);
    }

    int txPwr = _txPower ? *_txPower : WIFI_TX_POWER_DEFAULT;
    if (extractJsonInt(body, "txPower", txPwr)) {
        if (txPwr >= 8 && txPwr <= 78 && _txPower) {
            *_txPower = (int8_t)txPwr;
            esp_wifi_set_max_tx_power(*_txPower);
        }
    }

    if (saveNetwork && _uid && _bindingPhrase && _txPower) {
        String phrase = *_bindingPhrase;
        extractJsonString(body, "phrase", phrase);
        *_bindingPhrase = phrase;

        bool manualUid = false;
        extractJsonBool(body, "manualUid", manualUid);

        uint8_t parsedUid[6] = {0};
        bool gotUidArr = extractJsonUidArray(body, parsedUid);

        if (phrase.length() > 0 && !manualUid) {
            HeadTracker::computeElrsUid(phrase.c_str(), _uid);
            LOG_INFO("[WEB-IF] Computed UID from binding phrase via MD5: {%u, %u, %u, %u, %u, %u}",
                     _uid[0], _uid[1], _uid[2], _uid[3], _uid[4], _uid[5]);
        } else if (gotUidArr) {
            memcpy(_uid, parsedUid, 6);
            _uid[0] &= ~0x01; // Unicast bit fix
            LOG_INFO("[WEB-IF] Applied manual 6-byte UID: {%u, %u, %u, %u, %u, %u}",
                     _uid[0], _uid[1], _uid[2], _uid[3], _uid[4], _uid[5]);
        }

        HeadTracker::saveNetworkConfig(_uid, _bindingPhrase->c_str(), *_txPower);
        _ht->saveSettings();
    }
}

void WebConfigManager::handleOtaComplete()
{
    bool ok = !Update.hasError();
    _server.sendHeader("Connection", "close");
    if (ok) {
        _server.send(200, "application/json", "{\"ok\":true}");
        LOG_INFO("[OTA] Firmware Update SUCCESS! Rebooting...");
        if (_buttons) _buttons->setLed(0, 100, 0);
        delay(600);
        ESP.restart();
    } else {
        _server.send(500, "text/plain", Update.errorString());
        LOG_ERROR("[OTA] Firmware Update FAILED: %s", Update.errorString());
    }
}

void WebConfigManager::handleOtaUpload()
{
    HTTPUpload& upload = _server.upload();
    if (upload.status == UPLOAD_FILE_START) {
        LOG_INFO("[OTA] Starting wireless firmware update: %s", upload.filename.c_str());
        if (_buttons) _buttons->flashLed(0, 80, 120, 2000);
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            Update.printError(Serial);
        }
        if (_buttons) _buttons->flashLed(122, 0, 184, 200);
    } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {
            LOG_INFO("[OTA] Upload finished (%u bytes). Finalizing flash...", (unsigned int)upload.totalSize);
        } else {
            Update.printError(Serial);
        }
    }
}

void WebConfigManager::pollWebSockets()
{
    for (int i = 0; i < MAX_WS_CLIENTS; i++) {
        WiFiClient& c = _wsClients[i];
        if (!c || !c.connected()) continue;

        while (c.available() >= 2) {
            uint8_t b0 = c.read();
            uint8_t b1 = c.read();
            uint8_t opcode = b0 & 0x0F;
            bool masked = (b1 & 0x80) != 0;
            size_t len = b1 & 0x7F;

            if (len == 126) {
                if (c.available() < 2) break;
                len = ((size_t)c.read() << 8) | c.read();
            } else if (len == 127) {
                // Ignore oversized frames
                c.stop();
                break;
            }

            uint8_t mask[4] = {0};
            if (masked) {
                if (c.available() < 4) break;
                c.read(mask, 4);
            }

            if (len > 256) {
                c.stop();
                break;
            }

            char payload[257];
            size_t readBytes = c.read((uint8_t*)payload, len);
            if (masked) {
                for (size_t k = 0; k < readBytes; k++) {
                    payload[k] ^= mask[k & 3];
                }
            }
            payload[readBytes] = '\0';

            if (opcode == 0x08) { // Close frame
                c.stop();
                break;
            } else if (opcode == 0x09) { // Ping -> Pong
                uint8_t pong[2] = {0x8A, 0x00};
                c.write(pong, 2);
            } else if (opcode == 0x01) { // Text command frame
                if (strstr(payload, "\"center\"") != nullptr) {
                    if (_ht) _ht->center();
                    if (_buttons) _buttons->flashLed(0, 80, 80, 500); // Cyan
                    LOG_INFO("[WS] Center / Zero triggered from Web-IF (Cyan LED)");
                } else if (strstr(payload, "\"tare\"") != nullptr) {
                    if (_buttons) _buttons->flashLed(80, 80, 0, 800); // Yellow
                    if (_ht) _ht->calibrate();
                    LOG_INFO("[WS] Tare / Calibrate triggered from Web-IF (Yellow LED)");
                }
            }
        }
    }
}

void WebConfigManager::sendWsText(WiFiClient& client, const char* payload, size_t len)
{
    uint8_t frame[264];
    size_t headerLen = 0;
    frame[0] = 0x81; // FIN + Text Frame
    if (len < 126) {
        frame[1] = (uint8_t)len;
        headerLen = 2;
    } else {
        frame[1] = 126;
        frame[2] = (uint8_t)((len >> 8) & 0xFF);
        frame[3] = (uint8_t)(len & 0xFF);
        headerLen = 4;
    }
    if (headerLen + len <= sizeof(frame)) {
        memcpy(frame + headerLen, payload, len);
        client.write(frame, headerLen + len);
    }
}

void WebConfigManager::broadcastTelemetry()
{
    uint32_t now = millis();
    if (now - _lastWsBroadcastMs < 33) { // ~30 Hz real-time telemetry stream
        return;
    }
    _lastWsBroadcastMs = now;

    bool anyClient = false;
    for (int i = 0; i < MAX_WS_CLIENTS; i++) {
        if (_wsClients[i] && _wsClients[i].connected()) {
            anyClient = true;
            break;
        }
    }
    if (!anyClient || !_ht) return;

    char msg[220];
    int n = snprintf(msg, sizeof(msg),
                     "{\"pan\":%.2f,\"tilt\":%.2f,\"roll\":%.2f,"
                     "\"cPan\":%u,\"cTilt\":%u,\"cRoll\":%u,"
                     "\"sensor\":\"%s\"}",
                     _ht->getPan(),
                     _ht->getTilt(),
                     _ht->getRoll(),
                     _ht->getCrsfPan(),
                     _ht->getCrsfTilt(),
                     _ht->getCrsfRoll(),
                     _ht->getSensorName());
    if (n <= 0) return;

    for (int i = 0; i < MAX_WS_CLIENTS; i++) {
        if (_wsClients[i] && _wsClients[i].connected()) {
            sendWsText(_wsClients[i], msg, (size_t)n);
        }
    }
}

void WebConfigManager::update()
{
    if (!_active) return;

    _dnsServer.processNextRequest();
    _server.handleClient();
    pollWebSockets();
    broadcastTelemetry();

    if (_buttons) {
        _buttons->updateApModeLed();
    }
}

bool WebConfigManager::extractJsonString(const String& json, const char* key, String& outVal)
{
    String pattern = String("\"") + key + "\"";
    int idx = json.indexOf(pattern);
    if (idx < 0) return false;
    int colon = json.indexOf(':', idx + pattern.length());
    if (colon < 0) return false;
    int q1 = json.indexOf('"', colon + 1);
    if (q1 < 0) return false;
    int q2 = q1 + 1;
    String result = "";
    while (q2 < (int)json.length()) {
        char c = json[q2];
        if (c == '\\' && q2 + 1 < (int)json.length()) {
            result += json[q2 + 1];
            q2 += 2;
            continue;
        }
        if (c == '"') break;
        result += c;
        q2++;
    }
    outVal = result;
    return true;
}

bool WebConfigManager::extractJsonFloat(const String& json, const char* key, float& outVal)
{
    String pattern = String("\"") + key + "\"";
    int idx = json.indexOf(pattern);
    if (idx < 0) return false;
    int colon = json.indexOf(':', idx + pattern.length());
    if (colon < 0) return false;
    int start = colon + 1;
    while (start < (int)json.length() && (json[start] == ' ' || json[start] == '\t')) start++;
    outVal = json.substring(start).toFloat();
    return true;
}

bool WebConfigManager::extractJsonInt(const String& json, const char* key, int& outVal)
{
    String pattern = String("\"") + key + "\"";
    int idx = json.indexOf(pattern);
    if (idx < 0) return false;
    int colon = json.indexOf(':', idx + pattern.length());
    if (colon < 0) return false;
    int start = colon + 1;
    while (start < (int)json.length() && (json[start] == ' ' || json[start] == '\t')) start++;
    outVal = json.substring(start).toInt();
    return true;
}

bool WebConfigManager::extractJsonBool(const String& json, const char* key, bool& outVal)
{
    String pattern = String("\"") + key + "\"";
    int idx = json.indexOf(pattern);
    if (idx < 0) return false;
    int colon = json.indexOf(':', idx + pattern.length());
    if (colon < 0) return false;
    int start = colon + 1;
    while (start < (int)json.length() && (json[start] == ' ' || json[start] == '\t')) start++;
    if (json.substring(start).startsWith("true")) {
        outVal = true;
        return true;
    }
    if (json.substring(start).startsWith("false")) {
        outVal = false;
        return true;
    }
    return false;
}

bool WebConfigManager::extractJsonUidArray(const String& json, uint8_t outUid[6])
{
    int idx = json.indexOf("\"uid\"");
    if (idx < 0) return false;
    int b1 = json.indexOf('[', idx);
    int b2 = json.indexOf(']', b1);
    if (b1 < 0 || b2 < 0 || b2 <= b1) return false;

    String arrStr = json.substring(b1 + 1, b2);
    int pos = 0;
    for (int i = 0; i < 6; i++) {
        int comma = arrStr.indexOf(',', pos);
        String token = (comma >= 0) ? arrStr.substring(pos, comma) : arrStr.substring(pos);
        token.trim();
        if (token.length() == 0) return false;
        int val = token.toInt();
        if (val < 0) val = 0;
        if (val > 255) val = 255;
        outUid[i] = (uint8_t)val;
        if (comma < 0 && i < 5) return false;
        pos = comma + 1;
    }
    return true;
}
#endif
