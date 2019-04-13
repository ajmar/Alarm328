#include "PinChangeInterrupt.h"
#include "EEPROM.h"

// Set pinout constants
const uint8_t register_data = 7;
const uint8_t register_clock = 5;
const uint8_t register_latch = 6;
const uint8_t pin_alarm_buzzer = 3;
const uint8_t pin_interrupt_1 = 2;
const uint8_t pin_alarm_switch = 11;
const uint8_t pin_pot_1_switch = 10;
const uint8_t pin_pot_2_switch = 9;
const uint8_t pin_pot_1_clock = 4; // INVALID
const uint8_t pin_pot_1_data = 4;
const uint8_t pin_pot_2_clock = 4;
const uint8_t pin_pot_2_data = 4;
uint8_t pot_1_prev_clock = 0;
uint8_t pot_1_prev_data = 0;
uint8_t pot_2_prev_clock = 0;
uint8_t pot_2_prev_data = 0;
long debounce_time = 0;
int debounce_delay = 0.01;
// Variable that toggles the alarm
volatile byte alarm_enabled_var = 0;
volatile byte pot_1_enabled_var = 0;
volatile byte pot_2_enabled_var = 0;
uint8_t alarm_timer = 0;
// Time variables
uint8_t time_main_seconds,time_main_minutes,time_main_hours;
uint8_t time_alarm_seconds,time_alarm_minutes,time_alarm_hours;
uint8_t time_clock_seconds,time_clock_minutes,time_clock_hours;
// Null / Zero variables for functions
uint8_t var_zero = 0;
// Prototype functions
static void register_output(uint8_t & source);
static void alarm_trigger();
void isr_alarm();
void isr_pot_1();
void isr_pot_2();

void setup() {
  time_clock_seconds = EEPROM.read(396);
  time_clock_minutes = EEPROM.read(397);
  time_clock_hours = EEPROM.read(398);
  time_main_seconds = time_clock_seconds;
  time_main_minutes = time_clock_minutes;
  time_main_hours = time_clock_hours;
  pinMode(pin_alarm_switch, INPUT_PULLUP);
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
  uint8_t pot_1_prev_clock = digitalRead(pin_pot_1_clock);
  uint8_t pot_1_prev_data = digitalRead(pin_pot_1_data);
  uint8_t pot_2_prev_clock = digitalRead(pin_pot_2_clock);
  uint8_t pot_2_prev_data = digitalRead(pin_pot_2_data);
  attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(pin_alarm_switch), isr_alarm, FALLING);
  attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(pin_pot_1_switch), isr_pot_1, FALLING);
  attachPinChangeInterrupt(digitalPinToPinChangeInterrupt(pin_pot_2_switch), isr_pot_2, FALLING);
}

void loop() {
  
  if (time_main_minutes == time_alarm_minutes && time_main_hours == time_alarm_hours) {
    alarm_timer = 1;
  }
  while (!(alarm_enabled_var == 0) && alarm_timer < 250 && !(alarm_timer == 0)) {
    alarm_trigger();
    alarm_timer++;
  }
  alarm_timer = 0;
    
  while (!pot_1_enabled_var == 0) {
     if ((millis() - debounce_time) > debounce_delay) {
       check_rotary(pin_pot_1_data, pin_pot_1_clock, pot_1_prev_clock, pot_1_prev_data);
       pot_1_prev_clock=digitalRead(pin_pot_1_clock);
       pot_1_prev_data=digitalRead(pin_pot_1_data);
       debounce_time=millis();
     }
  }
  while (!pot_2_enabled_var == 0) {
     if ((millis() - debounce_time) > debounce_delay) {
       check_rotary(pin_pot_2_data, pin_pot_2_clock, pot_2_prev_clock, pot_2_prev_data);
       pot_2_prev_clock=digitalRead(pin_pot_2_clock);
       pot_2_prev_data=digitalRead(pin_pot_2_data);
       debounce_time=millis();
     }
  }
  
  delay(600);
  time_main_seconds++;
  time_main_minutes++;
  time_main_hours++;
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
    register_output(time_alarm_seconds);
    register_output(time_alarm_minutes);
    register_output(time_alarm_hours);
    digitalWrite(pin_alarm_buzzer, HIGH);
    delay(300);
    register_output(var_zero);
    register_output(var_zero);
    register_output(var_zero);
    digitalWrite(pin_alarm_buzzer, LOW);
    delay(300);
}

void isr_alarm() {
  alarm_enabled_var = ~alarm_enabled_var;
}

void isr_pot_1() {
  pot_1_enabled_var = ~pot_1_enabled_var;
}

void isr_pot_2() {
  pot_2_enabled_var = ~pot_2_enabled_var;
}

static uint8_t check_rotary(const uint8_t & pin_data, const uint8_t & pin_clock, uint8_t & previous_clock, uint8_t previous_data) {
  if ((previous_clock == 0) && (previous_data == 1)) {
    if ((digitalRead(pin_clock) == 1) && (digitalRead(pin_data) == 0)) {
      return 1;
    }
    if ((digitalRead(pin_clock) == 1) && (digitalRead(pin_data) == 1)) {
      return 0;
    }
  }
  if ((previous_clock == 1) && (previous_data == 0)) {
    if ((digitalRead(pin_clock) == 0) && (digitalRead(pin_data) == 1)) {
      return 1;
    }
    if ((digitalRead(pin_clock) == 0) && (digitalRead(pin_data) == 0)) {
      return 0;
    }
  }
  if ((previous_clock == 1) && (previous_data == 1)) {
    if ((digitalRead(pin_clock) == 0) && (digitalRead(pin_data) == 1)) {
      return 1;
    }
    if ((digitalRead(pin_clock) == 0) && (digitalRead(pin_data) == 0)) {
      return 0;
    }
  }  
  if ((previous_clock == 0) && (previous_data == 0)) {
    if ((digitalRead(pin_clock) == 1) && (digitalRead(pin_data) == 0)) {
      return 1;
    }
    if ((digitalRead(pin_clock) == 1) && (digitalRead(pin_data) == 1)) {
      return 0;
    }
  }            
}
