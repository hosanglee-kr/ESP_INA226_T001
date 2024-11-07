#ifndef CONFIG_H_
#define CONFIG_H_

// 코어 정의 (ESP32와 같은 듀얼 코어 CPU에서 특정 코어에서 작업을 실행할 때 사용)
#define CORE_0				 0	// 첫 번째 코어 (Core 0)
#define CORE_1				 1	// 두 번째 코어 (Core 1)

// 핀 설정
#define pinFET1				 18	 // 1.0Ω 샤운트 저항을 제어하는 FET (스위치 역할)
#define pinFET05			 19	 // 0.05Ω 샤운트 저항을 제어하는 FET (스위치 역할)
#define pinSDA				 22	 // I2C SDA 핀 (INA226 전류 센서와 통신)
#define pinSCL				 21	 // I2C SCL 핀 (INA226 전류 센서와 통신)
#define pinGate				 4	 // 외부 전류 모니터의 게이트 신호를 수신하는 핀
#define pinAlert			 5	 // INA226의 알림 핀 (전류/전압 초과 등 이벤트 발생 시)
#define pinLED				 14	 // 상태 LED를 제어하는 핀

// 측정 모드 정의
#define MODE_CURRENT_VOLTAGE 11	 // 전류 및 전압 측정 모드
#define MODE_FREQUENCY		 22	 // 주파수 측정 모드
#define MODE_INVALID		 33	 // 유효하지 않은 모드 (에러 처리용)

// 전류 및 전압 측정을 위한 구조체 정의
typedef struct {
	// 입력 (설정 파라미터)
	uint16_t cfg;		// 측정 구성을 설정하는 변수 (설정 플래그 등)
	int		 scale;		// 측정 스케일 (샤운트 저항 값에 따른 스케일)
	int		 nSamples;	// 측정할 샘플의 개수
	uint32_t periodUs;	// 샘플링 주기 (마이크로초 단위)

	// 출력 (측정 결과)
	float sampleRate;  // 샘플링 속도 (Hz 단위)
	float vavg;		   // 평균 전압 (V 단위)
	float vmax;		   // 최대 전압 (V 단위)
	float vmin;		   // 최소 전압 (V 단위)
	float iavgma;	   // 평균 전류 (mA 단위)
	float imaxma;	   // 최대 전류 (mA 단위)
	float iminma;	   // 최소 전류 (mA 단위)
} CV_MEASURE_t;

// 주파수 측정을 위한 구조체 정의
typedef struct {
	int frequencyHz;  // 측정된 주파수 (Hz 단위)
} FREQ_MEASURE_t;

// 측정 모드를 구분하여 전류/전압 측정 및 주파수 측정을 수행하는 구조체 정의
typedef struct {
	int mode;  // 현재 측정 모드 (MODE_CURRENT_VOLTAGE, MODE_FREQUENCY 등)

	// 모드에 따라 다른 측정 구조체를 사용 (공용체)
	union {
		CV_MEASURE_t   cv_meas;	 // 전류 및 전압 측정 데이터를 저장
		FREQ_MEASURE_t f_meas;	 // 주파수 측정 데이터를 저장
	} m;
} MEASURE_t;

// 전역 변수 선언
extern volatile MEASURE_t Measure;	// 측정 결과와 설정을 담은 전역 구조체
extern volatile int16_t*  Buffer;	// 측정 데이터 버퍼 (정수형 배열)

#endif
