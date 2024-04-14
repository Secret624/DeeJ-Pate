#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <esp_task_wdt.h>

#define SCREEN_WIDTH 128 // OLED width,  in pixels
#define SCREEN_HEIGHT 64 // OLED height, in pixels
#define WDT_TIMEOUT 100

#define I2C_SDA 14
#define I2C_SCL 21
#define button1 13
#define encoderCW 16
#define encoderCCW 17
#define encoderSW 18
#define debounce 10

Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT);

const int NUM_SLIDERS = 6;
int analogSliderValues[NUM_SLIDERS];

volatile int state = 0;
volatile int roll = 0;
volatile unsigned long last_time;

void IRAM_ATTR ISR() {
  //bob do something
}
void IRAM_ATTR encSW() {
  if ((millis() - last_time) < debounce)
    return;
  state++;
  if(state > 5){state = 0;}
  roll = analogSliderValues[state];
  last_time = millis();
}
void IRAM_ATTR encRollCW() {
  if ((millis() - last_time) < debounce)
    return;
  if (digitalRead(encoderCCW == HIGH) && roll > 0){
    roll--;
  }
  analogSliderValues[state] = roll;
  last_time = millis();
}
void IRAM_ATTR encRollCCW() {
  if ((millis() - last_time) < debounce)
    return;
  if (digitalRead(encoderCW == HIGH) && roll < 100){
    roll++;
  }
  analogSliderValues[state] = roll;
  last_time = millis();
}

void sendSliderValues() {
  String builtString = String("");

  for (int i = 0; i < NUM_SLIDERS; i++) {
    builtString += String((int)analogSliderValues[i]);

    if (i < NUM_SLIDERS - 1) {
      builtString += String("|");
    }
  }
  
  Serial.println(builtString);
}

void setup() {
  esp_task_wdt_init(WDT_TIMEOUT, true); //enable panic so ESP32 restarts
  esp_task_wdt_add(NULL); //add current thread to WDT watch

  pinMode(encoderCW, INPUT);
  pinMode(encoderCCW, INPUT);

  Wire.begin(I2C_SDA, I2C_SCL);
  attachInterrupt(button1, ISR, RISING);
  attachInterrupt(encoderSW, encSW, RISING);
  attachInterrupt(encoderCW, encRollCW, RISING);
  attachInterrupt(encoderCCW, encRollCCW, RISING);

  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (1);
  }

  Serial.begin(115200); //debugging only

  esp_task_wdt_reset();

  delay(2000);
  oled.clearDisplay();
  oled.setTextColor(WHITE);
  oled.dim(1);
}

void loop() {
  oled.clearDisplay();
  oled.setCursor(0, 4);
  for (int i = 0; i < NUM_SLIDERS; i++){
    if (state == i){
      oled.print("--");
    } else{
      oled.print("  ");
    }
    oled.print("Channel ");
    oled.print(i);
    oled.print(" vol ");
    oled.println(analogSliderValues[i]);
  }

  sendSliderValues();
  oled.display(); //reeee
  esp_task_wdt_reset(); //remember to feed the dogs
}
