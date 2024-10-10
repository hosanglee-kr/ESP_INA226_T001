// 차트 초기화 변수 설정
let c = document.getElementById("myChart"); // HTML에서 'myChart' ID를 가진 canvas 요소 가져오기
let Ctxt = document.getElementById("myChart").getContext("2d"); // 차트의 2D 컨텍스트 가져오기

let timeMs = 0.0; // 시간(ms) 초기값 설정
let periodMs = 0.5; // 데이터 사이의 시간 간격(ms)
let iScale = 0.05; // 전류 데이터의 스케일
let vScale = 0.00125; // 전압 데이터의 스케일
let Time = []; // 시간 데이터를 저장할 배열
let Data_mA = []; // 전류 데이터를 저장할 배열 (mA 단위)
let Data_V = []; // 전압 데이터를 저장할 배열 (V 단위)

// 1000개의 초기값을 배열에 설정
for (let inx = 0; inx < 1000; inx++) {
    Time.push(periodMs * inx); // 시간 배열에 주기적으로 값을 추가
    Data_mA.push(0); // 전류 데이터를 0으로 초기화
    Data_V.push(0); // 전압 데이터를 0으로 초기화
}

var ChartInst; // 차트 인스턴스를 저장할 변수

// 새 차트를 생성하는 함수
function new_chart() {
    ChartInst = new Chart(Ctxt, { // Chart.js 라이브러리 사용
        type: "line", // 차트 타입: 선형 그래프
        data: {
            labels: Time, // X축 레이블: 시간 데이터
            datasets: [
                {
                    label: 'mA', // 전류 데이터셋 레이블
                    yAxisID: 'mA', // Y축 ID
                    backgroundColor: "rgb(209, 20, 61)", // 전류 데이터 배경색
                    borderColor: "rgb(209, 20, 61)", // 전류 데이터 경계색
                    data: Data_mA, // 전류 데이터
                    cubicInterpolationMode: 'monotone', // 부드러운 곡선 연결 방식
                },
                {
                    label: 'V', // 전압 데이터셋 레이블
                    yAxisID: 'V', // Y축 ID
                    backgroundColor: "rgb(34, 73, 228)", // 전압 데이터 배경색
                    borderColor: "rgb(34, 73, 228)", // 전압 데이터 경계색
                    data: Data_V, // 전압 데이터
                    cubicInterpolationMode: 'monotone', // 부드러운 곡선 연결 방식
                }
            ],
        },
        options: {
            animation: {
                duration: 0 // 애니메이션 없음
            },
            responsive: true, // 반응형 설정
            maintainAspectRatio: false, // 종횡비 유지 비활성화
            borderWidth: 1, // 경계 너비 설정
            pointRadius: 0, // 포인트 반경 0
            scales: {
                mA: { // 전류 데이터 Y축 설정
                    type: 'linear', // 선형 스케일
                    position: 'left', // 왼쪽에 위치
                    ticks: {
                        color: "rgb(209, 20, 61)" // Y축 눈금의 색상
                    }
                },
                V: { // 전압 데이터 Y축 설정
                    type: 'linear', // 선형 스케일
                    position: 'right', // 오른쪽에 위치
                    ticks: {
                        color: "rgb(34, 73, 228)" // Y축 눈금의 색상
                    }
                }
            }
        },
    });
}

// 슬라이더 초기화 함수
function init_sliders() {
    let sliderSections = document.getElementsByClassName("range-slider"); // 슬라이더 요소들 가져오기
    for (let i = 0; i < sliderSections.length; i++) {
        let sliders = sliderSections[i].getElementsByTagName("input"); // 각 슬라이더에 대해
        for (let j = 0; j < sliders.length; j++) {
            if (sliders[j].type === "range") {
                sliders[j].oninput = update_chart; // 슬라이더 값 변경 시 차트 업데이트
                sliders[j].value = (j == 0 ? 0.0 : Time.length * periodMs); // 슬라이더 초기값 설정
                sliders[j].min = 0.0; // 최소값 설정
                sliders[j].max = parseFloat(Time.length) * periodMs; // 최대값 설정
                sliders[j].oninput(); // 슬라이더 초기 입력값으로 업데이트 실행
            }
        }
    }
}

// 차트를 업데이트하는 함수
function update_chart() {
    let slides = document.getElementsByTagName("input"); // 슬라이더 요소 가져오기
    let min = parseFloat(slides[0].value); // 첫 번째 슬라이더 값
    let max = parseFloat(slides[1].value); // 두 번째 슬라이더 값

    // 최소값과 최대값 비교 후 교체
    if (min > max) {
        let tmp = max;
        max = min;
        min = tmp;
    }

    // 슬라이더 범위에 따른 시간, 전류, 전압 데이터 추출
    let time_slice = [];
    let data_mA_slice = [];
    let data_V_slice = [];

    let min_index = min / periodMs; // 슬라이더 범위에 따른 최소 인덱스 계산
    let max_index = max / periodMs; // 슬라이더 범위에 따른 최대 인덱스 계산

    time_slice = JSON.parse(JSON.stringify(Time)).slice(min_index, max_index); // 시간 데이터 슬라이스
    data_mA_slice = JSON.parse(JSON.stringify(Data_mA)).slice(min_index, max_index); // 전류 데이터 슬라이스
    data_V_slice = JSON.parse(JSON.stringify(Data_V)).slice(min_index, max_index); // 전압 데이터 슬라이스

    // 차트 업데이트
    ChartInst.data.labels = time_slice; // 차트의 X축 업데이트
    ChartInst.data.datasets[0].data = data_mA_slice; // 전류 데이터 업데이트
    ChartInst.data.datasets[1].data = data_V_slice; // 전압 데이터 업데이트
    ChartInst.update(0); // 차트 업데이트 (애니메이션 없음)

    // 평균, 최대, 최소 값 계산
    let iAvg = 0.0;
    let vAvg = 0.0;
    let iMax = -9999999.0;
    let iMin = 9999999.0;
    let vMax = -9999999.0;
    let vMin = 9999999.0;
    for (let t = 0; t < time_slice.length; t++) {
        let i = parseFloat(data_mA_slice[t]);
        let v = parseFloat(data_V_slice[t]);
        iAvg = iAvg + i;
        vAvg = vAvg + v;
        if (i > iMax) iMax = i;
        if (v > vMax) vMax = v;
        if (i < iMin) iMin = i;
        if (v < vMin) vMin = v;
    }
    iAvg = iAvg / time_slice.length;
    vAvg = vAvg / time_slice.length;

    // 범위 및 통계 정보를 HTML에 표시
    let displayElement = document.getElementsByClassName("rangeValues")[0];
    displayElement.innerHTML = "[" + min + "," + max + "]mS";
    document.getElementById("istats").innerHTML =
        "avg : " + iAvg.toFixed(3) + "mA<br>" +
        "min : " + iMin.toFixed(3) + "mA<br>" +
        "max : " + iMax.toFixed(3) + "mA";
    document.getElementById("vstats").innerHTML =
        "avg : " + vAvg.toFixed(3) + "V<br>" +
        "min : " + vMin.toFixed(3) + "V<br>" +
        "max : " + vMax.toFixed(3) + "V";
}

// WebSocket 초기화
let gateway = `ws://${window.location.hostname}/ws`; // WebSocket 게이트웨이 주소 설정
var websocket; // WebSocket 객체 선언

window.addEventListener('load', on_window_load); // 페이지 로드 시 WebSocket과 차트 초기화

function on_window_load(event) {
    new_chart(); // 차트 생성
    init_sliders(); // 슬라이더 초기화
    init_web_socket(); // WebSocket 연결 초기화
    init_capture_buttons(); // 캡처 버튼 초기화
}

window.onbeforeunload = function () {
    websocket.onclose = function () { }; // WebSocket이 닫힐 때 이벤트 비활성화
    websocket.close(); // WebSocket 연결 종료
}

// WebSocket 연결 처리
function init_web_socket() {
    console.log('Trying to open a WebSocket connection...'); // 연결 시도 메시지 출력
    websocket = new WebSocket(gateway); // WebSocket 객체 생성
    websocket.binaryType = "arraybuffer"; // WebSocket 데이터 타입을 바이너리로 설정
    websocket.onopen = on_ws
