#include <WiFi.h>
#include <WebServer.h>
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
#define debounce 50

const char* ssid = "";
const char* password = "";

WebServer server(80);
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT);

const int numSliders = 6;
int sliderValues[numSliders] = {0};

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
  if(state > (numSliders - 1)){state = 0;}
  roll = sliderValues[state];
  last_time = millis();
}
void IRAM_ATTR encRollCW() {
  if ((millis() - last_time) < debounce)
    return;
  if (digitalRead(encoderCCW == HIGH) && roll > 0){
    roll = roll - 16;
  }
  sliderValues[state] = roll;
  last_time = millis();
}
void IRAM_ATTR encRollCCW() {
  if ((millis() - last_time) < debounce)
    return;
  if (digitalRead(encoderCW == HIGH) && roll < 1023){
    roll = roll + 16;
  }
  sliderValues[state] = roll;
  last_time = millis();
}

void sendSliderValues() {
  String builtString = String("");

  for (int i = 0; i < numSliders; i++) {
    builtString += String((int)sliderValues[i]);

    if (i < numSliders - 1) {
      builtString += String("|");
    }
  }
  
  Serial.println(builtString);
}

void handleRoot() {
  String content = "<html><head><title>DeeJ</title></head><body>";
  content += "<h1>DeeJ</h1>";
  content += "<form action=\"/set\" method=\"get\">";
  for (int i = 0; i < numSliders; i++) {
    content += "Channel " + String(i) + ": <input type=\"range\" name=\"slider" + String(i) + "\" min=\"0\" max=\"1023\" value=\"" + String(sliderValues[i]) + "\"><br>";
  }
  content += "<input type=\"submit\" value=\"Set\">";
  content += "</form></body></html>";
  
  server.send(200, "text/html", content);
}

void handleSet() {
  for (int i = 0; i < numSliders; i++) {
    sliderValues[i] = server.arg("slider" + String(i)).toInt();
  }
  
  String message = "Settings saved:<br>";
  for (int i = 0; i < numSliders; i++) {
    message += "Slider " + String(i+1) + ": " + String(sliderValues[i]) + "<br>";
  }

  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);
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
  delay(2000);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  
  Serial.println("Connected to WiFi");

  server.on("/", handleRoot);
  server.on("/set", handleSet);

  server.begin();

  oled.clearDisplay();
  oled.setTextColor(WHITE);
  oled.dim(1);
}

void loop() {
  oled.clearDisplay();
  oled.setCursor(0, 4);
  for (int i = 0; i < numSliders; i++){
    if (state == i){
      oled.print("--");
    } else{
      oled.print("  ");
    }
    oled.print("Channel ");
    oled.print(i);
    oled.print(" vol ");
    oled.println(sliderValues[i]);
  }
  oled.print("IP:");
  oled.println(WiFi.localIP());
  oled.display(); //reeee

  esp_task_wdt_reset();
  server.handleClient();
  sendSliderValues();
}