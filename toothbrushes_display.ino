/*
edel+white toothbrushes display
 author: Yongyutha Kunapinun
version: 1.0.0
   note: for PCB version 1.0 with Arduino Nano Every
*/
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HX711.h>

#define STEP_1_PIN 3
#define STEP_2_PIN 5
#define DIR_1_PIN 2
#define DIR_2_PIN 4
// These two states keep track of where the moving blocks are on rail 1 and rail 2, respectively
//(assuming that the moving blocks always move to their destinations without any interruption.)
// -1: unknown (when starting up the system)
//  0: The block's position is at the starting point.
//  1: The block's position is at the end point.
int rail_1_state = -1;
int rail_2_state = -1;
// by changing the delay between steps we can change the rotation speed
// maximum value of the step delay ramp (when a stepper is at its slowest)
#define MAX_STEP_DELAY 1000
// minimum value of the step delay ramp (when a stepper is at its fastest)
#define MIN_STEP_DELAY 250
// slope of the step delay ramp (must be negative)
#define STEP_RAMP_DOWN_SLOPE -10
#define LIM_SW_1_PIN 6
#define LIM_SW_2_PIN 7
#define BTN_1_PIN 14
#define BTN_2_PIN 15
#define BTN_3_PIN 16
#define BTN_4_PIN 17
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

void display_text(Adafruit_SSD1306 display, char *string){
  // Clear the buffer
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 0);
  display.println(string);
  display.display();
}

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
    digitalWrite(DIR_1_PIN, HIGH);
    digitalWrite(STEP_1_PIN, HIGH);
    delayMicroseconds(step_delay);
    digitalWrite(STEP_1_PIN, LOW);
    delayMicroseconds(step_delay);
    return 0; 
  } else if (stepper_number == 2) {
    digitalWrite(DIR_2_PIN, HIGH);
    digitalWrite(STEP_2_PIN, HIGH);
    delayMicroseconds(step_delay);
    digitalWrite(STEP_2_PIN, LOW);
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
    digitalWrite(DIR_1_PIN, LOW);
    digitalWrite(STEP_1_PIN, HIGH);
    delayMicroseconds(step_delay);
    digitalWrite(STEP_1_PIN, LOW);
    delayMicroseconds(step_delay);
    return 0; 
  } else if (stepper_number == 2) {
    digitalWrite(DIR_2_PIN, LOW);
    digitalWrite(STEP_2_PIN, HIGH);
    delayMicroseconds(step_delay);
    digitalWrite(STEP_2_PIN, LOW);
    delayMicroseconds(step_delay);
    return 0;
  } else {
    Serial.println("invalid stepper motor number");
    return -1;
  }
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
  pinMode(STEP_1_PIN, OUTPUT);
  pinMode(STEP_2_PIN, OUTPUT);
  pinMode(DIR_1_PIN, OUTPUT);
  pinMode(DIR_2_PIN, OUTPUT);
  pinMode(LIM_SW_1_PIN, INPUT_PULLUP);
  pinMode(LIM_SW_2_PIN, INPUT_PULLUP);
  pinMode(BTN_1_PIN, INPUT_PULLUP);
  pinMode(BTN_2_PIN, INPUT_PULLUP);
  pinMode(BTN_3_PIN, INPUT_PULLUP);
  pinMode(BTN_4_PIN, INPUT_PULLUP);
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
  for(int i = 0; i < 15000; i++) {
    if (digitalRead(LIM_SW_1_PIN) == LOW) {
      break;
    }
    step_backward(1, ramp_down_step_delay(MIN_STEP_DELAY, MAX_STEP_DELAY, STEP_RAMP_DOWN_SLOPE, i));
  }
  rail_1_state = 0;
  for(int i = 0; i < 15000; i++) {
    if (digitalRead(LIM_SW_2_PIN) == LOW) {
      break;
    }
    step_backward(2, ramp_down_step_delay(MIN_STEP_DELAY, MAX_STEP_DELAY, STEP_RAMP_DOWN_SLOPE, i));
  }
  rail_2_state = 0;
  display.clearDisplay();
  display_text(display, "calibrating...");
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.tare();
  scale.set_scale(100.82);
  delay(3000);
  display.clearDisplay();
}

int btn_1_reading;
int btn_2_reading;
int btn_3_reading;
int btn_4_reading;

void loop() {
  // put your main code here, to run repeatedly:
  btn_1_reading = digitalRead(BTN_1_PIN);
  btn_2_reading = digitalRead(BTN_2_PIN);
  btn_3_reading = digitalRead(BTN_3_PIN);
  btn_4_reading = digitalRead(BTN_4_PIN);
  if (btn_1_reading == LOW || btn_2_reading == LOW || btn_3_reading == LOW || btn_4_reading == LOW) {
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }

  if (btn_1_reading == LOW && rail_1_state == 0) {
    display_text(display, "moving...");
    // Makes 200 pulses for making one full cycle rotation
    for(int i = 0; i < 15000; i++) {
      //Serial.print("delay: ");
      //Serial.println(ramp_down_step_delay(MIN_STEP_DELAY, MAX_STEP_DELAY, STEP_RAMP_DOWN_SLOPE, i));
      step_forward(1, ramp_down_step_delay(MIN_STEP_DELAY, MAX_STEP_DELAY, STEP_RAMP_DOWN_SLOPE, i));
    }
    rail_1_state = 1;
  }
  else if (btn_2_reading == LOW && rail_1_state == 1) {
    display_text(display, "moving...");
    for(int i = 0; i < 15000; i++) {
      if (digitalRead(LIM_SW_1_PIN) == LOW) {
        break;
      }
      //Serial.print("delay: ");
      //Serial.println(ramp_down_step_delay(MIN_STEP_DELAY, MAX_STEP_DELAY, STEP_RAMP_DOWN_SLOPE, i));
      step_backward(1, ramp_down_step_delay(MIN_STEP_DELAY, MAX_STEP_DELAY, STEP_RAMP_DOWN_SLOPE, i));
    }
    rail_1_state = 0;
  }
  else if (btn_3_reading == LOW && rail_2_state == 0) {
    display_text(display, "moving...");
    for(int i = 0; i < 5000; i++) {
      //Serial.print("delay: ");
      //Serial.println(ramp_down_step_delay(MIN_STEP_DELAY, MAX_STEP_DELAY, STEP_RAMP_DOWN_SLOPE, i));
      step_forward(2, ramp_down_step_delay(MIN_STEP_DELAY, MAX_STEP_DELAY, STEP_RAMP_DOWN_SLOPE, i));
    }
    rail_2_state = 1;
  }
  else if (btn_4_reading == LOW  && rail_2_state == 1) {
    display_text(display, "moving...");
    for(int i = 0; i < 5000; i++) {
      if (digitalRead(LIM_SW_2_PIN) == LOW) {
        break;
      }
      //Serial.print("delay: ");
      //Serial.println(ramp_down_step_delay(MIN_STEP_DELAY, MAX_STEP_DELAY, STEP_RAMP_DOWN_SLOPE, i));
      step_backward(2, ramp_down_step_delay(MIN_STEP_DELAY, MAX_STEP_DELAY, STEP_RAMP_DOWN_SLOPE, i));
    }
    rail_2_state = 0;
  }
  display_weight_in_grams(display, scale.get_units());
  //display_weight_in_grams(display, DISPLAY_WEIGHT_DIGITS, scale.get_units());
  delay(100);
}
