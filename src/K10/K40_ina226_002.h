/*
 * INA226 전류 및 전압 측정 라이브러리
 *
 * 이 라이브러리는 INA226을 사용하여 전류 및 전압을 측정하는 다양한 기능을 제공합니다.
 * INA226은 I2C 통신을 통해 제어되며, 이 라이브러리를 통해 레지스터 설정, 측정 데이터 캡처 및 전송을 제어할 수 있습니다.
 *
 * 주요 기능:
 * 1. INA226 레지스터 읽기/쓰기 기능
 * 2. 원샷(One-shot) 모드에서 전류 및 전압 측정
 * 3. 평균(Averaged) 샘플링을 통한 측정
 * 4. 트리거 기반 데이터 캡처
 * 5. 게이트 기반 데이터 캡처
 * 6. 스케일 전환을 통한 자동 측정 범위 설정
 * 7. 측정 주기 및 샘플 수 설정
 * 8. 캡처된 데이터를 버퍼에 저장 및 전송
 *
 * 각 기능의 요약:
 *
 * 1. K40_INA226_write_reg(uint8_t regAddr, uint16_t data)
 *    - 지정된 레지스터에 16비트 데이터를 쓰는 함수입니다.
 *    - I2C 통신을 통해 데이터를 전송합니다.
 *
 * 2. K40_INA226_read_reg(uint8_t regAddr)
 *    - 지정된 레지스터에서 16비트 데이터를 읽어 반환하는 함수입니다.
 *    - I2C 통신을 통해 데이터를 읽어옵니다.
 *
 * 3. K40_INA226_reset()
 *    - INA226 장치를 리셋하여 설정을 초기화합니다.
 *    - 시스템 리셋 명령을 레지스터에 기록하여 실행합니다.
 *
 * 4. K40_INA226_capture_oneshot(volatile MEASURE_t &measure, volatile int16_t* buffer, bool manualScale)
 *    - 원샷 모드에서 전류 및 전압을 한 번 측정하고 버퍼에 데이터를 저장하는 함수입니다.
 *    - 측정된 샘플이 오프스케일(최대 범위를 벗어난 값)이면 이를 감지하고 처리합니다.
 *
 * 5. K40_INA226_capture_averaged_sample(volatile MEASURE_t &measure, volatile int16_t* buffer, bool manualScale)
 *    - 여러 샘플을 측정하여 평균값을 계산하고 버퍼에 저장하는 함수입니다.
 *    - 연속 모드에서 여러 샘플을 캡처한 후 평균값을 반환합니다.
 *
 * 6. K40_INA226_capture_buffer_triggered(volatile MEASURE_t &measure, volatile int16_t* buffer)
 *    - 트리거 기반으로 데이터를 캡처하고, 일정 샘플 수가 쌓이면 데이터를 전송하는 함수입니다.
 *    - 샘플링 주기에 맞춰 데이터를 버퍼에 저장하고 전송합니다.
 *
 * 7. K40_INA226_capture_buffer_gated(volatile MEASURE_t &measure, volatile int16_t* buffer)
 *    - 외부 게이트 신호가 활성화된 동안 데이터를 캡처하는 함수입니다.
 *    - 게이트 신호가 LOW로 유지되는 동안 샘플을 수집하고, 게이트가 닫히면 측정을 중지합니다.
 *
 * 8. K50_INA226_test_capture()
 *    - 원샷 샘플 캡처 기능을 테스트하는 함수입니다.
 *    - 다양한 설정에서 원샷 모드 측정을 수행하여 성능을 테스트합니다.
 *
 * 기타 주요 변수:
 * - g_K40_INA226_Config[]: 측정에 사용되는 설정 값 배열입니다. 각 설정은 측정 주기 및 평균 샘플 수를 정의합니다.
 * - g_K40_MaxSamples: 최대 샘플 수를 저장하는 변수입니다.
 * - g_K40_INA226_DataReadyFlag, g_K40_INA226_MeterReadyFlag 등: 다양한 상태 플래그를 정의하여, 데이터 준비 상태 또는 측정 완료 상태를 관리합니다.
 *
 * INA226을 사용한 전류 및 전압 측정 작업을 용이하게 하며, 이 라이브러리를 통해 다양한 캡처 모드 및 전송 방식을 사용할 수 있습니다.
 */

#pragma once

#include <Arduino.h>
#include <Wire.h>

// INA226 I2C 주소 정의
// 이 값은 데이터 시트에서 제공하는 INA226의 기본 7비트 주소입니다.
#define     G_K40_INA226_I2C_ADDR     0x40

// INA226 레지스터 주소 정의
// 각각의 레지스터는 INA226에서 데이터를 읽거나 쓰기 위해 사용됩니다.
#define     G_K40_INA226_REG_CFG                0x00          // 설정 레지스터, 장치 설정을 위한 레지스터
#define     G_K40_INA226_REG_SHUNT            0x01          // 션트 전압 레지스터, 션트 저항에서 측정된 전압을 저장
#define     G_K40_INA226_REG_VBUS            0x02          // 버스 전압 레지스터, 전원 공급선에서 측정된 전압을 저장
#define     G_K40_INA226_REG_MASK            0x06          // 마스크/활성 레지스터, 경고 및 알림 제어
#define     G_K40_INA226_REG_ALERT            0x07          // 경고 한계 레지스터, 경고 임계값을 설정
#define     G_K40_INA226_REG_ID                0xFE          // 제조사 ID 레지스터

// 전류 측정 스케일 정의
// 이 값들은 측정하려는 전류의 범위에 따라 션트 저항을 변경하기 위한 값입니다.
#define G_K40_INA226_SCALE_HI        0  // 고스케일: 션트 저항 = 0.05옴, 최대 전류 = 1.64A
#define G_K40_INA226_SCALE_LO        1  // 저스케일: 션트 저항 = 1.05옴, 최대 전류 = 78mA
#define G_K40_INA226_SCALE_AUTO        2  // 자동 모드: 측정에 따라 스케일을 자동으로 전환

// 메시지 ID 정의
// 각 메시지는 특정 상태나 명령을 전달하기 위한 코드로 사용됩니다.
#define G_K40_INA226_MSG_GATE_OPEN        1234  // 게이트가 열렸을 때 보내는 메시지
#define G_K40_INA226_MSG_TX_START        1111  // 전송 시작 메시지
#define G_K40_INA226_MSG_TX                2222  // 데이터 전송 중 메시지
#define G_K40_INA226_MSG_TX_COMPLETE     3333  // 데이터 전송 완료 메시지
#define G_K40_INA226_MSG_TX_CV_METER     4444  // CV 미터 데이터 전송 메시지

// K40_INA226_CONFIG_t 구조체 정의
// 이 구조체는 측정을 위한 설정 값을 저장하는데 사용됩니다.
typedef struct {
    uint16_t reg;        // 설정 레지스터 값 (장치 설정)
    uint32_t periodUs;    // 측정 주기, 마이크로초 단위로 설정됨
} K40_INA226_CONFIG_t;

    #define G_K40_INA226_NUM_CFG 4  // 설정 배열의 크기 (4개의 설정이 존재함)

// 외부 변수 선언
//extern const K40_INA226_CONFIG_t g_K40_INA226_Config[];               // 측정을 위한 설정 값 배열
int              g_K40_MaxSamples;               // 최대 샘플 수
//extern volatile bool  g_K40_INA226_GateOpenFlag;           // 게이트가 열렸는지 여부를 나타내는 플래그
// volatile bool  g_K40_INA226_DataReadyFlag;               // 데이터가 준비되었는지 여부를 나타내는 플래그
extern volatile int      g_K40_INA226_TxSamples;               // 전송할 샘플의 수
//extern volatile bool  g_K40_INA226_EndCaptureFlag;      // 캡처가 종료되었는지 여부를 나타내는 플래그
//extern volatile bool  g_K40_INA226_MeterReadyFlag;      // 미터가 준비되었는지 여부를 나타내는 플래그

// 함수 선언
void     K40_INA226_write_reg(uint8_t regAddr, uint16_t data);                                                           // 레지스터에 값을 쓰는 함수
uint16_t K40_INA226_read_reg(uint8_t regAddr);                                                                           // 레지스터에서 값을 읽는 함수
void     K40_INA226_reset();                                                                                           // INA226을 리셋하는 함수
bool     K40_INA226_capture_oneshot(volatile MEASURE_t& measure, volatile int16_t* buffer, bool manualScale);           // 원샷 캡처 함수
bool     K40_INA226_capture_averaged_sample(volatile MEASURE_t& measure, volatile int16_t* buffer, bool manualScale);  // 평균 샘플 캡처 함수
void     K40_INA226_capture_buffer_triggered(volatile MEASURE_t& measure, volatile int16_t* buffer);                   // 트리거된 버퍼 캡처 함수
void     K40_INA226_capture_buffer_gated(volatile MEASURE_t& measure, volatile int16_t* buffer);                       // 게이트된 버퍼 캡처 함수
void     K50_INA226_test_capture();                                                                                       // 테스트 캡처 함수



// ina226.cpp 파일

// 로그 태그 정의
// ESP32 및 기타 플랫폼에서 로그 출력을 위한 태그를 설정합니다.
#define         G_K40_TAG    "K40_ina226"
//static const char* G_K40_TAG = "ina226";


// 전역 플래그 변수 초기화
volatile bool             g_K40_INA226_DataReadyFlag    = false;  // 데이터 준비 완료 플래그
volatile bool             g_K40_INA226_MeterReadyFlag    = false;  // 미터 준비 완료 플래그
volatile bool             g_K40_INA226_GateOpenFlag    = false;  // 게이트 열림 플래그

extern volatile bool     g_K40_INA226_CVCaptureFlag;                 // CV 캡처 플래그
volatile bool             g_K40_INA226_EndCaptureFlag    = false;     // 캡처 종료 플래그
//extern volatile bool         LastPacketAckFlag = false;     // 마지막 패킷 확인 플래그

// g_K40_INA226_Config 배열 초기화
// 각각의 설정은 샘플링 주기와 평균을 설정하며, 각기 다른 샘플링 주기와 평균값을 제공합니다.
const K40_INA226_CONFIG_t g_K40_INA226_Config[G_K40_INA226_NUM_CFG] = {
    // 첫 번째 설정: 평균 1, 버스 204uS, 션트 204uS, 주기 500uS
    {0x4000 | (0 << 9) | (1 << 6) | (1 << 3), 500},
    // 두 번째 설정: 평균 1, 버스 332uS, 션트 332uS, 주기 1000uS
    {0x4000 | (0 << 9) | (2 << 6) | (2 << 3), 1000},
    // 세 번째 설정: 평균 4, 버스 204uS, 션트 204uS, 주기 2500uS
    {0x4000 | (1 << 9) | (1 << 6) | (1 << 3), 2500},
    // 네 번째 설정: 평균 128, 버스 1.1mS, 션트 1.1mS, 주기 557.57mS
    {0x4000 | (4 << 9) | (4 << 6) | (4 << 3), 1064000}};

// 스케일 전환 함수
// 션트 저항을 스케일에 맞춰 고스케일과 저스케일로 전환하는 함수입니다.
static void K50_INA226_switch_scale(int scale) {
    // SCALE_HI인 경우 FET1을 LOW로 설정하고 FET05를 HIGH로 설정합니다.
    // SCALE_LO인 경우 반대로 설정됩니다.
    digitalWrite(g_K00_PIN_FET_1Ohm, scale == G_K40_INA226_SCALE_HI ? LOW : HIGH);
    digitalWrite(g_K00_PIN_FET_05hm, scale == G_K40_INA226_SCALE_HI ? HIGH : LOW);
}

// INA226 레지스터 쓰기 함수
// 지정된 레지스터 주소에 16비트 데이터를 쓰는 함수입니다.
void K40_INA226_write_reg(uint8_t regAddr, uint16_t data) {
    Wire.beginTransmission(G_K40_INA226_I2C_ADDR);
    Wire.write(regAddr);                          // 레지스터 주소 전송
    Wire.write((uint8_t)((data >> 8) & 0x00FF));  // 상위 바이트 전송
    Wire.write((uint8_t)(data & 0x00FF));          // 하위 바이트 전송
    Wire.endTransmission();                          // 전송 완료
}

// INA226 레지스터 읽기 함수
// 지정된 레지스터 주소에서 16비트 데이터를 읽어 반환하는 함수입니다.
uint16_t K40_INA226_read_reg(uint8_t regAddr) {
    Wire.beginTransmission(G_K40_INA226_I2C_ADDR);
    Wire.write(regAddr);                   // 읽고자 하는 레지스터 주소 전송
    Wire.endTransmission(false);           // I2C 통신 재시작
    Wire.requestFrom(G_K40_INA226_I2C_ADDR, 2);  // 2바이트 요청
    uint8_t buf[2];
    buf[0] = Wire.read();  // 상위 바이트 읽기
    buf[1] = Wire.read();  // 하위 바이트 읽기
    // 상위 바이트와 하위 바이트 결합
    uint16_t res = ((uint16_t)buf[1]) | (((uint16_t)buf[0]) << 8);
    return res;
}

// INA226 시스템 리셋 함수
// 시스템 리셋을 통해 설정 값을 초기화합니다.
void K40_INA226_reset() {
    ESP_LOGI(G_K40_TAG, "INA226 시스템 리셋");
    K40_INA226_write_reg(G_K40_INA226_REG_CFG, 0x8000);    // 리셋 명령 전송
    delay(50);                            // 리셋 완료 대기
}

// int K40_Calc_MaxSamples(){
//     return (maxBufferBytes - 8) / 4;
// }

// K40_INA226_capture_oneshot: 원샷(단일) 샘플 캡처 함수
// 이 함수는 INA226에서 한 번의 션트와 버스 전압 샘플을 캡처하고 이를 통해 전류를 계산합니다.
// 원샷 모드에서는 션트와 버스 전압이 한 번만 측정되며, 그 결과가 반환됩니다.
// 매뉴얼 스케일을 사용하면 자동으로 스케일을 전환하지 않습니다.
bool K40_INA226_capture_oneshot(volatile MEASURE_t& measure, volatile int16_t* buffer, bool manualScale) {
    uint16_t reg_bus, reg_shunt;

    // 현재 설정된 측정 스케일에 따라 션트 저항을 전환합니다.
    K50_INA226_switch_scale(measure.m.cv_meas.scale);

    // 경고 핀(알림 핀)이 준비되면 LOW로 전환됨
    // G_K40_INA226_REG_MASK 레지스터에 0x0400을 쓰면 변환 완료 시 알림 핀이 LOW로 설정됨.
    K40_INA226_write_reg(G_K40_INA226_REG_MASK, 0x0400);

    // 션트 및 버스 전압을 원샷 모드로 설정하고 변환 시작
    buffer[0] = G_K40_INA226_MSG_TX_CV_METER;                                // 측정 데이터 전송 메시지 준비
    buffer[1] = measure.m.cv_meas.scale;                        // 현재 스케일을 버퍼에 기록
    K40_INA226_write_reg(G_K40_INA226_REG_CFG, measure.m.cv_meas.cfg | 0x0003);    // 원샷 변환 시작 (션트 및 버스)

    // 첫 번째 샘플 무시
    while (digitalRead(g_K00_PIN_INA226_ALERT) == HIGH);     // 알림 핀이 LOW가 될 때까지 대기
    reg_shunt = K40_INA226_read_reg(G_K40_INA226_REG_SHUNT);     // 션트 전압 읽기
    reg_bus      = K40_INA226_read_reg(G_K40_INA226_REG_VBUS);     // 버스 전압 읽기

    // 두 번째 샘플을 측정
    uint32_t tstart = micros();                                    // 측정 시작 시간 기록
    K40_INA226_write_reg(G_K40_INA226_REG_CFG, measure.m.cv_meas.cfg | 0x0003);    // 변환 시작
    while (digitalRead(g_K00_PIN_INA226_ALERT) == HIGH);                        // 알림 핀이 LOW가 될 때까지 대기
    uint32_t tend = micros();                                    // 측정 완료 시간 기록

    // 샘플을 읽고 버퍼에 저장
    reg_shunt          = K40_INA226_read_reg(G_K40_INA226_REG_SHUNT);     // 션트 전압 읽기
    reg_bus              = K40_INA226_read_reg(G_K40_INA226_REG_VBUS);     // 버스 전압 읽기
    int16_t shunt_i16 = (int16_t)reg_shunt;             // 션트 값 저장

    // 션트 값이 오프스케일인지(최대 값에 도달했는지) 확인
    bool offScale = ((shunt_i16 == 32767) || (shunt_i16 == -32768)) ? true : false;
    buffer[2]      = shunt_i16;           // 션트 전압 결과를 버퍼에 저장
    buffer[3]      = (int16_t)reg_bus;  // 버스 전압 결과를 버퍼에 저장
    buffer[4]      = offScale ? 1 : 0;  // 오프스케일 상태 저장

    // 측정된 전류 및 전압 값을 계산
    measure.m.cv_meas.iavgma     = (measure.m.cv_meas.scale == G_K40_INA226_SCALE_HI) ? shunt_i16 * 0.05f : shunt_i16 * 0.002381f;
    measure.m.cv_meas.iminma     = measure.m.cv_meas.iavgma;
    measure.m.cv_meas.imaxma     = measure.m.cv_meas.iavgma;
    measure.m.cv_meas.vavg         = reg_bus * 0.00125f;
    measure.m.cv_meas.vmax         = measure.m.cv_meas.vavg;
    measure.m.cv_meas.vmin         = measure.m.cv_meas.vavg;
    measure.m.cv_meas.sampleRate = 1000000.0f / (float)(tend - tstart);     // 샘플링 속도 계산

    // 측정 결과 로그 출력
    ESP_LOGI(G_K40_TAG, "OneShot : [0x%04X scale=%d] %dus %dHz %.1fV %.3fmA\n",
             measure.m.cv_meas.cfg, measure.m.cv_meas.scale, (tend - tstart),
             (int)(measure.m.cv_meas.sampleRate + 0.5f), measure.m.cv_meas.vavg, measure.m.cv_meas.iavgma);

    // 매뉴얼 스케일인 경우 미터 준비 완료 상태로 설정
    if (manualScale == true) {
        g_K40_INA226_MeterReadyFlag = true;
    } else {
        // 오프스케일이 아닌 경우에만 미터 준비 완료 플래그를 설정
        g_K40_INA226_MeterReadyFlag = !offScale;
    }

    return !offScale;  // 오프스케일 상태면 false 반환
}

// K40_INA226_capture_averaged_sample: 평균 샘플 캡처 함수
// 여러 샘플을 수집하여 평균값을 계산하는 함수입니다.
// 이 함수는 주어진 주기 동안 여러 샘플을 취해 평균 전류 및 전압을 계산합니다.
bool K40_INA226_capture_averaged_sample(volatile MEASURE_t& measure, volatile int16_t* buffer, bool manualScale) {
    int16_t     data_i16;                        // 션트와 버스 읽기 값 저장 변수
    int32_t     savg, bavg;                    // 션트 및 버스 값 누적을 위한 평균 계산 변수
    uint16_t reg_bus, reg_shunt;            // 읽은 션트와 버스 레지스터 값을 저장
    buffer[0] = G_K40_INA226_MSG_TX_CV_METER;            // 미터 메시지
    buffer[1] = measure.m.cv_meas.scale;    // 현재 스케일 저장
    K50_INA226_switch_scale(measure.m.cv_meas.scale);    // 스케일 전환

    // 변환 준비가 완료되면 알림 핀이 LOW로 설정됨
    K40_INA226_write_reg(G_K40_INA226_REG_MASK, 0x0400);

    // 션트 및 버스 전압을 연속 변환 모드로 설정
    K40_INA226_write_reg(G_K40_INA226_REG_CFG, measure.m.cv_meas.cfg | 0x0007);

    // 첫 번째 샘플 무시
    while (digitalRead(g_K00_PIN_INA226_ALERT) == HIGH);     // 알림 핀이 LOW가 될 때까지 대기
    reg_shunt = K40_INA226_read_reg(G_K40_INA226_REG_SHUNT);     // 션트 전압 읽기
    reg_bus      = K40_INA226_read_reg(G_K40_INA226_REG_VBUS);     // 버스 전압 읽기

    // 평균을 내기 위한 여러 샘플 수집 시작
    uint32_t tstart = micros();
    int         inx    = 0;
    savg = bavg        = 0;
    int     numSamples = 400000 / (int)measure.m.cv_meas.periodUs;     // 주어진 주기 동안의 샘플 수 계산
    bool offScale    = false;

    // 주어진 주기 동안 샘플을 수집
    while (inx < numSamples) {
        uint32_t t1 = micros();
        while (digitalRead(g_K00_PIN_INA226_ALERT) == HIGH);     // 알림 핀이 LOW가 될 때까지 대기
        reg_shunt = K40_INA226_read_reg(G_K40_INA226_REG_SHUNT);     // 션트 전압 읽기
        reg_bus      = K40_INA226_read_reg(G_K40_INA226_REG_VBUS);     // 버스 전압 읽기

        data_i16 = (int16_t)reg_shunt;
        if ((data_i16 == 32767) || (data_i16 == -32768)) {
            offScale = true;  // 오프스케일 상태 체크
        }
        savg += (int32_t)data_i16;    // 션트 값 누적

        data_i16 = (int16_t)reg_bus;
        bavg += (int32_t)data_i16;    // 버스 값 누적

        while ((micros() - t1) < measure.m.cv_meas.periodUs);  // 샘플링 주기 대기
        inx++;
    }

    uint32_t us                     = micros() - tstart;                              // 전체 측정 시간 계산
    measure.m.cv_meas.sampleRate = (1000000.0f * (float)numSamples) / (float)us;  // 샘플링 속도 계산

    // 션트와 버스 평균값 계산
    savg                     = savg / numSamples;
    measure.m.cv_meas.iavgma = (measure.m.cv_meas.scale == G_K40_INA226_SCALE_HI) ? savg * 0.05f : savg * 0.002381f;
    bavg                     = bavg / numSamples;
    measure.m.cv_meas.vavg     = bavg * 0.00125f;

    buffer[2] = (int16_t)savg;       // 평균 션트 값 버퍼에 저장
    buffer[3] = (int16_t)bavg;       // 평균 버스 값 버퍼에 저장
    buffer[4] = offScale ? 1 : 0;  // 오프스케일 상태 버퍼에 저장

    // 결과 로그 출력
    ESP_LOGI(G_K40_TAG, "CV Meter sample : %s %.1fV %.3fmA\n", measure.m.cv_meas.scale == G_K40_INA226_SCALE_LO ? "LO" : "HI", measure.m.cv_meas.vavg, measure.m.cv_meas.iavgma);

    // 매뉴얼 스케일 설정
    if (manualScale == true) {
        g_K40_INA226_MeterReadyFlag = true;
    } else {
        g_K40_INA226_MeterReadyFlag = !offScale;     // 오프스케일이 아니면 준비 완료
    }

    return !offScale;  // 오프스케일이면 false 반환
}

// K40_INA226_capture_buffer_triggered: 트리거 기반의 버퍼 캡처 함수
// 이 함수는 지정된 수의 샘플을 버퍼에 저장하며, 전환이 완료되면 데이터가 전송됩니다.
// 트리거는 일정한 주기 동안 반복해서 데이터를 캡처하고, 버퍼가 가득 차면 이를 전송하는 방식입니다.
void K40_INA226_capture_buffer_triggered(volatile MEASURE_t& measure, volatile int16_t* buffer) {
    int16_t     smax, smin, bmax, bmin, data_i16;                           // 션트 및 버스 전압 최소/최대 값
    int32_t     savg, bavg;                                               // 션트 및 버스 값 누적을 위한 평균 계산 변수
    uint16_t reg_bus, reg_shunt;                                       // 션트 및 버스 레지스터 값
    smax = bmax = -32768;                                               // 초기 최소값 설정
    smin = bmin = 32767;                                               // 초기 최대값 설정
    savg = bavg             = 0;                                           // 션트 및 버스 평균 값 누적 초기화
    int samplesPerSecond = 1000000 / (int)measure.m.cv_meas.periodUs;  // 초당 샘플 수 계산
    K50_INA226_switch_scale(measure.m.cv_meas.scale);                               // 스케일 전환

    // 전환 준비가 완료되면 알림 핀이 LOW로 설정됨
    K40_INA226_write_reg(G_K40_INA226_REG_MASK, 0x0400);
    // 션트 및 버스 전압을 연속 변환 모드로 설정
    K40_INA226_write_reg(G_K40_INA226_REG_CFG, measure.m.cv_meas.cfg | 0x0007);

    // 첫 번째 샘플 무시
    while (digitalRead(g_K00_PIN_INA226_ALERT) == HIGH);     // 알림 핀이 LOW가 될 때까지 대기
    reg_shunt = K40_INA226_read_reg(G_K40_INA226_REG_SHUNT);     // 션트 전압 읽기
    reg_bus      = K40_INA226_read_reg(G_K40_INA226_REG_VBUS);     // 버스 전압 읽기

    uint32_t tstart = micros();     // 측정 시작 시간 기록
    // 버퍼의 헤더에 전송 시작 메시지와 샘플 주기 및 스케일 정보 저장
    buffer[0]       = G_K40_INA226_MSG_TX_START;
    buffer[1]       = (int16_t)measure.m.cv_meas.periodUs;
    buffer[2]       = measure.m.cv_meas.scale;
    int offset       = 3;         // 버퍼 시작 오프셋
    g_K40_INA226_EndCaptureFlag = false;     // 캡처 종료 플래그 초기화
    int inx           = 0;         // 샘플 인덱스 초기화

    // 측정할 샘플 수만큼 반복
    while (inx < measure.m.cv_meas.nSamples) {
        uint32_t t1          = micros();
        int         bufIndex = offset + 2 * inx;    // 버퍼 인덱스 계산
        while (digitalRead(g_K00_PIN_INA226_ALERT) == HIGH);    // 알림 핀이 LOW가 될 때까지 대기
        // 션트 및 버스 전압 읽기
        reg_shunt = K40_INA226_read_reg(G_K40_INA226_REG_SHUNT);
        reg_bus      = K40_INA226_read_reg(G_K40_INA226_REG_VBUS);

        // 션트 전압 저장 및 최소/최대 값 갱신
        data_i16         = (int16_t)reg_shunt;
        buffer[bufIndex] = data_i16;
        savg += (int32_t)data_i16;
        if (data_i16 > smax)
            smax = data_i16;
        if (data_i16 < smin)
            smin = data_i16;

        // 버스 전압 저장 및 최소/최대 값 갱신
        data_i16             = (int16_t)reg_bus;
        buffer[bufIndex + 1] = data_i16;
        bavg += (int32_t)data_i16;
        if (data_i16 > bmax)
            bmax = data_i16;
        if (data_i16 < bmin)
            bmin = data_i16;

        // 일정 시간마다 패킷을 분할하여 전송
        if (((inx + 1) % samplesPerSecond) == 0) {
            // 패킷 메시지 추가 및 전송 샘플 수 설정
            buffer[bufIndex + 2] = G_K40_INA226_MSG_TX;
            offset++;
            g_K40_INA226_TxSamples       = samplesPerSecond;
            g_K40_INA226_EndCaptureFlag = (inx == (measure.m.cv_meas.nSamples - 1)) ? true : false;    // 마지막 샘플인지 확인
            g_K40_INA226_DataReadyFlag  = true;                                                        // 데이터 준비 완료 플래그 설정
        }
        // 주기 내에서 다음 샘플링 시간까지 대기
        while ((micros() - t1) < measure.m.cv_meas.periodUs);
        inx++;
    }

    // 전체 측정 시간이 종료된 후 처리
    uint32_t us                     = micros() - tstart;
    measure.m.cv_meas.sampleRate = (1000000.0f * (float)measure.m.cv_meas.nSamples) / (float)us;
    // 션트 전압 평균 값 계산
    savg = savg / measure.m.cv_meas.nSamples;
    if (measure.m.cv_meas.scale == G_K40_INA226_SCALE_HI) {
        measure.m.cv_meas.iavgma = savg * 0.05f;
        measure.m.cv_meas.imaxma = smax * 0.05f;
        measure.m.cv_meas.iminma = smin * 0.05f;
    } else {
        measure.m.cv_meas.iavgma = savg * 0.002381f;
        measure.m.cv_meas.imaxma = smax * 0.002381f;
        measure.m.cv_meas.iminma = smin * 0.002381f;
    }

    // 버스 전압 평균 값 계산
    bavg                   = bavg / measure.m.cv_meas.nSamples;
    measure.m.cv_meas.vavg = bavg * 0.00125f;
    measure.m.cv_meas.vmax = bmax * 0.00125f;
    measure.m.cv_meas.vmin = bmin * 0.00125f;

    // 최종 로그 출력
    ESP_LOGI(G_K40_TAG, "CV Buffer Triggered : 0x%04X %s %.1fHz %.1fV %.3fmA\n",
             measure.m.cv_meas.cfg, measure.m.cv_meas.scale == G_K40_INA226_SCALE_LO ? "LO" : "HI",
             measure.m.cv_meas.sampleRate, measure.m.cv_meas.vavg, measure.m.cv_meas.iavgma);
}

// K40_INA226_capture_buffer_gated: 게이트 기반의 버퍼 캡처 함수
// 게이트 신호가 들어오면 데이터를 캡처하고, 게이트가 닫히면 캡처를 중지합니다.
// 주로 외부에서 특정 신호(게이트)가 들어올 때만 측정하고 싶을 때 사용됩니다.
void K40_INA226_capture_buffer_gated(volatile MEASURE_t& measure, volatile int16_t* buffer) {
    int16_t     smax, smin, bmax, bmin, data_i16;                           // 션트 및 버스 전압 최소/최대 값
    int32_t     savg, bavg;                                               // 션트 및 버스 값 누적을 위한 평균 계산 변수
    uint16_t reg_bus, reg_shunt;                                       // 션트 및 버스 레지스터 값
    smax = bmax = -32768;                                               // 최소값 초기화
    smin = bmin = 32767;                                               // 최대값 초기화
    savg = bavg             = 0;                                           // 누적 값 초기화
    int samplesPerSecond = 1000000 / (int)measure.m.cv_meas.periodUs;  // 초당 샘플 수 계산
    K50_INA226_switch_scale(measure.m.cv_meas.scale);                               // 스케일 전환

    // 전환 준비가 완료되면 알림 핀이 LOW로 설정됨
    K40_INA226_write_reg(G_K40_INA226_REG_MASK, 0x0400);
    // 션트 및 버스 전압을 연속 변환 모드로 설정
    K40_INA226_write_reg(G_K40_INA226_REG_CFG, measure.m.cv_meas.cfg | 0x0007);
    // 첫 번째 샘플 무시
    while (digitalRead(g_K00_PIN_INA226_ALERT) == HIGH);     // 알림 핀이 LOW가 될 때까지 대기
    reg_shunt = K40_INA226_read_reg(G_K40_INA226_REG_SHUNT);     // 션트 전압 읽기
    reg_bus      = K40_INA226_read_reg(G_K40_INA226_REG_VBUS);     // 버스 전압 읽기

    // 게이트 신호가 활성화되면 데이터 캡처 시작
    buffer[0]       = G_K40_INA226_MSG_TX_START;                           // 버퍼의 시작 위치에 시작 메시지 기록
    buffer[1]       = (int16_t)measure.m.cv_meas.periodUs;  // 샘플 주기 저장
    buffer[2]       = measure.m.cv_meas.scale;               // 현재 스케일 저장
    int offset       = 3;                                       // 버퍼 시작 위치 설정
    int numSamples = 0;                                       // 캡처된 샘플 수 초기화
    g_K40_INA226_EndCaptureFlag = false;                                   // 캡처 종료 플래그 초기화
    // 게이트 신호가 LOW인 경우에만 샘플링 수행
    while (digitalRead(g_K00_PIN_GATE) == HIGH);  // 게이트 신호가 활성화될 때까지 대기
    g_K40_INA226_GateOpenFlag    = true;                   // 게이트가 열렸음을 알림
    uint32_t tstart = micros();               // 캡처 시작 시간 기록

    // 게이트가 활성화된 동안 샘플을 수집
    while ((digitalRead(g_K00_PIN_GATE) == LOW) && (numSamples < g_K40_MaxSamples)) {
        uint32_t t1          = micros();
        int         bufIndex = offset + 2 * numSamples;  // 버퍼 인덱스 계산
        while (digitalRead(g_K00_PIN_INA226_ALERT) == HIGH);          // 알림 핀이 LOW가 될 때까지 대기
        // 션트 및 버스 전압 읽기
        reg_shunt = K40_INA226_read_reg(G_K40_INA226_REG_SHUNT);
        reg_bus      = K40_INA226_read_reg(G_K40_INA226_REG_VBUS);

        // 션트 전압 저장 및 최소/최대 값 갱신
        data_i16         = (int16_t)reg_shunt;
        buffer[bufIndex] = data_i16;
        savg += (int32_t)data_i16;
        if (data_i16 > smax)
            smax = data_i16;
        if (data_i16 < smin)
            smin = data_i16;

        // 버스 전압 저장 및 최소/최대 값 갱신
        data_i16             = (int16_t)reg_bus;
        buffer[bufIndex + 1] = data_i16;
        bavg += (int32_t)data_i16;
        if (data_i16 > bmax)
            bmax = data_i16;
        if (data_i16 < bmin)
            bmin = data_i16;

        // 일정 시간마다 패킷을 분할하여 전송
        if (((numSamples + 1) % samplesPerSecond) == 0) {
            buffer[bufIndex + 2] = G_K40_INA226_MSG_TX;    // 패킷 메시지 추가
            offset++;
            g_K40_INA226_TxSamples      = samplesPerSecond;
            g_K40_INA226_DataReadyFlag = true;  // 데이터 준비 완료 플래그 설정
        }

        while ((micros() - t1) < measure.m.cv_meas.periodUs);  // 샘플링 주기 대기
        numSamples++;
    }

    uint32_t us                     = micros() - tstart;                              // 캡처 종료 시간 기록
    g_K40_INA226_TxSamples                     = numSamples % samplesPerSecond;                  // 남은 샘플 수 계산
    g_K40_INA226_EndCaptureFlag                 = true;                                          // 캡처 종료 플래그 설정
    g_K40_INA226_DataReadyFlag                 = true;                                          // 데이터 준비 완료 플래그 설정
    measure.m.cv_meas.nSamples     = numSamples;                                      // 총 샘플 수 저장
    measure.m.cv_meas.sampleRate = (1000000.0f * (float)numSamples) / (float)us;  // 샘플 속도 계산

    // 션트 전압 평균 값 계산
    savg = savg / numSamples;
    if (measure.m.cv_meas.scale == G_K40_INA226_SCALE_HI) {
        measure.m.cv_meas.iavgma = savg * 0.05f;
        measure.m.cv_meas.imaxma = smax * 0.05f;
        measure.m.cv_meas.iminma = smin * 0.05f;
    } else {
        measure.m.cv_meas.iavgma = savg * 0.002381f;
        measure.m.cv_meas.imaxma = smax * 0.002381f;
        measure.m.cv_meas.iminma = smin * 0.002381f;
    }

    // 버스 전압 평균 값 계산
    bavg                   = bavg / numSamples;
    measure.m.cv_meas.vavg = bavg * 0.00125f;
    measure.m.cv_meas.vmax = bmax * 0.00125f;
    measure.m.cv_meas.vmin = bmin * 0.00125f;

    // 최종 로그 출력
    ESP_LOGI(G_K40_TAG, "CV g_K10_Buffer Gated : %.3fsecs 0x%04X %s %.1fHz %.1fV %.3fmA\n",
             (float)us / 1000000.0f, measure.m.cv_meas.cfg, measure.m.cv_meas.scale == G_K40_INA226_SCALE_LO ? "LO" : "HI",
             measure.m.cv_meas.sampleRate, measure.m.cv_meas.vavg, measure.m.cv_meas.iavgma);
}

// K50_INA226_test_capture: 테스트용 원샷 샘플 캡처 함수
// 각 설정에 대해 원샷 샘플을 캡처하고 성능을 테스트합니다.
void K50_INA226_test_capture() {
    ESP_LOGI(G_K40_TAG, "Measuring one-shot sample times");     // 테스트 시작 로그
    g_K10_Measure.m.cv_meas.scale = G_K40_INA226_SCALE_HI;                         // 고스케일로 설정
    for (int inx = 0; inx < G_K40_INA226_NUM_CFG; inx++) {
        g_K10_Measure.m.cv_meas.cfg = g_K40_INA226_Config[inx].reg;        // 각 설정에 대해 원샷 캡처 수행
        K40_INA226_capture_oneshot(g_K10_Measure, g_K10_Buffer, true);    // 원샷 캡처 함수 호출
    }
}
