// freq_counter.h 및 freq_counter.cpp 합본 파일
#pragma once

#include <Arduino.h>
#include <stdio.h>                            // 표준 입출력 라이브러리
#include <driver/ledc.h>                      // ESP32 LEDC 라이브러리
#include <soc/pcnt_struct.h>                  // Pulse Counter 관련 구조체

#include <driver/pulse_cnt.h>
//#include <driver/pcnt.h>                      // ESP32 PCNT (Pulse Counter) 라이브러리


#include "K00_config_002.h"
//#include "config.h"                           // 프로젝트 설정 관련 헤더


// 주파수 측정 관련 전역 변수들
volatile bool FreqReadyFlag = false;          // 주파수 측정 완료 플래그
volatile bool FreqCaptureFlag = false;        // 주파수 캡처 플래그
volatile int  FrequencyHz = 0;                // 측정된 주파수 값 (Hz)

volatile SemaphoreHandle_t FreqSemaphore;     // 주파수 측정 완료를 위한 세마포어


#define MSG_TX_FREQUENCY 5555                 // 메시지 전송 시 사용할 식별자


// 주파수 발생기 관련 전역 변수들
uint32_t OscFreqHz = 23456;                   // 주파수 발생기 초기 주파수 값 (1Hz~40MHz)
bool OscFreqFlag = false;                     // 주파수 발생기 플래그


// 주파수 측정 타스크 함수 정의
void K20_task_freq_counter(void* pvParam);



// -------- freq_counter.cpp 구현부 --------

// ESP32 주파수 카운터 초기화 및 설정
static const char* G_K20_TAG = "freq_meter";

// Pulse Counter 설정 상수
#define PCNT_COUNT_UNIT       PCNT_UNIT_0     // Pulse Counter Unit 0
#define PCNT_COUNT_CHANNEL    PCNT_CHANNEL_0  // Pulse Counter Channel 0

#define PCNT_INPUT_SIG_IO     GPIO_NUM_34     // 주파수 입력 GPIO 핀 34
#define LEDC_HS_CH0_GPIO      GPIO_NUM_33     // LEDC 출력 핀 33 (주파수 발생기 출력)
#define PCNT_INPUT_CTRL_IO    GPIO_NUM_35     // Pulse Counter 제어 핀 35
#define OUTPUT_CONTROL_GPIO   GPIO_NUM_32     // 출력 제어 GPIO 핀 32
#define PCNT_H_LIM_VAL        overflow        // Pulse Counter overflow 값



// Pulse Counter 관련 변수들
uint32_t overflow = 20000;                    // Pulse Counter 최대 값
int16_t pulses = 0;                           // Pulse Counter 값
uint32_t multPulses = 0;                      // Pulse Counter overflow 카운트
uint32_t sample_time = 999955;                // 샘플링 시간 (1초)
uint32_t mDuty = 0;                           // 듀티 사이클 값 (50%)
uint32_t resolution = 0;                      // 주파수 발생기의 해상도
char buf[32];                                 // 문자열 버퍼



// 타이머 설정을 위한 구조체
esp_timer_create_args_t create_args;
esp_timer_handle_t timer_handle;              // 타이머 핸들러

portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED; // 타이머 동기화용 뮤텍스



static void read_PCNT(void *p);                                 // Read Pulse Counter
static char *ultos_recursive(unsigned long val, char *s, unsigned radix, int pos); // Format an unsigned long (32 bits) into a string
static char *ltos(long val, char *s, int radix);                // Format an long (32 bits) into a string
static void init_PCNT(void);                                    // Initialize and run PCNT unit
static void IRAM_ATTR pcnt_intr_handler(void *arg);             // Counting overflow pulses
static void init_osc_freq ();                                   // Initialize Oscillator to test Freq Meter
static void init_frequency_meter ();


// 주파수 발생기를 초기화하는 함수
static void init_osc_freq() {
	resolution = (log(80000000 / OscFreqHz) / log(2)) / 2;  // 주파수에 따른 해상도 계산
	if (resolution < 1) resolution = 1;                     // 최소 해상도 설정
	// Serial.println(resolution);                               // Print
	mDuty = (pow(2, resolution)) / 2;                       // 듀티 사이클 50%로 설정
	// Serial.println(mDuty);                                    // Print

	ledc_timer_config_t ledc_timer = {};                    // LEDC 타이머 설정
	ledc_timer.duty_resolution = ledc_timer_bit_t(resolution);  // 해상도 설정
	ledc_timer.freq_hz = OscFreqHz;                         // 주파수 설정
	ledc_timer.speed_mode = LEDC_HIGH_SPEED_MODE;           // 고속 모드 설정
	ledc_timer.timer_num = LEDC_TIMER_0;                    // 타이머 0 사용
	ledc_timer_config(&ledc_timer);                         // LEDC 타이머 설정 적용

	ledc_channel_config_t ledc_channel = {};                // LEDC 채널 설정
	ledc_channel.channel = LEDC_CHANNEL_0;                  // 채널 0 사용
	ledc_channel.duty = mDuty;                              // 듀티 사이클 설정
	ledc_channel.gpio_num = LEDC_HS_CH0_GPIO;               // GPIO 33에 출력
	ledc_channel.intr_type = LEDC_INTR_DISABLE;             // 인터럽트 비활성화
	ledc_channel.speed_mode = LEDC_HIGH_SPEED_MODE;         // 고속 모드
	ledc_channel.timer_sel = LEDC_TIMER_0;                  // 타이머 0 선택
	ledc_channel_config(&ledc_channel);                     // LEDC 채널 설정 적용
}

// Pulse Counter overflow 처리 함수 (인터럽트 서비스 루틴)
static void IRAM_ATTR pcnt_intr_handler(void *arg) {
	portENTER_CRITICAL_ISR(&timerMux);      // 임계 구역 진입 (인터럽트 비활성화)
	multPulses++;                           // Overflow 발생 시 카운트 증가
	PCNT.int_clr.val = BIT(PCNT_COUNT_UNIT); // Pulse Counter 인터럽트 비트 클리어
	portEXIT_CRITICAL_ISR(&timerMux);       // 임계 구역 종료 (인터럽트 활성화)
}

// Pulse Counter 초기화 함수
static void init_PCNT(void) {
	pcnt_config_t pcnt_config = {};         // Pulse Counter 설정 구조체 초기화
	
	pcnt_config.pulse_gpio_num = PCNT_INPUT_SIG_IO; // Pulse 입력 핀 (GPIO 34)
	pcnt_config.ctrl_gpio_num = PCNT_INPUT_CTRL_IO; // 제어 입력 핀 (GPIO 35)
	pcnt_config.unit = PCNT_COUNT_UNIT;     // Pulse Counter Unit 0
	pcnt_config.channel = PCNT_COUNT_CHANNEL; // Channel 0
	pcnt_config.counter_h_lim = PCNT_H_LIM_VAL; // 카운터 최대 값 (overflow 시)
	pcnt_config.pos_mode = PCNT_COUNT_INC;  // 상승 에지에서 카운터 증가
	pcnt_config.neg_mode = PCNT_COUNT_INC;  // 하강 에지에서도 카운터 증가
	pcnt_config.lctrl_mode = PCNT_MODE_DISABLE; // 낮은 제어 신호에서 카운터 비활성화
	pcnt_config.hctrl_mode = PCNT_MODE_KEEP;    // 높은 제어 신호에서 카운터 유지
	pcnt_unit_config(&pcnt_config);         // Pulse Counter 설정 적용

	pcnt_counter_pause(PCNT_COUNT_UNIT);    // 카운터 일시 정지
	pcnt_counter_clear(PCNT_COUNT_UNIT);    // 카운터 초기화
	pcnt_event_enable(PCNT_COUNT_UNIT, PCNT_EVT_H_LIM); // Overflow 이벤트 활성화
	pcnt_isr_register(pcnt_intr_handler, NULL, 0, NULL); // 인터럽트 핸들러 등록
	pcnt_intr_enable(PCNT_COUNT_UNIT);      // Pulse Counter 인터럽트 활성화
	pcnt_counter_resume(PCNT_COUNT_UNIT);   // 카운터 시작
}

// Pulse Counter 값을 읽는 함수
static void read_PCNT(void *p) {
	gpio_set_level(OUTPUT_CONTROL_GPIO, 0); // 카운터 정지 (출력 제어 LOW)
	pcnt_get_counter_value(PCNT_COUNT_UNIT, &pulses); // Pulse Counter 값 읽기
	xSemaphoreGive(FreqSemaphore);           // 세마포어 신호 전송
}

// 주파수 측정기 초기화 함수
static void init_frequency_meter() {
	init_osc_freq();                          // 주파수 발생기 초기화
	init_PCNT();                              // Pulse Counter 초기화

	gpio_pad_select_gpio(OUTPUT_CONTROL_GPIO); // GPIO 설정
	gpio_set_direction(OUTPUT_CONTROL_GPIO, GPIO_MODE_OUTPUT); // GPIO 32 출력 모드 설정

	create_args.callback = read_PCNT;         // 타이머 콜백 함수 설정
	esp_timer_create(&create_args, &timer_handle); // 타이머 생성

	gpio_matrix_in(PCNT_INPUT_SIG_IO, SIG_IN_FUNC226_IDX, false); // GPIO 매트릭스 설정
}


#if 0
// Format an unsigned long (32 bits) into a string
static char *ultos_recursive(unsigned long val, char *s, unsigned radix, int pos) {
	int c;
	if (val >= radix) {
		s = ultos_recursive(val / radix, s, radix, pos + 1);
		}
	c = val % radix;
	c += (c < 10 ? '0' : 'a' - 10);
	*s++ = c;
	if (pos % 3 == 0) {
		*s++ = ',';
		}
	return s;
	}


// Format an long (32 bits) into a string
static char *ltos(long val, char *s, int radix) {              
	if (radix < 2 || radix > 36) {
		s[0] = 0;
		} 
	else {
		char *p = s;
		if (radix == 10 && val < 0) {
			val = -val;
			*p++ = '-';
			}
		p = ultos_recursive(val, p, radix, 0) - 1;
		*p = 0;
		}
	return s;
	}
#endif

// 주파수 측정을 위한 타스크
void K20_task_freq_counter(void* pvParam) {
    ESP_LOGI(G_K20_TAG, "J300_task_freq_counter running on core %d with priority %d", xPortGetCoreID(), uxTaskPriorityGet(NULL));

	FreqSemaphore = xSemaphoreCreateBinary(); // 주파수 측정 세마포어 생성
	init_frequency_meter();                   // 주파수 측정기 초기화
	
	esp_timer_start_periodic(timer_handle, sample_time); // 주기적으로 타이머 시작

	while (true) {
		if (OscFreqFlag) {
			ledc_stop(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 0); // LEDC 타이머 정지
			init_osc_freq();                      // 새로운 주파수로 초기화
			OscFreqFlag = false;                  // 플래그 리셋
		}
		gpio_set_level(OUTPUT_CONTROL_GPIO, 1);   // 카운터 시작 (출력 제어 HIGH)
		delayMicroseconds(500);                   // 0.5ms 지연

		if (xSemaphoreTake(FreqSemaphore, 2000 / portTICK_PERIOD_MS) == pdTRUE) {
			FrequencyHz = ((multPulses * overflow) + pulses) / (sample_time / 1000000); // 주파수 계산
			multPulses = 0;                       // overflow 카운터 초기화
			FreqCaptureFlag = true;               // 주파수 캡처 완료 플래그 설정
		}
	}
}

