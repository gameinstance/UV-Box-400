/*
 * UV-Box 400
 * A UV double exposure box using three high voltage
 * series totaling 200 LEDs, controlled using the 
 * Atmega168 MCU.
 * 
 * GameInstance.com
 * 2018
 */
#include <LiquidCrystal.h>
#include <EEPROM.h>

static const unsigned char RED_BUTTON = 10;
static const unsigned char GREEN_BUTTON = 11;
static const unsigned char BLUE_BUTTON = 12;
static const unsigned char YELLOW_BUTTON = 13;
static const unsigned char LID_SENSOR = 2;
static const unsigned char RELAY_OUT = 3;

static const bool LID_SENSOR_ENABLED = true;

struct HourMinuteSecond {
  
  unsigned int hours, minutes, seconds;
  
  HourMinuteSecond(): hours(0), minutes(0), seconds(0) {
    //
  }
  void Init(unsigned int h = 0, unsigned int m = 0, unsigned int s = 0) {
    //
    hours = h;
    minutes = m;
    seconds = s;
  }
  bool EEPROMRead(unsigned int address) {
    //
    hours = EEPROM.read(address);
    minutes = EEPROM.read(address + 1);
    seconds = EEPROM.read(address + 2);
    if ((hours > 9) || (minutes > 59) || (seconds > 59)) {
      //
      Init(0, 8, 30);
      return false;
    }
    //
    return true;
  }
  void EEPROMWrite(unsigned int address) {
    //
    EEPROM.write(address, hours);
    EEPROM.write(address + 1, minutes);
    EEPROM.write(address + 2, seconds);
  }
  unsigned int ToSeconds() {
    //
    return seconds + minutes * 60 + hours * 3600;
  }
  void FromSeconds(unsigned int sec) {
    //
    seconds = sec % 60;
    minutes = (sec / 60) % 60;
    hours = sec / 3600;
  }
  void Increment(int m = 0, int s = 5) {
    //
    unsigned long int sec = ToSeconds() + s + m * 60;
    if (sec > 35999) {
      // overflowing 9:59:59
      sec = s < 0 ? 0 : 35995;
    }
    FromSeconds(sec);
    EEPROMWrite(0);
  }
};

unsigned int state = 0;
unsigned long int elapsed_seconds, start_ts, button_ts;
HourMinuteSecond total_hms, elapsed_hms;

LiquidCrystal lcd(4, 5, 6, 7, 8, 9);

namespace LCD {

  void Clear () {
    //
    lcd.setCursor(0, 0);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print("                ");      
  }

  void Splash() {
    //
    lcd.setCursor(0, 0);
    lcd.print("GameInstance.com");
    lcd.setCursor(3, 1);
    lcd.print("UV-Box 400");  
  }

  void DisplayHMS(HourMinuteSecond &hms, unsigned int x, unsigned int y, String message = "") {
    //
    static unsigned int flicker_count = 0;
    lcd.setCursor(x, y);
    if (message.length() > 0) {
      // 
      flicker_count ++;
      if (flicker_count > 100) {
        //
        flicker_count = 0;
      }
      if (flicker_count <= 20) {
        // 
        while(message.length() < 8) {
          // 
          message += " ";
        }
        lcd.print(message);
        return;
      }
    }
    lcd.print(hms.hours);
    lcd.print(":");
    lcd.print(hms.minutes <= 9 ? "0" : "");
    lcd.print(hms.minutes);
    lcd.print(":");
    lcd.print(hms.seconds <= 9 ? "0" : "");
    lcd.print(hms.seconds);
    lcd.print(" ");
  }

  void DisplayTimeLine(String message = "") {
    // 
    LCD::DisplayHMS(elapsed_hms, 0, 0, message);
    lcd.setCursor(8, 0);
    lcd.print(" ");
    LCD::DisplayHMS(total_hms, 9, 0);
  }

  void SensorState(bool R, bool G, bool B, bool Y, bool L) {
    // 
    lcd.setCursor(0, 1);
    lcd.print("R");
    lcd.setCursor(1, 1);
    lcd.print(R ? "1" : "0");
  
    lcd.setCursor(3, 1);
    lcd.print("G");
    lcd.setCursor(4, 1);
    lcd.print(G ? "1" : "0");
  
    lcd.setCursor(6, 1);
    lcd.print("B");
    lcd.setCursor(7, 1);
    lcd.print(B ? "1" : "0");
  
    lcd.setCursor(9, 1);
    lcd.print("Y");
    lcd.setCursor(10, 1);
    lcd.print(Y ? "1" : "0");
  
    lcd.setCursor(12, 1);
    lcd.print("L");
    lcd.setCursor(13, 1);
    lcd.print(L ? "1" : "0");
  }
}

void setup() {
  // 
  pinMode(RED_BUTTON, INPUT);
  pinMode(GREEN_BUTTON, INPUT);
  pinMode(BLUE_BUTTON, INPUT);
  pinMode(YELLOW_BUTTON, INPUT);
  pinMode(LID_SENSOR, INPUT);
  pinMode(RELAY_OUT, OUTPUT);
  digitalWrite(RELAY_OUT, LOW);
  lcd.begin(16, 2);
}

void loop() {
  // 
  if (button_ts == 0) {
    // 
    if (digitalRead(GREEN_BUTTON) == LOW) {
      // green button down
      button_ts = millis();
      total_hms.Increment(0, 5);
    }
    if (digitalRead(BLUE_BUTTON) == LOW) {
      // blue button down
      button_ts = millis();
      total_hms.Increment(0, -5);
    }
  } else {
    // 
    if ((digitalRead(GREEN_BUTTON) == HIGH) 
      && (digitalRead(BLUE_BUTTON) == HIGH)) {
      // all time change buttons up
      button_ts = 0;
    }
    if (millis() - button_ts > 500) {
      // time change buttons change expired
      button_ts = 0;
    }
  }
  
  if (state == 0) {
    // init
    LCD::Clear();
    LCD::Splash();
    delay(2000);
    state = 100;
  }
  if (state == 100) {
    //
    total_hms.EEPROMRead(0);
    elapsed_hms.Init();
    elapsed_seconds = 0;
    state = 1;
  }
  if (state == 1) {
    // ready to start
    state = 2;
  }
  if (state == 2) {
    // 
    if (LID_SENSOR_ENABLED 
      && (digitalRead(LID_SENSOR) == HIGH)) {
      // lid open
      LCD::DisplayTimeLine("Lid open");
      lcd.setCursor(0, 1);
      lcd.print("Close to prepare");
    } else {
      //
      LCD::DisplayTimeLine("Ready");
      lcd.setCursor(0, 1);
      lcd.print("  Red to start  ");
      
      if (digitalRead(RED_BUTTON) == LOW) {
        // red button down
        state = 3;
      }
    }
  }
  if (state == 3) {
    // start: exposure start
    start_ts = millis();
    digitalWrite(RELAY_OUT, HIGH);
    lcd.setCursor(0, 1);
    lcd.print("Yellow to pause  ");
    state = 4;
  }
  if (state == 4) {
    // 
    if (digitalRead(RED_BUTTON) == HIGH) {
      // red button up again
      state = 5;
    }
  }
  if (state == 5) {
    //
    unsigned long int actual_elapsed_seconds = elapsed_seconds + (millis() - start_ts) / 1000;
    if (actual_elapsed_seconds > total_hms.ToSeconds()) {
      // time's up
      state = 20;
    } else {
      //
      elapsed_hms.FromSeconds(actual_elapsed_seconds);
      LCD::DisplayTimeLine("Exposing");
  
      if (LID_SENSOR_ENABLED 
        && (digitalRead(LID_SENSOR) == HIGH)) {
        // lid open
        state = 11;
      }
      
      if (digitalRead(YELLOW_BUTTON) == LOW) {
        // yellow button down
        state = 6;
      }
    }
  }
  if (state == 6) {
    // pause: exposure stop
    digitalWrite(RELAY_OUT, LOW);
    elapsed_seconds += (millis() - start_ts) / 1000;
    lcd.setCursor(0, 1);
    lcd.print("Yellow to resume ");
    state = 7;
  }
  if (state == 7) {
    // 
    if (digitalRead(YELLOW_BUTTON) == HIGH) {
      // yellow button up again
      state = 8;
    }
  }
  if (state == 8) {
    // 
    LCD::DisplayTimeLine("Paused");
    
    if (digitalRead(YELLOW_BUTTON) == LOW) {
      // yellow button down
      state = 9;
    }
  }
  if (state == 9) {
    // resumed: exposure start
    digitalWrite(RELAY_OUT, HIGH);
    start_ts = millis();
    lcd.setCursor(0, 1);
    lcd.print("Yellow to pause  ");
    state = 10;
  }
  if (state == 10) {
    // 
    if (digitalRead(YELLOW_BUTTON) == HIGH) {
      // red button up again
      state = 5;
    }
  }
  if (state == 11) {
    // lid open: exposure stop
    digitalWrite(RELAY_OUT, LOW);
    elapsed_seconds += (millis() - start_ts) / 1000;
    lcd.setCursor(0, 1);
    lcd.print("Close to resume ");
    state = 12;
  }
  if (state == 12) {
    // 
    LCD::DisplayTimeLine("Lid open");
    
    if (LID_SENSOR_ENABLED 
      && (digitalRead(LID_SENSOR) == LOW)) {
      // lid closed
      state = 13;
    }
  }
  if (state == 13) {
    // lid closed: exposure start
    digitalWrite(RELAY_OUT, HIGH);
    start_ts = millis();
    lcd.setCursor(0, 1);
    lcd.print("Yellow to pause  ");
    state = 5;
  }

  if (state == 20) {
    // time exipred: exposure stop
    digitalWrite(RELAY_OUT, LOW);
    elapsed_seconds += (millis() - start_ts) / 1000;
    lcd.setCursor(0, 1);
    lcd.print("Open the lid    ");
    state = 21;
  }
  if (state == 21) {
    // 
    LCD::DisplayTimeLine("Finished");
    
    if (digitalRead(LID_SENSOR) == HIGH) {
      // lid open
      state = 100;
    }
  }
  
  delay(50);
}
