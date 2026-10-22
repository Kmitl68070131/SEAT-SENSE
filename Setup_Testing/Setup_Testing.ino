/*
Ultrasonic1 Trig : D2
            Echo : D3
Ultrasonic2 Trig : D4
            Echo : D5

I2C LCD     SDA : A4
            SCL : A5

Keypad      Row1 : D6
            Row2 : D7
            Row3 : D8
            Row4 : D9
            Col1 : D10
            Col2 : D11
            Col3 : D12
            Col4 : D13

TMP         Vout : A0

RGB LED     R : A1
            G : A2
            B : A3
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ==========================================
// 1. การกำหนดขาอุปกรณ์ (Pin Mapping)
// ==========================================
const int TRIG_1 = 2;
const int ECHO_1 = 3;

const int TRIG_2 = 4;
const int ECHO_2 = 5;

const int TMP36_PIN = A0;

// ขา RGB LED (Common Anode)
const int LED_R = A1;
const int LED_G = A2;
const int LED_B = A3;

LiquidCrystal_I2C lcd(0x27, 16, 2);

// ==========================================
// ตัวแปรระบบ
// ==========================================
unsigned long lastSensorRead = 0;
const long sensorInterval = 1000; // อ่านค่าเซนเซอร์ทุกๆ 1 วินาที
char currentKeyPressed = '-';     // เก็บสถานะปุ่มกดปัจจุบัน (เริ่มต้นเป็น '-')

// ตัวแประบบทดสอบ RGB LED
unsigned long lastRgbChange = 0;
const long rgbInterval = 1000;    // สลับสีทุก 1 วินาที (1000ms)
int rgbState = 0;                 // 0 = Red, 1 = Green, 2 = Blue

// ฟังก์ชันเปิดสี RGB (Common Anode: LOW = ติด, HIGH = ดับ)
void setRGBColor(bool red, bool green, bool blue) {
  digitalWrite(LED_R, red ? LOW : HIGH);
  digitalWrite(LED_G, green ? LOW : HIGH);
  digitalWrite(LED_B, blue ? LOW : HIGH);
}

// ฟังก์ชันอ่านระยะทาง Ultrasonic (cm)
float readUltrasonic(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(4);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH, 30000); 
  if (duration == 0) return -1.0; 
  return (duration * 0.0343) / 2.0;
}

// ตัวแประดับ Global สำหรับจำค่าอุณหภูมิที่ผ่านการกรองแล้ว
float smoothedTemp = -999.0; 

float readTMP36() {
  // 1. อ่านค่าสะสม 50 ครั้งเพื่อตัด Noise สัญญาณไฟฟ้าเบื้องต้น
  long sumRaw = 0;
  int samples = 50;
  for (int i = 0; i < samples; i++) {
    sumRaw += analogRead(TMP36_PIN);
    delayMicroseconds(300);
  }
  
  float avgRaw = (float)sumRaw / samples;
  float voltage = avgRaw * (5.0 / 1023.0); 
  float rawTemp = ((voltage - 0.5) * 100.0 * -1.0) + 17.0; // สูตรชดเชยค่าเดิมของคุณ

  // 2. กำหนดค่าเริ่มต้นในครั้งแรกที่เปิดเครื่อง
  if (smoothedTemp == -999.0) {
    smoothedTemp = rawTemp;
  }

  // 3. ใช้ Exponential Moving Average (EMA) เพื่อกรองการกระชาก
  // alpha = 0.05 หมายถึง รับค่าใหม่ 5% + ค่าเก่า 95% (ยิ่งค่าน้อย ยิ่งนิ่งนุ่มนวล)
  float alpha = 0.05; 
  smoothedTemp = (alpha * rawTemp) + ((1.0 - alpha) * smoothedTemp);

  return smoothedTemp + 9;
}

// ฟังก์ชันจัดฟอร์แมตและแสดงผลบน LCD 16x2
void updateLCD(float dist1, float dist2, float temp) {
  // --- บรรทัดที่ 1 (Row 0): U1 (ซ้าย) & U2 (ขวา) ---
  char u1Str[10], u2Str[10];
  if (dist1 >= 0) snprintf(u1Str, sizeof(u1Str), "U1:%dcm", (int)dist1);
  else strcpy(u1Str, "U1:ERR");

  if (dist2 >= 0) snprintf(u2Str, sizeof(u2Str), "U2:%dcm", (int)dist2);
  else strcpy(u2Str, "U2:ERR");

  int len1 = strlen(u1Str);
  int len2 = strlen(u2Str);
  int spaces0 = 16 - len1 - len2;
  if (spaces0 < 0) spaces0 = 0;

  char line0[17];
  snprintf(line0, sizeof(line0), "%s%*s%s", u1Str, spaces0, "", u2Str);

  lcd.setCursor(0, 0);
  lcd.print(line0);

  // --- บรรทัดที่ 2 (Row 1): TMP (ซ้าย) & KP (ขวา) ---
  char tmpStr[10], kpStr[6];
  snprintf(tmpStr, sizeof(tmpStr), "TMP:%.1fC", temp);
  snprintf(kpStr, sizeof(kpStr), "KP:%c", currentKeyPressed);

  int lenT = strlen(tmpStr);
  int lenK = strlen(kpStr);
  int spaces1 = 16 - lenT - lenK;
  if (spaces1 < 0) spaces1 = 0;

  char line1[17];
  snprintf(line1, sizeof(line1), "%s%*s%s", tmpStr, spaces1, "", kpStr);

  lcd.setCursor(0, 1);
  lcd.print(line1);
}

void setup() {
  Serial.begin(9600);

  pinMode(TRIG_1, OUTPUT);
  pinMode(ECHO_1, INPUT);
  pinMode(TRIG_2, OUTPUT);
  pinMode(ECHO_2, INPUT);

  // ตั้งค่าขา RGB LED
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  setRGBColor(false, false, false); // ปิดไฟทั้งหมดก่อนเริ่มต้น

  // ตั้งค่าขา Keypad (Rows 6-9 เป็น OUTPUT, Cols 10-13 เป็น INPUT_PULLUP)
  for (int i = 6; i <= 9; i++) {
    pinMode(i, OUTPUT);
    digitalWrite(i, HIGH);
  }
  for (int i = 10; i <= 13; i++) {
    pinMode(i, INPUT_PULLUP);
  }

  lcd.init();
  lcd.backlight();
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("== System Test ==");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  
  delay(1500);
  lcd.clear();
}

void loop() {
  // ----------------------------------------------------
  // 1. สแกน Keypad Manual
  // ----------------------------------------------------
  char detectedKey = 0;

  // Row 1 (D6 = LOW) -> 1, 2, 3, A
  digitalWrite(6, LOW); digitalWrite(7, HIGH); digitalWrite(8, HIGH); digitalWrite(9, HIGH);
  if      (digitalRead(10) == LOW) detectedKey = '1';
  else if (digitalRead(11) == LOW) detectedKey = '2';
  else if (digitalRead(12) == LOW) detectedKey = '3';
  else if (digitalRead(13) == LOW) detectedKey = 'A';

  // Row 2 (D7 = LOW) -> 4, 5, 6, B
  digitalWrite(6, HIGH); digitalWrite(7, LOW); digitalWrite(8, HIGH); digitalWrite(9, HIGH);
  if      (digitalRead(10) == LOW) detectedKey = '4';
  else if (digitalRead(11) == LOW) detectedKey = '5';
  else if (digitalRead(12) == LOW) detectedKey = '6';
  else if (digitalRead(13) == LOW) detectedKey = 'B';

  // Row 3 (D8 = LOW) -> 7, 8, 9, C
  digitalWrite(6, HIGH); digitalWrite(7, HIGH); digitalWrite(8, LOW); digitalWrite(9, HIGH);
  if      (digitalRead(10) == LOW) detectedKey = '7';
  else if (digitalRead(11) == LOW) detectedKey = '8';
  else if (digitalRead(12) == LOW) detectedKey = '9';
  else if (digitalRead(13) == LOW) detectedKey = 'C';

  // Row 4 (D9 = LOW) -> *, 0, #, D
  digitalWrite(6, HIGH); digitalWrite(7, HIGH); digitalWrite(8, HIGH); digitalWrite(9, LOW);
  if      (digitalRead(10) == LOW) detectedKey = '*';
  else if (digitalRead(11) == LOW) detectedKey = '0';
  else if (digitalRead(12) == LOW) detectedKey = '#';
  else if (digitalRead(13) == LOW) detectedKey = 'D';

  // กำหนดค่าปุ่มกดปัจจุบัน: ถ้ามีการกดให้ใช้ปุ่มนั้น ถ้ากดปล่อยมือให้เปลี่ยนเป็น '-'
  char keyNow = (detectedKey != 0) ? detectedKey : '-';

  // อัปเดตเฉพาะจุดขวาล่างทันทีเมื่อมีการเปลี่ยนแปลง (เพื่อให้ตอบสนองได้รวดเร็ว)
  if (keyNow != currentKeyPressed) {
    currentKeyPressed = keyNow;
    lcd.setCursor(15, 1);
    lcd.print(currentKeyPressed);
  }

  // ----------------------------------------------------
  // 2. ทดสอบสลับสี RGB LED ทุกๆ 1 วินาที (แดง -> เขียว -> น้ำเงิน)
  // ----------------------------------------------------
  if (millis() - lastRgbChange >= rgbInterval) {
    lastRgbChange = millis();

    if (rgbState == 0) {
      setRGBColor(true, false, false); // สีแดง
    } else if (rgbState == 1) {
      setRGBColor(false, true, false); // สีเขียว
    } else if (rgbState == 2) {
      setRGBColor(false, false, true); // สีน้ำเงิน
    }

    rgbState = (rgbState + 1) % 3; // วนรอบสเตท 0 -> 1 -> 2 -> 0
  }

  // ----------------------------------------------------
  // 3. อ่านค่าเซนเซอร์และอัปเดตหน้าจอ LCD ทุกๆ 1 วินาที
  // ----------------------------------------------------
  if (millis() - lastSensorRead >= sensorInterval) {
    lastSensorRead = millis();

    float dist1 = readUltrasonic(TRIG_1, ECHO_1);
    delay(30); 
    float dist2 = readUltrasonic(TRIG_2, ECHO_2);
    float temp = readTMP36();

    // แสดงผลออก Serial Monitor
    Serial.println("-----------------------------------------");
    Serial.print("U1 (D2/D3) : ");
    if (dist1 >= 0) { Serial.print(dist1, 1); Serial.println(" cm"); }
    else { Serial.println("ERR"); }

    Serial.print("U2 (D4/D5) : ");
    if (dist2 >= 0) { Serial.print(dist2, 1); Serial.println(" cm"); }
    else { Serial.println("ERR"); }

    Serial.print("TMP36 (A0) : ");
    Serial.print(temp, 1);
    Serial.println(" C");

    updateLCD(dist1, dist2, temp);


  }
}
