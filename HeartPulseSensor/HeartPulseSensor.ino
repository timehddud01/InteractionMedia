#include <Arduino.h>
//https://github.com/WorldFamousElectronics/PulseSensor_Amped_Arduino

#define SERIAL_PLOTTER 2

// --- 핀 설정 ---
int pulsePin = 0; // A0

// --- 상태 머신 (State Machine) ---
enum State { IDLE, ACTIVE }; 
State currentState = IDLE;   

const int HAND_ON_THRESHOLD = 10;    
const int HAND_OFF_THRESHOLD = 1010; 

const long HAND_OFF_COOLDOWN = 2000; 
unsigned long handOffCoolDownStart = 0; 
// ▼▼▼ "손 댐" 2초 체크용 타이머 변수 ▼▼▼
unsigned long handOnTimerStart = 0; 

// --- 내부 로직용 변수 ---
volatile int Signal;
volatile boolean Pulse = false;
volatile unsigned long sampleCounter = 0;
volatile unsigned long lastBeatTime = 0;
volatile int P = 512;
volatile int T = 512;
volatile int thresh = 530;
volatile int amp = 100;

// --- 스무딩(필터링)용 변수 ---
volatile float smoothSignal = 512.0;

// --- 콘트라스트(증폭)용 변수 ---
const float EXAGGERATION_FACTOR = 2.0; 

// --- TouchDesigner 전송용 변수 ---
volatile int outputSignal = 512; 
volatile int beatMarker = 0; // (내부 로직용)

static int outputType = SERIAL_PLOTTER;

void setup() {
  Serial.begin(115200); 
  interruptSetup();     
}

void loop() {
  int currentOutputSignal = outputSignal;

  switch (currentState) {
    
    case IDLE:
      // 1. 시작 직후 상태: 중간값 표시, active 0
      Serial.print(512); 
      Serial.print(",");
      Serial.println(0); // 0 = IDLE

      // 2. 손을 댐 (0이 되었을 때)
      // "손 댐"이 감지되고(신호 < 10), "손 뗌" 쿨다운이 끝났는지 확인
      if ( (currentOutputSignal < HAND_ON_THRESHOLD) && 
           (millis() - handOffCoolDownStart > HAND_OFF_COOLDOWN) ) {
        
        // "손 댐" 상태 감지. 타이머 시작 (아직 안 켰다면)
        if (handOnTimerStart == 0) {
          handOnTimerStart = millis();
        }

        // "손 댐" 상태가 2초(2000ms) 이상 지속되었는지 확인
        if (millis() - handOnTimerStart > 2000) {
          currentState = ACTIVE; // 2초 확인 후 ACTIVE로 전환
          handOnTimerStart = 0; // 타이머 리셋
        }
        
      } else {
        // "손 댐" 상태가 아니거나(신호가 10 이상) 쿨다운 중이면
        // 2초 타이머를 리셋함
        handOnTimerStart = 0;
      }
      break; 

    case ACTIVE:
      // "손 댐" 타이머 리셋 (다음에 IDLE로 돌아갔을 때를 대비)
      handOnTimerStart = 0; 

      // 2. (계속) active 1, 측정 값 표시
      Serial.print(currentOutputSignal);
      Serial.print(",");
      Serial.println(1); // 1 = ACTIVE

      // 3. 손을 떼었을 때 (1023이 되었을 때)
      if (currentOutputSignal > HAND_OFF_THRESHOLD) {
        currentState = IDLE; 
        handOffCoolDownStart = millis(); // 쿨다운 타이머 시작
        // (리셋 코드는 주석 처리된 상태 유지)
      }
      break; 
  }
  
  // 4. (IDLE 상태가 됨)
  delay(50); // 1초에 20번 전송
}


// (interruptSetup 및 ISR 함수는 이전과 동일하게 유지)

void interruptSetup() {
  TCCR2A = 0x02;
  TCCR2B = 0x06;
  OCR2A = 0X7C;
  TIMSK2 = 0x02;
  sei();
}

ISR(TIMER2_COMPA_vect) {
  cli(); 

  int rawSignal = analogRead(pulsePin);
  smoothSignal = (0.1 * rawSignal) + (0.9 * smoothSignal);
  Signal = (int)smoothSignal; 

  sampleCounter += 2;
  int N = sampleCounter - lastBeatTime;

  // --- ISR의 자동 보정 로직 ---
  if (Signal < thresh) {
    if (Signal < T) { T = Signal; }
  }
  if (Signal > thresh && Signal > P) {
    P = Signal;
  }
  if (N > 250) {
    if ( (Signal > thresh) && (Pulse == false) ) {
      Pulse = true;
      beatMarker = 1; 
      lastBeatTime = sampleCounter;
    }
  }
  if (Signal < thresh && Pulse == true) {
    Pulse = false;
    beatMarker = 0; 
    amp = P - T;
    thresh = amp / 2 + T; // <-- 핵심: thresh가 자동으로 갱신됨
    P = thresh;
    T = thresh;
  }
  
  // (중요) 2.5초간 맥박이 없으면(손을 떼거나, 적응 중일 때)
  // thresh를 530으로 리셋하여 '손 댐' 감지 상태로 복귀
  if (N > 2500) { 
    thresh = 530;
    P = 512;
    T = 512;
    lastBeatTime = sampleCounter;
    beatMarker = 0; 
  }

  // --- 증폭 로직 (그대로 유지) ---
  float deviation = Signal - thresh; 
  float amplifiedDeviation = deviation * EXAGGERATION_FACTOR;
  float amplifiedSignal = 512.0 + amplifiedDeviation;
  outputSignal = (int)constrain(amplifiedSignal, 0, 1023);

  sei(); 
}