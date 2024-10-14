/*
 * 주요 기능 요약
 *
 * 1. 전류(mA) 및 전압(V) 값을 수신하여 화면에 표시합니다.
 * 2. 전류 및 전압의 배경색을 오프스케일 상태에 따라 변경합니다.
 * 3. WebSocket을 통해 서버와 연결하고 데이터를 주기적으로 요청합니다.
 * 4. 페이지가 닫히기 전 WebSocket 연결을 정리합니다.
 */

let iScale = 0.05; // 전류 배율 초기화
let vScale = 0.00125; // 전압 배율 초기화
let offScale = 0; // 오프스케일 상태 초기화
let i, v, bgColor; // 전류, 전압, 배경색 변수 선언

// 전류와 전압 값을 화면에 업데이트하는 함수
function update_meter() {
    document.getElementById("ma").innerHTML = i.toFixed(3) + " mA"; // 전류 값을 mA로 표시
    if (offScale == 1) { // 오프스케일 상태일 경우
        document.getElementById("ma").style.backgroundColor = 'orange'; // 배경색을 오렌지색으로 변경
    } else {
        document.getElementById("ma").style.backgroundColor = bgColor; // 기본 배경색으로 복원
    }
    document.getElementById("volts").innerHTML = v.toFixed(3) + " V"; // 전압 값을 V로 표시
}

// WebSocket 초기화
let gateway = `ws://${window.location.hostname}/ws`; // WebSocket 게이트웨이 주소
var websocket; // WebSocket 객체 선언

// 페이지 로드 시 호출되는 이벤트 리스너 추가
window.addEventListener('load', on_window_load);

// 페이지가 로드되면 WebSocket을 초기화하는 함수
function on_window_load(event) {
    bgColor = document.getElementById("ma").style.backgroundColor; // 기본 배경색 저장
    init_web_socket(); // WebSocket 초기화 호출
}

// 서버에 데이터를 요청하는 함수
function trigger_capture() {
    let scale = document.getElementById("scale").value; // UI에서 선택한 배율 값
    websocket.send("m" + scale); // 서버에 요청 메시지 전송
}

// 1초마다 trigger_capture를 호출하는 타이머 설정
var periodicTrigger = setInterval(trigger_capture, 1000); 

// 페이지를 떠나기 전에 실행되는 이벤트 핸들러
window.onbeforeunload = function() {
    clearInterval(periodicTrigger); // 주기적 호출 중지
    websocket.onclose = function () {}; // WebSocket이 닫힐 때 onclose 이벤트 비활성화
    websocket.close(); // WebSocket 연결 종료
}

// WebSocket 초기화 및 이벤트 핸들러 설정
function init_web_socket() {
    console.log('Trying to open a WebSocket connection...'); // 연결 시도 메시지 출력
    websocket = new WebSocket(gateway); // WebSocket 객체 생성
    websocket.binaryType = "arraybuffer"; // WebSocket 데이터 타입을 바이너리로 설정
    websocket.onopen = on_ws_open; // 연결이 열렸을 때 실행되는 핸들러 설정
    websocket.onclose = on_ws_close; // 연결이 닫혔을 때 실행되는 핸들러 설정
    websocket.onmessage = on_ws_message; // 메시지를 수신했을 때 실행되는 핸들러 설정
}

// WebSocket 연결이 성공적으로 열렸을 때 호출되는 함수
function on_ws_open(event) {
    console.log('Connection opened'); // 연결 성공 메시지 출력
}

// WebSocket 연결이 닫혔을 때 호출되는 함수
function on_ws_close(event) {
    console.log('Connection closed'); // 연결 종료 메시지 출력
    setTimeout(init_web_socket, 2000); // 2초 후 WebSocket을 다시 시도 (자동 재연결)
}

// WebSocket으로 수신한 메시지를 처리하는 함수
function on_ws_message(event) {
    let view = new Int16Array(event.data); // 수신한 바이너리 데이터를 16비트 정수 배열로 변환
    if ((view.length == 5) && (view[0] == 4444)) { // 메시지가 다섯 개의 16비트 정수로 이루어져 있고 첫 번째 값이 4444일 때
        iScale = (view[1] == 0 ? 0.05 : 0.002381); // iScale 값을 설정
        i = view[2] * iScale; // 수신한 전류 값을 배율로 변환
        v = view[3] * vScale; // 수신한 전압 값을 배율로 변환
        offScale = view[4]; // 오프스케일 상태 업데이트
        update_meter(); // 화면에 전류와 전압 갱신
    } 
    // 패킷 수신을 확인하기 위해 서버에 "x" 메시지를 전송
    websocket.send("x");
}

/*
 * 웹소켓 데이터 형식 예시
 *
 * 1. 전류 및 전압 데이터를 수신하는 경우:
 *    - 데이터 형식: Int16Array (16비트 정수 배열)
 *    - 메시지 구조: [header, iScaleIndex, current, voltage, offScale]
 *        - header: 4444 (정적 값, 메시지 식별용)
 *        - iScaleIndex: 전류 배율 선택 (0 또는 1)
 *        - current: 전류 값 (전송된 데이터의 두 번째 16비트 정수)
 *        - voltage: 전압 값 (전송된 데이터의 세 번째 16비트 정수)
 *        - offScale: 오프스케일 상태 (0 또는 1)
 *
 *    예시:
 *    수신된 데이터: new Int16Array([4444, 0, 2000, 1200, 0])
 *    설명:
 *        - 첫 번째 값인 4444는 메시지의 식별자 역할을 하며,
 *          두 번째 값인 0은 전류 배율로, 0.05의 배율을 사용함.
 *          세 번째 값인 2000은 전류 값으로, iScale을 곱해 100mA로 변환됨.
 *          네 번째 값인 1200은 전압 값으로, vScale을 곱해 1.5V로 변환됨.
 *          마지막 값인 0은 오프스케일 상태로, 정상 범위 내에 있음을 나타냄.
 */
