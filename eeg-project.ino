#include <WiFi.h>
#include <ESP32Servo.h>
#include "esp_camera.h"
#include "Arduino.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "fb_gfx.h"
#include "soc/soc.h"             // disable brownout problems
#include "soc/rtc_cntl_reg.h"    // disable brownout problems
#include <PubSubClient.h>

const char* ssid = "Dika";
const char* password = "dikta221";
const char* mqtt_server = "192.168.32.247";
const char* mqtt_commands_topic = "esp32/commands";
const char* mqtt_camera_topic = "esp32/camera";

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define MOTOR_1_PIN_1     14
#define MOTOR_1_PIN_2     2
#define MOTOR_2_PIN_1     13
#define MOTOR_2_PIN_2     15

#define SERVO_PIN 12

Servo servo;
int servoDegree = 90;
int servoIncrement = 0;

WiFiClient espClient;
PubSubClient client(espClient);

int lastWifiStatus;
unsigned long lastCaptureTime = 0;

void callback(char* topic, byte* payload, unsigned int length) {
  String command = "";
  for (unsigned int i = 0; i < length; i++) {
    command += (char)payload[i];
  }

  Serial.println("command: " + command);

  if (command == "forward") {
    Serial.println("Forward");
    digitalWrite(MOTOR_1_PIN_1, 1);
    digitalWrite(MOTOR_1_PIN_2, 0);
    digitalWrite(MOTOR_2_PIN_1, 1);
    digitalWrite(MOTOR_2_PIN_2, 0);
  }
  else if (command == "left") {
    Serial.println("Left");
    digitalWrite(MOTOR_1_PIN_1, 0);
    digitalWrite(MOTOR_1_PIN_2, 0);
    digitalWrite(MOTOR_2_PIN_1, 1);
    digitalWrite(MOTOR_2_PIN_2, 0);
  }
  else if (command == "right") {
    Serial.println("Right");
    digitalWrite(MOTOR_1_PIN_1, 1);
    digitalWrite(MOTOR_1_PIN_2, 0);
    digitalWrite(MOTOR_2_PIN_1, 0);
    digitalWrite(MOTOR_2_PIN_2, 0);
  }
  else if (command == "backward") {
    Serial.println("Backward");
    digitalWrite(MOTOR_1_PIN_1, 0);
    digitalWrite(MOTOR_1_PIN_2, 1);
    digitalWrite(MOTOR_2_PIN_1, 0);
    digitalWrite(MOTOR_2_PIN_2, 1);
  }
  else if (command == "stop") {
    Serial.println("Stop");
    digitalWrite(MOTOR_1_PIN_1, 0);
    digitalWrite(MOTOR_1_PIN_2, 0);
    digitalWrite(MOTOR_2_PIN_1, 0);
    digitalWrite(MOTOR_2_PIN_2, 0);
  }
  else if (command == "servo-left") {
    Serial.println("Servo Left");
    servoIncrement = -10;
  }
  else if (command == "servo-right") {
    Serial.println("Servo Right");
    servoIncrement = 10;
  }
  else if (command == "servo-stop") {
    Serial.println("Servo Stop");
    servoIncrement = 0;
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("EEG-CAR")) {
      Serial.println("connected");
      client.subscribe(mqtt_commands_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 3 seconds");
      delay(3000);
    }
  }
}

String get_wifi_status(int status) {
  switch (status) {
    case WL_IDLE_STATUS: return "WL_IDLE_STATUS";
    case WL_SCAN_COMPLETED: return "WL_SCAN_COMPLETED";
    case WL_NO_SSID_AVAIL: return "WL_NO_SSID_AVAIL";
    case WL_CONNECT_FAILED: return "WL_CONNECT_FAILED";
    case WL_CONNECTION_LOST: return "WL_CONNECTION_LOST";
    case WL_DISCONNECTED: return "WL_DISCONNECTED";
    case WL_CONNECTED: return "WL_CONNECTED";
    default: return "IDK";
  }
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

  pinMode(MOTOR_1_PIN_1, OUTPUT);
  pinMode(MOTOR_1_PIN_2, OUTPUT);
  pinMode(MOTOR_2_PIN_1, OUTPUT);
  pinMode(MOTOR_2_PIN_2, OUTPUT);
  pinMode(SERVO_PIN, OUTPUT);

  servo.attach(SERVO_PIN);
  servo.write(servoDegree);

  Serial.begin(115200);
  Serial.setDebugOutput(false);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_LATEST;
  config.frame_size = FRAMESIZE_SVGA;
  config.jpeg_quality = 50;
  config.fb_count = 2;

  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // Wi-Fi connection
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  int status = WiFi.status();
  while (status != WL_CONNECTED) {
    delay(500);
    status = WiFi.status();
    // if (status == lastWifiStatus) continue;
    lastWifiStatus = status;
    Serial.println(get_wifi_status(status));
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP Address: http://");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void captureAndPublishImage() {
  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  if (fb->format != PIXFORMAT_JPEG) {
    Serial.println("Non-JPEG data not supported");
    esp_camera_fb_return(fb);
    return;
  }

  client.beginPublish(mqtt_camera_topic, fb->len, false);
  client.write(fb->buf, fb->len);
  client.endPublish();

  esp_camera_fb_return(fb);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }

  unsigned long currentTime = millis();

  if (currentTime - lastCaptureTime >= 500) {

    captureAndPublishImage();
    client.loop();
    lastCaptureTime = currentTime;
    servoDegree += servoIncrement;

    if (servoDegree != servo.read() && servoDegree > 0 && servoDegree < 180) {
      servo.write(servoDegree);
    }
  }
}
