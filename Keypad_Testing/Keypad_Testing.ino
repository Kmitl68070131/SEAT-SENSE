/*Keypad      Row1 : D6
            Row2 : D7
            Row3 : D8
            Row4 : D9
            Col1 : D10
            Col2 : D11
            Col3 : D12
            Col4 : D13
*/
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
void setup()
{
  for (int i = 6; i <= 9; i++) {
    pinMode(i, OUTPUT);
    digitalWrite(i, HIGH);
  }
  for (int i = 10; i <= 13; i++) {
    pinMode(i, INPUT_PULLUP);
  }
  
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Number is :");
}

void loop()
{
  digitalWrite(6, LOW);
  digitalWrite(7, HIGH);
  digitalWrite(8, HIGH);
  digitalWrite(9, HIGH);

  lcd.setCursor(12, 0);
  if      (digitalRead(10) == LOW) { lcd.print("1"); } // Col 1
  else if (digitalRead(11) == LOW) { lcd.print("2"); } // Col 2
  else if (digitalRead(12) == LOW) { lcd.print("3"); } // Col 3
  else if (digitalRead(13) == LOW) { lcd.print("A"); } // Col 4

  // ----------------------------------------------------
  // สแกน Row 2 (D7 = LOW) -> ปุ่ม 4, 5, 6, B
  // ----------------------------------------------------
  digitalWrite(6, HIGH);
  digitalWrite(7, LOW);
  digitalWrite(8, HIGH);
  digitalWrite(9, HIGH);

  lcd.setCursor(12, 0);
  if      (digitalRead(10) == LOW) { lcd.print("4"); } // Col 1
  else if (digitalRead(11) == LOW) { lcd.print("5"); } // Col 2
  else if (digitalRead(12) == LOW) { lcd.print("6"); } // Col 3
  else if (digitalRead(13) == LOW) { lcd.print("B"); } // Col 4

  // ----------------------------------------------------
  // สแกน Row 3 (D8 = LOW) -> ปุ่ม 7, 8, 9, C
  // ----------------------------------------------------
  digitalWrite(6, HIGH);
  digitalWrite(7, HIGH);
  digitalWrite(8, LOW);
  digitalWrite(9, HIGH);

  lcd.setCursor(12, 0);
  if      (digitalRead(10) == LOW) { lcd.print("7"); } // Col 1
  else if (digitalRead(11) == LOW) { lcd.print("8"); } // Col 2
  else if (digitalRead(12) == LOW) { lcd.print("9"); } // Col 3
  else if (digitalRead(13) == LOW) { lcd.print("C"); } // Col 4

  // ----------------------------------------------------
  // สแกน Row 4 (D9 = LOW) -> ปุ่ม *, 0, #, D
  // ----------------------------------------------------
  digitalWrite(6, HIGH);
  digitalWrite(7, HIGH);
  digitalWrite(8, HIGH);
  digitalWrite(9, LOW);

  lcd.setCursor(12, 0);
  if      (digitalRead(10) == LOW) { lcd.print("*"); } // Col 1
  else if (digitalRead(11) == LOW) { lcd.print("0"); } // Col 2
  else if (digitalRead(12) == LOW) { lcd.print("#"); } // Col 3
  else if (digitalRead(13) == LOW) { lcd.print("D"); } // Col 4
  
  delay(20);
}