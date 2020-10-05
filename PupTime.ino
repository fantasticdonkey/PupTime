#include <string>
#include <Wire.h>
#include <M5StickC.h>
#include "PupTimeBitmaps1.h"        // Reference to a separate file holding the 80x80 bitmaps (change for different bitmaps)

unsigned char ledPin = 10;          // M5StickC GPIO pin of red LED, 0 (LOW) is LED on
int ledLastChange = 0;              // When the LED was last turned on
bool ledStatus = false;             // LED on or off (false=off, true=on)
int ledChangeDuration = 1000;       // Duration the LED should be on and off for during standby - ms

unsigned char buttonMenuPin = 37;   // M5StickC GPIO pin of button 1
int buttonMenuStatus = 1;           // 0 (LOW) is pressed
unsigned char buttonActionPin = 39; // M5StickC GPIO pin of button 1
int buttonActionStatus = 1;         // 0 (LOW) is pressed
int buttonLastPress = 0;            // Last time any button was pressed
int buttonLastPressDuration = 1000; // Used to filter multiple button presses (de-bouncing) - ms

int screenLastActivated = 0;                  // Last time screen was re-activated after timeout
int screenTimeoutDuration = 20000;            // Duration after which screen is dimmed - ms
unsigned char screenCurrentDisplay = 1;       // Current display on the screen (1=clock (default), 2=timer, 3=interval, 4=MPU)
bool screenState = true;                      // Screen state (false=standby, true=on)

RTC_TimeTypeDef RTC_TimeStruct;
RTC_DateTypeDef RTC_DateStruct;

char watchDisplayLine1Now[16];            // Current line 1 of watch screen
char watchDisplayLine1Previous[16];       // Previous line 1 of watch screen (used to determine if refresh is required)
char watchDisplayLine2Now[16];            // Current line 2 of watch screen
char watchDisplayLine2Previous[16];       // Previous line 2 of watch screen (used to determine if refresh is required)

bool stopWatchRunningStatus = false;      // Stopwatch false if stopped, true if running
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

unsigned char intervalRunningStatus = 0;                  // Phase of the interval training (0=stopped, 1=rest, 2=active)
int intervalRestDuration = 120;                           // Duration the rest phase lasts for - s
int intervalRestRemainingTime = intervalRestDuration;     // Remaining time of the rest phase - s
int intervalRestRemainingTimePrevious;                    // Previous remaining time of the rest phase (info used for screen refresh) - s
int intervalRestLastStart = 0;                            // Time rest phase started
int intervalActiveDuration = 30;                          // Duration the active phase lasts for - s
int intervalActiveRemainingTime = intervalActiveDuration; // Remaining time of the active phase - s
int intervalActiveRemainingTimePrevious;                  // Previous remaining time of the active phase (info used for screen refresh) - s
int intervalActiveLastStart = 0;                          // Time active phase started

float accelerometerX = 0;       // Accelerometer x
float accelerometerY = 0;       // Accelerometer y
float accelerometerZ = 0;       // Accelerometer z
int accelerometerXPixels = 0;   // Accelerometer x (in screen pixels)
int accelerometerYPixels = 0;   // Accelerometer y (in screen pixels)
int accelerometerZPixels = 0;   // Accelerometer z (in screen pixels)
float gyroscopeX = 0;           // Gyroscope x
float gyroscopeY = 0;           // Gyroscope y
float gyroscopeZ = 0;           // Gyroscope z
float gyroscopeXPixels = 0;     // Gyroscope x (in screen pixels)
float gyroscopeYPixels = 0;     // Gyroscope y (in screen pixels)
float gyroscopeZPixels = 0;     // Gyroscope z (in screen pixels)

int mpuDisplayStatus = 0;   // Display accelerometer (0) or gyroscope (1) readings

float batteryVoltage;         // Current battery voltage
int batteryVoltagePercent;    // Current battery voltage, in percentage

void firstTimeSetup() {
  // Set RTC with current date and time (one-time only)
  RTC_TimeTypeDef TimeStruct;
  TimeStruct.Hours = 22;          // Change to current value
  TimeStruct.Minutes = 47;        // Change to current value
  TimeStruct.Seconds = 30;        // Change to current value
  M5.Rtc.SetTime(&TimeStruct);
  RTC_DateTypeDef DateStruct;
  DateStruct.WeekDay = 7;         // Change to current value
  DateStruct.Month = 10;           // Change to current value
  DateStruct.Date = 4;           // Change to current value
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
  if (stopWatchRunningStatus == false) {
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
    if (stopWatchRunningStatus == false) {
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

void displayMPU() {
  // Display MPU readings on display 4
  M5.Lcd.fillRect(91, 25, 69, 55, BLACK);
  if (mpuDisplayStatus == 0) {
    // Scale values to fit space (scaled to +/- 1200)
    accelerometerXPixels = (int) (accelerometerX * 1000 * 35 / 1200);
    if (accelerometerXPixels > 35) {
      accelerometerXPixels = 35;
    } else if (accelerometerXPixels < -35) {
      accelerometerXPixels = -35;
    }
    accelerometerYPixels = (int) (accelerometerY * 1000 * 35 / 1200);
    if (accelerometerYPixels > 35) {
      accelerometerYPixels = 35;
    } else if (accelerometerYPixels < -35) {
      accelerometerYPixels = -35;
    }
    accelerometerZPixels = (int) (accelerometerZ * 1000 * 35 / 1200);
    if (accelerometerZPixels > 35) {
      accelerometerZPixels = 35;
    } else if (accelerometerZPixels < -35) {
      accelerometerZPixels = -35;
    }
    if (accelerometerXPixels > 0) {
      M5.Lcd.fillRect(127, 25, accelerometerXPixels, 13, RED);
    } else {
      M5.Lcd.fillRect(127 + accelerometerXPixels, 25, -accelerometerXPixels, 13, RED);
    }
    if (accelerometerYPixels > 0) {
      M5.Lcd.fillRect(127, 40, accelerometerYPixels, 13, ORANGE);
    } else {
      M5.Lcd.fillRect(127 + accelerometerYPixels, 40, -accelerometerYPixels, 13, ORANGE);
    }
    if (accelerometerZPixels > 0) {
      M5.Lcd.fillRect(127, 55, accelerometerZPixels, 13, GREEN);
    } else {
      M5.Lcd.fillRect(127 + accelerometerZPixels, 55, -accelerometerZPixels, 13, GREEN);
    }
  } else if (mpuDisplayStatus == 1) {
    // Scale values to fit space (scaled to +/- 1200)
    gyroscopeXPixels = (int) (gyroscopeX * 35 / 1200);
    if (gyroscopeXPixels > 35) {
      gyroscopeXPixels = 35;
    } else if (gyroscopeXPixels < -35) {
      gyroscopeXPixels = -35;
    }
    gyroscopeYPixels = (int) (gyroscopeY * 35 / 1200);
    if (gyroscopeYPixels > 35) {
      gyroscopeYPixels = 35;
    } else if (gyroscopeYPixels < -35) {
      gyroscopeYPixels = -35;
    }
    gyroscopeZPixels = (int) (gyroscopeZ * 35 / 1200);
    if (gyroscopeZPixels > 35) {
      gyroscopeZPixels = 35;
    } else if (gyroscopeZPixels < -35) {
      gyroscopeZPixels = -35;
    }
    if (gyroscopeXPixels > 0) {
      M5.Lcd.fillRect(127, 25, gyroscopeXPixels, 13, RED);
    } else {
      M5.Lcd.fillRect(127 + gyroscopeXPixels, 25, -gyroscopeXPixels, 13, RED);
    }
    if (gyroscopeYPixels > 0) {
      M5.Lcd.fillRect(127, 40, gyroscopeYPixels, 13, ORANGE);
    } else {
      M5.Lcd.fillRect(127 + gyroscopeYPixels, 40, -gyroscopeYPixels, 13, ORANGE);
    }
    if (gyroscopeZPixels > 0) {
      M5.Lcd.fillRect(127, 55, gyroscopeZPixels, 13, GREEN);
    } else {
      M5.Lcd.fillRect(127 + gyroscopeZPixels, 55, -gyroscopeZPixels, 13, GREEN);
    }
  }
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

void calculateMPU() {
  M5.MPU6886.getAccelData(&accelerometerX, &accelerometerY, &accelerometerZ);
  M5.MPU6886.getGyroData(&gyroscopeX, &gyroscopeY, &gyroscopeZ);
}

void changeLED() {
  // Alternate the LED on and off if interval training
  if (intervalRunningStatus > 0) {
    if ((millis() - ledLastChange) > ledChangeDuration) {
      if (ledStatus == false) {
        digitalWrite(ledPin, LOW);
        ledStatus = true;
      } else if (ledStatus == true) {
        digitalWrite(ledPin, HIGH);
        ledStatus = false;
      }
      ledLastChange = millis();
    }
  } else {
    if (ledStatus == true) {
      digitalWrite(ledPin, HIGH);
      ledStatus = false;
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
    if (stopWatchRunningStatus == false) {
      stopWatchLastStart = millis();
      stopWatchRunningStatus = true;
    } else {
      stopWatchTotal = ((millis() - stopWatchLastStart) / 1000) + stopWatchTotal;
      stopWatchRunningStatus = false;
      // Force change in text colour if stopped
      memset(stopWatchDisplayLine1Previous, 0, sizeof(stopWatchDisplayLine1Previous));
      memset(stopWatchDisplayLine2Previous, 0, sizeof(stopWatchDisplayLine2Previous));
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

void displayScreen4() {
  // Display MPU screen
  displayBattery();
  displayWiFi();  // TODO: Currently always shows Wi-Fi as connected
  displayMPU();
  buttonActionStatus = digitalRead(buttonActionPin);
  if ((buttonActionStatus == LOW) && ((millis() - buttonLastPress) > buttonLastPressDuration)) {
    buttonLastPress = millis();
    if (mpuDisplayStatus == 0) {
      mpuDisplayStatus = 1;
    } else if (mpuDisplayStatus == 1) {
      mpuDisplayStatus = 0;
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
  // firstTimeSetup();
  screenLastActivated = millis();
  M5.Axp.ScreenBreath(10);
  displayOnScreen(bitmapDisplay1);
  // Initialise IMU
  M5.MPU6886.Init();
}

void loop() {
  // Application loop
  buttonMenuStatus = digitalRead(buttonMenuPin);
  // Check if menu button is pressed, and cycle through displays
  if ((buttonMenuStatus == LOW) && ((millis() - buttonLastPress) > buttonLastPressDuration)) {
    buttonLastPress = millis();
    if (screenState == true) {
      if (screenCurrentDisplay == 1) {
        // Transition from display 1 to display 2
        memset(watchDisplayLine1Previous, 0, sizeof(watchDisplayLine1Previous));
        memset(watchDisplayLine2Previous, 0, sizeof(watchDisplayLine2Previous));
        displayOnScreen(bitmapDisplay2);
        screenCurrentDisplay++;
      } else if (screenCurrentDisplay == 2) {
        // Transition from display 2 to display 3
        memset(stopWatchDisplayLine1Previous, 0, sizeof(stopWatchDisplayLine1Previous));
        memset(stopWatchDisplayLine2Previous, 0, sizeof(stopWatchDisplayLine2Previous));
        displayOnScreen(bitmapDisplay3);
        screenCurrentDisplay++;
      } else if (screenCurrentDisplay == 3) {
        // Transition from display 3 to display 4
        intervalActiveRemainingTimePrevious = -1;
        intervalRestRemainingTimePrevious = -1;
        displayOnScreen(bitmapDisplay4);
        screenCurrentDisplay++;
        // Clear the screen and ready for MPU display
        M5.Lcd.fillRect(81, 25, 79, 55, BLACK);
        M5.Lcd.setCursor(81, 25);
        M5.Lcd.setTextColor(RED);
        M5.Lcd.printf("X:");
        M5.Lcd.setCursor(81, 40);
        M5.Lcd.setTextColor(ORANGE);
        M5.Lcd.printf("Y:");
        M5.Lcd.setCursor(81, 55);
        M5.Lcd.setTextColor(GREEN);
        M5.Lcd.printf("Z:");
      } else if (screenCurrentDisplay == 4) {
        // Transition from display 4 back to display 1
        intervalActiveRemainingTimePrevious = -1;
        intervalRestRemainingTimePrevious = -1;
        displayOnScreen(bitmapDisplay1);
        screenCurrentDisplay = 1;
      }
    }
    screenLastActivated = millis();
    M5.Axp.ScreenBreath(10);
    screenState = true;
  }
  if (screenCurrentDisplay == 1) {
    displayScreen1();
  } else if (screenCurrentDisplay == 2) {
    displayScreen2();
  } else if (screenCurrentDisplay == 3) {
    displayScreen3();
  } else if (screenCurrentDisplay == 4) {
    displayScreen4();
  }
  // Timeout and dim screen if inactive
  if ((millis() - screenLastActivated) > screenTimeoutDuration) {
    screenState = false;
    M5.Axp.ScreenBreath(7);
  }
  // If interval training, track current countdowns
  calculateInterval();
  calculateMPU();
  changeLED();
  delay(5);
}
