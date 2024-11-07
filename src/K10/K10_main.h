/*
 * ESP32 전류 및 전압 측정 시스템 (INA226 기반)
 *
 * 이 시스템은 ESP32를 사용하여 INA226 센서를 통해 전류 및 전압을 측정하고,
 * Wi-Fi 및 웹소켓을 통해 실시간 데이터를 클라이언트로 전송하는 기능을 제공합니다.
 *
 * 주요 기능:
 * - Wi-Fi를 통한 웹 서버 및 웹소켓 통신
 * - 실시간 전류 및 전압 측정 (INA226)
 * - 주파수 측정 기능
 * - 다중 코어 작업 처리 (Wi-Fi 작업과 측정 작업을 다른 코어에서 실행)
 * - 상태 플래그를 통해 데이터 전송 및 수신 상태 관리
 */

#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <Wire.h>

#include "K00_config_v2.h"
#include "K20_freq_counter_v2.h"
#include "K30_wifi_cfg_v2.h"
#include "K40_ina226_v2.h"
#include "K50_nv_data_v2.h"

// 펌웨어 버전 및 로그 태그 설정
const char*		   FwRevision = "0.97";
static const char* G_K10_TAG  = "main";

// 전역 변수 선언
volatile int		 TxSamples;			   // 웹소켓으로 전송할 샘플 수
extern volatile bool SocketConnectedFlag;  // 웹소켓 연결 상태 플래그
extern uint32_t		 ClientID;			   // 연결된 웹소켓 클라이언트 ID

// 태스크 우선순위 설정
#define WIFI_TASK_PRIORITY			  1
#define CURRENT_VOLTAGE_TASK_PRIORITY (configMAX_PRIORITIES - 1)
#define FREQUENCY_TASK_PRIORITY		  (configMAX_PRIORITIES - 2)

volatile MEASURE_t Measure;		   // 측정 데이터를 저장하는 구조체
volatile int16_t*  Buffer = NULL;  // 측정 데이터를 저장할 버퍼
int				   MaxSamples;	   // 측정할 수 있는 최대 샘플 수

// 시스템 상태 정의
#define ST_IDLE			  1
#define ST_TX			  2
#define ST_TX_COMPLETE	  3
#define ST_METER_COMPLETE 4
#define ST_FREQ_COMPLETE  5

// 함수 선언
static void wifi_task(void* pvParameter);			  // Wi-Fi 태스크
static void current_voltage_task(void* pvParameter);  // 전류 및 전압 측정 태스크
static void reset_flags();							  // 플래그 초기화 함수

/*
 * setup 함수: 시스템 초기화 및 태스크 생성
 * - 핀 설정, 직렬 통신 설정, 저장된 네트워크 옵션 불러오기, 태스크 생성 등
 * - 다중 코어를 활용하여 Wi-Fi 및 웹소켓 태스크는 코어 0에서, 전류/전압 측정은 코어 1에서 실행
 */
void K10_init() {
	// 핀 설정
	pinMode(pinAlert, INPUT);	// INA226의 알림 핀 (외부 풀업, active low)
	pinMode(pinGate, INPUT);	// 측정 게이트 제어 핀 (외부 풀업, active low)
	pinMode(pinFET1, OUTPUT);	// FET 제어 핀 1 (외부 풀다운)
	pinMode(pinFET05, OUTPUT);	// FET 제어 핀 2 (외부 풀다운)
	pinMode(pinLED, OUTPUT);	// 상태 LED
	digitalWrite(pinLED, LOW);	// LED 초기 상태 (꺼짐)

	// 직렬 포트 초기화
	Serial.begin(115200);
	ESP_LOGI(G_K10_TAG, "ESP32_INA226 v%s compiled on %s at %s\n\n", FwRevision, __DATE__, __TIME__);
	ESP_LOGI(G_K10_TAG, "Max task priority = %d", configMAX_PRIORITIES - 1);
	ESP_LOGI(G_K10_TAG, "arduino loopTask : setup() running on core %d with priority %d", xPortGetCoreID(), uxTaskPriorityGet(NULL));

	// 저장된 옵션을 불러오기 (Wi-Fi SSID, 비밀번호 등)
	nv_options_load(Options);

	// 기본 측정 모드 설정 (전류/전압 측정)
	Measure.mode = MODE_CURRENT_VOLTAGE;

	// Wi-Fi 태스크 생성 (코어 0에서 실행)
	xTaskCreatePinnedToCore(&wifi_task, "wifi_task", 4096, NULL, WIFI_TASK_PRIORITY, NULL, CORE_0);
	// 주파수 측정 태스크 생성 (코어 1에서 실행)
	xTaskCreatePinnedToCore(&J300_task_freq_counter, "freq_task", 4096, NULL, FREQUENCY_TASK_PRIORITY, NULL, CORE_1);
	// 전류/전압 측정 태스크 생성 (코어 1에서 실행)
	xTaskCreatePinnedToCore(&current_voltage_task, "cv_task", 4096, NULL, CURRENT_VOLTAGE_TASK_PRIORITY, NULL, CORE_1);

	// Arduino 기본 loopTask 삭제 (사용하지 않으므로 제거)
	vTaskDelete(NULL);
}

/*
 * loop 함수: Arduino의 기본 loop 함수는 사용되지 않으므로 정의만 해둠
 * - 실제로 이 함수는 호출되지 않음, setup에서 loopTask가 삭제됨
 */
void K10_run() {
}

/*
 * 플래그 초기화 함수
 * - 각종 상태 플래그를 초기화하여 통신 및 측정 상태를 리셋
 */
void reset_flags() {
	DataReadyFlag	  = false;
	GateOpenFlag	  = false;
	EndCaptureFlag	  = false;
	MeterReadyFlag	  = false;
	FreqReadyFlag	  = false;
	LastPacketAckFlag = false;
}

/*
 * Wi-Fi 태스크: 웹 서버 및 웹소켓 처리
 * - 웹 서버를 시작하고, 웹소켓을 통해 클라이언트와 실시간 데이터를 주고받음
 * - 클라이언트로부터의 요청에 따라 전류/전압 또는 주파수 데이터를 측정하고 전송
 */
static void wifi_task(void* pVParameter) {
	ESP_LOGD(G_K10_TAG, "wifi_task running on core %d with priority %d", xPortGetCoreID(), uxTaskPriorityGet(NULL));
	ESP_LOGI(G_K10_TAG, "Starting web server");

	// LittleFS 파일 시스템을 마운트 (실패 시 재부팅)
	if (!LittleFS.begin(false)) {
		ESP_LOGE(G_K10_TAG, "Cannot mount LittleFS, Rebooting");
		delay(1000);
		ESP.restart();
	}

	// Wi-Fi 및 웹소켓 초기화 (웹 서버 및 웹소켓 서버 시작)
	wifi_init();

	int				  state		   = ST_IDLE;  // 상태 초기화 (대기 상태)
	int				  bufferOffset = 0;		   // 데이터 버퍼 오프셋 초기화
	int				  numBytes;				   // 전송할 데이터의 바이트 수
	volatile int16_t* pb;
	int16_t			  msg;
	uint32_t		  t1, t2;  // 전송 시간 측정 변수
	reset_flags();			   // 상태 플래그 초기화

	// 메인 루프: 웹소켓 클라이언트와의 통신 처리
	while (1) {
		vTaskDelay(1);		  // 다른 태스크가 실행될 수 있도록 잠시 대기
		ws.cleanupClients();  // 웹소켓 클라이언트 정리 (연결 종료된 클라이언트 정리)

		if (SocketConnectedFlag == true) {	// 클라이언트가 연결된 상태일 때
			switch (Measure.mode) {			// 현재 측정 모드에 따라 처리
				default:
					break;

				// 전류/전압 측정 모드 처리
				case MODE_CURRENT_VOLTAGE:
					switch (state) {
						default:
							break;

						case ST_IDLE:					   // 대기 상태
							if (MeterReadyFlag == true) {  // 전류/전압 측정 완료 시
								MeterReadyFlag	  = false;
								LastPacketAckFlag = false;
								numBytes		  = 5 * sizeof(int16_t);		  // 전송할 데이터 크기 (5개의 int16_t 데이터)
								ws.binary(ClientID, (uint8_t*)Buffer, numBytes);  // 웹소켓을 통해 클라이언트로 데이터 전송
								state = ST_METER_COMPLETE;						  // 측정 완료 상태로 전환
							} else if (GateOpenFlag) {							  // 게이트가 열렸을 때
								GateOpenFlag = false;
								ESP_LOGD(G_K10_TAG, "Socket msg : Capture Gate Open");
								msg = MSG_GATE_OPEN;					 // 게이트 열림 메시지
								ws.binary(ClientID, (uint8_t*)&msg, 2);	 // 게이트 열림 상태를 클라이언트로 전송
							} else if (DataReadyFlag == true) {			 // 데이터가 준비된 경우
								DataReadyFlag = false;
								ESP_LOGD(G_K10_TAG, "Socket msg : Tx Start");
								numBytes = (3 + TxSamples * 2) * sizeof(int16_t);  // 전송할 데이터 크기 계산
								t1		 = micros();							   // 전송 시작 시간 기록
								ws.binary(ClientID, (uint8_t*)Buffer, numBytes);   // 데이터 전송
								bufferOffset += numBytes / 2;					   // 버퍼 오프셋 업데이트 (샘플 단위)
								if (EndCaptureFlag == true) {					   // 캡처가 종료되면
									EndCaptureFlag = false;
									state		   = ST_TX_COMPLETE;  // 전송 완료 상태로 전환
								} else {
									state = ST_TX;	// 계속 전송 상태 유지
								}
							}
							break;

						case ST_TX:														   // 데이터 전송 중 상태
							if ((DataReadyFlag == true) && (LastPacketAckFlag == true)) {  // 데이터 준비 완료 및 마지막 패킷 ACK 수신
								LastPacketAckFlag = false;
								DataReadyFlag	  = false;
								t2				  = micros();								// 전송 완료 시간 기록
								ESP_LOGD(G_K10_TAG, "Socket msg : %dus, Tx ...", t2 - t1);	// 전송 시간 출력
								t1		 = t2;												// 새로운 전송 시간 갱신
								pb		 = Buffer + bufferOffset;							// 전송할 데이터 버퍼
								numBytes = (1 + TxSamples * 2) * sizeof(int16_t);			// 전송할 바이트 수 계산
								ws.binary(ClientID, (uint8_t*)pb, numBytes);				// 웹소켓으로 데이터 전송
								bufferOffset += numBytes / 2;								// 버퍼 오프셋 갱신
								if (EndCaptureFlag == true) {								// 캡처 종료 플래그 확인
									EndCaptureFlag = false;
									state		   = ST_TX_COMPLETE;  // 전송 완료 상태로 전환
								}
							}
							break;

						case ST_TX_COMPLETE:				  // 전송 완료 상태
							if (LastPacketAckFlag == true) {  // 마지막 패킷에 대한 ACK 수신
								t2 = micros();
								ESP_LOGD(G_K10_TAG, "Socket msg : %dus, Tx ...", t2 - t1);
								ESP_LOGD(G_K10_TAG, "Socket msg : Tx Complete");
								msg = MSG_TX_COMPLETE;					 // 전송 완료 메시지
								ws.binary(ClientID, (uint8_t*)&msg, 2);	 // 클라이언트로 전송 완료 메시지 전송
								reset_flags();							 // 플래그 초기화
								state		 = ST_IDLE;					 // 대기 상태로 전환
								TxSamples	 = 0;						 // 전송할 샘플 수 초기화
								bufferOffset = 0;						 // 버퍼 오프셋 초기화
							}
							break;

						case ST_METER_COMPLETE:				  // 전류/전압 측정 완료 상태
							if (LastPacketAckFlag == true) {  // 마지막 패킷에 대한 ACK 수신
								reset_flags();				  // 플래그 초기화
								state = ST_IDLE;			  // 대기 상태로 전환
							}
							break;
					}
					break;

				// 주파수 측정 모드 처리
				case MODE_FREQUENCY:
					switch (state) {
						default:
							break;

						case ST_IDLE:					  // 대기 상태
							if (FreqReadyFlag == true) {  // 주파수 측정이 완료된 경우
								FreqReadyFlag	  = false;
								LastPacketAckFlag = false;
								int32_t buffer[2];
								buffer[0] = MSG_TX_FREQUENCY;  // 주파수 전송 메시지
								buffer[1] = FrequencyHz;	   // 측정된 주파수 값
								numBytes  = 2 * sizeof(int32_t);
								ws.binary(ClientID, (uint8_t*)buffer, numBytes);  // 클라이언트로 주파수 데이터 전송
								state = ST_FREQ_COMPLETE;						  // 주파수 전송 완료 상태로 전환
							}
							break;

						case ST_FREQ_COMPLETE:				  // 주파수 전송 완료 상태
							if (LastPacketAckFlag == true) {  // 마지막 패킷에 대한 ACK 수신
								reset_flags();				  // 플래그 초기화
								state = ST_IDLE;			  // 대기 상태로 전환
							}
							break;
					}
					break;
			}
		} else {
			// 소켓 연결 해제 시, 상태 및 플래그 초기화
			reset_flags();
			Measure.mode = MODE_INVALID;  // 측정 모드를 무효로 설정
			state		 = ST_IDLE;		  // 대기 상태로 전환
		}
	}
	vTaskDelete(NULL);	// 태스크 종료
}

/*
 * 전류/전압 측정 태스크: INA226 센서를 통해 전류 및 전압을 측정
 * - I2C 통신을 사용하여 INA226 센서에서 데이터를 읽고, 클라이언트에 전송
 * - 설정된 샘플링 주기와 샘플 수에 따라 데이터를 수집
 */
static void current_voltage_task(void* pvParameter) {
	ESP_LOGI(G_K10_TAG, "current_voltage_task running on core %d with priority %d", xPortGetCoreID(), uxTaskPriorityGet(NULL));
	CVCaptureFlag = false;

	// I2C 초기화 (SDA, SCL 핀 설정 및 400kHz로 통신 설정)
	Wire.begin(pinSDA, pinSCL);
	Wire.setClock(400000);

	// INA226 센서 ID 확인
	uint16_t id = ina226_read_reg(REG_ID);
	if (id != 0x5449) {
		ESP_LOGE(G_K10_TAG, "INA226 Manufacturer ID read = 0x%04X, expected 0x5449\n", id);
		ESP_LOGE(G_K10_TAG, "Halting...");
		while (1) {
			vTaskDelay(1);	// 오류 시 무한 대기
		}
	}

	ina226_reset();	 // INA226 센서 리셋

	// 최대 할당 가능한 메모리 크기 계산
	int32_t maxBufferBytes = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
	ESP_LOGI(G_K10_TAG, "Free memory malloc-able for sample Buffer = %d bytes", maxBufferBytes);

	// 측정 데이터 버퍼 할당
	Buffer = (int16_t*)malloc(maxBufferBytes);
	if (Buffer == nullptr) {
		ESP_LOGE(G_K10_TAG, "Could not allocate sample Buffer with %d bytes", maxBufferBytes);
		ESP_LOGE(G_K10_TAG, "Halting...");
		while (1) {
			vTaskDelay(1);	// 메모리 할당 실패 시 무한 대기
		}
	}

	// 버퍼에 저장할 수 있는 최대 샘플 수 계산
	MaxSamples = (maxBufferBytes - 8) / 4;
	ESP_LOGI(G_K10_TAG, "Max Samples = %d", MaxSamples);

	// 측정 루프: CVCaptureFlag가 설정되면 측정 시작
	while (1) {
		if (CVCaptureFlag == true) {
			CVCaptureFlag = false;
			if (Measure.m.cv_meas.nSamples == 0) {	// 게이트 기반 샘플 캡처
				ESP_LOGD(G_K10_TAG, "Capturing gated samples using cfg = 0x%04X, scale %d", Measure.m.cv_meas.cfg, Measure.m.cv_meas.scale);
				ina226_capture_buffer_gated(Measure, Buffer);
			} else if (Measure.m.cv_meas.nSamples == 1) {  // 단일 샘플 캡처 (저속, 고속, 자동 스케일)
				int scalemode = Measure.m.cv_meas.scale;
				if (scalemode == SCALE_LO) {  // 저속 스케일로 측정
					ESP_LOGD(G_K10_TAG, "Capturing meter sample using low scale");
					Measure.m.cv_meas.scale = SCALE_LO;
					bool res				= ina226_capture_averaged_sample(Measure, Buffer, true);
					if (!res)
						ESP_LOGD(G_K10_TAG, "Warning : offscale reading");
				} else if (scalemode == SCALE_HI) {	 // 고속 스케일로 측정
					ESP_LOGD(G_K10_TAG, "Capturing meter sample using hi scale");
					Measure.m.cv_meas.scale = SCALE_HI;
					bool res				= ina226_capture_averaged_sample(Measure, Buffer, true);
					if (!res)
						ESP_LOGD(G_K10_TAG, "Warning : offscale reading");
				} else {  // 자동 스케일로 측정
					ESP_LOGD(G_K10_TAG, "Capturing meter sample autorange LO");
					Measure.m.cv_meas.scale = SCALE_LO;
					bool res				= ina226_capture_averaged_sample(Measure, Buffer, false);
					if (!res) {
						Measure.m.cv_meas.scale = SCALE_HI;
						ESP_LOGD(G_K10_TAG, "Capturing meter sample autorange HI");
						res = ina226_capture_averaged_sample(Measure, Buffer, true);
						if (!res)
							ESP_LOGD(G_K10_TAG, "Warning : offscale reading");
					}
				}
			} else {  // 다중 샘플 캡처
				ESP_LOGD(G_K10_TAG, "Capturing %d samples using cfg = 0x%04X, scale %d", Measure.m.cv_meas.nSamples, Measure.m.cv_meas.cfg, Measure.m.cv_meas.scale);
				ina226_capture_buffer_triggered(Measure, Buffer);
			}
		}
		vTaskDelay(1);	// 잠시 대기 후 다시 실행
	}
	vTaskDelete(NULL);	// 태스크 종료
}
