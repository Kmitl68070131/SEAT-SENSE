/*
Ultrasonic1 Trig : D2
            Echo : D3
Ultrasonic2 Trig : D4
            Echo : D5

I2C LCD     SDA : SDA (A4)
            SCL : SCL (A5)
Keypad      Row1 : D6
            Row2 : D7
            Row3 : D8
            Row4 : D9
            Col1 : D10
            Col2 : D11
            Col3 : D12
            Col4 : D13

TMP         Vout : A0
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

// ==========================================
// 1. การกำหนดขาอุปกรณ์ (Pin Mapping)
// ==========================================
const int TRIG_1 = 2;
const int ECHO_1 = 3;

const int TRIG_2 = 4;
const int ECHO_2 = 5;

const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {6, 7, 8, 9};       
byte colPins[COLS] = {10, 11, 12, 13};   
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

const int TMP36_PIN = A0;

LiquidCrystal_I2C lcd(0x27, 16, 2);

// ==========================================
// ตัวแปรระบบ
// ==========================================
unsigned long lastSensorRead = 0;
const long sensorInterval = 1000; // อ่านค่าเซนเซอร์ทุกๆ 1 วินาที
char lastKeyPressed = '-';        // แสดง '-' เมื่อยังไม่มีการกดปุ่ม

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

// ฟังก์ชันอ่านอุณหภูมิ TMP36 (°C)
float readTMP36() {
  int rawValue = analogRead(TMP36_PIN);
  float voltage = rawValue * (5.0 / 1023.0); 
  return (voltage - 0.5) * 100.0;
}

// ฟังก์ชันจัดฟอร์แมตและแสดงผลบน LCD 16x2
void updateLCD(float dist1, float dist2, float temp) {
  // --- บรรทัดที่ 1 (Row 0): U1 (ซ้าย) & U2 (ขวา) ---
  char u1Str[10], u2Str[10];
  if (dist1 >= 0) snprintf(u1Str, sizeof(u1Str), "U1:%dcm", (int)dist1);
  else strcpy(u1Str, "U1:ERR");

  if (dist2 >= 0) snprintf(u2Str, sizeof(u2Str), "U2:%dcm", (int)dist2);
  else strcpy(u2Str, "U2:ERR");

  // คำนวณช่องว่างเพื่อให้ U1 ชิดซ้าย และ U2 ชิดขวาพอดี 16 ช่อง
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
  snprintf(kpStr, sizeof(kpStr), "KP:%c", lastKeyPressed);

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

  lcd.init();
  lcd.backlight();
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("== System Test ==");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  
  Serial.println("=========================================");
  Serial.println("    Arduino R4 WiFi Full System Test     ");
  Serial.println("=========================================");
  
  delay(1500);
  lcd.clear();
}

void loop() {
  // 1. ตรวจจับการกด Keypad (ตอบสนองและอัปเดตหน้าจอทันที)
  char key = keypad.getKey();
  if (key) {
    lastKeyPressed = key;
    Serial.print("[KEYPAD] Pressed: ");
    Serial.println(key);

    // อัปเดตเฉพาะตัวอักษรปุ่มกดตรงตำแหน่งขวาล่างสุด (Col 15, Row 1) ทันทีที่กด
    lcd.setCursor(15, 1);
    lcd.print(lastKeyPressed);
  }

  // 2. อ่านค่าเซนเซอร์และอัปเดตหน้าจอ LCD ทุกๆ 1 วินาที
  if (millis() - lastSensorRead >= sensorInterval) {
    lastSensorRead = millis();

    float dist1 = readUltrasonic(TRIG_1, ECHO_1);
    delay(30); // หน่วงเวลา 30ms ป้องกันคลื่นเสียงกวนกัน
    float dist2 = readUltrasonic(TRIG_2, ECHO_2);
    float temp = readTMP36();

    // แสดงผลบน Serial Monitor
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

    // อัปเดตข้อมูลบนจอ LCD
    updateLCD(dist1, dist2, temp);
  }
}