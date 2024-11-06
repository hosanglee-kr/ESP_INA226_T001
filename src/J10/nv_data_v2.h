// nv_data.h

#ifndef NVDATA_H_
#define NVDATA_H_

#include <Arduino.h>
#include <Preferences.h>



// nv_data.cpp 구현부
Preferences Prefs; // Preferences 객체 생성

// OPTIONS_t 구조체 정의
typedef struct {
    String ssid;      // Wi-Fi SSID
    String password;  // Wi-Fi 비밀번호
} OPTIONS_t;

OPTIONS_t Options; // OPTIONS_t 타입의 전역 변수 선언

#define MODE_READ_WRITE false // 읽기/쓰기 모드
#define MODE_READ_ONLY true    // 읽기 전용 모드

// 함수 프로토타입 선언
void nv_options_store(OPTIONS_t &options); // 옵션을 저장하는 함수
void nv_options_load(OPTIONS_t &options);  // 옵션을 로드하는 함수
void nv_options_reset(OPTIONS_t &options); // 옵션을 초기화하는 함수
void nv_options_print(OPTIONS_t &options); // 옵션을 출력하는 함수


// 옵션을 로드하는 함수
void nv_options_load(OPTIONS_t &options) {
    // Preferences 시작
    if (Prefs.begin("options", MODE_READ_ONLY) == false) {
        Serial.println("Preferences 'options' namespace not found, creating with defaults.");
        Prefs.end(); // Preferences 종료
        nv_options_reset(options); // 기본값으로 초기화
    } else {
        options.ssid = Prefs.getString("ssid", ""); // SSID 읽기
        options.password = Prefs.getString("pwd", ""); // 비밀번호 읽기
        Prefs.end(); // Preferences 종료
        nv_options_print(options); // 읽은 옵션 출력
    }
}

// 옵션을 출력하는 함수
void nv_options_print(OPTIONS_t &options) {
    Serial.println("SSID = " + options.ssid); // SSID 출력
}

// 옵션을 초기화하는 함수
void nv_options_reset(OPTIONS_t &options) {
    options.ssid = ""; // SSID 초기화
    options.password = ""; // 비밀번호 초기화
    nv_options_store(options); // 초기화된 옵션 저장
    Serial.println("Set Default Options"); // 기본값으로 설정되었다고 출력
    nv_options_print(options); // 초기화된 옵션 출력
}

// 옵션을 저장하는 함수
void nv_options_store(OPTIONS_t &options) {
    Prefs.begin("options", MODE_READ_WRITE); // Preferences 시작
    Prefs.clear(); // 기존 데이터 삭제
    Prefs.putString("ssid", options.ssid); // SSID 저장
    Prefs.putString("pwd", options.password); // 비밀번호 저장
    Prefs.end(); // Preferences 종료
    nv_options_print(options); // 저장된 옵션 출력
}

#endif // NVDATA_H_
