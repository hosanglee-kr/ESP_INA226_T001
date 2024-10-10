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


