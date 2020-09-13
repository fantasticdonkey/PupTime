#include <string>
#include <Wire.h>
#include <M5StickC.h>
#include "PupTimeBitmaps.h" // Reference to a separate file holding the 80x80 bitmaps

int ledPin = 10;                    // M5StickC GPIO pin of red LED, 0 (LOW) is LED on
int ledLastChange = 0;              // When the LED was last turned on
int ledStatus = 0;                  // LED on or off (0=off, 1=on)
int ledChangeDuration = 1000;       // Duration the LED should be on and off for during standby - ms

int buttonMenuPin = 37;             // M5StickC GPIO pin of button 1
int buttonMenuStatus = 1;           // 0 (LOW) is pressed
int buttonActionPin = 39;           // M5StickC GPIO pin of button 1
int buttonActionStatus = 1;         // 0 (LOW) is pressed
int buttonLastPress = 0;            // Last time any button was pressed
int buttonLastPressDuration = 1000; // Used to filter multiple button presses (de-bouncing) - ms

int screenLastActivated = 0;        // Last time screen was re-activated after timeout
int screenTimeoutDuration = 20000;  // Duration after which screen is dimmed - ms
int screenCurrentDisplay = 1;       // Current display on the screen (1=clock (default), 2=timer, 3=interval)

RTC_TimeTypeDef RTC_TimeStruct;
RTC_DateTypeDef RTC_DateStruct;

char watchDisplayLine1Now[16];            // Current line 1 of watch screen
char watchDisplayLine1Previous[16];       // Previous line 1 of watch screen (used to determine if refresh is required)
char watchDisplayLine2Now[16];            // Current line 2 of watch screen
char watchDisplayLine2Previous[16];       // Previous line 2 of watch screen (used to determine if refresh is required)

int stopWatchRunningStatus = 0;           // 0 if stopwatch is stopped, 1 if running
int stopWatchLastStart = 0;               // Last time stopwatch was started
int stopWatchTotal = 0;                   // Total time on stopwatch
int stopWatchTotalDisplay = 0;            // Total time to be displayed on stopwatch
int stopWatchTotalDisplayHours = 0;       // Total time to be displayed on stopwatch (hours)
int stopWatchTotalDisplayMinutes = 0;     // Total time to be displayed on stopwatch (minutes)
int stopWatchTotalDisplaySeconds = 0;     // Total time to be displayed on stopwatch (seconds)
char stopWatchDisplayLine1Now[16];        // Current line 1 of stop watch screen
char stopWatchDisplayLine1Previous[16];   // Previous line 1 of stop watch screen (used to determine if refresh is required)
char stopWatchDisplayLine2Now[16];        // Current line 2 of stop watch screen
char stopWatchDisplayLine2Previous[16];   // Previous line 2 of stop watch screen (used to determine if refresh is required)

int intervalRunningStatus = 0;                            // Phase of the interval training (0=stopped, 1=rest, 2=active)
int intervalRestDuration = 120;                           // Duration the rest phase lasts for - s
int intervalRestRemainingTime = intervalRestDuration;     // Remaining time of the rest phase - s
int intervalRestRemainingTimePrevious;                    // Previous remaining time of the rest phase (info used for screen refresh) - s
int intervalRestLastStart = 0;                            // Time rest phase started
int intervalActiveDuration = 30;                          // Duration the active phase lasts for - s
int intervalActiveRemainingTime = intervalActiveDuration; // Remaining time of the active phase - s
int intervalActiveRemainingTimePrevious;                  // Previous remaining time of the active phase (info used for screen refresh) - s
int intervalActiveLastStart = 0;                          // Time active phase started

float batteryVoltage;         // Current battery voltage
int batteryVoltagePercent;    // Current battery voltage, in percentage

void firstTimeSetup() {
  // Set RTC with current date and time (one-time only)
  RTC_TimeTypeDef TimeStruct;
  TimeStruct.Hours = 04;          // Change to current value
  TimeStruct.Minutes = 41;        // Change to current value
  TimeStruct.Seconds = 30;        // Change to current value
  M5.Rtc.SetTime(&TimeStruct);
  RTC_DateTypeDef DateStruct;
  DateStruct.WeekDay = 0;         // Change to current value
  DateStruct.Month = 8;           // Change to current value
  DateStruct.Date = 23;           // Change to current value
  DateStruct.Year = 2020;         // Change to current value
  M5.Rtc.SetData(&DateStruct);
}

void displayBattery() {
  // Display battery utilisation icon on screen
  // Use voltage reference range of 3.6V - 4.2V
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
  // Draw Wi-Fi symbol outline on screen
  // TODO: Make bars reflect actual WiFi RSSI
  M5.Lcd.fillRect(125, 12, 6, 7, RED);
  M5.Lcd.fillRect(133, 8, 6, 11, ORANGE);
  M5.Lcd.fillRect(141, 4, 6, 15, GREEN);
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

void displayDateTime() {
  // Display watch (date and time) on display 1
  M5.Lcd.setTextColor(WHITE);
  M5.Rtc.GetTime(&RTC_TimeStruct);
  M5.Rtc.GetData(&RTC_DateStruct);
  sprintf(watchDisplayLine1Now, "%02d-%02d", RTC_DateStruct.Month, RTC_DateStruct.Date);
  sprintf(watchDisplayLine2Now, "%02d:%02d", RTC_TimeStruct.Hours, RTC_TimeStruct.Minutes);
  // Only refresh screen if display date or time has changed
  if ((strcmp(watchDisplayLine1Previous, watchDisplayLine1Now) != 0) || (strcmp(watchDisplayLine2Previous, watchDisplayLine2Now) != 0)){
    M5.Lcd.fillRect(81, 25, 79, 55, BLACK);
    M5.Lcd.setCursor(81, 32);
    M5.Lcd.printf("%s", watchDisplayLine1Now);
    M5.Lcd.setCursor(81, 57);
    M5.Lcd.printf("%s", watchDisplayLine2Now);
  }  
  strcpy(watchDisplayLine1Previous, watchDisplayLine1Now);
  strcpy(watchDisplayLine2Previous, watchDisplayLine2Now);
}

void displayStopWatch() {
  // Display stopwatch on display 2
  if (stopWatchRunningStatus == 0) {
    stopWatchTotalDisplay = stopWatchTotal;
  } else {
    stopWatchTotalDisplay = ((millis() - stopWatchLastStart) / 1000 + stopWatchTotal);
  }
  stopWatchTotalDisplayHours = stopWatchTotalDisplay / 3600;
  stopWatchTotalDisplayMinutes = (stopWatchTotalDisplay / 60) - (60 * stopWatchTotalDisplayHours);  // Caps minutes at 60 minutes and returns to 0
  stopWatchTotalDisplaySeconds = stopWatchTotalDisplay % 60;
  sprintf(stopWatchDisplayLine1Now, "%02d hr", stopWatchTotalDisplayHours);
  sprintf(stopWatchDisplayLine2Now, "%02d:%02d", stopWatchTotalDisplayMinutes, stopWatchTotalDisplaySeconds);
  // Only refresh screen if stop watch values have changed
  if ((strcmp(stopWatchDisplayLine1Previous, stopWatchDisplayLine1Now) != 0) || (strcmp(stopWatchDisplayLine2Previous, stopWatchDisplayLine2Now) != 0)){
    M5.Lcd.fillRect(81, 25, 79, 55, BLACK);
    if (stopWatchRunningStatus == 0) {
      M5.Lcd.setTextColor(RED);
    } else {
      M5.Lcd.setTextColor(GREEN);
    }
    M5.Lcd.setCursor(81, 32);
    M5.Lcd.printf("%s", stopWatchDisplayLine1Now);
    M5.Lcd.setCursor(81, 57);
    M5.Lcd.printf("%s", stopWatchDisplayLine2Now); 
  }
  strcpy(stopWatchDisplayLine1Previous, stopWatchDisplayLine1Now);
  strcpy(stopWatchDisplayLine2Previous, stopWatchDisplayLine2Now);  
}

void displayInterval() {
  // Display interval training on display 3
  // Only refresh screen if interval countdown values have changed
  if ((intervalActiveRemainingTimePrevious != intervalActiveRemainingTime) || (intervalRestRemainingTimePrevious != intervalRestRemainingTime)){
    M5.Lcd.fillRect(81, 25, 79, 55, BLACK);
    if (intervalRunningStatus == 2) {
      M5.Lcd.fillRect(84, 32, intervalActiveRemainingTime * 60 / intervalActiveDuration, 18, RED);
      M5.Lcd.fillRect(84 + intervalActiveRemainingTime * 60 / intervalActiveDuration, 32, 60 - intervalActiveRemainingTime * 60 / intervalActiveDuration, 18, WHITE);
      M5.Lcd.setCursor(84, 57);
      M5.Lcd.setTextColor(RED);
      M5.Lcd.printf("%ds", intervalActiveRemainingTime);
    } else {
      M5.Lcd.fillRect(84, 32, intervalRestRemainingTime * 60 / intervalRestDuration, 18, GREEN);
      M5.Lcd.fillRect(84 + intervalRestRemainingTime * 60 / intervalRestDuration, 32, 60 - intervalRestRemainingTime * 60 / intervalRestDuration, 18, WHITE);
      M5.Lcd.setCursor(84, 57);
      M5.Lcd.setTextColor(GREEN);
      M5.Lcd.printf("%ds", intervalRestRemainingTime);
    }
  }
  intervalActiveRemainingTimePrevious = intervalActiveRemainingTime;
  intervalRestRemainingTimePrevious = intervalRestRemainingTime;
}

void calculateInterval() {
  // If interval training is running, track the phase it is in, and remaining time
  if (intervalRunningStatus >= 1) {
    if (intervalRestRemainingTime > 0) {
      intervalRestRemainingTime = ((intervalRestDuration * 1000) - (millis() - intervalRestLastStart)) / 1000;
    } else {
      if (intervalRunningStatus == 1) {
        intervalRunningStatus = 2;
        intervalActiveLastStart = millis();
      }
      if (intervalActiveRemainingTime > 0) {
        intervalActiveRemainingTime = ((intervalActiveDuration * 1000) - (millis() - intervalActiveLastStart)) / 1000;
      } else {
        intervalRunningStatus = 1;
        intervalActiveRemainingTime = intervalActiveDuration;
        intervalRestRemainingTime = intervalRestDuration;
        intervalRestLastStart = millis();
      }
    }
  }
}

void changeLED() {
  // Alternate the LED on and off if interval training
  if (intervalRunningStatus > 0) {
    if ((millis() - ledLastChange) > ledChangeDuration) {
      if (ledStatus == 0) {
        digitalWrite(ledPin, LOW);
        ledStatus = 1;
      } else if (ledStatus == 1) {
        digitalWrite(ledPin, HIGH);
        ledStatus = 0;
      }
      ledLastChange = millis();
    }
  } else {
    if (ledStatus == 1) {
      digitalWrite(ledPin, HIGH);
      ledStatus = 0;
    }
  }
}

void displayScreen1() {
  // Display watch screen
  displayBattery();
  displayWiFi();  // TODO: Currently always shows Wi-Fi as connected
  displayDateTime();
  buttonActionStatus = digitalRead(buttonActionPin);
  if ((buttonActionStatus == LOW) && ((millis() - buttonLastPress) > buttonLastPressDuration)) {
    buttonLastPress = millis();
    screenLastActivated = millis();
    M5.Axp.ScreenBreath(10);
  }
}

void displayScreen2() {
  // Display stopwatch screen
  displayBattery();
  displayWiFi();    // TODO: Currently always shows Wi-Fi as connected
  displayStopWatch();
  buttonActionStatus = digitalRead(buttonActionPin);
  if ((buttonActionStatus == LOW) && ((millis() - buttonLastPress) > buttonLastPressDuration)) {
    buttonLastPress = millis();
    if (stopWatchRunningStatus == 0) {
      stopWatchLastStart = millis();
      stopWatchRunningStatus = 1;
    } else {
      stopWatchTotal = ((millis() - stopWatchLastStart) / 1000) + stopWatchTotal;
      stopWatchRunningStatus = 0;
    }
    screenLastActivated = millis();
    M5.Axp.ScreenBreath(10);
  }
}

void displayScreen3() {
  // Display interval training screen
  displayBattery();
  displayWiFi();  // TODO: Currently always shows Wi-Fi as connected
  displayInterval();
  buttonActionStatus = digitalRead(buttonActionPin);
  if ((buttonActionStatus == LOW) && ((millis() - buttonLastPress) > buttonLastPressDuration)) {
    buttonLastPress = millis();
    if (intervalRunningStatus == 0) {
      intervalActiveRemainingTime = intervalActiveDuration;
      intervalRestRemainingTime = intervalRestDuration;
      intervalRestLastStart = millis();
      intervalRunningStatus = 1;
    } else {
      intervalRunningStatus = 0;
    }
    screenLastActivated = millis();
    M5.Axp.ScreenBreath(10);
  }
}

void setup() {
  // One-time setup
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  pinMode(buttonMenuPin, INPUT);
  pinMode(buttonActionPin, INPUT);
  M5.begin();
  M5.Lcd.setRotation(3);
  M5.Lcd.setTextSize(2);
  // Only include the following code when setting up the time initially
  // and adjust to actual date and time during first run.
  //firstTimeSetup();
  screenLastActivated = millis();
  M5.Axp.ScreenBreath(10);
  displayOnScreen(trackerBitmap);
}

void loop() {
  // Application loop
  buttonMenuStatus = digitalRead(buttonMenuPin);
  // Check if menu button is pressed, and cycle through displays
  if ((buttonMenuStatus == LOW) && ((millis() - buttonLastPress) > buttonLastPressDuration)) {
    buttonLastPress = millis();
    if (screenCurrentDisplay == 1) {
      // Transition from display 1 to display 2
      memset(watchDisplayLine1Previous, 0, sizeof(watchDisplayLine1Previous));
      memset(watchDisplayLine2Previous, 0, sizeof(watchDisplayLine2Previous));
      displayOnScreen(zumaBitmap);
      screenCurrentDisplay++;
    } else if (screenCurrentDisplay == 2) {
      // Transition from display 2 to display 3
      memset(stopWatchDisplayLine1Previous, 0, sizeof(stopWatchDisplayLine1Previous));
      memset(stopWatchDisplayLine2Previous, 0, sizeof(stopWatchDisplayLine2Previous));
      displayOnScreen(rockyBitmap);
      screenCurrentDisplay++;
    } else if (screenCurrentDisplay == 3) {
      // Transition from display 3 back to display 1
      intervalActiveRemainingTimePrevious = -1;
      intervalRestRemainingTimePrevious = -1;
      displayOnScreen(trackerBitmap);
      screenCurrentDisplay = 1;
    }
    screenLastActivated = millis();
    M5.Axp.ScreenBreath(10);
  }
  if (screenCurrentDisplay == 1) {
    displayScreen1();
  }
  if (screenCurrentDisplay == 2) {
    displayScreen2();
  }
  if (screenCurrentDisplay == 3) {
    displayScreen3();
  }
  // Timeout and dim screen if inactive
  if ((millis() - screenLastActivated) > screenTimeoutDuration) {
    M5.Axp.ScreenBreath(7);
  }
  // If interval training, track current countdowns
  calculateInterval();
  changeLED();
  delay(5);
}
