// nv_data.h

#pragma once

#include <Arduino.h>
#include <Preferences.h>



// nv_data.cpp 구현부
Preferences g_K50_NV_Prefs; // Preferences 객체 생성

// K50_OPTIONS_t 구조체 정의
typedef struct {
    String ssid;      // Wi-Fi SSID
    String password;  // Wi-Fi 비밀번호
} K50_OPTIONS_t;

K50_OPTIONS_t g_K50_NV_Options; // K50_OPTIONS_t 타입의 전역 변수 선언

#define G_K50_NV_MODE_READ_WRITE false // 읽기/쓰기 모드
#define G_K50_NV_MODE_READ_ONLY true    // 읽기 전용 모드

// 함수 프로토타입 선언
void K50_NV_options_store(K50_OPTIONS_t &p_options); // 옵션을 저장하는 함수
void K50_NV_options_load(K50_OPTIONS_t &p_options);  // 옵션을 로드하는 함수
void K50_NV_options_reset(K50_OPTIONS_t &p_options); // 옵션을 초기화하는 함수
void K50_NV_options_print(K50_OPTIONS_t &p_options); // 옵션을 출력하는 함수


// 옵션을 로드하는 함수
void K50_NV_options_load(K50_OPTIONS_t &p_options) {
    // Preferences 시작
    if (g_K50_NV_Prefs.begin("options", G_K50_NV_MODE_READ_ONLY) == false) {
        Serial.println("Preferences 'options' namespace not found, creating with defaults.");
        g_K50_NV_Prefs.end(); // Preferences 종료
        K50_NV_options_reset(p_options); // 기본값으로 초기화
    } else {
        p_options.ssid = g_K50_NV_Prefs.getString("ssid", ""); // SSID 읽기
        p_options.password = g_K50_NV_Prefs.getString("pwd", ""); // 비밀번호 읽기
        g_K50_NV_Prefs.end(); // Preferences 종료
        K50_NV_options_print(p_options); // 읽은 옵션 출력
    }
}

// 옵션을 출력하는 함수
void K50_NV_options_print(K50_OPTIONS_t &p_options) {
    Serial.println("SSID = " + p_options.ssid); // SSID 출력
}

// 옵션을 초기화하는 함수
void K50_NV_options_reset(K50_OPTIONS_t &p_options) {
    p_options.ssid = ""; // SSID 초기화
    p_options.password = ""; // 비밀번호 초기화
    K50_NV_options_store(p_options); // 초기화된 옵션 저장
    Serial.println("Set Default Options"); // 기본값으로 설정되었다고 출력
    K50_NV_options_print(p_options); // 초기화된 옵션 출력
}

// 옵션을 저장하는 함수
void K50_NV_options_store(K50_OPTIONS_t &p_options) {
    g_K50_NV_Prefs.begin("options", G_K50_NV_MODE_READ_WRITE); // Preferences 시작
    g_K50_NV_Prefs.clear(); // 기존 데이터 삭제
    g_K50_NV_Prefs.putString("ssid", p_options.ssid); // SSID 저장
    g_K50_NV_Prefs.putString("pwd", p_options.password); // 비밀번호 저장
    g_K50_NV_Prefs.end(); // Preferences 종료
    K50_NV_options_print(p_options); // 저장된 옵션 출력
}

