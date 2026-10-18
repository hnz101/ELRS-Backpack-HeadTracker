#if defined(ESP32)
#pragma message "ESP32 stuff happening!"
#else
#pragma message "ESP8266 stuff happening!"
#endif

// ===== Logging Level =====
//log level ing logging.h 
#include "logging.h"

// ======================================================
// 1. Includes
// ======================================================

#include <terseCRSF.h>  // https://github.com/zs6buj/terseCRSF   use v 0.0.6 or later
#include "msp.h"   // MSP Paket Handling
#include "msptypes.h"
#include "fake_vrx_fake_trainer.h"
#include "application.h"
#include "types.h"
#include "my_config.h"

#if defined(ESP32)
#include "display_manager.h"
#include "headtracker.h"
#include "button_manager.h"
#include "serial_display.h"
#include "web_config.h"
#endif

// ======================================================
// 2. Platform Selection (ESP32 / ESP8266)
// ======================================================

#if defined(ESP32)
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <Preferences.h>
#else
#include <ESP8266WiFi.h>
#include <espnow.h>
#endif

enum RadioMode
{
    MODE_RX_ONLY = 0,
    MODE_TX_ONLY = 1,
    MODE_BOTH    = 2
};

// ======================================================
// 3. Configuration (Defines, UID, WiFi, Logging)
// ======================================================

// Please use ExpressLRS Configurator Runtime Options to obtain your UID (unique MAC hashed 
// from binding_phrase) Insert the six numbers between the curly brackets below
//For ESP NOW / ELRS Backpack you need to enter the UID resulting from hashing your 
//secret binding phrase. This must be obtained by launching https://expresslrs.github.io/web-flasher/, 
//enter your binding phrase, then make a note of the UID. Enter the 6 numbers between the commas
// Config Elrs Binding
//uint8_t UID[6] = {0,0,0,0,0,0}; // this is my UID. You have to change it to your once, should look 
//uint8_t UID[6] = {106,19,19,206,193,30};

// Prüfen, ob die externe Config-Datei existiert
#if __has_include("my_config.h")
#include "my_config.h"
#endif

// Wenn die Config geladen wurde und die UID definiert ist, nimm diese.
// Andernfalls nimm den Standard-Fallback.
#ifdef MY_CUSTOM_UID
uint8_t UID[6] = MY_CUSTOM_UID;
#else
uint8_t UID[6] = {106,19,19,206,193,30};
#endif

// ======================================================
// RadioMode Configuration
// ======================================================
//
// MODE_RX_ONLY  : Only receive ESP-NOW data (telemetry).
//                 Sending (trainer/VTX channels) is disabled.
//
// MODE_TX_ONLY  : Only send ESP-NOW data (trainer/VTX channels).
//                 Receiving telemetry is disabled.
//
// MODE_BOTH     : Send and receive ESP-NOW data.
//                 Full bidirectional operation.
//
// You can change the default mode below.
// It can also be changed later via Serial commands.
// ======================================================
RadioMode radioMode = MODE_BOTH;   // Default startup mode

// ===== AP Wifi Config =====
const char* ssid = "Backpack_ELRS_Crsf";           // SSID des Access Points
const char* password = "12345678";                  // Passwort des Access Points

// ===== Config for Channel output =====
const uint8_t NUM_CHANNELS = 16;
#define CHANNEL_MIN 172
#define CHANNEL_MAX 1811
// ===== in the Example there is a wave for the output channels that you can see the channes are moving
#define WAVE_SPEED 0.05
float wavePhase = 0.0;
#define STEP_INTERVAL 100   // 100ms Offset for each channel when the wave starts to the channel bevor

// ======================================================
// 4. Global Objects
// ======================================================

uint16_t rampChannels[NUM_CHANNELS];
uint32_t lastStepTime = 0;
uint8_t activeChannel = 0;
bool rampUp = true;

volatile bool espnow_received = false;
volatile uint16_t crsf_len = 0;

#if defined(ESP32)
volatile uint16_t espnow_len = 0;
uint8_t espnow_buffer[250];
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
#endif

// ======================================================
// Platform Specific
// ======================================================
#if defined(ESP32)
QueueHandle_t rxqueue;
#endif

// ======================================================
// Module Instances
// ======================================================
FakeVRXFakeTrainer vrxModule;
MSP recv_msp;
CRSF crsf;

#if defined(ESP32)
#if FEATURE_DISPLAY
DisplayManager display;
#endif
HeadTracker headtracker;
ButtonManager buttons;
SerialDisplay serialDisplay;
WebConfigManager webConfig;
String bindingPhrase = "";
bool webConfigMode = false;
bool waveMode = false;
uint32_t lastTxTime = 0;
uint32_t lastDisplayTime = 0;
uint32_t rxPacketCount = 0;
uint32_t lastRxPacketTime = 0;
int8_t currentTxPower = WIFI_TX_POWER_DEFAULT;
#endif

// ===============================
// RC Channels
// ===============================
uint16_t rc_channels[NUM_CHANNELS] = {0};

// ===============================
// Telemetry Values
// ===============================
int16_t hud_bat1_volts = 0;
int16_t hud_bat1_amps  = 0;
uint16_t hud_bat1_mAh  = 0;

bool motArmed = false;

// ===============================
// GPS Struct
// ===============================

Location hom = {0,0,0,0,0};
Location cur = {0,0,0,0,0};

// ===============================
bool finalHomeStored = false;

// ======================================================
// ESP-NOW Receive Callback
// ======================================================
#if defined(ESP32)
#if defined(ESP_IDF_VERSION) && ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
void OnDataRecv(const esp_now_recv_info_t *esp_now_info, const uint8_t *incomingData, int len) {
#else
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
#endif
    if (len <= 8 || incomingData == NULL) return;

    if (incomingData[8] == 0 && incomingData[9] == 0 && len > 10) {
        return; 
    }

    crsf_len = len - 8;

    if (crsf_len > sizeof(espnow_buffer)) {
        crsf_len = sizeof(espnow_buffer);
    }

    memcpy(espnow_buffer, incomingData + 8, crsf_len);

    espnow_received = true;
}
#else
void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
    if (len <= 8) return;

    crsf_len = len - 8;

    memcpy(crsf.crsf_buf, incomingData + 8, crsf_len);

    espnow_received = true;
}
#endif

// ======================================================
// ESP-NOW Send Callback
// ======================================================

#if defined(ESP32)
#if defined(ESP_IDF_VERSION) && ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
void OnDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
#else
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
#endif
    // Beim ESP32 ist status 0 (ESP_NOW_SEND_SUCCESS)
    bool success = (status == ESP_NOW_SEND_SUCCESS);
#else
void OnDataSent(uint8_t *mac_addr, uint8_t status) {
    // Beim ESP8266 ist status 0 meist Erfolg
    bool success = (status == 0);
#endif

    if (success) {
        LOG_DEBUG_INLINE("ESP-NOW Send OK          ");
    } else {
        LOG_ERROR_INLINE("ESP-NOW Send FAIL (Status: %d) t=%lu ms      ", status, millis());
    }
}

// ======================================================
// 7. Setup()
// ======================================================

void initSerial()
{
    Serial.begin(115200);
    LOG_INFO("Start");
}

void initWiFi()
{
    UID[0] &= ~0x01;   // unicast fix
    
    #if defined(ESP32)
        WiFi.disconnect();
        WiFi.mode(WIFI_STA); 
        // DAS IST PFLICHT FÜR DEN RAW-FRAME:
        esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B);
        esp_wifi_set_max_tx_power(currentTxPower);
    #else
        WiFi.mode(WIFI_STA);
        //WiFi.mode(WIFI_AP_STA); // Ermöglicht AP und Station gleichzeitig
        WiFi.disconnect();
        wifi_set_channel(1);
    #endif
}

void initMac()
{
    #if defined(ESP32)
        esp_wifi_set_mac(WIFI_IF_STA, UID);
        esp_wifi_start(); 
        esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
        esp_wifi_set_max_tx_power(currentTxPower);
        LOG_INFO("WiFi TX Power set to %d (%.2f dBm)", currentTxPower, (float)currentTxPower * 0.25f);
    #else
        wifi_set_macaddr(STATION_IF, UID);
    #endif
}

void initESPNow()
{
    #if defined(ESP32)
        if (esp_now_init() != ESP_OK)
    #else
        if (esp_now_init() != 0)
    #endif
    {
        LOG_ERROR("ESP-NOW init failed");
        return;
    }

    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);

    #if defined(ESP32)
        // KEINE PEER REGISTRIERUNG MEHR NÖTIG!
        // Der custom_esp_now_send Bypass feuert an der API vorbei.
    #else
        esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
        esp_now_add_peer(UID, ESP_NOW_ROLE_COMBO, 0, NULL, 0);
    #endif
}

void initRamp()
{
    for(int i = 0; i < NUM_CHANNELS; i++)
        rampChannels[i] = CHANNEL_MIN;
}

void initESP32Queue()
{
#if defined(ESP32)
    rxqueue = xQueueCreate(20, sizeof(mspPacket_t));

    if (rxqueue == NULL)
    {
        LOG_ERROR("Queue creation failed");
    }
#endif
}

void initInfo()
{
    LOG_INFO("==================================================");
    LOG_INFO("Radio Mode Configuration");
    LOG_INFO("Viewpoint: ESP Module Role (not the RC transmitter)");
    LOG_INFO("--------------------------------------------------");
    LOG_INFO("RX_ONLY : ESP receives ESP-NOW data and decodes it.");
    LOG_INFO("          Sending is disabled.");
    LOG_INFO("TX_ONLY : ESP sends ESP-NOW data.");
    LOG_INFO("          Receiving/decoding is disabled.");
    LOG_INFO("BOTH    : Full bidirectional operation.");
    LOG_INFO("--------------------------------------------------");
    LOG_INFO("Change mode anytime via Serial:");
    LOG_INFO("  1 = RX_ONLY");
    LOG_INFO("  2 = TX_ONLY");
    LOG_INFO("  3 = BOTH");
    LOG_INFO("  W = Enter WiFi Web-IF Configuration Mode");
    LOG_INFO("==================================================");
}

void setup() {
    #if defined(ESP32)
    // 1. Initialize Buttons & RGB LED immediately (turns LED Blue and arms GPIO 9 interrupt at ms 0)
    buttons.begin();
    #endif

    initSerial();
    #if defined(ESP32)
    delay(250);

    #if FEATURE_DISPLAY
    display.begin();
    #endif
    headtracker.begin();

    // 2. Load stored UID, Binding Phrase, and TX Power from NVS
    bool hasValidUid = HeadTracker::loadNetworkConfig(UID, bindingPhrase, currentTxPower);
    LOG_INFO("Loaded ELRS UID: {%u, %u, %u, %u, %u, %u} (Valid=%s)",
             UID[0], UID[1], UID[2], UID[3], UID[4], UID[5],
             hasValidUid ? "YES" : "NO");

    // 3. 5-Second Blue LED Boot Window:
    //    Enter Web Configuration Mode if:
    //    - BOOT button (GPIO 9) was clicked/held during IMU init or during this 5s window, OR
    //    - No valid binding phrase/UID is stored in NVS, OR
    //    - Serial command 'W' is received
    bool enterWebMode = !hasValidUid;
    if (buttons.wasBootButtonLatched()) {
        LOG_INFO("[BOOT] BOOT button press latched during startup -> Entering Web Config Mode!");
        enterWebMode = true;
    }

    if (!enterWebMode) {
        LOG_INFO("Boot Window (5s Blue LED): Press BOOT button (GPIO 9) or send 'W' for Web-IF AP Mode...");
        uint32_t bootWindowStart = millis();

        while (millis() - bootWindowStart < 5000) {
            buttons.updateBootPulseLed();
            headtracker.update();

            if (buttons.wasBootButtonLatched() || buttons.isBootButtonPressed()) {
                delay(25); // Brief debounce check
                if (buttons.wasBootButtonLatched() || buttons.isBootButtonPressed()) {
                    LOG_INFO("[BOOT] BOOT button pressed during 5s window -> Entering Web Config Mode!");
                    enterWebMode = true;
                    break;
                }
            }

            if (Serial.available()) {
                char c = Serial.peek();
                if (c == 'W') {
                    Serial.read();
                    LOG_INFO("[BOOT] Serial 'W' received during 5s window -> Entering Web Config Mode!");
                    enterWebMode = true;
                    break;
                }
            }
            delay(5);
        }
    } else if (!hasValidUid) {
        LOG_WARN("[BOOT] No valid UID stored in NVS -> Entering Web Config Mode automatically!");
    }

    if (enterWebMode) {
        webConfigMode = true;
        buttons.clearBootButtonLatch();
        buttons.flashLed(122, 0, 184, 800); // Immediate Magenta confirmation
        // Wait for button release so it doesn't immediately trigger Center/Tare
        uint32_t releaseWait = millis();
        while (buttons.isBootButtonPressed() && (millis() - releaseWait < 2000)) {
            buttons.updateApModeLed();
            delay(10);
        }
        buttons.clearBootButtonLatch();
        webConfig.begin(&headtracker, &buttons, UID, &bindingPhrase, &currentTxPower);
        serialDisplay.begin();
        return;
    }

    // 4. Proceed to Normal 50Hz ESP-NOW Mode
    LOG_INFO("[BOOT] Proceeding to Normal 50Hz ESP-NOW HeadTracker Mode");
    initWiFi();
    initMac();
    initESP32Queue();
    initESPNow();
    vrxModule.init(UID);
    initRamp();
    initInfo();
    serialDisplay.begin();
    #else
    initWiFi();
    initMac();
    initESPNow();
    vrxModule.init(UID);
    initRamp();
    initInfo();
    #endif
}

// ======================================================
// 8. Loop()
// ======================================================

void loop()
{
    // ======================================================
    // Serial Command Handling
    // ======================================================
    if (Serial.available())
    {
        char c = Serial.read();

        if (c >= '1' && c <= '3')
        {
            radioMode = (RadioMode)(c - '1');
            LOG_INFO("RadioMode changed to %d", radioMode + 1);
        }
        #if defined(ESP32)
        else if (c == 'W')
        {
            if (!webConfigMode) {
                webConfigMode = true;
                webConfig.begin(&headtracker, &buttons, UID, &bindingPhrase, &currentTxPower);
            }
        }
        else if (c == 'c' || c == ' ')
        {
            headtracker.center();
            buttons.flashLed(0, 80, 80, 500); // Cyan acknowledge
            LOG_INFO("[EVENT] HeadTracker Centered / Zeroed (Serial)");
        }
        else if (c == 'r')
        {
            buttons.flashLed(80, 80, 0, 800); // Yellow calibrate
            headtracker.calibrate();
            LOG_INFO("[EVENT] HeadTracker Calibrated / Tared (Serial)");
        }
        else if (c == 'w')
        {
            waveMode = !waveMode;
            LOG_INFO("Mode switched: %s", waveMode ? "TEST WAVE" : "LIVE HEADTRACKER");
        }
        else if (c == 'f')
        {
            headtracker.setFusionMode(headtracker.getFusionMode() + 1);
            LOG_INFO("Switched IMU Fusion Mode to: %s", headtracker.getSensorName());
        }
        else if (c == 'p')
        {
            headtracker.toggleInvertTilt();
            buttons.flashLed(0, 80, 80, 300);
        }
        else if (c == 's')
        {
            headtracker.toggleSwapPitchRoll();
            buttons.flashLed(80, 80, 0, 300);
        }
        else if (c == 'y')
        {
            headtracker.toggleInvertPan();
            buttons.flashLed(0, 80, 80, 300);
        }
        else if (c == 'o')
        {
            headtracker.toggleInvertRoll();
            buttons.flashLed(0, 80, 80, 300);
        }
        else if (c == 'm')
        {
            serialDisplay.toggleMode();
        }
        else if (c == 't')
        {
            // Cycle WiFi TX Power: 40 (10dBm) -> 34 (8.5dBm) -> 20 (5dBm) -> 8 (2dBm) -> 78 (19.5dBm) -> 60 (15dBm) -> 40 (10dBm)
            const int8_t powerSteps[] = {40, 34, 20, 8, 78, 60};
            const int numSteps = sizeof(powerSteps) / sizeof(powerSteps[0]);
            int curIdx = 0;
            for (int i = 0; i < numSteps; i++) {
                if (powerSteps[i] == currentTxPower) {
                    curIdx = i;
                    break;
                }
            }
            currentTxPower = powerSteps[(curIdx + 1) % numSteps];
            esp_wifi_set_max_tx_power(currentTxPower);
            HeadTracker::saveNetworkConfig(UID, bindingPhrase.c_str(), currentTxPower);
            buttons.flashLed(0, 80, 80, 200);
            LOG_INFO(">>> WiFi TX Power set to %d (%.2f dBm) [saved to NVS] <<<", currentTxPower, (float)currentTxPower * 0.25f);
        }
        else if (c == 'd')
        {
            serialDisplay.toggleEnabled();
            LOG_INFO(">>> Serial Debug Output: %s <<<", serialDisplay.isEnabled() ? "ENABLED" : "MUTED");
        }
        else if (c == '?' || c == 'h')
        {
            serialDisplay.printHelp();
        }
        #endif
    }

    #if defined(ESP32)
    // ======================================================
    // Button & HeadTracker Handling
    // ======================================================
    ButtonEvent evt = buttons.update();
    if (evt == BTN_CENTER_CLICK) {
        headtracker.center();
        buttons.flashLed(0, 80, 80, 500); // Cyan acknowledge
        LOG_INFO("[EVENT] HeadTracker Centered / Zeroed (BOOT Switch)");
    } else if (evt == BTN_CENTER_LONG) {
        buttons.flashLed(80, 80, 0, 800); // Yellow calibrate
        headtracker.calibrate();
        LOG_INFO("[EVENT] HeadTracker Calibrated / Tared (BOOT Switch)");
    } else if (evt == BTN_ENTER_WEB_CONFIG) {
        if (!webConfigMode) {
            LOG_INFO("[EVENT] BOOT Switch held >= 5.0s -> Entering Web Configuration Mode!");
            webConfigMode = true;
            webConfig.begin(&headtracker, &buttons, UID, &bindingPhrase, &currentTxPower);
        }
    } else if (evt == BTN_RIGHT_CLICK) {
        waveMode = !waveMode; // Toggle live HT vs Test Wave
        LOG_INFO("Mode switched: %s", waveMode ? "TEST WAVE" : "LIVE HEADTRACKER");
    } else if (evt == BTN_LEFT_CLICK) {
        #if FEATURE_DISPLAY
        display.toggleDriver(); // Toggle ST7789 <-> GC9A01
        #endif
    }

    headtracker.update();

    // If Web Configuration AP Mode is active, run Web Server / 30Hz WebSockets / Breathing Magenta LED
    // and keep ESP-NOW transmission paused.
    if (webConfigMode) {
        webConfig.update();
        return;
    }
    #endif

    // ======================================================
    // ESP-NOW Receive Handling (CRSF)
    // ======================================================
   if (radioMode != MODE_TX_ONLY)
    {
        if (espnow_received)
        {
            #if defined(ESP32)
            uint16_t len_to_process = 0;
            portENTER_CRITICAL(&mux);
            len_to_process = crsf_len;
            memcpy(crsf.crsf_buf, espnow_buffer, len_to_process);
            espnow_received = false;
            rxPacketCount++;
            lastRxPacketTime = millis();
            portEXIT_CRITICAL(&mux);
            processCRSFFrame(crsf.crsf_buf, len_to_process);
            #else
            noInterrupts();
            espnow_received = false;
            interrupts();
            processCRSFFrame(crsf.crsf_buf, crsf_len);
            #endif
        }
    }

    // ======================================================
    // Transmit to ELRS Backpack
    // ======================================================
    if (radioMode != MODE_RX_ONLY)
    {
        #if defined(ESP32)
        uint32_t now = millis();
        if (now - lastTxTime >= HT_UPDATE_INTERVAL_MS) {
            lastTxTime = now;
            if (waveMode) {
                vrxModule.updateChannelRamp();
            } else {
                uint16_t channels[16];
                headtracker.getChannels16(channels);
                vrxModule.sendTrainerMode16ch(channels);
            }

            if (vrxModule.getLastSendSuccess()) {
                buttons.setHeartbeatLed(0, 40, 0); // Green heartbeat
            } else {
                buttons.setHeartbeatLed(40, 0, 0); // Red on send error
            }
        }

        #if FEATURE_DISPLAY
        // Display refresh (~20 Hz)
        if (now - lastDisplayTime >= 50) {
            lastDisplayTime = now;
            display.renderUI(
                headtracker.getPan(),
                headtracker.getTilt(),
                headtracker.getRoll(),
                headtracker.getCrsfPan(),
                headtracker.getCrsfTilt(),
                headtracker.getCrsfRoll(),
                vrxModule.getPacketCount(),
                vrxModule.getLastSendSuccess(),
                rxPacketCount,
                lastRxPacketTime,
                waveMode ? "TEST-WAVE" : headtracker.getSensorName(),
                UID
            );
        }
        #endif

        // Serial Debug & ASCII Level Rendering
        serialDisplay.render(
            headtracker.getPan(),
            headtracker.getTilt(),
            headtracker.getRoll(),
            headtracker.getCrsfPan(),
            headtracker.getCrsfTilt(),
            headtracker.getCrsfRoll(),
            vrxModule.getPacketCount(),
            vrxModule.getLastSendSuccess(),
            rxPacketCount,
            lastRxPacketTime,
            headtracker.getSensorName(),
            waveMode,
            (uint8_t)radioMode,
            currentTxPower
        );
        #else
        vrxModule.updateChannelRamp();   // ESP-NOW senden
        #endif
    }
}