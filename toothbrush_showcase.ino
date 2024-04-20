/*
 author: Yongyutha Kunapinun
version: 1.0.1
   note: for PCB version 1.0 with Arduino Nano Every
*/
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HX711.h>

#define RAIL_1_STP_PIN 3
#define RAIL_2_STP_PIN 5
#define RAIL_1_DIR_PIN 2
#define RAIL_2_DIR_PIN 4
// These two variables keep track of where the moving blocks are on rail 1 and rail 2, respectively
int rail_1_abs_stp = -1;
int rail_2_abs_stp = -1;
// delay parameters below are used for toggle-to-move mode
// maximum value of the step delay ramp (when a stepper is at its slowest)
#define MAX_STEP_DELAY 1000
// minimum value of the step delay ramp (when a stepper is at its fastest)
#define MIN_STEP_DELAY 250
// slope of the step delay ramp (must be negative)
#define STEP_RAMP_DOWN_SLOPE -10
#define MAX_RAIL_1_ABS_STP 15000
#define MAX_RAIL_2_ABS_STP 5000
#define MAX_RAIL_STP 15000
#define RAIL_1_LIM_SW_PIN 6
#define RAIL_2_LIM_SW_PIN 7
#define RAIL_1_FWD_BTN_PIN 14
#define RAIL_1_BWD_BTN_PIN 15
#define RAIL_2_FWD_BTN_PIN 16
#define RAIL_2_BWD_BTN_PIN 17
// OLED display width, in pixels
#define SCREEN_WIDTH 128
// OLED display height, in pixels
#define SCREEN_HEIGHT 32
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
// Reset pin # (or -1 if sharing Arduino reset pin)
#define OLED_RESET -1
//< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#define DISPLAY_WEIGHT_DIGITS 6
#define LOADCELL_DOUT_PIN 21
#define LOADCELL_SCK_PIN 20
// depends on each loadcell, obained from calibration 
#define WEIGHT_SCLAE_FACTOR 100.82
HX711 scale;

int ramp_down_step_delay(int min_delay, int max_delay, int down_slope, int step) {
  if (min_delay < 0) {
    Serial.println("min_delay must not be negative");
    return -1;
  }
  if (max_delay < 0) {
    Serial.println("max_delay must not be negative");
    return -1;
  }
  if (max_delay < min_delay) {
    Serial.println("max_delay must not be less than min_delay");
    return -1;
  }
  if (down_slope > 0) {
    Serial.println("down_slope must be negative");
    return -1;
  }
  if (step < 0) {
    Serial.println("step must not be negative");
    return -1;
  }
  if (step < (min_delay-max_delay)/down_slope) {
    return max_delay+down_slope*step;
  } else if (min_delay > 100){
    return min_delay;
  } else {
    Serial.println("step delay must not be less than 100 microseconds");
    return -1;
  }
}

void step_backward(int stepper_number, int step_delay) {
  if (step_delay < 100) {
    Serial.println("step_delay must not be less than 100 microseconds");
    return -1;
  }
  if (stepper_number == 1) {
    digitalWrite(RAIL_1_DIR_PIN, HIGH);
    digitalWrite(RAIL_1_STP_PIN, HIGH);
    delayMicroseconds(step_delay);
    digitalWrite(RAIL_1_STP_PIN, LOW);
    delayMicroseconds(step_delay);
    return 0; 
  } else if (stepper_number == 2) {
    digitalWrite(RAIL_2_DIR_PIN, HIGH);
    digitalWrite(RAIL_2_STP_PIN, HIGH);
    delayMicroseconds(step_delay);
    digitalWrite(RAIL_2_STP_PIN, LOW);
    delayMicroseconds(step_delay);
    return 0; 
  } else {
    Serial.println("invalid stepper motor number");
    return -1;
  }
}

int step_forward(int stepper_number, int step_delay) {
  if (step_delay < 100) {
    Serial.println("step_delay must not be less than 100 microseconds");
    return -1;
  }
  if (stepper_number == 1) {
    digitalWrite(RAIL_1_DIR_PIN, LOW);
    digitalWrite(RAIL_1_STP_PIN, HIGH);
    delayMicroseconds(step_delay);
    digitalWrite(RAIL_1_STP_PIN, LOW);
    delayMicroseconds(step_delay);
    return 0; 
  } else if (stepper_number == 2) {
    digitalWrite(RAIL_2_DIR_PIN, LOW);
    digitalWrite(RAIL_2_STP_PIN, HIGH);
    delayMicroseconds(step_delay);
    digitalWrite(RAIL_2_STP_PIN, LOW);
    delayMicroseconds(step_delay);
    return 0;
  } else {
    Serial.println("invalid stepper motor number");
    return -1;
  }
}

void display_text(Adafruit_SSD1306 display, char *string){
  // Clear the buffer
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 0);
  display.println(string);
  display.display();
}

void display_weight_in_grams(Adafruit_SSD1306 display, int weight) {
  display.clearDisplay();
  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 0);
  if (weight <= 0) {
    display.print(0);
    display.println(" g");
  } else {
    display.print(weight);
    display.println(" g");
  }
  display.display();
}

void display_weight_in_grams(Adafruit_SSD1306 display, int digits, int weight) {
  // this function still has some bugs
  //Serial.print("weight: ");
  //Serial.println(weight);
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 0);
  if (digits <= 0) {
    Serial.println(F("error: number of digits must be at least 1"));
    return;
  }
  const int str_len = digits + (digits-1)/3 + 1;
  char *display_weight = (char*) malloc(sizeof(char) * str_len);
  for (int i = 0; i < str_len; i++) {
    if (i < str_len - 1) {
      *(display_weight + sizeof(char) * i) = ' ';
    } else {
      *(display_weight + sizeof(char) * i) = '\0';
    }
  }
  if (weight <= 0) {
    *(display_weight + (str_len - 2)) = '0';
  } else {
    for(int i = str_len - 2; weight > 0 && i >= 0; i--) {
      if ((str_len - 1 - i) % 4 == 0) {
        *(display_weight + i) = ',';
      }
      else {
        *(display_weight + i) = '0' + weight % 10;
        weight /= 10;
      }
    }
  }
  display.print(display_weight);
  display.println(" g");
  // Serial.print(display_weight);
  // Serial.println(" g");
  display.display();
  free(display_weight);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(57600);
  pinMode(RAIL_1_STP_PIN, OUTPUT);
  pinMode(RAIL_2_STP_PIN, OUTPUT);
  pinMode(RAIL_1_DIR_PIN, OUTPUT);
  pinMode(RAIL_2_DIR_PIN, OUTPUT);
  pinMode(RAIL_1_LIM_SW_PIN, INPUT_PULLUP);
  pinMode(RAIL_2_LIM_SW_PIN, INPUT_PULLUP);
  pinMode(RAIL_1_FWD_BTN_PIN, INPUT_PULLUP);
  pinMode(RAIL_1_BWD_BTN_PIN, INPUT_PULLUP);
  pinMode(RAIL_2_FWD_BTN_PIN, INPUT_PULLUP);
  pinMode(RAIL_2_BWD_BTN_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    exit(0);
  }
  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000); // Pause for 2 seconds
  display_text(display, "setting up...");
  for(int i = 0; i < MAX_RAIL_STP; i++) {
    //int tik = micros();
    if (digitalRead(RAIL_1_LIM_SW_PIN) == LOW) {
      break;
    }
    step_backward(1, ramp_down_step_delay(MIN_STEP_DELAY, MAX_STEP_DELAY, STEP_RAMP_DOWN_SLOPE, i));
    // int tok = micros();
    // Serial.print("time per step in homing rail 1: ");
    // Serial.println(tok-tik);
  }
  rail_1_abs_stp = 0;
  for(int i = 0; i < MAX_RAIL_STP; i++) {
    //int tik = micros();
    if (digitalRead(RAIL_2_LIM_SW_PIN) == LOW) {
      break;
    }
    step_backward(2, ramp_down_step_delay(MIN_STEP_DELAY, MAX_STEP_DELAY, STEP_RAMP_DOWN_SLOPE, i));
    // int tok = micros();
    // Serial.print("time per step in homing rail 2: ");
    // Serial.println(tok-tik);
  }
  rail_2_abs_stp = 0;
  display.clearDisplay();
  display_text(display, "calibrating...");
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.tare();
  scale.set_scale(100.82);
  delay(3000);
  display.clearDisplay();
}

int rail_1_fwd_btn;
int rail_1_bwd_btn;
int rail_2_fwd_btn;
int rail_2_bwd_btn;
enum move_states {
  NONE,
  RAIL_1_FWD,
  RAIL_1_BWD,
  RAIL_2_FWD,
  RAIL_2_BWD
};
enum move_states move_state = NONE;
// consecutive relative step count variable
int consec_rel_stp = 0;

void loop() {
  // put your main code here, to run repeatedly:
  //int tik = micros();
  rail_1_fwd_btn = digitalRead(RAIL_1_FWD_BTN_PIN);
  rail_1_bwd_btn = digitalRead(RAIL_1_BWD_BTN_PIN);
  rail_2_fwd_btn = digitalRead(RAIL_2_FWD_BTN_PIN);
  rail_2_bwd_btn = digitalRead(RAIL_2_BWD_BTN_PIN);
  if (rail_1_fwd_btn == LOW || rail_1_bwd_btn == LOW || rail_2_fwd_btn == LOW || rail_2_bwd_btn == LOW) {
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }

  if (rail_1_fwd_btn == LOW && rail_1_abs_stp < MAX_RAIL_1_ABS_STP) {
    if (move_state == NONE) {
        display_text(display, "moving...");
    }
    if (move_state != RAIL_1_FWD){
      move_state = RAIL_1_FWD;
      consec_rel_stp = 0;
    }
    step_forward(1, ramp_down_step_delay(MIN_STEP_DELAY, MAX_STEP_DELAY, STEP_RAMP_DOWN_SLOPE, consec_rel_stp++));
    rail_1_abs_stp += 1;
  } else if (rail_1_bwd_btn == LOW && rail_1_abs_stp > 0) {
    if (move_state == NONE) {
        display_text(display, "moving...");
    }
    if (move_state != RAIL_1_BWD){
      move_state = RAIL_1_BWD;
      consec_rel_stp = 0;
    }
    step_backward(1, ramp_down_step_delay(MIN_STEP_DELAY, MAX_STEP_DELAY, STEP_RAMP_DOWN_SLOPE, consec_rel_stp++));
    rail_1_abs_stp -= 1;
  } else if (rail_2_fwd_btn == LOW && rail_2_abs_stp < MAX_RAIL_2_ABS_STP) {
    if (move_state == NONE) {
        display_text(display, "moving...");
    }
    if (move_state != RAIL_2_FWD){
      move_state = RAIL_2_FWD;
      consec_rel_stp = 0;
    }
    step_forward(2, ramp_down_step_delay(MIN_STEP_DELAY, MAX_STEP_DELAY, STEP_RAMP_DOWN_SLOPE, consec_rel_stp++));
    rail_2_abs_stp += 1;
  } else if (rail_2_bwd_btn == LOW && rail_2_abs_stp > 0) {
    if (move_state == NONE) {
        display_text(display, "moving...");
    }
    if (move_state != RAIL_2_BWD){
      move_state = RAIL_2_BWD;
      consec_rel_stp = 0;
    }
    step_backward(2, ramp_down_step_delay(MIN_STEP_DELAY, MAX_STEP_DELAY, STEP_RAMP_DOWN_SLOPE, consec_rel_stp++));
    rail_2_abs_stp -= 1;
  } else {
    move_state = NONE;
    consec_rel_stp = 0;
    display_weight_in_grams(display, scale.get_units());
    //display_weight_in_grams(display, DISPLAY_WEIGHT_DIGITS, scale.get_units());
  }
  // int tok = micros();
  // Serial.print("time per step in main loop: ");
  // Serial.println(tok-tik);
}
