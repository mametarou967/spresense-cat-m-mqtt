#define TINY_GSM_MODEM_SIM7080
#define TINY_GSM_RX_BUFFER 650  // なくても動いたけど、あったほうが安定する気がする
#define TINY_GSM_YIELD() { delay(2); } // なくても動いたけど、あったほうが安定する気がする
#include <TinyGsmClient.h>
#include <PubSubClient.h>

#include "mqtt-config.h"

const char apn[]      = "povo.jp";
const char* broker = MY_BROKER;

const char* topicTest       = "test";
const char* topicTest2       = "test2";
#define GSM_AUTOBAUD_MIN 9600
#define GSM_AUTOBAUD_MAX 115200

// TinyGsmClient
TinyGsm        modem(Serial2);
TinyGsmClient  client(modem);
PubSubClient  mqtt(client);

uint32_t lastReconnectAttempt = 0;

void mqttCallback(char* topic, byte* payload, unsigned int len) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.println("]: ");
  Serial.print("bin:");
  for(int i = 0;i < len;i++){
    Serial.print(payload[i]);
  }
  Serial.println();

  char payloadBuf[256] = {0};
  strncpy(payloadBuf,(const char *)payload,len);
  
  String str = String(payloadBuf);
  Serial.print("str:");
  Serial.println(str);

  Serial.print("Message send to ");
  Serial.println(topicTest2);
  Serial.println(str);

  char buf[256];
  str.toCharArray(buf, 256);
  mqtt.publish(topicTest2, buf);
}

boolean mqttConnect() {
  Serial.print("Connecting to ");
  Serial.println(broker);

  // Connect to MQTT Broker
  boolean status = mqtt.connect("GsmClientTest");

  // Or, if you want to authenticate MQTT:
  // boolean status = mqtt.connect("GsmClientName", "mqtt_user", "mqtt_pass");

  if (status == false) {
    Serial.println(" fail");
    return false;
  }
  Serial.println(" success");
  mqtt.subscribe(topicTest);
  return mqtt.connected();
}

/* After M5Core is started or reset
the program in the setUp () function will be run, and this part will only be run once.
在 M5Core 启动或者复位后，即会开始执行setup()函数中的程序，该部分只会执行一次。 */
void setup(){
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1);

  Serial.println("Wait...");  // Print text on the screen (string) 在屏幕上打印文本(字符串)
  // Set GSM module baud rate

  // モデムのリスタート
  Serial.println("Initializing modem...");  // Print text on the screen (string) 在屏幕上打印文本(字符串)
  modem.restart();

  // モデムの情報の取得
  String modemInfo = modem.getModemInfo();
  Serial.print("Modem Info: ");
  Serial.println(modemInfo);


  // GPRS connection parameters are usually set after network registration
  Serial.print(F("Connecting to "));
  Serial.print(apn);
  if (!modem.gprsConnect(apn, "", "")) {
    Serial.println("-> fail");
    delay(10000);
    return;
  }
  Serial.println("-> success");

  if (modem.isGprsConnected()) { Serial.println("GPRS connected"); }

  // ★参考ページ:https://github.com/vshymanskyy/TinyGSM/blob/master/examples/MqttClient/MqttClient.ino
  mqtt.setServer(broker, 1883);
  mqtt.setCallback(mqttCallback);

  mqtt.publish(topicTest2, "Hello Server! I'm m5stack");
}

/* After the program in setup() runs, it runs the program in loop()
The loop() function is an infinite loop in which the program runs repeatedly
在setup()函数中的程序执行完后，会接着执行loop()函数中的程序
loop()函数是一个死循环，其中的程序会不断的重复运行 */
void loop() {
  // Make sure we're still registered on the network
  if (!modem.isNetworkConnected()) {
      Serial.println("Network disconnected");
      
      if (!modem.waitForNetwork(180000L, true)) {
        Serial.println(" fail");
        delay(10000);
        return;
      }
      
      if (modem.isNetworkConnected()) {
        Serial.println("Network re-connected");
      }
      
      // and make sure GPRS/EPS is still connected
      if (!modem.isGprsConnected()) {
        Serial.println("GPRS disconnected!");
        Serial.print(F("Connecting to "));
        Serial.print(apn);
        if (!modem.gprsConnect(apn, "", "")) {
          Serial.println(" fail");
          delay(10000);
          return;
        }
        if (modem.isGprsConnected()) { Serial.println("GPRS reconnected"); }
      }
  }
      
  if (!mqtt.connected()) {
    Serial.println("=== MQTT NOT CONNECTED ===");
    // Reconnect every 10 seconds
    uint32_t t = millis();
    if (t - lastReconnectAttempt > 10000L) {
      lastReconnectAttempt = t;
      if (mqttConnect()) { lastReconnectAttempt = 0; }
    }
    delay(100);
    return;
  }
  
  mqtt.loop();
}
