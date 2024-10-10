/* 
//  주요기능
1. 웹소켓 연결 설정: 서버와의 실시간 통신을 위해 웹소켓을 초기화하고 연결 상태를 관리합니다.
2. 차트 초기화: Chart.js 라이브러리를 사용하여 전류(mA)와 전압(V)을 시각화하는 차트를 생성하고, 초기 데이터를 설정합니다.
3. 데이터 수신 및 처리: 서버로부터 수신한 데이터를 파싱하여 전류와 전압 값을 업데이트하며, 이 데이터를 차트에 반영합니다.
4. 슬라이더 설정: 사용자가 데이터의 시간 범위를 조절할 수 있도록 슬라이더를 초기화하고, 선택된 범위에 따라 차트를 업데이트합니다.
5. 캡처 버튼 기능: 사용자가 데이터 캡처를 시작하거나 종료할 수 있도록 버튼 이벤트를 설정합니다. 캡처 세부 정보를 웹소켓을 통해 서버에 전송합니다.
6. 통계 표시: 선택된 데이터 범위의 평균, 최소, 최대 전류 및 전압 값을 계산하여 사용자에게 표시합니다.

// 주요함수

// on_window_load 함수: 웹 페이지가 로드될 때 호출됨.
// 차트와 슬라이더를 초기화하고 웹소켓 연결을 설정함.

// new_chart() 호출: Chart.js를 사용하여 새로운 차트를 생성함.

// init_sliders() 호출: 슬라이더를 초기화하여 사용자가 데이터 범위를 조절할 수 있도록 함.

// 웹소켓 연결: new WebSocket(gateway)를 통해 웹소켓을 생성하고 서버에 연결함.

// onopen 이벤트: 웹소켓 연결이 성공적으로 이루어지면 콘솔에 메시지를 출력함.

// onmessage 이벤트: 서버에서 메시지를 수신할 때 호출됨.
// 메시지를 JSON 형식으로 파싱하고, 특정 ID가 '0'일 때 데이터가 포함된 경우:
// - 데이터 길이가 1000을 초과하면 가장 오래된 데이터를 제거함.
// - 새로운 전류 및 전압 값을 스케일을 적용하여 배열에 추가함.
// - 현재 시간을 배열에 추가하고, 시간이 업데이트된 후 차트를 업데이트함.

// onclose 이벤트: 웹소켓 연결이 종료되면 콘솔에 메시지를 출력함.

// onerror 이벤트: 웹소켓에서 오류가 발생하면 오류 메시지를 콘솔에 출력함.


*/


// WebSocket 데이터 형식 예시와 설명

/*
1. **시작 패킷 (Start Packet)**: 
   - 시작 패킷은 `view[0] == 1111`일 때를 의미하며, 새로운 데이터를 수신할 준비가 되었음을 알립니다.
   - `view[1]`은 샘플 주기(period)를 나타내며, 단위는 밀리초(ms)입니다.
   - `view[2]`는 전류 스케일을 정의하며, 값이 0이면 0.05로 스케일링하고, 값이 1이면 0.002381로 스케일링합니다.
   - 나머지 데이터는 전류(mA)와 전압(V) 값이 순차적으로 배열되어 있습니다.
   예시:
   [1111, 500, 0, 150, 240, 148, 239, 147, 238]
   - 1111: 시작 패킷
   - 500: 샘플 주기 500ms
   - 0: 전류 스케일 0.05
   - 전류 및 전압 데이터: 
     - 150 * 0.05 = 7.5mA
     - 240 * 0.00125 = 0.3V
     - 148 * 0.05 = 7.4mA
     - 239 * 0.00125 = 0.29875V
     - 147 * 0.05 = 7.35mA
     - 238 * 0.00125 = 0.2975V
*/

/*
2. **데이터 패킷 (Data Packet)**:
   - `view[0] == 2222`일 때, 이는 연속된 데이터 패킷이며, 현재까지의 데이터를 지속해서 전송합니다.
   - 이 패킷도 전류(mA)와 전압(V) 값이 교대로 배열됩니다.
   예시:
   [2222, 145, 237, 143, 236, 140, 235]
   - 2222: 데이터 패킷
   - 전류 및 전압 데이터:
     - 145 * 0.05 = 7.25mA
     - 237 * 0.00125 = 0.29625V
     - 143 * 0.05 = 7.15mA
     - 236 * 0.00125 = 0.295V
     - 140 * 0.05 = 7.0mA
     - 235 * 0.00125 = 0.29375V
*/

/*
3. **종료 패킷 (End Packet)**:
   - `view[0] == 3333`일 때, 데이터 전송이 완료되었음을 나타냅니다.
   예시:
   [3333]
   - 3333: 데이터 전송 종료
*/


// 차트 초기화
let c = document.getElementById("myChart"); // HTML에서 차트 캔버스를 찾기
let Ctxt = document.getElementById("myChart").getContext("2d"); // 2D 컨텍스트 가져오기

let timeMs = 0.0; // 시간 변수 (밀리초 단위)
let periodMs = 0.5; // 샘플링 주기 (밀리초 단위)
let iScale = 0.05; // 전류 스케일 (mA 변환을 위한 비율)
let vScale = 0.00125; // 전압 스케일 (V 변환을 위한 비율)
let Time = []; // 시간 데이터를 저장할 배열
let Data_mA = []; // 전류 데이터를 저장할 배열
let Data_V = []; // 전압 데이터를 저장할 배열

// 초기 데이터 생성
for (let inx = 0; inx < 1000; inx++) {
    Time.push(periodMs * inx); // 시간 배열에 시간 값 추가
    Data_mA.push(0); // 전류 배열에 초기값 추가
    Data_V.push(0); // 전압 배열에 초기값 추가
}

var ChartInst; // 차트 인스턴스를 저장할 변수

// 차트 생성 함수
function new_chart() {
    // Chart.js를 사용하여 새로운 차트 인스턴스 생성
    ChartInst = new Chart(Ctxt, {
        type: "line", // 차트 유형 (선형 차트)
        data: {
            labels: Time, // X축 레이블 (시간 데이터)
            datasets: [
                {
                    label: 'mA', // 첫 번째 데이터셋 레이블
                    yAxisID: 'mA', // Y축 ID
                    backgroundColor: "rgb(209, 20, 61)", // 배경색
                    borderColor: "rgb(209, 20, 61)", // 선 색상
                    data: Data_mA, // Y축 데이터 (전류)
                    cubicInterpolationMode: 'monotone', // 곡선 보간 모드 설정
                },
                {
                    label: 'V', // 두 번째 데이터셋 레이블
                    yAxisID: 'V', // Y축 ID
                    backgroundColor: "rgb(34, 73, 228)", // 배경색
                    borderColor: "rgb(34, 73, 228)", // 선 색상
                    data: Data_V, // Y축 데이터 (전압)
                    cubicInterpolationMode: 'monotone', // 곡선 보간 모드 설정
                }
            ],
        },
        options: {
            animation: {
                duration: 0 // 애니메이션 지속 시간 설정 (0ms로 설정하여 즉시 업데이트)
            },
            responsive: true, // 반응형 차트 설정
            maintainAspectRatio: false, // 비율 유지하지 않도록 설정
            borderWidth: 1, // 차트 테두리 두께 설정
            pointRadius: 0, // 데이터 포인트의 반경 설정 (0으로 설정하여 포인트 숨김)
            scales: {
                mA: {
                    type: 'linear', // 선형 스케일
                    position: 'left', // 왼쪽 Y축
                    ticks: {
                        color: "rgb(209, 20, 61)" // Y축 눈금 색상 설정
                    }
                },
                V: {
                    type: 'linear', // 선형 스케일
                    position: 'right', // 오른쪽 Y축
                    ticks: {
                        color: "rgb(34, 73, 228)" // Y축 눈금 색상 설정
                    }
                }
            }
        },
    });
}

// 슬라이더 초기화 함수
function init_sliders() {
    let sliderSections = document.getElementsByClassName("range-slider"); // 슬라이더 섹션 찾기
    for (let i = 0; i < sliderSections.length; i++) {
        let sliders = sliderSections[i].getElementsByTagName("input"); // 각 슬라이더 섹션 내의 input 요소 찾기
        for (let j = 0; j < sliders.length; j++) {
            if (sliders[j].type === "range") { // 타입이 range인 경우
                sliders[j].oninput = update_chart; // 슬라이더 변경 시 차트 업데이트 함수 호출
                sliders[j].value = (j == 0 ? 0.0 : Time.length * periodMs); // 슬라이더 초기값 설정
                sliders[j].min = 0.0; // 슬라이더 최소값 설정
                sliders[j].max = parseFloat(Time.length) * periodMs; // 슬라이더 최대값 설정
                // 처음 슬라이더 값을 수동으로 변경하여 값 표시
                sliders[j].oninput();
            }
        }
    }
}

// 차트 업데이트 함수
function update_chart() {
    // 슬라이더 값 가져오기
    let slides = document.getElementsByTagName("input"); // 모든 input 요소 찾기
    let min = parseFloat(slides[0].value); // 첫 번째 슬라이더 값 (최소값)
    let max = parseFloat(slides[1].value); // 두 번째 슬라이더 값 (최대값)

    // 슬라이더 값이 올바르게 설정되었는지 확인
    if (min > max) {
        let tmp = max;
        max = min;
        min = tmp; // 최소값이 최대값보다 크면 교환
    }

    let time_slice = []; // 시간 슬라이스
    let data_mA_slice = []; // 전류 슬라이스
    let data_V_slice = []; // 전압 슬라이스

    let min_index = min / periodMs; // 최소 인덱스 계산
    let max_index = max / periodMs; // 최대 인덱스 계산

    // 슬라이스 데이터 추출
    time_slice = JSON.parse(JSON.stringify(Time)).slice(min_index, max_index);
    data_mA_slice = JSON.parse(JSON.stringify(Data_mA)).slice(min_index, max_index);
    data_V_slice = JSON.parse(JSON.stringify(Data_V)).slice(min_index, max_index);

    // 차트 데이터 업데이트
    ChartInst.data.labels = time_slice;
    ChartInst.data.datasets[0].data = data_mA_slice;
    ChartInst.data.datasets[1].data = data_V_slice;
    ChartInst.update(0); // 애니메이션 없이 업데이트

    // 통계 계산
    let iAvg = 0.0; // 전류 평균
    let vAvg = 0.0; // 전압 평균
    let iMax = -9999999.0; // 전류 최대
    let iMin = 9999999.0; // 전류 최소
    let vMax = -9999999.0; // 전압 최대
    let vMin = 9999999.0; // 전압 최소

    // 슬라이스 데이터에서 통계 계산
    for (let t = 0; t < time_slice.length; t++) {
        let i = parseFloat(data_mA_slice[t]);
        let v = parseFloat(data_V_slice[t]);
        iAvg += i; // 전류 합계
        vAvg += v; // 전압 합계
        if (i > iMax) iMax = i; // 전류 최대값 업데이트
        if (v > vMax) vMax = v; // 전압 최대값 업데이트
        if (i < iMin) iMin = i; // 전류 최소값 업데이트
        if (v < vMin) vMin = v; // 전압 최소값 업데이트
    }
    
    // 평균 계산
    iAvg = iAvg / time_slice.length;
    vAvg = vAvg / time_slice.length;

    // 결과 표시
    let displayElement = document.getElementsByClassName("rangeValues")[0]; // 범위 값 표시 요소 찾기
    displayElement.innerHTML = "[" + min + "," + max + "]mS"; // 범위 값 업데이트
    document.getElementById("istats").innerHTML =
        "avg : " + iAvg.toFixed(3) + "mA<br>" +
        "min : " + iMin.toFixed(3) + "mA<br>" +
        "max : " + iMax.toFixed(3) + "mA"; // 전류 통계 표시
    document.getElementById("vstats").innerHTML =
        "avg : " + vAvg.toFixed(3) + "V<br>" +
        "min : " + vMin.toFixed(3) + "V<br>" +
        "max : " + vMax.toFixed(3) + "V"; // 전압 통계 표시
}

// 웹소켓 초기화
let gateway = `ws://${window.location.hostname}/ws`; // 웹소켓 서버 주소
var websocket; // 웹소켓 인스턴스

window.addEventListener('load', on_window_load); // 윈도우 로드 이벤트 리스너 추가

function on_window_load(event) {
    new_chart(); // 차트 생성 함수 호출
    init_sliders(); // 슬라이더 초기화 함수 호출

    websocket = new WebSocket(gateway); // 웹소켓 인스턴스 생성
    websocket.onopen = function(event) {
        console.log("WebSocket 연결됨."); // 웹소켓 연결 성공 로그
    };

    websocket.onmessage = function(event) {
        let msg = JSON.parse(event.data); // 수신한 메시지를 JSON으로 파싱
        // 수신한 데이터 처리
        if (msg.id === '0') {
            // 데이터 형식: {"id": "0", "value": {"mA": 전류값, "V": 전압값}}
            // 전류 및 전압 값 업데이트
            if (Data_mA.length >= 1000) { // 데이터 길이가 1000 이상이면
                Data_mA.shift(); // 가장 오래된 데이터 제거
                Data_V.shift(); // 가장 오래된 데이터 제거
                Time.shift(); // 가장 오래된 시간 제거
            }
            Data_mA.push(msg.value.mA * iScale); // 새로운 전류 값 추가 (스케일 적용)
            Data_V.push(msg.value.V * vScale); // 새로운 전압 값 추가 (스케일 적용)
            Time.push(timeMs); // 현재 시간을 추가
            timeMs += periodMs; // 시간 업데이트
            update_chart(); // 차트 업데이트 함수 호출
        }
    };

    websocket.onclose = function(event) {
        console.log("WebSocket 연결 종료됨."); // 웹소켓 연결 종료 로그
    };

    websocket.onerror = function(event) {
        console.error("WebSocket 오류:", event); // 웹소켓 오류 로그
    };
}


