/*******************************************************
 * 1) FND + 수위 센서 + 상태 표시 ("1","2",OFF)
 * 2) 도트매트릭스 + (A2=추가, A3=삭제) 카운트 표시
 *    ※ 카운트 수는 FND에 표시하지 않음
 * 3) 버튼 LED: 평상시 LED3 ON, 버튼 누르면 LED3 OFF + LED4 ON
 * 4) D7: 수위센서 감지 시 HIGH
 * 5) D6: carCount가 4 이상일 때 HIGH
 *******************************************************/

// ===================== [A] FND 제어 핀 ======================
#define F_DATA_PIN   8
#define F_LATCH_PIN  9
#define F_CLOCK_PIN 10

// ===================== FND 세그 패턴 ========================
#define SEG_OFF 0b00000000
#define SEG_1   0b01100000
#define SEG_2   0b11011010

// ===================== 수위/버튼 핀 =========================
#define WATER_SENSOR_PIN A0
#define BUTTON_PIN       2
#define BUTTON_LED_PIN   3      // 평상시 ON → 버튼 누르면 OFF
#define BUTTON_LED2_PIN  4      // 버튼 누를 때 ON
#define BUZZER_PIN       5
#define WATER_THRESHOLD  600

// 새로 추가: 상태 출력 핀
#define LEVEL4_PIN       6      // carCount >= 4 이면 HIGH
#define WATER_ALERT_PIN  7      // 수위센서 감지(waterHigh)면 HIGH

// ===================== [B] DOT MATRIX 핀 =====================
int M_DataPin  = 13;
int M_LatchPin = 12;
int M_ClockPin = 11;

// ====== 카운트 버튼 ======
const int addBtnPin   = A2;  // +1
const int clearBtnPin = A3;  // -1

int prevAddBtn   = HIGH;
int prevClearBtn = HIGH;
int carCount = 0;            // 0~5만 표시

// ====== 도트 매트릭스 행/열 데이터 ======
byte row[8] = {
  -B00000001, -B00000010, -B00000100, -B00001000,
  -B00010000, -B00100000, -B01000000, -B10000000
};
byte col[8] = {0,0,0,0,0,0,0,0};

// ====== 도트 매트릭스 패턴 ======
const byte leftColBit   = B01000000;
const byte rightColBit  = B00000010;
const byte centerColBit = B00001000;

byte staticCol[8] = {
  0x00,
  leftColBit | rightColBit,
  leftColBit | rightColBit,
  leftColBit | rightColBit,
  leftColBit | rightColBit,
  leftColBit | rightColBit,
  leftColBit | rightColBit,
  0x00
};

// ===================== 함수 (FND) ======================
void sendDigit(byte p) {
  digitalWrite(F_LATCH_PIN, LOW);
  shiftOut(F_DATA_PIN, F_CLOCK_PIN, LSBFIRST, p);
  digitalWrite(F_LATCH_PIN, HIGH);
}

byte patternForState(int state) {
  switch (state) {
    case 1: return SEG_1;
    case 2: return SEG_2;
  }
  return SEG_OFF;
}

// ===================== 함수 (DOT) ======================
void updatePattern() {
  for (int i = 0; i < 8; i++) col[i] = staticCol[i];

  for (int i = 0; i < 5; i++) {
    int r = 1 + i;  // row 1~5
    if (i < carCount) col[r] |= centerColBit;
  }
}

// ===================== setup ======================
void setup() {
  Serial.begin(9600);

  // [A] FND + 수위 + 버튼 + LED + BUZZER
  pinMode(F_LATCH_PIN, OUTPUT);
  pinMode(F_DATA_PIN,  OUTPUT);
  pinMode(F_CLOCK_PIN, OUTPUT);

  pinMode(WATER_SENSOR_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  pinMode(BUTTON_LED_PIN,  OUTPUT);
  pinMode(BUTTON_LED2_PIN, OUTPUT);
  pinMode(BUZZER_PIN,      OUTPUT);

  // 새로 추가된 상태 핀
  pinMode(LEVEL4_PIN,      OUTPUT);
  pinMode(WATER_ALERT_PIN, OUTPUT);

  digitalWrite(BUTTON_LED_PIN,  HIGH); // 평상시 켜짐
  digitalWrite(BUTTON_LED2_PIN, LOW);  // 평상시 꺼짐
  digitalWrite(BUZZER_PIN,      LOW);
  digitalWrite(LEVEL4_PIN,      LOW);
  digitalWrite(WATER_ALERT_PIN, LOW);

  // [B] 도트 매트릭스
  pinMode(M_LatchPin, OUTPUT);
  pinMode(M_ClockPin, OUTPUT);
  pinMode(M_DataPin,  OUTPUT);
  pinMode(addBtnPin,   INPUT_PULLUP);
  pinMode(clearBtnPin, INPUT_PULLUP);
  updatePattern();

  sendDigit(SEG_OFF);
}

// ===================== loop ======================
void loop() {

  /********* [A] 수위 + 버튼 상태 **********/
  int  waterRaw      = analogRead(WATER_SENSOR_PIN);
  bool waterHigh     = (waterRaw >= WATER_THRESHOLD);
  bool buttonPressed = (digitalRead(BUTTON_PIN) == LOW);

  int state = 0;
  if (waterHigh) state = 1;
  else if (buttonPressed) state = 2;

  // LED 동작: 평상시 LED3 ON / 버튼 누르면 LED3 OFF + LED4 ON
  if (buttonPressed) {
    digitalWrite(BUTTON_LED_PIN,  LOW);
    digitalWrite(BUTTON_LED2_PIN, HIGH);
  } else {
    digitalWrite(BUTTON_LED_PIN,  HIGH);
    digitalWrite(BUTTON_LED2_PIN, LOW);
  }

  // 부저: 물이 높을 때만 울림
  digitalWrite(BUZZER_PIN, waterHigh ? HIGH : LOW);

  // D7: 물 감지 시 HIGH, 아니면 LOW
  digitalWrite(WATER_ALERT_PIN, waterHigh ? HIGH : LOW);

  // 7-세그 표시
  sendDigit(patternForState(state));

  /********* [B] 도트 매트릭스 버튼 **********/
  int addNow   = digitalRead(addBtnPin);
  int clearNow = digitalRead(clearBtnPin);

  if (prevAddBtn == HIGH && addNow == LOW) {
    if (carCount < 5) carCount++;
    updatePattern();
    delay(50);
  }
  if (prevClearBtn == HIGH && clearNow == LOW) {
    if (carCount > 0) carCount--;
    updatePattern();
    delay(50);
  }

  prevAddBtn   = addNow;
  prevClearBtn = clearNow;

  // D6: carCount가 4 이상이면 HIGH, 아니면 LOW
  digitalWrite(LEVEL4_PIN, (carCount >= 4) ? HIGH : LOW);

  // 도트 스캔
  for (int i = 0; i < 8; i++) {
    digitalWrite(M_LatchPin, LOW);
    shiftOut(M_DataPin, M_ClockPin, LSBFIRST, row[i]);
    shiftOut(M_DataPin, M_ClockPin, LSBFIRST, col[i]);
    digitalWrite(M_LatchPin, HIGH);
    delay(1);
  }
}                                                                                                            
                                                                                                                                                                                                                                                                                                                       
