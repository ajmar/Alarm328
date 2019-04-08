#include <EEPROM.h>
#include <DS3231.h>
#include <Wire.h>
DS3231 Clock;

//// Define constants
// Potentiometer 1 for time
const uint8_t POT_1_SW = 4;
const uint8_t POT_1_CLK = 5;
const uint8_t POT_1_DAT = 6;
// Potentiometer 2 for alarm
const uint8_t POT_2_SW = 14;
const uint8_t POT_2_CLK = 12;
const uint8_t POT_2_DAT = 13;
// Test LED
const uint8_t LED_1 = 11;
const uint8_t REG_DATA = 16;
const uint8_t REG_LATCH = 17;
const uint8_t REG_CLOCK = 18;
const uint8_t ALM_TGL = 26;
const uint8_t CLK_SDA = 27;
const uint8_t CLK_SCL = 28;
// Potentiometer variables
unsigned long pot_debouncetime = 0;
unsigned long pot_debouncedelay = 0.01;
int pot_previousCLK;   
int pot_previousDAT;
int pot2_previousCLK;   
int pot2_previousDAT;
// Variable for outputting bits
int bitSet = 0;
bool CLKset = false;
bool ALMset = false;
// N/A Clock values
byte Year = 2001;
byte Month = 8;
byte Date = 23;
byte DoW = 1;
// Program clock values
uint8_t MAIN_hours = 0;
uint8_t MAIN_minutes = 0;
uint8_t MAIN_seconds = 0;
uint8_t CLK_hours = 0;
uint8_t CLK_minutes = 0;
uint8_t CLK_seconds = 0;
byte ALM_hours = 0;
byte ALM_minutes = 0;
int ALM_hours_ADDR = 512;
int ALM_minutes_ADDR = 513;
uint8_t BCD_minutes = 0;
uint8_t BCD_seconds = 0;
// N/A Clock.getHours boolean values
bool h12 = false;
bool pm = false;
// N/A 0 int value
byte SECONDS_0 = 0;
// Alarm enable, debouncing
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 100;
uint8_t ALM_enabled = 0;
int buttonState;
int lastButtonState = LOW;
// BCD calculation variables
uint8_t BCD_var_1 = 0;
uint8_t BCD_var_2 = 0;
uint8_t BCD_var_3 = 0;

uint8_t BCD_0to59(uint8_t & source) {
  if (source > 9) {
    BCD_var_1 = source / 10;
    BCD_var_2 = BCD_var_1 * 10;
    BCD_var_2 = source - BCD_var_2;
    BCD_var_1 = BCD_var_1 << 4;
    return (BCD_var_1 | BCD_var_2);
  } else {
    return source;
  }
}

uint8_t BCD_0to12(uint8_t & source) {
  if (source > 9) {
    BCD_var_2 = 0;
    BCD_var_1 = source / 10;
    BCD_var_3 = BCD_var_1 * 10;
    BCD_var_1 = BCD_var_1 << 4;
    if (source > 11) {
      BCD_var_2 = 1;
      BCD_var_2 = BCD_var_2 << 7;
    }
    BCD_var_1 = BCD_var_1 | BCD_var_2;
    BCD_var_3 = source - BCD_var_3;
    return (BCD_var_1 | BCD_var_3);
  } else {
    return source; 
  }
}

void outputREG(byte & Hours, byte & Minutes, byte & Seconds) {
  for (int i = 0; i < 8; i++) {
     bitSet = Seconds & (1 << i);
     if (bitSet = 0) {
      digitalWrite(REG_DATA, LOW);
     } else {
      digitalWrite(REG_DATA, HIGH);
     }
     digitalWrite(REG_CLOCK, HIGH);
  } 

  for (int i = 0; i < 8; i++) {
     bitSet = Minutes & (1 << i);
     if (bitSet = 0) {
      digitalWrite(REG_DATA, LOW);
     } else {
      digitalWrite(REG_DATA, HIGH);
     }
     digitalWrite(REG_CLOCK, HIGH);
  } 

  for (int i = 0; i < 8; i++) {
     bitSet = Hours & (1 << i);
     if (bitSet = 0) {
      digitalWrite(REG_DATA, LOW);
     } else {
      digitalWrite(REG_DATA, HIGH);
     }
     digitalWrite(REG_CLOCK, HIGH);
  } 
  digitalWrite(REG_LATCH, HIGH);
}

bool check_rotary(const uint8_t * pin_clock, const uint8_t * pin_data) {
  if ((pot_previousCLK == 0) && (pot_previousDAT == 1)) {
    if ((digitalRead(*pin_clock) == 1) && (digitalRead(*pin_data) == 0)) {
      return true;
    }
    if ((digitalRead(*pin_clock) == 1) && (digitalRead(*pin_data) == 1)) {
      return false;
    }
  }

  if ((pot_previousCLK == 1) && (pot_previousDAT == 0)) {
    if ((digitalRead(*pin_clock) == 0) && (digitalRead(*pin_data) == 1)) {
      return true;
    }
    if ((digitalRead(*pin_clock) == 0) && (digitalRead(*pin_data) == 0)) {
      return false;
    }
  }

  if ((pot_previousCLK == 1) && (pot_previousDAT == 1)) {
    if ((digitalRead(*pin_clock) == 0) && (digitalRead(*pin_data) == 1)) {
      return true;
    }
    if ((digitalRead(*pin_clock) == 0) && (digitalRead(*pin_data) == 0)) {
      return false;
    }
  }  

  if ((pot_previousCLK == 0) && (pot_previousDAT == 0)) {
    if ((digitalRead(*pin_clock) == 1) && (digitalRead(*pin_data) == 0)) {
      return true;
    }
    if ((digitalRead(*pin_clock) == 1) && (digitalRead(*pin_data) == 1)) {
      return false;
    }
  }            
}

void setup() {
  pinMode(LED_1, OUTPUT);
  pinMode(REG_DATA, OUTPUT);
  pinMode(REG_LATCH, OUTPUT);
  pinMode(REG_CLOCK, OUTPUT);
  pinMode(POT_1_SW, INPUT_PULLUP);
  pinMode(POT_1_CLK, INPUT);
  pinMode(POT_1_DAT, INPUT);
  pinMode(POT_2_SW, INPUT_PULLUP);
  pinMode(POT_2_CLK, INPUT);
  pinMode(POT_2_DAT, INPUT);
  pinMode(ALM_TGL, INPUT_PULLUP);
  Wire.begin();
  pot_previousCLK=digitalRead(POT_1_CLK);
  pot_previousDAT=digitalRead(POT_1_DAT);
  pot2_previousCLK=digitalRead(POT_2_CLK);
  pot2_previousDAT=digitalRead(POT_2_DAT);
  digitalWrite(LED_1, HIGH);
  delay(2000);
  digitalWrite(LED_1, LOW);
  MAIN_hours = Clock.getHour(h12, pm); // 24 Hour clock
  MAIN_minutes = Clock.getMinute();
  MAIN_seconds = Clock.getSecond();
  CLK_hours = MAIN_hours;
  CLK_minutes = MAIN_minutes;
  ALM_hours = EEPROM.read(ALM_hours_ADDR);
  ALM_minutes = EEPROM.read(ALM_minutes_ADDR);
}

void loop() {
  while (digitalRead(POT_1_SW) == LOW) {
    if ((millis() - pot_debouncetime) > pot_debouncedelay) {
      if (check_rotary(&POT_1_CLK, &POT_1_DAT) == true) {
        // Potentiometer was increased
        if (CLK_minutes = 59) {
          CLK_minutes = 0;
          if (CLK_hours = 23) {
            CLK_hours = 0;
          } else {
            CLK_hours++;
          }
        } else {
          CLK_minutes++;
        }
        outputREG(CLK_hours, CLK_minutes, SECONDS_0);
        CLKset = true;
      } else {
        // Potentiometer was decreased
        if (CLK_minutes = 0) {
          CLK_minutes = 59;
          if (CLK_hours = 0) {
            CLK_hours = 23;
          } else {
            CLK_hours--;
          }
        } else {
          CLK_minutes--;
        }
        outputREG(CLK_hours, CLK_minutes, SECONDS_0);
        CLKset = true;
      }
      
      pot_previousCLK=digitalRead(POT_1_CLK);
      pot_previousDAT=digitalRead(POT_1_DAT);
      pot_debouncetime=millis();  // Set variable to current millis() timer
    }
  }
  if (CLKset == true) {
    Clock.setClockMode(false);
    Clock.setHour(CLK_hours);
    Clock.setMinute(CLK_minutes);
    Clock.setSecond(0);
    MAIN_hours = CLK_hours;
    MAIN_minutes = CLK_minutes;
    MAIN_seconds = CLK_seconds;
  }
  
  while (digitalRead(POT_2_SW) == LOW) {
    if ((millis() - pot_debouncetime) > pot_debouncedelay) {
      if (check_rotary(&POT_2_CLK, &POT_2_DAT) == true) {
        // Potentiometer was increased
        if (ALM_minutes = 59) {
          ALM_minutes = 0;
          if (ALM_hours = 23) {
            ALM_hours = 0;
          } else {
            ALM_hours++;
          }
        } else {
          ALM_minutes++;
        }
        outputREG(ALM_hours, ALM_minutes, SECONDS_0);
        ALMset = true;
      } else {
        // Potentiometer was decreased
        if (ALM_minutes = 0) {
          ALM_minutes = 59;
          if (ALM_hours = 0) {
            ALM_hours = 23;
          } else {
            ALM_hours--;
          }
        } else {
          ALM_minutes--;
        }
        outputREG(ALM_hours, ALM_minutes, SECONDS_0);
        ALMset = true;
      }
      
      pot_previousCLK=digitalRead(POT_2_CLK);
      pot_previousDAT=digitalRead(POT_2_DAT);
      pot_debouncetime=millis();  // Set variable to current millis() timer
    }
  }
  if (ALMset == true) {
    EEPROM.write(ALM_hours_ADDR, ALM_hours);
    EEPROM.write(ALM_minutes_ADDR, ALM_minutes);
  }

  if ((ALM_enabled = 0) && (MAIN_hours = ALM_hours) && (MAIN_minutes = ALM_minutes)) {
    digitalWrite(LED_1, HIGH);
  } else {
    digitalWrite(LED_1, LOW);
  }

  int reading = digitalRead(ALM_TGL);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == HIGH) {
        ALM_enabled = ~ALM_enabled;
      }
    }
  }
  
  MAIN_hours = Clock.getHour(h12, pm);
  MAIN_minutes = Clock.getMinute();
  MAIN_seconds = Clock.getSecond();
  
  outputREG(MAIN_hours, MAIN_minutes, MAIN_seconds);
}
