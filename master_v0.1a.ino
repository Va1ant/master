#include <LiquidCrystal.h>
#include "OneWireSlave.h"

#define DOWN_BTN 7
#define OK_BTN 6

uint8_t data[5];
uint8_t er0;

uint8_t menu, sub, subMax;
const uint8_t menuMax = 3;
uint8_t page = 1;

uint8_t brightness = 0, target = 255;
const uint8_t rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;

LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
OneWireSlave ds(8);

void WAKE_UP_AND_LISTEN(uint8_t cmd) {
  pinMode(8, OUTPUT);
  digitalWrite(8, LOW);
  delayMicroseconds(15);
  ds.send(cmd);
}

void setup() {
  pinMode(DOWN_BTN, INPUT_PULLUP);
  pinMode(OK_BTN, INPUT_PULLUP);

  lcd.begin(16, 2);

  while (brightness != target) {
    analogWrite(9, ++brightness);
    delay(3);
  }

  show();
}

void loop() {
  if (!digitalRead(DOWN_BTN)) {
    if (brightness != target) {
      while (brightness != target) {
        analogWrite(9, ++brightness);
        delay(3);
      }
      lcd.display();
    }

    if (sub) {
      if (++sub > subMax) sub = 1;
    } else if (++menu > menuMax) menu = 0;

    show();
    delay(150);
  }

  if (!digitalRead(OK_BTN)) {
    switch (menu) {
      case 0:
        switch (sub) {
          case 0: connect(); break;
          case 2: newSetpoint(); break;
          case 3: newDelta(); break;
          case 4: newK(); break;
          case 7: confirm(); break;
          case 8: back(); break;
        }
        break;
      case 1:
        switch (sub) {
          case 0: options(); break;
          case 1: setBrightness(); break;
          case 2: back(); break;
        }
        break;
      case 2: clearErrsum(); break;
      case 3: turnOff(); break;
    }
    delay(175);
  }
}

void show() {
  bool pointer;
  char lcdString[2][17];

  if (!sub) {
    pointer = menu & 1;

    if ((menu >> 1) == page) {
      lcd.setCursor(0, !pointer);
      lcd.print(F(" "));
      lcd.setCursor(0, pointer);
      lcd.print(F(">"));
      return;
    }

    page = menu >> 1;
    switch (page) {
      case 0:
        sprintf_P(lcdString[0], PSTR(" Connect module"));
        sprintf_P(lcdString[1], PSTR(" Options"));
        break;
      case 1:
        sprintf_P(lcdString[0], PSTR(" Clear errsum"));
        sprintf_P(lcdString[1], PSTR(" Shut down"));
        break;
    }
  } else {
    char fval[7], fval2[6];
    pointer = ~sub & 1;

    if (sub == 1) page = 1;
    if (((sub - 1) >> 1) == page) {
      lcd.setCursor(0, !pointer);
      lcd.print(F(" "));
      lcd.setCursor(0, pointer);
      lcd.print(F(">"));
      return;
    }

    page = (sub - 1) >> 1;
    switch (menu) {
      case 0:
        switch (page) {
          case 0:
            dtostrf((data[1] / 4.0), -6, 2, fval);
            sprintf_P(lcdString[0], PSTR(" Temp: %s"), fval);
            dtostrf((data[2] / 4.0), -5, 2, fval2);
            sprintf_P(lcdString[1], PSTR(" Setpoint: %s"), fval2);
            break;
          case 1:
            dtostrf((data[3] / 16.0), -5, 2, fval2);
            sprintf_P(lcdString[0], PSTR(" Delta: %s"), fval2);
            sprintf_P(lcdString[1], PSTR(" k: %hd"), (int8_t)data[4]);
            break;
          case 2:
            sprintf_P(lcdString[0], PSTR(" ERRSUM: %#x"), data[0]);
            sprintf_P(lcdString[1], PSTR(" errno: %#x"), er0);
            break;
          case 3:
            sprintf_P(lcdString[0], PSTR(" Update"));
            sprintf_P(lcdString[1], PSTR(" Back"));
            break;
        }
        break;
      case 1:
        sprintf_P(lcdString[0], PSTR(" Brightness: %d"), brightness);
        sprintf_P(lcdString[1], PSTR(" Back"));
        break;
    }
  }

  lcdString[pointer][0] = *(PSTR(">"));

  lcd.clear();
  lcd.print(lcdString[0]);
  lcd.setCursor(0, 1);
  lcd.print(lcdString[1]);
}

void connect(void) {
  WAKE_UP_AND_LISTEN(0x01);

  for (uint8_t i = 0; i < 5; i++) {
    data[i] = ds.recv();
    if (ds.errno) er0 |= (1 << i);
    else er0 &= ~(1 << i);
  }

  if (er0 == 0x1F) {
    for (uint8_t i = 0; i < 4; i++) {
      lcd.setCursor(0, 0);
      lcd.print(F("Connection error"));
      delay(250);
      lcd.setCursor(0, 0);
      lcd.print(F("                "));
      delay(250);
    }
    lcd.setCursor(0, 0);
    lcd.print(F(">Connect module"));
    return;
  }

  sub = 1;
  subMax = 8;
  show();
}

void clearErrsum(void) {
  WAKE_UP_AND_LISTEN(0x03);

  lcd.setCursor(0, 1);
  lcd.print(F("Done!     "));
  for (uint8_t i = 0; i < 2; i++) {
    delay(250);
    lcd.setCursor(0, 1);
    lcd.print(F("     "));
    delay(250);
    lcd.setCursor(0, 1);
    lcd.print(F("Done!"));
  }
  delay(500);

  page = 255;
  show();
}

void options(void) {
  sub = 1;
  subMax = 2;

  show();
}

void turnOff(void) {
  while (brightness) {
    analogWrite(9, --brightness);
    delay(3);
  }
  lcd.noDisplay();
}

void back(void) {
  sub = 0;
  page = 255;
  show();
}

void setBrightness(void) {
  delay(250);

  for (;;) {
    if (!digitalRead(DOWN_BTN)) {
      lcd.setCursor(13, 0);
      lcd.print(--brightness);
      lcd.print(F(" "));
      analogWrite(9, brightness);
      delay(50);
    }
    if (!digitalRead(OK_BTN)) {
      if (!brightness) brightness = 1;
      target = brightness;
      return;
    }
  }
}

void newSetpoint(void) {
  delay(250);

  for (;;) {
    if (!digitalRead(DOWN_BTN)) {
      lcd.setCursor(11, 1);
      lcd.print((--data[2]) / 4.0);
      lcd.print(F(" "));
      delay(100);
    }
    if (!digitalRead(OK_BTN)) {
      return;
    }
  }
}

void newDelta(void) {
  delay(250);

  for (;;) {
    if (!digitalRead(DOWN_BTN)) {
      if (!(data[3] -= 4)) data[3] = 252;
      lcd.setCursor(8, 0);
      lcd.print(data[3] / 16.0);
      lcd.print(F(" "));
      delay(100);
    }
    if (!digitalRead(OK_BTN)) {
      return;
    }
  }
}

void newK(void) {
  int8_t k = data[4];

  delay(250);

  for (;;) {
    if (!digitalRead(DOWN_BTN)) {
      if (!(--k)) k = -1;
      lcd.setCursor(4, 1);
      lcd.print(k);
      lcd.print(F(" "));
      delay(100);
    }
    if (!digitalRead(OK_BTN)) {
      data[4] = k;
      return;
    }
  }
}

void confirm(void) {
  WAKE_UP_AND_LISTEN(0x02);

  ds.send(data[2]);
  ds.send(data[3]);
  ds.send(data[4]);

  lcd.setCursor(8, 0);
  lcd.print(F("Done!"));
  for (uint8_t i = 0; i < 2; i++) {
    delay(250);
    lcd.setCursor(8, 0);
    lcd.print(F("     "));
    delay(250);
    lcd.setCursor(8, 0);
    lcd.print(F("Done!"));
  }
}
