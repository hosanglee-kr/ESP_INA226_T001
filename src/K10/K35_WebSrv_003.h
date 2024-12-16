
/*
 * ESP32 Wi-Fi 및 웹소켓 기반 전력/전류 모니터링 시스템
 *
 * 이 시스템은 ESP32를 이용하여 Wi-Fi 통신과 웹소켓(WebSocket) 기반의 실시간 데이터 송수신을 구현합니다.
 * 전류 및 전압, 주파수를 실시간으로 측정하고 웹을 통해 데이터를 모니터링할 수 있습니다.
 * 이 시스템은 다양한 네트워크 환경에서 사용될 수 있으며, AP 모드와 스테이션 모드를 지원합니다.
 * 또한 mDNS 서비스를 사용하여 클라이언트가 로컬 네트워크에서 쉽게 ESP32에 접속할 수 있습니다.
 *
 * 주요 기능 요약:
 *
 * 1. **Wi-Fi 모드 설정 (AP/Station)**
 *    - AP(액세스 포인트) 모드와 Station(클라이언트) 모드를 지원하여 다양한 네트워크 환경에 맞춰 연결할 수 있습니다.
 *    - 스테이션 모드에서는 DHCP를 통해 동적 IP를 받거나, 정적 IP를 설정할 수 있습니다.
 *    - AP 모드에서는 사용자가 정의한 SSID로 네트워크를 생성하고, 클라이언트가 이를 통해 접속할 수 있습니다.
 *
 * 2. **mDNS 서비스**
 *    - mDNS를 사용하여 네트워크에서 "http://meter.local"과 같은 로컬 도메인을 통해 쉽게 ESP32에 접근할 수 있습니다.
 *    - 포트는 80번을 사용하며, HTTP 요청을 처리합니다.
 *
 * 3. **웹서버 기능**
 *    - ESP32에 내장된 웹서버는 클라이언트가 HTML 페이지에 접속하여 설정을 변경하거나 데이터를 확인할 수 있도록 합니다.
 *    - 주요 핸들러:
 *      - `/`: 기본 홈 페이지 (index.html)를 제공
 *      - `/defaults`: 네트워크 설정을 기본값으로 재설정
 *      - `/get`: 클라이언트가 SSID 및 비밀번호를 설정할 수 있도록 함
 *      - `/restart`: ESP32를 재시작하는 요청 처리
 *    - LittleFS 파일 시스템을 사용하여 정적 웹 리소스(HTML, JS, CSS 등)를 제공합니다.
 *
 * 4. **웹소켓 (WebSocket) 통신**
 *    - 웹소켓을 통해 클라이언트와 ESP32 간의 실시간 양방향 통신을 구현합니다.
 *    - 실시간 데이터 전송을 통해 전류, 전압 및 주파수 데이터를 웹 클라이언트에서 실시간으로 확인할 수 있습니다.
 *    - 명령어 기반으로 클라이언트에서 웹소켓을 통해 다양한 측정 작업을 수행할 수 있습니다.
 *      - `x`: 마지막 패킷 ACK 처리
 *      - `m`: 전류 및 전압 측정 모드 설정
 *      - `f`: 주파수 측정 모드 설정
 *      - `cv_capture`: JSON 형식으로 전송된 명령어로 전류/전압 측정을 캡처
 *      - `oscfreq`: JSON 형식으로 전송된 주파수 측정 설정
 *
 * 5. **전류/전압 및 주파수 측정**
 *    - INA226과 같은 외부 센서를 사용하여 전류 및 전압을 측정하며, 주파수 측정도 가능합니다.
 *    - 클라이언트가 요청하면 ESP32는 실시간으로 데이터를 캡처하여 웹소켓을 통해 클라이언트에 전달합니다.
 *    - 다양한 샘플링 주기와 샘플 수를 설정할 수 있으며, 측정된 값은 웹 인터페이스에서 실시간으로 확인할 수 있습니다.
 *
 * 6. **설정 저장 및 기본값 복원**
 *    - 사용자는 ESP32의 설정(SSID, 비밀번호 등)을 변경할 수 있으며, 이는 비휘발성 저장소에 저장됩니다.
 *    - 설정을 기본값으로 복원할 수 있는 기능도 제공되어, 네트워크 문제 발생 시 손쉽게 복구할 수 있습니다.
 *
 * 7. **LED 상태 표시**
 *    - LED를 통해 AP 모드, 스테이션 모드, 연결 상태를 시각적으로 표시합니다.
 *    - 연결 성공 시 LED가 꺼지고, AP 모드에서 활성화된 경우에는 LED가 켜집니다.
 *
 * 시스템 흐름:
 * - ESP32가 부팅되면 Wi-Fi 설정에 따라 스테이션 또는 AP 모드로 동작을 시작합니다.
 * - 스테이션 모드로 연결되면, mDNS를 통해 클라이언트가 "http://meter.local"로 접속할 수 있습니다.
 * - 웹 클라이언트는 HTTP 요청이나 웹소켓을 통해 ESP32에 데이터를 요청하거나 명령어를 전송할 수 있습니다.
 * - 웹소켓을 사용하여 실시간 데이터를 주고받으며, 실시간 전력 모니터링 및 주파수 분석이 가능합니다.
 *
 * 주요 파일 설명:
 * - **wifi_cfg.h**: 전역 변수 및 함수 선언
 * - **wifi_cfg.cpp**: Wi-Fi 설정, 웹서버 및 웹소켓 설정, mDNS 서비스 설정, 클라이언트 요청 처리 함수 구현
 *
 */
#pragma once


#include <Arduino.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <FS.h>
#include <LittleFS.h>
#include <WiFi.h>

#include "K00_config_002.h"
//#include "config.h"

// #ifdef K20_FREQ_COUNTER_ENABLE
//     #include "K20_freq_counter_002.h"
// #endif

#include "K40_ina226_002.h"
#include "K50_nv_data_002.h"
extern K50_OPTIONS_t g_K50_NV_Options; 

static const char*    G_K35_TAG = "K25_WebSrv_cfg";    // ESP32 로그 태그

extern const char*    G_K10_Firmware_Rev;    // 펌웨어 버전 정보

// 전역 변수 선언
#define               G_K25_ASYNC_WEBSRV_PORT        80
AsyncWebSocket        g_K35_WebSocket("/ws");         // 웹소켓 서버 엔드포인트 설정
AsyncWebServer*       g_K35_pWebSrv = NULL;     // 웹 서버 포인터


uint32_t              g_K35_WS_ClientID = 0;    // 연결된 클라이언트 ID 저장

volatile bool         g_K35_WebSocket_ConnectedFlag    = false;    // 소켓 연결 상태 플래그
volatile bool         g_K40_INA226_CVCaptureFlag        = false;    // 전류/전압 캡처 플래그
extern volatile bool  g_K20_FreqCaptureFlag;                // 주파수 캡처 플래그
volatile bool         LastPacketAckFlag;                // 마지막 패킷 ACK 플래그



void                K35_WebSrv_init();
void                K35_WebSocket_event_handler(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void                K35_WebSocket_handle_message(void *arg, uint8_t *data, size_t len);

static String       K35_Web_string_processor(const String &var);
static void         K35_Web_not_found_handler(AsyncWebServerRequest *request);
static void         K35_Web_index_page_handler(AsyncWebServerRequest *request);
static void         K35_Web_set_defaults_handler(AsyncWebServerRequest *request);
static void         K35_Web_get_handler(AsyncWebServerRequest *request);
static void         K35_Web_restart_handler(AsyncWebServerRequest *request);
//static void         K35_Web_capture_handler(AsyncWebServerRequest *request);
static void         K35_Web_cv_chart_handler(AsyncWebServerRequest *request);
static void         K35_Web_cv_meter_handler(AsyncWebServerRequest *request);
static void         K35_Web_freq_counter_handler(AsyncWebServerRequest *request);


void K35_WebSrv_init(){
    g_K35_pWebSrv = new AsyncWebServer(G_K25_ASYNC_WEBSRV_PORT);  // HTTP 서버 생성 (포트 80)
    if (g_K35_pWebSrv == nullptr) {
        ESP_LOGE(G_K30_TAG, "Error creating AsyncWebServer!");    // 서버 생성 실패 시 로그 출력
        ESP.restart();                                            // 재시작
    }

    // 웹소켓 핸들러 설정
    g_K35_WebSocket.onEvent(K35_WebSocket_event_handler);  // 웹소켓 이벤트 처리 함수 등록
    g_K35_pWebSrv->addHandler(&g_K35_WebSocket);           // 웹소켓을 HTTP 서버에 추가

    // 웹서버 핸들러 설정
    g_K35_pWebSrv->onNotFound(K35_Web_not_found_handler);                       // 잘못된 경로로의 요청을 처리하는 핸들러 (404 응답)

    g_K35_pWebSrv->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        String v_logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
        //v_logmessage += " IR";
        Serial.println(v_logmessage);

        request->send(LittleFS, "/J10/index.html", String(), false, K35_Web_string_processor);    // HTML 파일 전송 
        // request->send(LittleFS, X35_URL_IR_UI1_USER_FILE, String(), false, X25_Set_HtmlVarable_CallBack);
    });
    // g_K35_pWebSrv->on("/", HTTP_GET, K35_Web_index_page_handler);               // 루트 경로("/")로의 GET 요청을 처리하는 핸들러 (홈페이지)

    
    g_K35_pWebSrv->on("/defaults", HTTP_GET, K35_Web_set_defaults_handler);  // 설정 재설정 요청을 처리하는 핸들러
    g_K35_pWebSrv->on("/get", HTTP_GET, K35_Web_get_handler);                   // GET 요청을 처리하여 클라이언트에서 SSID 및 비밀번호 변경 가능
    g_K35_pWebSrv->on("/restart", HTTP_GET, K35_Web_restart_handler);           // ESP32 재시작 요청을 처리하는 핸들러

    g_K35_pWebSrv->on("/cv_chart", HTTP_GET, K35_Web_cv_chart_handler);
    g_K35_pWebSrv->on("/cv_meter", HTTP_GET, K35_Web_cv_meter_handler);
    g_K35_pWebSrv->on("/freq_counter", HTTP_GET, K35_Web_freq_counter_handler);

    // LittleFS 파일 시스템에서 정적 파일 제공 (예: HTML, CSS, JS 파일)
    g_K35_pWebSrv->serveStatic("/", LittleFS, "/");

    g_K35_pWebSrv->begin();                     // 웹 서버 시작
    MDNS.addService("http", "tcp", 80);     // mDNS 서비스에 HTTP 추가
}



//  웹페이지의 변수(%txt%)를 치환하는 함수.
//  웹 페이지에서 사용될 텍스트를 처리하기 위한 함수로, SSID, 패스워드 및 펌웨어 버전을 포함하여
//  동적으로 값을 반환합니다. 예를 들어 웹 페이지의 텍스트 중 %SSID%가 있다면,
//  현재 설정된 SSID 값을 반환합니다.
static String K35_Web_string_processor(const String &var) {
    if (var == "FW_REV") {    // 펌웨어 버전을 요청한 경우
        return G_K10_Firmware_Rev;
    } else if (var == "SSID") {     // 현재 SSID를 요청한 경우
        return g_K50_NV_Options.ssid;
    } else if (var == "PASSWORD") {     // 현재 WiFi 비밀번호를 요청한 경우
        return g_K50_NV_Options.password;
    } else {
        return "?";     // 해당하는 변수가 없을 경우
    }
}

//  404 Not Found 핸들러.
//  요청한 경로가 존재하지 않을 때 호출되며, 클라이언트에 404 상태 코드를 반환합니다.
//  이는 잘못된 경로로의 요청을 처리하는 기본 핸들러입니다.

static void K35_Web_not_found_handler(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");    // 404 응답 반환
}

// 기본 홈 페이지(index.html)를 제공하는 핸들러.
// 웹 페이지 요청에 따라 LittleFS 파일 시스템에서 페이지를 로드합니다.
// "/index.html" 파일을 브라우저에 전송합니다.
static void K35_Web_index_page_handler(AsyncWebServerRequest *request) {
    request->send(LittleFS, "/J10/index.html", String(), false, K35_Web_string_processor);    // HTML 파일 전송
}

static void K35_Web_cv_chart_handler(AsyncWebServerRequest *request) {
    request->send(LittleFS, "/J10/cv_chart.html", String(), false, K35_Web_string_processor);
    }


static void K35_Web_cv_meter_handler(AsyncWebServerRequest *request) {
    request->send(LittleFS, "/J10/cv_meter.html", String(), false, K35_Web_string_processor);
    }

static void K35_Web_freq_counter_handler(AsyncWebServerRequest *request) {
    request->send(LittleFS, "/J10/freq_counter.html", String(), false, K35_Web_string_processor);
    }

// 기본 설정을 재설정하는 핸들러.
// 네트워크 설정을 기본 값으로 재설정한 후, 웹페이지로 결과를 반환합니다.
// 클라이언트는 설정이 초기화되었다는 메시지를 확인할 수 있습니다.
static void K35_Web_set_defaults_handler(AsyncWebServerRequest *request) {
    K50_NV_options_reset(g_K50_NV_Options);                                                                          // 옵션을 기본 값으로 재설정
    request->send(200, "text/html", "Default options set<br><a href=\"/\">Return to Home Page</a>");  // 설정 완료 메시지 전송
}

//  ESP32를 재시작하는 핸들러.
//  요청을 받은 후, 클라이언트에 "Restarting ..." 메시지를 보여주고, ESP32를 재시작합니다.
//  esp_restart() 함수를 호출하여 즉시 재시작합니다.
static void K35_Web_restart_handler(AsyncWebServerRequest *request) {
    request->send(200, "text/html", "Restarting ...");    // 클라이언트에 재시작 메시지 전송
    ESP_LOGI(G_K35_TAG, "Restarting ESP32");            // 로그 메시지 출력
    Serial.flush();                                        // 직렬 통신 버퍼 비우기
    delay(100);                                            // 잠시 대기 후
    esp_restart();                                        // ESP32 재시작
}

//  GET 요청 핸들러.
//  클라이언트에서 보낸 파라미터를 처리하며, SSID 및 패스워드를 변경하는 기능을 담당합니다.
//  변경된 정보는 영구 저장소에 저장됩니다.
//  클라이언트가 GET 요청을 통해 SSID나 패스워드를 변경할 때 이 함수가 호출됩니다.
static void K35_Web_get_handler(AsyncWebServerRequest *request) {
    String inputMessage;
    bool   bChange = false;

    // 요청에 SSID 파라미터가 있으면 이를 새로운 SSID로 저장
    if (request->hasParam("ssid")) {
        inputMessage = request->getParam("ssid")->value();
        bChange         = true;
        g_K50_NV_Options.ssid = inputMessage;
    }

    // 요청에 패스워드 파라미터가 있으면 이를 새로운 패스워드로 저장
    if (request->hasParam("password")) {
        inputMessage     = request->getParam("password")->value();
        bChange             = true;
        g_K50_NV_Options.password = inputMessage;
    }

    // 변경 사항이 있으면 옵션을 저장소에 저장
    if (bChange) {
        ESP_LOGI(G_K35_TAG, "Options changed");
        K50_NV_options_store(g_K50_NV_Options);
    }

    // 클라이언트에 처리 완료 메시지 전송
    request->send(200, "text/html", "Input Processed<br><a href=\"/\">Return to Home Page</a>");
}



// 웹소켓 이벤트 핸들러 함수.
// 클라이언트가 웹소켓에 연결, 메시지를 보냄, 연결을 끊음 등의 이벤트가 발생할 때 호출됩니다.
// AWS(WebSocket) 이벤트 타입에 따라 다르게 처리합니다.
void K35_WebSocket_event_handler(AsyncWebSocket       *server,
                          AsyncWebSocketClient *client,
                          AwsEventType            type,
                          void                   *arg,
                          uint8_t               *data,
                          size_t                len) {
    switch (type) {
        case WS_EVT_CONNECT:  // 클라이언트가 웹소켓에 연결되었을 때
            ESP_LOGI(G_K35_TAG, "WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            g_K35_WS_ClientID            = client->id();     // 클라이언트 ID 저장
            g_K35_WebSocket_ConnectedFlag = true;             // 소켓 연결 플래그 설정
            break;

        case WS_EVT_DISCONNECT:     // 클라이언트가 웹소켓 연결을 끊었을 때
            ESP_LOGI(G_K35_TAG, "WebSocket client #%u disconnected\n", client->id());
            g_K35_WebSocket_ConnectedFlag = false;  // 소켓 연결 플래그 해제
            g_K35_WS_ClientID            = 0;      // 클라이언트 ID 초기화
            break;

        case WS_EVT_DATA:                            // 클라이언트가 데이터를 보냈을 때
            K35_WebSocket_handle_message(arg, data, len);    // 데이터 처리 함수 호출
            break;

        case WS_EVT_PONG:    // PONG 메시지 (웹소켓에서 핑에 대한 응답) 발생 시
        case WS_EVT_ERROR:    // 웹소켓 오류 발생 시
            break;
    }
}


// 클라이언트로부터 수신된 메시지를 처리하는 함수.
// 웹소켓을 통해 수신된 데이터를 분석하여 명령어를 처리합니다.
// 메시지가 텍스트 형식(WS_TEXT)으로 전송되었는지 확인 후 처리합니다.
 
void K35_WebSocket_handle_message(void *arg, uint8_t *data, size_t len) {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;

    // 메시지가 완성되었고, 텍스트 형식이며, 길이가 맞는지 확인
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        // 수신된 메시지에 따른 동작 수행
        if (data[0] == 'x') {
            // 'x' 명령어: 마지막 패킷 ACK 플래그 설정
            LastPacketAckFlag = true;
        } else if (data[0] == 'm') {
            // 'm' 명령어: 전류 및 전압 측정 모드 설정
            g_K10_Measure.mode               = G_K00_MEASURE_MODE_CURRENT_VOLTAGE;
            g_K10_Measure.m.cv_meas.nSamples = 1;                        // 샘플 수 설정
            g_K10_Measure.m.cv_meas.cfg       = g_K40_INA226_Config[1].reg;            // 측정 설정
            g_K10_Measure.m.cv_meas.periodUs = g_K40_INA226_Config[1].periodUs;    // 측정 주기 설정
            g_K10_Measure.m.cv_meas.scale       = (int)(data[1] - '0');    // 스케일 설정
            g_K40_INA226_CVCaptureFlag               = true;                    // 전류/전압 캡처 플래그 설정
        } else if (data[0] == 'f') {
            // 'f' 명령어: 주파수 측정 모드 설정
            g_K10_Measure.mode    = G_K00_MEASURE_MODE_FREQUENCY;
            g_K20_FreqCaptureFlag = true;     // 주파수 캡처 플래그 설정
        } else {
            JsonDocument json;

            // const uint8_t size = JSON_OBJECT_SIZE(4);  // JSON 객체 크기 설정
            // StaticJsonDocument<size> json;  // JSON 문서 생성

            DeserializationError err = deserializeJson(json, data);     // JSON 데이터 역직렬화
            if (err) {
                ESP_LOGI(G_K35_TAG, "deserializeJson() failed with code %s", err.c_str());    // 오류 시 로그 출력
                return;
            }

            const char *szAction = json["action"];    // 액션 필드 추출

            // 'cv_capture' 명령어: 전류/전압 캡처 설정
            if (strcmp(szAction, "cv_capture") == 0) {
                const char *szCfgIndex         = json["cfgIndex"];
                const char *szCaptureSeconds = json["captureSecs"];
                const char *szScale             = json["scale"];

                int cfgIndex       = strtol(szCfgIndex, NULL, 10);           // 설정 인덱스 변환
                int captureSeconds = strtol(szCaptureSeconds, NULL, 10);   // 캡처 시간 변환
                int sampleRate       = 1000000 / g_K40_INA226_Config[cfgIndex].periodUs;  // 샘플링 속도 계산
                int numSamples       = captureSeconds * sampleRate;           // 총 샘플 수 계산
                int scale           = strtol(szScale, NULL, 10);               // 스케일 변환

                // 측정 모드 및 설정 적용
                g_K10_Measure.mode               = G_K00_MEASURE_MODE_CURRENT_VOLTAGE;
                g_K10_Measure.m.cv_meas.cfg       = g_K40_INA226_Config[cfgIndex].reg;
                g_K10_Measure.m.cv_meas.scale       = scale;
                g_K10_Measure.m.cv_meas.nSamples = numSamples;
                g_K10_Measure.m.cv_meas.periodUs = g_K40_INA226_Config[cfgIndex].periodUs;

                // 로그 출력
                ESP_LOGI(G_K35_TAG, "Mode = %d", g_K10_Measure.mode);
                ESP_LOGI(G_K35_TAG, "cfgIndex = %d", cfgIndex);
                ESP_LOGI(G_K35_TAG, "scale = %d", scale);
                ESP_LOGI(G_K35_TAG, "nSamples = %d", numSamples);
                ESP_LOGI(G_K35_TAG, "periodUs = %d", g_K40_INA226_Config[cfgIndex].periodUs);

                g_K40_INA226_CVCaptureFlag = true;  // 캡처 플래그 설정
            }
            // 'oscfreq' 명령어: 주파수 측정 설정
            else if (strcmp(szAction, "oscfreq") == 0) {
                g_K10_Measure.mode            = G_K00_MEASURE_MODE_FREQUENCY;
                const char *szOscFreqHz = json["freqhz"];

                ESP_LOGI(G_K35_TAG, "json[\"action\"]= %s\n", szAction);     // 액션 로그 출력
                ESP_LOGI(G_K35_TAG, "json[\"freqhz\"]= %s\n", szOscFreqHz);     // 주파수 로그 출력

                OscFreqHz    = (uint32_t)strtol(szOscFreqHz, NULL, 10);    // 주파수 값 변환
                OscFreqFlag = true;                                        // 주파수 측정 플래그 설정
            }
        }
    }
}

