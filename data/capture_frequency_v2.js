let freqHz; // 서버에서 수신한 주파수를 저장하는 변수

// 주파수 값을 HTML에 표시하는 함수
function update_frequency() {
    document.getElementById("hz").innerHTML = freqHz + " Hz"; // freqHz 값을 "Hz" 단위로 표시
}

// WebSocket 연결을 위한 게이트웨이 URL 설정
let gateway = `ws://${window.location.hostname}/ws`; // 현재 호스트명에 기반한 WebSocket 주소
var websocket; // WebSocket 객체 선언

// 페이지가 로드되었을 때 호출되는 이벤트 리스너 추가
window.addEventListener('load', on_window_load);

function on_window_load(event) {
    init_web_socket(); // 페이지가 로드되면 WebSocket을 초기화
}

// 주기적으로 서버에 데이터를 요청하는 함수
function trigger_capture() {
    websocket.send("f"); // 서버에 "f" 메시지를 전송하여 데이터를 요청
}

// 1초마다 주기적으로 trigger_capture를 호출하는 타이머 설정
var periodicTrigger = setInterval(trigger_capture, 1000); 

// 페이지를 떠나기 전에 실행되는 이벤트 핸들러
window.onbeforeunload = function() {
    clearInterval(periodicTrigger); // 주기적 호출을 중지
    websocket.onclose = function () {}; // WebSocket이 닫힐 때 발생하는 onclose 이벤트 비활성화
    websocket.close(); // WebSocket 연결을 종료
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
    let view = new Int32Array(event.data); // 수신한 바이너리 데이터를 32비트 정수 배열로 변환
    if ((view.length == 2) && (view[0] == 5555)) { // 메시지가 두 개의 32비트 정수로 이루어져 있고 첫 번째 값이 5555일 때
        freqHz = view[1]; // 두 번째 값이 주파수 데이터이므로 freqHz에 저장
        update_frequency(); // 화면에 주파수 갱신
    }
    // 서버에 "x" 메시지를 전송하여 패킷 수신을 확인
    websocket.send("x");
}

// 사용자 인터페이스에서 주파수를 변경할 때 호출되는 함수
function on_osc_freq_change(selectObject) {
    let value = selectObject.value; // 사용자가 선택한 주파수 값
    let jsonObj = {}; // 서버로 보낼 JSON 객체 생성
    jsonObj["action"] = "oscfreq"; // 주파수 변경 액션을 명시
    jsonObj["freqhz"] = value; // 변경할 주파수 값 설정
    websocket.send(JSON.stringify(jsonObj)); // JSON 형식으로 서버에 전송
}
