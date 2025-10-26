#line 1 "C:\\KDY\\25-2\\Interaction\\ArduinoCode\\InteractionMedia\\HeartPulseSensor\\HeartPulseSensor.ino"
//https://github.com/WorldFamousElectronics/PulseSensor_Amped_Arduino

#include <Arduino.h>


#define SERIAL_PLOTTER  2

int pulsePin = 0; //A0


//volatile 변수: 언제든 변경될 수 있는 변수임을 컴파일러에 알려줌
volatile int Signal; //센서에서 읽은 초기 값

volatile boolean Pulse = false; //심박수 감지 여부, 감지되면 true, 아니면 false

volatile int beatMarker = 0 ; // 심박 감지 시 1로 설정(플래그 역할)



//Srial plotter로 아두이노IDE의 시리얼 플로터에서 그래프로 볼 수 있음
static int outputType = SERIAL_PLOTTER;

volatile unsigned long sampleCounter = 0; //시작 후 2ms마다 1씩 증가함
volatile unsigned long lastBeatTime = 0; //마지막 심박이 감지된 순간의 시간 기록
volatile int P = 512; //'peak' , 심박수 최고점
volatile int T = 512; // trough , 심박수 최저점
volatile int thresh = 530; //임계값
volatile int amp = 100; // 심박수의 진폭( P - T 를 저장)


#line 31 "C:\\KDY\\25-2\\Interaction\\ArduinoCode\\InteractionMedia\\HeartPulseSensor\\HeartPulseSensor.ino"
void setup();
#line 36 "C:\\KDY\\25-2\\Interaction\\ArduinoCode\\InteractionMedia\\HeartPulseSensor\\HeartPulseSensor.ino"
void loop();
#line 58 "C:\\KDY\\25-2\\Interaction\\ArduinoCode\\InteractionMedia\\HeartPulseSensor\\HeartPulseSensor.ino"
void sendDataToSerial(char symbol, int data);
#line 69 "C:\\KDY\\25-2\\Interaction\\ArduinoCode\\InteractionMedia\\HeartPulseSensor\\HeartPulseSensor.ino"
void interruptSetup();
#line 31 "C:\\KDY\\25-2\\Interaction\\ArduinoCode\\InteractionMedia\\HeartPulseSensor\\HeartPulseSensor.ino"
void setup() {
    Serial.begin(115200); //시리얼 통신 시작, 보드레이트 115200
    interruptSetup(); //인터럽트 설정 함수 호출
}

void loop() {

    //현재 값 복사
    int currentSignal = Signal; 
    int currentBeatMarker = beatMarker;
    
    //2.터치디자이너로 전송하기
    Serial.print(currentSignal);
    Serial.print(",");
    Serial.println(currentBeatMarker);
    
    if(currentBeatMarker == 1) {
      //심박이 감지되었으면 다시 0으로 리셋
      beatMarker = 0; 
    }

    delay(10);

}



void sendDataToSerial(char symbol, int data) 
{
  // 시리얼 포트로 데이터를 전송하는 함수
  Serial.print(symbol); // 먼저 심볼 문자를 전송(S)
  Serial.println(data); // 그 다음에 값(data)을 전송
}


//아두이노의 Timer2 인터럽트를 설정하는 함수

//1초에 500번 (즉, 2ms마다 한 번씩) ISR(TIMER2_COMPA_vect) 함수를 실행하게 함
void interruptSetup() 
{
    TCCR2A = 0x02;
    TCCR2B = 0x06;
    OCR2A = 0X7C;
    TIMSK2 = 0x02;
    sei();
    
}


ISR(TIMER2_COMPA_vect) {
  cli();
  Signal = analogRead(pulsePin);
  sampleCounter += 2;
  int N = sampleCounter - lastBeatTime;

  if (Signal < thresh) {
    if (Signal < T) { T = Signal; }
  }

  if (Signal > thresh && Signal > P) {
    P = Signal;
  }

  // 심박 감지 로직
  if (N > 250) {
    if ( (Signal > thresh) && (Pulse == false) ) {
      Pulse = true;
      // LED 켜는 대신 깃발(Marker)을 1로 설정
      beatMarker = 1; 
      lastBeatTime = sampleCounter;
    }
  }

  // 심박 종료 감지
  if (Signal < thresh && Pulse == true) {
    // LED 끄는 코드 제거
    Pulse = false;
    amp = P - T;
    thresh = amp / 2 + T;
    P = thresh;
    T = thresh;
  }

  // 타임아웃
  if (N > 2500) {
    thresh = 530;
    P = 512;
    T = 512;
    lastBeatTime = sampleCounter;
  }

  sei();
}








