#pragma once

#include <WiFi.h>
#include <ESPmDNS.h>

#include "K00_config_002.h"
#include "K50_nv_data_002.h"

extern K50_OPTIONS_t g_K50_NV_Options; 

static const char*    G_K30_TAG = "K30_wifi_cfg";    // ESP32 로그 태그


// WiFi 설정: 정적 IP 주소 설정
static IPAddress     g_K30_WifiStation_Local_IP(192, 168, 43, 236);  // ESP32에 할당될 고정 IP
static IPAddress     g_K30_WifiStation_Gateway(192, 168, 43, 1);       // 게이트웨이 주소
static IPAddress     g_K30_WifiStation_Subnet(255, 255, 255, 0);       // 서브넷 마스크
static IPAddress     g_K30_WifiStation_PrimaryDNS(8, 8, 8, 8);       // 기본 DNS 서버
static IPAddress     g_K30_WifiStation_SecondaryDNS(8, 8, 4, 4);       // 보조 DNS 서버

// AP 모드에서 사용될 SSID
const char*            g_K30_wifi_AP_SSID = "ESP32_METER";  // 비밀번호 없는 Access Point 모드의 SSID

// 함수 선언
void                  K30_wifi_init();
static void           K30_wifi_start_as_ap();
static void           K30_wifi_start_as_station();
static void           K30_wifi_start_as_station_static_IP();


/*
 * AP 모드에서 Wi-Fi 액세스 포인트를 시작하는 함수.
 * 지정된 SSID로 ESP32가 자체 액세스 포인트를 생성하며, 비밀번호는 설정되지 않습니다.
 * 사용자가 네트워크 설정을 입력하지 않았을 때 기본적으로 호출됩니다.
 */
static void K30_wifi_start_as_ap() {
    ESP_LOGI(G_K30_TAG, "Starting Access Point with SSID=%s, no password\n", g_K30_wifi_AP_SSID);     // AP 시작 로그 출력
    WiFi.softAP(g_K30_wifi_AP_SSID);                                                                 // AP 모드 시작
    IPAddress ipaddr = WiFi.softAPIP();                                                     // 할당된 AP IP 주소 가져오기
    ESP_LOGI(G_K30_TAG, "Web Server IP address : %s", ipaddr.toString().c_str());         // IP 주소 로그 출력
    digitalWrite(g_K00_PIN_LED, HIGH);                                                             // LED를 켜서 AP 모드 활성화 표시
}

/*
 * 정적 IP 설정을 사용하여 Wi-Fi 스테이션 모드를 시작하는 함수.
 * ESP32가 클라이언트로서 지정된 SSID에 접속을 시도하며, 고정 IP를 사용하여 네트워크에 연결됩니다.
 * 이 함수는 저장된 네트워크 정보로 자동 연결을 시도하며, 연결이 실패하면 AP 모드로 전환됩니다.
 */
static void K30_wifi_start_as_station_static_IP() {
    ESP_LOGI(G_K30_TAG, "Connecting as station static IP to Access Point with SSID=%s\n", g_K50_NV_Options.ssid);  // 연결 시도 로그 출력
    uint32_t startTick = millis();                                                                          // 연결 시작 시간 기록

    // 정적 IP 설정을 사용하여 네트워크 설정
    if (!WiFi.config(g_K30_WifiStation_Local_IP, g_K30_WifiStation_Gateway, g_K30_WifiStation_Subnet, g_K30_WifiStation_PrimaryDNS, g_K30_WifiStation_SecondaryDNS)) {
        ESP_LOGI(G_K30_TAG, "Station static IP config failure");  // IP 설정 실패 시 로그 출력
    }

    // 저장된 SSID와 비밀번호로 네트워크 연결 시도
    WiFi.begin(g_K50_NV_Options.ssid.c_str(), g_K50_NV_Options.password.c_str());

    // 연결 대기 (4초)
    if (WiFi.waitForConnectResult(4000UL) != WL_CONNECTED) {
        ESP_LOGI(G_K30_TAG, "Connection failed!");    // 연결 실패 시 로그 출력
        K30_wifi_start_as_ap();                            // 연결 실패 시 AP 모드로 전환
    } else {
        IPAddress ipaddr  = WiFi.localIP();                                                                                                      // 연결된 IP 주소 가져오기
        uint32_t  endTick = millis();                                                                                                          // 연결 종료 시간 기록
        ESP_LOGI(G_K30_TAG, "Connected in %.2f seconds with IP addr %s", (float)(endTick - startTick) / 1000.0f, ipaddr.toString().c_str());  // 연결 시간 출력
        digitalWrite(g_K00_PIN_LED, LOW);                                                                                                              // 연결 성공 시 LED 끔
    }
}

/*
 * 동적 IP 설정을 사용하여 Wi-Fi 스테이션 모드를 시작하는 함수.
 * ESP32가 클라이언트로서 네트워크에 접속하며, DHCP를 통해 IP를 할당받습니다.
 * 네트워크 연결에 성공하면 LED를 끄고, 실패하면 AP 모드로 전환합니다.
 */
static void K30_wifi_start_as_station() {
    ESP_LOGI(G_K30_TAG, "Connecting as station to Access Point with SSID=%s", g_K50_NV_Options.ssid);  // 연결 시도 로그 출력
    uint32_t startTick = millis();                                                              // 연결 시작 시간 기록

    WiFi.mode(WIFI_STA);                                         // 스테이션 모드로 설정
    WiFi.begin(g_K50_NV_Options.ssid.c_str(), g_K50_NV_Options.password.c_str());     // 저장된 SSID와 비밀번호로 연결 시도

    // 연결 대기 (10초)
    if (WiFi.waitForConnectResult(10000UL) != WL_CONNECTED) {
        ESP_LOGI(G_K30_TAG, "Connection failed!");    // 연결 실패 시 로그 출력
        K30_wifi_start_as_ap();                            // 실패하면 AP 모드로 전환
    } else {
        uint32_t  endTick = millis();                                                                                                          // 연결 종료 시간 기록
        IPAddress ipaddr  = WiFi.localIP();                                                                                                      // 연결된 IP 주소 가져오기
        ESP_LOGI(G_K30_TAG, "Connected in %.2f seconds with IP addr %s", (float)(endTick - startTick) / 1000.0f, ipaddr.toString().c_str());  // 연결 시간 로그 출력
        digitalWrite(g_K00_PIN_LED, LOW);                                                                                                              // 연결 성공 시 LED 끔
    }
}

/*
 * Wi-Fi 초기화 함수.
 * 설정에 따라 Wi-Fi 스테이션 또는 AP 모드를 선택하여 시작합니다.
 * 또한 mDNS 서비스를 시작하여 "http://meter.local" 도메인으로 접속할 수 있도록 설정합니다.
 * 웹서버와 웹소켓 서버도 이 함수에서 초기화됩니다.
 */

void K30_wifi_init() {
    delay(100);     // 초기화 딜레이

    // 저장된 SSID가 없으면 AP 모드로 시작
    if (g_K50_NV_Options.ssid == "") {
        K30_wifi_start_as_ap();
    } else {
        K30_wifi_start_as_station_static_IP();    // 있으면 고정 IP로 연결 시도
    }

    // mDNS 서비스 시작 (http://meter.local)
    if (!MDNS.begin("meter")) {
        ESP_LOGI(G_K30_TAG, "Error starting mDNS service");     // mDNS 시작 실패 로그
    }    
}
