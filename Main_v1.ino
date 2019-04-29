#include <PinChangeInterrupt.h>
#include <EEPROM.h>
#include <Wire.h>
#include <DS3231.h>
DS3231 Clock;
// Set pinout constants
const uint8_t register_data = 10,register_clock = 11,register_latch = 12;
const uint8_t pin_alarm_buzzer = 17;
const uint8_t pin_alarm_switch = 1;
const uint8_t pin_pot_2_switch = 0;
const uint8_t pin_pot_1_switch = 4;
const uint8_t pin_pot_1_clock = 2;
const uint8_t pin_pot_1_data = 3;
const uint8_t pin_pot_2_clock = 6;
const uint8_t pin_pot_2_data = 7;
uint8_t pot_1_prev_clock = 0;
uint8_t pot_1_prev_data = 0;
uint8_t pot_2_prev_clock = 0;
uint8_t pot_2_prev_data = 0;
long debounce_time = 0;
int debounce_delay = 0.02;
// Variable that toggles the alarm
volatile byte alarm_enabled_var = 0;
volatile byte pot_1_enabled_var = 0;
volatile byte pot_2_enabled_var = 0;
uint8_t alarm_timer = 0;
// Set to 24h
bool h24 = false;
// Time variables
uint8_t time_main_seconds,time_main_minutes,time_main_hours;
volatile uint8_t time_alarm_minutes,time_alarm_hours;
uint8_t time_clock_seconds,time_clock_minutes,time_clock_hours;
// Convert to outputtable data type
uint8_t output_minutes,output_hours;
// Null / Zero variables for functions
uint8_t var_zero = 0;
// Prototype functions
static void register_output(uint8_t & source);
static void alarm_trigger();
void isr_alarm();
void isr_pot_1();
void isr_pot_2();

int testval;

void setup() {
  Wire.begin();
  Clock.setClockMode(false);
  Clock.setSecond(20);
  Clock.setMinute(20);
  Clock.setHour(22);
  Clock.setDoW(4);
  Clock.setDate(25);
  Clock.setMonth(4);
  Clock.setYear(19);
  time_clock_seconds = Clock.getSecond();
  time_clock_minutes = Clock.getMinute();
  time_clock_hours = Clock.getHour(h24, h24);
  time_main_seconds = time_clock_seconds;
  time_main_minutes = time_clock_minutes;
  time_main_hours = time_clock_hours;
  pinMode(pin_alarm_switch, INPUT);
  pinMode(pin_pot_1_switch, INPUT_PULLUP);
  pinMode(pin_pot_2_switch, INPUT_PULLUP);
  pinMode(pin_pot_1_clock, INPUT);
  pinMode(pin_pot_1_data, INPUT);
  pinMode(pin_pot_2_clock, INPUT);
  pinMode(pin_pot_2_data, INPUT);
  pinMode(register_clock, OUTPUT);
  pinMode(register_data, OUTPUT);
  pinMode(register_latch, OUTPUT);
  pinMode(pin_alarm_buzzer, OUTPUT);
  digitalWrite(pin_alarm_buzzer, HIGH);
  delay(2000);
  digitalWrite(pin_alarm_buzzer, LOW);
  delay(2000);
  digitalWrite(pin_alarm_buzzer, HIGH);
  delay(2000);
  digitalWrite(pin_alarm_buzzer, LOW);
  uint8_t pot_1_prev_clock = digitalRead(pin_pot_1_clock);
  uint8_t pot_1_prev_data = digitalRead(pin_pot_1_data);
  uint8_t pot_2_prev_clock = digitalRead(pin_pot_2_clock);
  uint8_t pot_2_prev_data = digitalRead(pin_pot_2_data);
  time_alarm_minutes = EEPROM.read(420);
  time_alarm_hours = EEPROM.read(421);
}

void loop() {

  testval = digitalRead(pin_alarm_switch);
  if (testval == LOW) {
    digitalWrite(pin_alarm_buzzer, HIGH);
  } else {
    digitalWrite(pin_alarm_buzzer, LOW);
  }
  
  if ((time_main_minutes == time_alarm_minutes) && (time_main_hours == time_alarm_hours)) {
    alarm_timer = 1;
  }
  while ((alarm_enabled_var != 0) && (alarm_timer < 250) && (alarm_timer != 0)) {
    alarm_trigger();
    alarm_timer++;
  }
  alarm_timer = 0;
  
  if (pot_1_enabled_var != 0) {
    time_clock_seconds = var_zero;
    time_clock_minutes = time_main_minutes;
    time_clock_hours = time_main_hours;
    while (pot_1_enabled_var != 0) {
       if ((millis() - debounce_time) > debounce_delay) {
         pot_1_prev_clock=digitalRead(pin_pot_1_clock);
         pot_1_prev_data=digitalRead(pin_pot_1_data);
         debounce_time=millis();
         if (time_clock_minutes < 59 && time_clock_minutes > 0) {
           time_clock_minutes =+ check_rotary(pin_pot_1_data, pin_pot_1_clock, pot_1_prev_clock, pot_1_prev_data);
         } else if (time_clock_hours < 24 && time_clock_hours > 0) {
           time_clock_hours =+ check_rotary(pin_pot_1_data, pin_pot_1_clock, pot_1_prev_clock, pot_1_prev_data);
           time_clock_minutes = 0;
         } else {
           time_clock_hours = 0;
           time_clock_minutes = 0;
         }
         output_minutes = time_clock_minutes;
         output_hours = time_clock_hours;
         register_output(var_zero),register_output(output_minutes),register_output(output_hours);
       }
    }
    Clock.setSecond(var_zero);
    Clock.setMinute(time_clock_minutes);
    Clock.setHour(time_clock_hours);
    time_main_seconds = var_zero;
    time_main_minutes = time_clock_minutes;
    time_main_hours = time_clock_hours;
  }

  if (pot_2_enabled_var != 0) {
    while (pot_2_enabled_var != 0) {
       if ((millis() - debounce_time) > debounce_delay) {
         pot_2_prev_clock=digitalRead(pin_pot_2_clock);
         pot_2_prev_data=digitalRead(pin_pot_2_data);
         debounce_time=millis();
         if (time_alarm_minutes < 59 && time_alarm_minutes > 0) {
           time_alarm_minutes =+ check_rotary(pin_pot_2_data, pin_pot_2_clock, pot_2_prev_clock, pot_2_prev_data);
         } else if (time_alarm_hours < 24 && time_alarm_hours > 0) {
           time_alarm_hours =+ check_rotary(pin_pot_2_data, pin_pot_2_clock, pot_2_prev_clock, pot_2_prev_data);
           time_alarm_minutes = 0;
         } else {
           time_alarm_hours = 0;
           time_alarm_minutes = 0;
         }
         output_minutes = time_alarm_minutes;
         output_hours = time_alarm_hours;
         register_output(var_zero),register_output(output_minutes),register_output(output_hours);
       }
    }
    EEPROM.write(420, time_alarm_minutes);
    EEPROM.write(421, time_alarm_hours);
  }
  delay(10);
  time_main_seconds = Clock.getSecond();
  time_main_minutes = Clock.getMinute();
  time_main_hours = Clock.getHour(h24, h24);
  register_output(time_main_seconds),register_output(time_main_minutes),register_output(time_main_hours);
}

static void register_output(uint8_t & source) {
  digitalWrite(register_latch, LOW);
  for (int i = 0; i <= 7; i++) {
     digitalWrite(register_clock, LOW);
     digitalWrite(register_data, bitRead(source,i));
     digitalWrite(register_clock, HIGH);
  }
  digitalWrite(register_latch, HIGH);
}

static void alarm_trigger() {
    uint8_t alarm_output_minutes = time_alarm_minutes;
    uint8_t alarm_output_hours = time_alarm_hours;
    register_output(var_zero);
    register_output(alarm_output_minutes);
    register_output(alarm_output_hours);
    digitalWrite(pin_alarm_buzzer, HIGH);
    delay(300);
    register_output(var_zero);
    register_output(var_zero);
    register_output(var_zero);
    digitalWrite(pin_alarm_buzzer, LOW);
    delay(300);
}

static int8_t check_rotary(const uint8_t & pin_data, const uint8_t & pin_clock, uint8_t & previous_clock, uint8_t previous_data) {
  if ((previous_clock == 0) && (previous_data == 1)) {
    if ((digitalRead(pin_clock) == 1) && (digitalRead(pin_data) == 0)) {
      return 1;
    }
    if ((digitalRead(pin_clock) == 1) && (digitalRead(pin_data) == 1)) {
      return -1;
    }
  }
  if ((previous_clock == 1) && (previous_data == 0)) {
    if ((digitalRead(pin_clock) == 0) && (digitalRead(pin_data) == 1)) {
      return 1;
    }
    if ((digitalRead(pin_clock) == 0) && (digitalRead(pin_data) == 0)) {
      return -1;
    }
  }
  if ((previous_clock == 1) && (previous_data == 1)) {
    if ((digitalRead(pin_clock) == 0) && (digitalRead(pin_data) == 1)) {
      return 1;
    }
    if ((digitalRead(pin_clock) == 0) && (digitalRead(pin_data) == 0)) {
      return -1;
    }
  }  
  if ((previous_clock == 0) && (previous_data == 0)) {
    if ((digitalRead(pin_clock) == 1) && (digitalRead(pin_data) == 0)) {
      return 1;
    }
    if ((digitalRead(pin_clock) == 1) && (digitalRead(pin_data) == 1)) {
      return -1;
    }
  }            
}
