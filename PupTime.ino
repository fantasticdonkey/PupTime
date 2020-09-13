#include <string>
#include <Wire.h>
#include <M5StickC.h>
// Refernce to a seperate file holding the 80x80 bitmaps
#include "PawPatrolBitmaps.h"

int ledPin = 10;
// Menu button
int button1Pin = 37;
int button1State = 0;
// Action button
int button2Pin = 39;
int button2State = 0;
// Basic switch debouncing
int lastButtonPressMS = 0;
int lastButtonPressTresholdMS = 1000;
// Timeout for screen bedfore dimming
int screenTimeoutBaselineMS = 0;
int screenTimeoutMS = 20000;
// Start with display 1 (can be toggled with menu button)
int displayScreen = 1;

// Stopwatch values
int stopWatchState = 0;
int stopWatchBaselineMS = 0;
int stopWatchMS = 0;
int stopWatchMSTemp = 0;
int stopWatchSeconds = 0;
int stopWatchMinutes = 0;
int stopWatchHours = 0;
char currentStopWatchHours[16];
char previousStopWatchHours[16];
char currentStopWatchMinutesSeconds[16];
char previousStopWatchMinutesSeconds[16];

int intervalState = 0;
int intervalActiveS = 30;
int intervalActiveCountdownS = intervalActiveS;
int previousIntervalActiveCountdownS;
int intervalActiveMS = 0;
int intervalRestS = 120;
int intervalRestCountdownS = intervalRestS;
int previousIntervalRestCountdownS;
int intervalRestMS = 0;

RTC_TimeTypeDef RTC_TimeStruct;
RTC_DateTypeDef RTC_DateStruct;

char currentDate[16];
char previousDate[16];
char currentTime[16];
char previousTime[16];

float batteryVoltage;
int batteryVoltagePercent;

void firstTimeSetup() {
  // Adjust to current date and time
  // Only needs to be run once
  RTC_TimeTypeDef TimeStruct;
  TimeStruct.Hours = 04;
  TimeStruct.Minutes = 41;
  TimeStruct.Seconds = 30;
  M5.Rtc.SetTime(&TimeStruct);
  RTC_DateTypeDef DateStruct;
  DateStruct.WeekDay = 0;
  DateStruct.Month = 8;
  DateStruct.Date = 23;
  DateStruct.Year = 2020;
  M5.Rtc.SetData(&DateStruct);
}

void displayBattery() {
  // Use voltage range 3.6V - 4.2V
  batteryVoltage = M5.Axp.GetVbatData() * 1.1 / 1000;
  batteryVoltagePercent = ((batteryVoltage - 3.6) / (4.2 - 3.6)) * 100;
  // Draw battery symbol outline
  M5.Lcd.drawRect(81, 4, 30, 15, WHITE);
  M5.Lcd.fillRect(111, 7, 4, 10, WHITE);
  M5.Lcd.drawRect(83, 6, 36, 11, BLACK);
  M5.Lcd.fillRect(85, 8, 5, 7, RED);
  if (batteryVoltagePercent >= 50) {
    M5.Lcd.fillRect(94, 8, 5, 7, ORANGE);
  }
  if (batteryVoltagePercent >= 75) {
    M5.Lcd.fillRect(102, 8, 5, 7, GREEN);
  }
}

void displayWiFi() {
  // Draw Wi-Fi symbol outline
  M5.Lcd.fillRect(125, 12, 6, 7, RED);
  M5.Lcd.fillRect(133, 8, 6, 11, ORANGE);
  M5.Lcd.fillRect(141, 4, 6, 15, GREEN);
}

void displayDateTime() {
  // Function to display date and time on screen 1
  M5.Lcd.setTextColor(WHITE);
  M5.Rtc.GetTime(&RTC_TimeStruct);
  M5.Rtc.GetData(&RTC_DateStruct);
  sprintf(currentDate, "%02d-%02d", RTC_DateStruct.Month, RTC_DateStruct.Date);
  sprintf(currentTime, "%02d:%02d", RTC_TimeStruct.Hours, RTC_TimeStruct.Minutes);
  // Only refresh screen if display date or time has changed
  if ((strcmp(previousDate, currentDate) != 0) || (strcmp(previousTime, currentTime) != 0)){
    M5.Lcd.fillRect(81, 25, 79, 55, BLACK);
    M5.Lcd.setCursor(81, 32);
    M5.Lcd.printf("%s", currentDate);
    M5.Lcd.setCursor(81, 57);
    M5.Lcd.printf("%s", currentTime);
  }  
  strcpy(previousDate, currentDate);
  strcpy(previousTime, currentTime);
}

void displayStopWatch() {
  if (stopWatchState == 0) {
    stopWatchMSTemp = stopWatchMS;
  } else {
    stopWatchMSTemp = ((millis() - stopWatchBaselineMS) / 1000 + stopWatchMS);
  }
  stopWatchHours = stopWatchMSTemp / 3600;                            // Continues to count up the number of hours
  stopWatchMinutes = (stopWatchMSTemp / 60) - (60 * stopWatchHours);  // Caps minutes at 60 minutes and returns to 0
  stopWatchSeconds = stopWatchMSTemp % 60;
  sprintf(currentStopWatchHours, "%02d hr", stopWatchHours);
  sprintf(currentStopWatchMinutesSeconds, "%02d:%02d", stopWatchMinutes, stopWatchSeconds);
  if ((strcmp(previousStopWatchHours, currentStopWatchHours) != 0) || (strcmp(previousStopWatchMinutesSeconds, currentStopWatchMinutesSeconds) != 0)){
    M5.Lcd.fillRect(81, 25, 79, 55, BLACK);
    if (stopWatchState == 0) {
      M5.Lcd.setTextColor(RED);
    } else {
      M5.Lcd.setTextColor(GREEN);
    }
    M5.Lcd.setCursor(81, 32);
    M5.Lcd.printf("%s", currentStopWatchHours);
    M5.Lcd.setCursor(81, 57);
    M5.Lcd.printf("%s", currentStopWatchMinutesSeconds); 
  }
  strcpy(previousStopWatchHours, currentStopWatchHours);
  strcpy(previousStopWatchMinutesSeconds, currentStopWatchMinutesSeconds);  
}

void displayInterval() {
  if ((previousIntervalActiveCountdownS != intervalActiveCountdownS) || (previousIntervalRestCountdownS != intervalRestCountdownS)){
    M5.Lcd.fillRect(81, 25, 79, 55, BLACK);
    if (intervalState == 2) {
      M5.Lcd.fillRect(84, 32, intervalActiveCountdownS * 60 / intervalActiveS, 18, RED);
      M5.Lcd.fillRect(84 + intervalActiveCountdownS * 60 / intervalActiveS, 32, 60 - intervalActiveCountdownS * 60 / intervalActiveS, 18, WHITE);
      //M5.Lcd.fillRect(84, 57, intervalActiveCountdownS * 60 / intervalRestS, 18, GREEN);
      M5.Lcd.setCursor(84, 57);
      M5.Lcd.setTextColor(RED);
      M5.Lcd.printf("%ds", intervalActiveCountdownS);
    } else {
      M5.Lcd.fillRect(84, 32, intervalRestCountdownS * 60 / intervalRestS, 18, GREEN);
      M5.Lcd.fillRect(84 + intervalRestCountdownS * 60 / intervalRestS, 32, 60 - intervalRestCountdownS * 60 / intervalRestS, 18, WHITE);
      //M5.Lcd.fillRect(84, 57, intervalActiveCountdownS * 60 / intervalActiveS, 18, RED);
      M5.Lcd.setCursor(84, 57);
      M5.Lcd.setTextColor(GREEN);
      M5.Lcd.printf("%ds", intervalRestCountdownS);
    }
  }
  previousIntervalActiveCountdownS = intervalActiveCountdownS;
  previousIntervalRestCountdownS = intervalRestCountdownS;
}

void calculateInterval() {
  if (intervalState >= 1) {
    if (intervalRestCountdownS > 0) {
      Serial.println(intervalRestCountdownS);
      intervalRestCountdownS = ((intervalRestS * 1000) - (millis() - intervalRestMS)) / 1000;
    } else {
      if (intervalState == 1) {
        intervalState = 2;
        intervalActiveMS = millis();
      }
      if (intervalActiveCountdownS > 0) {
        Serial.println(intervalActiveS);
        Serial.println(intervalActiveMS);
        Serial.println(intervalActiveCountdownS);
        intervalActiveCountdownS = ((intervalActiveS * 1000) - (millis() - intervalActiveMS)) / 1000;
      } else {
        intervalState = 1;
        intervalActiveCountdownS = intervalActiveS;
        intervalRestCountdownS = intervalRestS;
        intervalRestMS = millis();
      }
    }
  }
}

void displayOnScreen(const uint16_t *bitmap) {
  // Draw 80x80 image on screen
  int i = 0;
  for (int y = 1; y <= 80; y++) {
    for (int x = 1; x <= 80; x++) {
      M5.Lcd.drawPixel(x, y, bitmap[i]);
      i++;
    }
  }
}

void displayScreen1() {
  displayBattery();
  displayWiFi();
  displayDateTime();
  button2State = digitalRead(button2Pin);
  if ((button2State == LOW) && ((millis() - lastButtonPressMS) > lastButtonPressTresholdMS)) {
    lastButtonPressMS = millis();
    screenTimeoutBaselineMS = millis();
    M5.Axp.ScreenBreath(10);
  }
}

void displayScreen2() {
  displayBattery();
  displayWiFi();
  displayStopWatch();
  button2State = digitalRead(button2Pin);
  if ((button2State == LOW) && ((millis() - lastButtonPressMS) > lastButtonPressTresholdMS)) {
    lastButtonPressMS = millis();
    if (stopWatchState == 0) {
      stopWatchBaselineMS = millis();
      stopWatchState = 1;
    } else {
      stopWatchMS = ((millis() - stopWatchBaselineMS) / 1000) + stopWatchMS;
      stopWatchState = 0;
    }
    screenTimeoutBaselineMS = millis();
    M5.Axp.ScreenBreath(10);
  }
}

void displayScreen3() {
  displayBattery();
  displayWiFi();
  displayInterval();
  button2State = digitalRead(button2Pin);
  if ((button2State == LOW) && ((millis() - lastButtonPressMS) > lastButtonPressTresholdMS)) {
    lastButtonPressMS = millis();
    if (intervalState == 0) {
      intervalActiveCountdownS = intervalActiveS;
      intervalRestCountdownS = intervalRestS;
      intervalRestMS = millis();
      intervalState = 1;
    } else {
      intervalState = 0;
    }
    screenTimeoutBaselineMS = millis();
    M5.Axp.ScreenBreath(10);
  }
}

void setup() {
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  pinMode(button1Pin, INPUT);
  pinMode(button2Pin, INPUT);
  M5.begin();
  M5.Lcd.setRotation(3);
  M5.Lcd.setTextSize(2);
  // Only include the following code when setting up the time initially
  // and adjust to actual date and time during first run.
  //firstTimeSetup();
  screenTimeoutBaselineMS = millis();
  M5.Axp.ScreenBreath(10);
  displayOnScreen(trackerBitmap);
}

void loop() {
  button1State = digitalRead(button1Pin);
  // Check if button 1 is pressed, and cycle through displays
  if ((button1State == LOW) && ((millis() - lastButtonPressMS) > lastButtonPressTresholdMS)) {
    lastButtonPressMS = millis();
    if (displayScreen == 1) {
      // Transition from display 1 to display 2
      memset(previousDate, 0, sizeof(previousDate));
      memset(previousTime, 0, sizeof(previousTime));
      displayOnScreen(zumaBitmap);
      displayScreen++;
    } else if (displayScreen == 2) {
      // Transition from display 2 to display 3
      memset(previousStopWatchHours, 0, sizeof(previousStopWatchHours));
      memset(previousStopWatchMinutesSeconds, 0, sizeof(previousStopWatchMinutesSeconds));
      displayOnScreen(rockyBitmap);
      displayScreen++;
    } else if (displayScreen == 3) {
      // Transition from display 3 back to display 1
      previousIntervalActiveCountdownS = -1;
      previousIntervalRestCountdownS = -1;
      displayOnScreen(trackerBitmap);
      displayScreen = 1;
    }
    screenTimeoutBaselineMS = millis();
    M5.Axp.ScreenBreath(10);
  }
  if (displayScreen == 1) {
    displayScreen1();
  }
  if (displayScreen == 2) {
    displayScreen2();
  }
  if (displayScreen == 3) {
    displayScreen3();
  }
  // Timeout and dim screen if inactive
  if ((millis() - screenTimeoutBaselineMS) > screenTimeoutMS) {
    M5.Axp.ScreenBreath(7);
  }
  // If interval training, track current countdowns
  calculateInterval();
  delay(5);
}
