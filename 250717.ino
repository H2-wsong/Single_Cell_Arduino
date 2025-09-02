/*
  Arduino Multi-Temp & Relay Firmware (5-channel Temp)
  - 릴레이 제어: '1' (켜기), '0' (끄기)
  - 온도 측정: 'a'(A0), 'b'(A1), 'c'(A2), 'd'(A3), 'e'(A4) - 고유 명령어로 변경
  - 안전 로직: 릴레이가 'OFF'('0') 상태로 2시간 이상 유지될 경우 자동으로 'ON'('1')으로 전환
*/

// --- 릴레이 설정 ---
const int RELAY_PIN = 8;
// --- 서미스터 설정 ---
const int THERMISTOR_PINS[] = {A0, A1, A2, A3, A4};
const int NUM_SENSORS = 5;

const float NOMINAL_RESISTANCE = 10000;
const float NOMINAL_TEMPERATURE = 25;
const float B_CONSTANT = 3435;
const float SERIES_RESISTOR = 10000;

// --- 안전 로직을 위한 변수 ---
unsigned long relayOffTimestamp; // 릴레이가 꺼진 시간을 기록 (밀리초)
// 2시간을 밀리초로 환산 (2 * 60 * 60 * 1000 = 7,200,000)
const unsigned long MAX_OFF_DURATION = 7200000UL; 
bool isRelayOn; // 릴레이의 현재 상태 (true: ON, false: OFF)

void setup() {
  Serial.begin(9600);
  pinMode(RELAY_PIN, OUTPUT);
  
  // 초기 상태를 'OFF'로 설정하고, 타이머를 시작합니다.
  digitalWrite(RELAY_PIN, HIGH); // HIGH = Relay Closed (OFF)
  isRelayOn = false;
  relayOffTimestamp = millis(); // OFF 상태로 시작하므로 현재 시간을 기록
  
  Serial.println("Arduino Control Ready (Multi-Temp & Relay).");
  Serial.println("안전 기능: 릴레이가 2시간 이상 OFF일 경우 자동으로 ON 됩니다.");
  Serial.println("초기 상태: 릴레이 꺼짐 (OFF)");
}

void loop() {
  // --- 시리얼 명령 처리 ---
  if (Serial.available() > 0) {
    char command = Serial.read();

    switch (command) {
      case '1': // 릴레이 켜기 (ON)
        digitalWrite(RELAY_PIN, LOW); // LOW = Relay Opened (ON)
        isRelayOn = true;
        Serial.println("Relay Opened (ON)");
        break;
        
      case '0': // 릴레이 끄기 (OFF)
        digitalWrite(RELAY_PIN, HIGH); // HIGH = Relay Closed (OFF)
        // 상태가 ON -> OFF로 변경될 때만 시간을 기록하여 타이머를 시작/재시작합니다.
        if (isRelayOn) { 
          isRelayOn = false;
          relayOffTimestamp = millis();
        }
        Serial.println("Relay Closed (OFF)");
        break;

      // --- 온도 측정 명령 ---
      case 'a': measureAndSendTemperature(0); break;
      case 'b': measureAndSendTemperature(1); break;
      case 'c': measureAndSendTemperature(2); break;
      case 'd': measureAndSendTemperature(3); break;
      case 'e': measureAndSendTemperature(4); break;
    }
  }

  // --- 안전 로직: 2시간 OFF 상태 감지 (루프마다 항상 실행) ---
  // 릴레이가 꺼져 있고 (isRelayOn == false)
  // 꺼진 시점으로부터 2시간이 지났는지 확인합니다.
  if (!isRelayOn && (millis() - relayOffTimestamp >= MAX_OFF_DURATION)) {
    Serial.println("안전 로직 발동: 2시간 동안 OFF 상태여서 강제로 릴레이를 켭니다.");
    digitalWrite(RELAY_PIN, LOW); // 강제로 릴레이를 켬 (ON)
    isRelayOn = true; // 릴레이 상태를 ON으로 업데이트
  }
}

void measureAndSendTemperature(int sensorIndex) {
  float adcValue = analogRead(THERMISTOR_PINS[sensorIndex]);

  if (adcValue < 1) { // 0에 가까운 값은 센서 연결 오류로 간주
    Serial.println("Sensor_Error");
    return;
  }
  
  float thermistorResistance = SERIES_RESISTOR * (1023.0 / adcValue - 1.0);
  
  float steinhart;
  steinhart = thermistorResistance / NOMINAL_RESISTANCE;
  steinhart = log(steinhart);
  steinhart /= B_CONSTANT;
  steinhart += 1.0 / (NOMINAL_TEMPERATURE + 273.15);
  steinhart = 1.0 / steinhart;
  float temperatureC = steinhart - 273.15;

  Serial.println(temperatureC, 2); // 소수점 2자리까지 출력
}