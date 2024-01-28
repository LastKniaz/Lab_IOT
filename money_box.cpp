#define COIN_AMOUNT 5      
float coinValue[COIN_AMOUNT] = {0.2, 0.5, 1.0, 2.0, 5.0};
String currency = "PLN";
int standbyTime = 10000;   

int coinSignal[COIN_AMOUNT];
int coinQuantity[COIN_AMOUNT];
byte emptySignal;
unsigned long standbyTimer, resetTimer;
float totalMoney = 0;

#include "LowPower.h"
#include "EEPROMex.h"
#include "LCD_1602_POL.h"

LCD_1602_POL lcd(0x27, 16, 2);

boolean recognitionFlag, sleepFlag = true;
#define WAKE_BUTTON 2      
#define CALIBR_BUTTON 3     
#define DISP_POWER 12      
#define LED_PIN 11         
#define IR_PIN 17          
#define IR_SENSOR 14        
int sensorSignal, lastSensorSignal;
boolean coinFlag = false;

void setup() {
  Serial.begin(9600);
  delay(500);

  pinMode(WAKE_BUTTON, INPUT_PULLUP);
  pinMode(CALIBR_BUTTON, INPUT_PULLUP);

  pinMode(DISP_POWER, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(IR_PIN, OUTPUT);

  digitalWrite(DISP_POWER, HIGH);
  digitalWrite(LED_PIN, HIGH);
  digitalWrite(IR_PIN, HIGH);

  attachInterrupt(0, wakeUp, CHANGE);

  emptySignal = analogRead(IR_SENSOR);

  // Initialize the display
  lcd.init();
  lcd.backlight();

  if (!digitalRead(CALIBR_BUTTON)) {
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print(L"Service");
    delay(500);
    resetTimer = millis();
    while (1) {
      if (millis() - resetTimer > 3000) {
        for (byte i = 0; i < COIN_AMOUNT; i++) {
          coinQuantity[i] = 0;
          EEPROM.writeInt(20 + i * 2, 0);
        }
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(L"Memory cleared");
        delay(100);
      }
      if (digitalRead(CALIBR_BUTTON)) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(L"Calibration");
        break;
      }
    }
    while (1) {
      for (byte i = 0; i < COIN_AMOUNT; i++) {
        lcd.setCursor(0, 1); lcd.print(coinValue[i]);
        lcd.setCursor(13, 1); lcd.print(currency);
        lastSensorSignal = emptySignal;
        while (1) {
          sensorSignal = analogRead(IR_SENSOR);
          if (sensorSignal > lastSensorSignal) lastSensorSignal = sensorSignal;
          if (sensorSignal - emptySignal > 3) coinFlag = true;
          if (coinFlag && (abs(sensorSignal - emptySignal)) < 2) {
            coinSignal[i] = lastSensorSignal;
            EEPROM.writeInt(i * 2, coinSignal[i]);
            coinFlag = false;
            break;
          }
        }
      }
      break;
    }
  }

  for (byte i = 0; i < COIN_AMOUNT; i++) {
    coinSignal[i] = EEPROM.readInt(i * 2);
    coinQuantity[i] = EEPROM.readInt(20 + i * 2);
    totalMoney += coinQuantity[i] * coinValue[i];
  }
  standbyTimer = millis();
}

void loop() {
  if (sleepFlag) {
    delay(500);
    lcd.init();
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print(L"Lamborghini");
    lcd.setCursor(0, 1); lcd.print(totalMoney);
    lcd.setCursor(13, 1); lcd.print(currency);
    emptySignal = analogRead(IR_SENSOR);
    sleepFlag = false;
  }

  lastSensorSignal = emptySignal;
  while (1) {
    sensorSignal = analogRead(IR_SENSOR);
    if (sensorSignal > lastSensorSignal) lastSensorSignal = sensorSignal;
    if (sensorSignal - emptySignal > 3) coinFlag = true;
    if (coinFlag && (abs(sensorSignal - emptySignal)) < 2) {
      recognitionFlag = false;
      for (byte i = 0; i < COIN_AMOUNT; i++) {
        int delta = abs(lastSensorSignal - coinSignal[i]);
        if (delta < 30) {
          totalMoney += coinValue[i];
          lcd.setCursor(0, 1); lcd.print(totalMoney);
          coinQuantity[i]++;
          recognitionFlag = true;
          break;
        }
      }
      coinFlag = false;
      standbyTimer = millis();
      break;
    }

    if (millis() - standbyTimer > standbyTime) {
      goodNight();
      break;
    }

    while (!digitalRead(WAKE_BUTTON)) {
      if (millis() - standbyTimer > 2000) {
        lcd.clear();
        for (byte i = 0; i < COIN_AMOUNT; i++) {
          lcd.setCursor(i * 3, 0); lcd.print((int)coinValue[i]);
          lcd.setCursor(i * 3, 1); lcd.print(coinQuantity[i]);
        }
      }
    }
  }
}

void goodNight() {
  for (byte i = 0; i < COIN_AMOUNT; i++) {
    EEPROM.updateInt(20 + i * 2, coinQuantity[i]);
  }
  sleepFlag = true;
  digitalWrite(DISP_POWER, LOW);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(IR_PIN, LOW);
  delay(100);
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
}

void wakeUp() {
  digitalWrite(DISP_POWER, HIGH);
  digitalWrite(LED_PIN, HIGH);
  digitalWrite(IR_PIN, HIGH);
  standbyTimer = millis();
}
