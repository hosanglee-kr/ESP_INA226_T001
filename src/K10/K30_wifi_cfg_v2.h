#pragma once

#include <WiFi.h>
#include <ESPmDNS.h>

#include "K00_config_v2.h"
#include "K50_nv_data_v2.h"

static const char*	G_K30_TAG = "K30_wifi_cfg";	// ESP32 로그 태그


// WiFi 설정: 정적 IP 주소 설정
static IPAddress 	g_K30_WifiStation_Local_IP(192, 168, 43, 236);  // ESP32에 할당될 고정 IP
static IPAddress 	g_K30_WifiStation_Gateway(192, 168, 43, 1);	   // 게이트웨이 주소
static IPAddress 	g_K30_WifiStation_Subnet(255, 255, 255, 0);	   // 서브넷 마스크
static IPAddress 	g_K30_WifiStation_PrimaryDNS(8, 8, 8, 8);	   // 기본 DNS 서버
static IPAddress 	g_K30_WifiStation_SecondaryDNS(8, 8, 4, 4);	   // 보조 DNS 서버

// AP 모드에서 사용될 SSID
const char*			g_K30_wifi_AP_SSID = "ESP32_METER";  // 비밀번호 없는 Access Point 모드의 SSID

// 함수 선언
void		  		K30_wifi_init();
static void	  		K30_wifi_start_as_ap();
static void	  		K30_wifi_start_as_station();
static void	  		K30_wifi_start_as_station_static_IP();

/*
void              K10_AsyncWebSrv_init();
void		  socket_event_handler(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void		  socket_handle_message(void *arg, uint8_t *data, size_t len);

static String     string_processor(const String &var);
static void	  not_found_handler(AsyncWebServerRequest *request);
static void	  index_page_handler(AsyncWebServerRequest *request);
static void	  set_defaults_handler(AsyncWebServerRequest *request);
static void	  get_handler(AsyncWebServerRequest *request);
static void	  restart_handler(AsyncWebServerRequest *request);
// static void	  capture_handler(AsyncWebServerRequest *request);
*/

/*
 * AP 모드에서 Wi-Fi 액세스 포인트를 시작하는 함수.
 * 지정된 SSID로 ESP32가 자체 액세스 포인트를 생성하며, 비밀번호는 설정되지 않습니다.
 * 사용자가 네트워크 설정을 입력하지 않았을 때 기본적으로 호출됩니다.
 */
static void K30_wifi_start_as_ap() {
	ESP_LOGI(G_K30_TAG, "Starting Access Point with SSID=%s, no password\n", g_K30_wifi_AP_SSID);	 // AP 시작 로그 출력
	WiFi.softAP(g_K30_wifi_AP_SSID);																 // AP 모드 시작
	IPAddress ipaddr = WiFi.softAPIP();													 // 할당된 AP IP 주소 가져오기
	ESP_LOGI(G_K30_TAG, "Web Server IP address : %s", ipaddr.toString().c_str());		 // IP 주소 로그 출력
	digitalWrite(g_K00_PIN_LED, HIGH);															 // LED를 켜서 AP 모드 활성화 표시
}

/*
 * 정적 IP 설정을 사용하여 Wi-Fi 스테이션 모드를 시작하는 함수.
 * ESP32가 클라이언트로서 지정된 SSID에 접속을 시도하며, 고정 IP를 사용하여 네트워크에 연결됩니다.
 * 이 함수는 저장된 네트워크 정보로 자동 연결을 시도하며, 연결이 실패하면 AP 모드로 전환됩니다.
 */
static void K30_wifi_start_as_station_static_IP() {
	ESP_LOGI(G_K30_TAG, "Connecting as station static IP to Access Point with SSID=%s\n", Options.ssid);  // 연결 시도 로그 출력
	uint32_t startTick = millis();																		  // 연결 시작 시간 기록

	// 정적 IP 설정을 사용하여 네트워크 설정
	if (!WiFi.config(g_K30_WifiStation_Local_IP, g_K30_WifiStation_Gateway, g_K30_WifiStation_Subnet, g_K30_WifiStation_PrimaryDNS, g_K30_WifiStation_SecondaryDNS)) {
		ESP_LOGI(G_K30_TAG, "Station static IP config failure");  // IP 설정 실패 시 로그 출력
	}

	// 저장된 SSID와 비밀번호로 네트워크 연결 시도
	WiFi.begin(Options.ssid.c_str(), Options.password.c_str());

	// 연결 대기 (4초)
	if (WiFi.waitForConnectResult(4000UL) != WL_CONNECTED) {
		ESP_LOGI(G_K30_TAG, "Connection failed!");	// 연결 실패 시 로그 출력
		K30_wifi_start_as_ap();							// 연결 실패 시 AP 모드로 전환
	} else {
		IPAddress ipaddr  = WiFi.localIP();																									  // 연결된 IP 주소 가져오기
		uint32_t  endTick = millis();																										  // 연결 종료 시간 기록
		ESP_LOGI(G_K30_TAG, "Connected in %.2f seconds with IP addr %s", (float)(endTick - startTick) / 1000.0f, ipaddr.toString().c_str());  // 연결 시간 출력
		digitalWrite(g_K00_PIN_LED, LOW);																											  // 연결 성공 시 LED 끔
	}
}

/*
 * 동적 IP 설정을 사용하여 Wi-Fi 스테이션 모드를 시작하는 함수.
 * ESP32가 클라이언트로서 네트워크에 접속하며, DHCP를 통해 IP를 할당받습니다.
 * 네트워크 연결에 성공하면 LED를 끄고, 실패하면 AP 모드로 전환합니다.
 */
static void K30_wifi_start_as_station() {
	ESP_LOGI(G_K30_TAG, "Connecting as station to Access Point with SSID=%s", Options.ssid);  // 연결 시도 로그 출력
	uint32_t startTick = millis();															  // 연결 시작 시간 기록

	WiFi.mode(WIFI_STA);										 // 스테이션 모드로 설정
	WiFi.begin(Options.ssid.c_str(), Options.password.c_str());	 // 저장된 SSID와 비밀번호로 연결 시도

	// 연결 대기 (10초)
	if (WiFi.waitForConnectResult(10000UL) != WL_CONNECTED) {
		ESP_LOGI(G_K30_TAG, "Connection failed!");	// 연결 실패 시 로그 출력
		K30_wifi_start_as_ap();							// 실패하면 AP 모드로 전환
	} else {
		uint32_t  endTick = millis();																										  // 연결 종료 시간 기록
		IPAddress ipaddr  = WiFi.localIP();																									  // 연결된 IP 주소 가져오기
		ESP_LOGI(G_K30_TAG, "Connected in %.2f seconds with IP addr %s", (float)(endTick - startTick) / 1000.0f, ipaddr.toString().c_str());  // 연결 시간 로그 출력
		digitalWrite(g_K00_PIN_LED, LOW);																											  // 연결 성공 시 LED 끔
	}
}

/*
 * Wi-Fi 초기화 함수.
 * 설정에 따라 Wi-Fi 스테이션 또는 AP 모드를 선택하여 시작합니다.
 * 또한 mDNS 서비스를 시작하여 "http://meter.local" 도메인으로 접속할 수 있도록 설정합니다.
 * 웹서버와 웹소켓 서버도 이 함수에서 초기화됩니다.
 */

void K10_wifi_init() {
	delay(100);	 // 초기화 딜레이

	// 저장된 SSID가 없으면 AP 모드로 시작
	if (Options.ssid == "") {
		K30_wifi_start_as_ap();
	} else {
		K30_wifi_start_as_station_static_IP();	// 있으면 고정 IP로 연결 시도
	}

	// mDNS 서비스 시작 (http://meter.local)
	if (!MDNS.begin("meter")) {
		ESP_LOGI(G_K30_TAG, "Error starting mDNS service");	 // mDNS 시작 실패 로그
	}	
}

/*
void K10_AsyncWebSrv_init(){
	pServer = new AsyncWebServer(80);  // HTTP 서버 생성 (포트 80)
	if (pServer == nullptr) {
		ESP_LOGE(G_K30_TAG, "Error creating AsyncWebServer!");	// 서버 생성 실패 시 로그 출력
		ESP.restart();											// 재시작
	}

	// 웹소켓 핸들러 설정
	ws.onEvent(socket_event_handler);  // 웹소켓 이벤트 처리 함수 등록
	pServer->addHandler(&ws);		   // 웹소켓을 HTTP 서버에 추가

	// 웹서버 핸들러 설정
	pServer->onNotFound(not_found_handler);					   // 잘못된 경로로의 요청을 처리하는 핸들러 (404 응답)
	pServer->on("/", HTTP_GET, index_page_handler);			   // 루트 경로("/")로의 GET 요청을 처리하는 핸들러 (홈페이지)
	pServer->on("/defaults", HTTP_GET, set_defaults_handler);  // 설정 재설정 요청을 처리하는 핸들러
	pServer->on("/get", HTTP_GET, get_handler);				   // GET 요청을 처리하여 클라이언트에서 SSID 및 비밀번호 변경 가능
	pServer->on("/restart", HTTP_GET, restart_handler);		   // ESP32 재시작 요청을 처리하는 핸들러

	//pServer->onNotFound(not_found_handler);
        //pServer->on("/", HTTP_GET, index_page_handler);
        pServer->on("/cv_chart", HTTP_GET, cv_chart_handler);
        pServer->on("/cv_meter", HTTP_GET, cv_meter_handler);
        pServer->on("/freq_counter", HTTP_GET, freq_counter_handler);
        // pServer->on("/defaults", HTTP_GET, set_defaults_handler);
        // pServer->on("/get", HTTP_GET, get_handler);
        // pServer->on("/restart", HTTP_GET, restart_handler);
	
	// LittleFS 파일 시스템에서 정적 파일 제공 (예: HTML, CSS, JS 파일)
	pServer->serveStatic("/", LittleFS, "/");

	pServer->begin();					 // 웹 서버 시작
	MDNS.addService("http", "tcp", 80);  // mDNS 서비스에 HTTP 추가
}
*/
/*
 * 웹소켓 이벤트 핸들러 함수.
 * 클라이언트가 웹소켓에 연결, 메시지를 보냄, 연결을 끊음 등의 이벤트가 발생할 때 호출됩니다.
 * AWS(WebSocket) 이벤트 타입에 따라 다르게 처리합니다.
 */
/*
void socket_event_handler(AsyncWebSocket	   *server,
						  AsyncWebSocketClient *client,
						  AwsEventType			type,
						  void				   *arg,
						  uint8_t			   *data,
						  size_t				len) {
	switch (type) {
		case WS_EVT_CONNECT:  // 클라이언트가 웹소켓에 연결되었을 때
			ESP_LOGI(G_K30_TAG, "WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
			ClientID			= client->id();	 // 클라이언트 ID 저장
			SocketConnectedFlag = true;			 // 소켓 연결 플래그 설정
			break;

		case WS_EVT_DISCONNECT:	 // 클라이언트가 웹소켓 연결을 끊었을 때
			ESP_LOGI(G_K30_TAG, "WebSocket client #%u disconnected\n", client->id());
			SocketConnectedFlag = false;  // 소켓 연결 플래그 해제
			ClientID			= 0;	  // 클라이언트 ID 초기화
			break;

		case WS_EVT_DATA:							// 클라이언트가 데이터를 보냈을 때
			socket_handle_message(arg, data, len);	// 데이터 처리 함수 호출
			break;

		case WS_EVT_PONG:	// PONG 메시지 (웹소켓에서 핑에 대한 응답) 발생 시
		case WS_EVT_ERROR:	// 웹소켓 오류 발생 시
			break;
	}
}
*/
/*
 * 클라이언트로부터 수신된 메시지를 처리하는 함수.
 * 웹소켓을 통해 수신된 데이터를 분석하여 명령어를 처리합니다.
 * 메시지가 텍스트 형식(WS_TEXT)으로 전송되었는지 확인 후 처리합니다.
 */
/*
void socket_handle_message(void *arg, uint8_t *data, size_t len) {
	AwsFrameInfo *info = (AwsFrameInfo *)arg;

	// 메시지가 완성되었고, 텍스트 형식이며, 길이가 맞는지 확인
	if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
		// 수신된 메시지에 따른 동작 수행
		if (data[0] == 'x') {
			// 'x' 명령어: 마지막 패킷 ACK 플래그 설정
			LastPacketAckFlag = true;
		} else if (data[0] == 'm') {
			// 'm' 명령어: 전류 및 전압 측정 모드 설정
			g_K10_Measure.mode			   = G_K00_MEASURE_MODE_CURRENT_VOLTAGE;
			g_K10_Measure.m.cv_meas.nSamples = 1;						// 샘플 수 설정
			g_K10_Measure.m.cv_meas.cfg	   = Config[1].reg;			// 측정 설정
			g_K10_Measure.m.cv_meas.periodUs = Config[1].periodUs;	// 측정 주기 설정
			g_K10_Measure.m.cv_meas.scale	   = (int)(data[1] - '0');	// 스케일 설정
			CVCaptureFlag			   = true;					// 전류/전압 캡처 플래그 설정
		} else if (data[0] == 'f') {
			// 'f' 명령어: 주파수 측정 모드 설정
			g_K10_Measure.mode	= G_K00_MEASURE_MODE_FREQUENCY;
			FreqCaptureFlag = true;	 // 주파수 캡처 플래그 설정
		} else {
			// JSON 형식의 메시지 처리 (고급 명령어)
			JsonDocument json;
			// const uint8_t size = JSON_OBJECT_SIZE(4);  // JSON 객체 크기 설정
			// StaticJsonDocument<size> json;  // JSON 문서 생성
			DeserializationError err = deserializeJson(json, data);	 // JSON 데이터 역직렬화
			if (err) {
				ESP_LOGI(G_K30_TAG, "deserializeJson() failed with code %s", err.c_str());	// 오류 시 로그 출력
				return;
			}

			const char *szAction = json["action"];	// 액션 필드 추출

			// 'cv_capture' 명령어: 전류/전압 캡처 설정
			if (strcmp(szAction, "cv_capture") == 0) {
				const char *szCfgIndex		 = json["cfgIndex"];
				const char *szCaptureSeconds = json["captureSecs"];
				const char *szScale			 = json["scale"];

				int cfgIndex	   = strtol(szCfgIndex, NULL, 10);		   // 설정 인덱스 변환
				int captureSeconds = strtol(szCaptureSeconds, NULL, 10);   // 캡처 시간 변환
				int sampleRate	   = 1000000 / Config[cfgIndex].periodUs;  // 샘플링 속도 계산
				int numSamples	   = captureSeconds * sampleRate;		   // 총 샘플 수 계산
				int scale		   = strtol(szScale, NULL, 10);			   // 스케일 변환

				// 측정 모드 및 설정 적용
				g_K10_Measure.mode			   = G_K00_MEASURE_MODE_CURRENT_VOLTAGE;
				g_K10_Measure.m.cv_meas.cfg	   = Config[cfgIndex].reg;
				g_K10_Measure.m.cv_meas.scale	   = scale;
				g_K10_Measure.m.cv_meas.nSamples = numSamples;
				g_K10_Measure.m.cv_meas.periodUs = Config[cfgIndex].periodUs;

				// 로그 출력
				ESP_LOGI(G_K30_TAG, "Mode = %d", g_K10_Measure.mode);
				ESP_LOGI(G_K30_TAG, "cfgIndex = %d", cfgIndex);
				ESP_LOGI(G_K30_TAG, "scale = %d", scale);
				ESP_LOGI(G_K30_TAG, "nSamples = %d", numSamples);
				ESP_LOGI(G_K30_TAG, "periodUs = %d", Config[cfgIndex].periodUs);

				CVCaptureFlag = true;  // 캡처 플래그 설정
			}
			// 'oscfreq' 명령어: 주파수 측정 설정
			else if (strcmp(szAction, "oscfreq") == 0) {
				g_K10_Measure.mode			= G_K00_MEASURE_MODE_FREQUENCY;
				const char *szOscFreqHz = json["freqhz"];

				ESP_LOGI(G_K30_TAG, "json[\"action\"]= %s\n", szAction);	 // 액션 로그 출력
				ESP_LOGI(G_K30_TAG, "json[\"freqhz\"]= %s\n", szOscFreqHz);	 // 주파수 로그 출력

				OscFreqHz	= (uint32_t)strtol(szOscFreqHz, NULL, 10);	// 주파수 값 변환
				OscFreqFlag = true;										// 주파수 측정 플래그 설정
			}
		}
	}
}
*/
