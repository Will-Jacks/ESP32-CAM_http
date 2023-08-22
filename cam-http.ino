#include "src/camera_config/OV2640.h"
#include "src/connections/WiFi/WiFiCon.h"
#include <WebServer.h>
#include <WiFiClient.h>
#define CAMERA_MODEL_AI_THINKER
#include "src/camera_config/camera_pins.h"
#include "src/camera_config/camera_config.h"
#include <PubSubClient.h>

WiFiClient wlanclient;
PubSubClient cliente(wlanclient);

const char* mqtt_broker = "broker.emqx.io";
const int port = 1883;
const char* topic = "CARRINHO/barco";
const char* topic_status = "CARRINHO/status";
const char* espID = "espcam111222";
String str_msg;
#define flashPin 4

OV2640 cam;

WebServer server(80);

const char HEADER[] = "HTTP/1.1 200 OK\r\n" \
                      "Access-Control-Allow-Origin: *\r\n" \
                      "Content-Type: multipart/x-mixed-replace; boundary=123456789000000000000987654321\r\n";
const char BOUNDARY[] = "\r\n--123456789000000000000987654321\r\n";
const char CTNTTYPE[] = "Content-Type: image/jpeg\r\nContent-Length: ";
const int hdrLen = strlen(HEADER);
const int bdrLen = strlen(BOUNDARY);
const int cntLen = strlen(CTNTTYPE);

void callback(char* topic, byte* payload, unsigned int length) {
  char message[5]={0x00};
  for (int i = 0; i < length; i++){
    message[i]=(char)payload[i]; 
    }
  message[length]=0x00;
  Serial.println (message);
  str_msg = String(message);
  recebeMensagem();
}

void recebeMensagem() {
  if(str_msg.equals("l")) {
    digitalWrite(flashPin, HIGH);
    int status = digitalRead(flashPin);
    if (status == 1) {
      cliente.publish(topic_status, "Ligado");
    }
  }

  if(str_msg.equals("d")) {
    digitalWrite(flashPin, LOW);
    int status = digitalRead(flashPin);
    if (status == 0) {
      cliente.publish(topic_status, "Desligado");
    }
  }

  if(str_msg.equals("cs")) {
    cliente.publish(topic_status, "Conectado");
  }
}

void statusLed() {
  int status = digitalRead(flashPin);
  if (status == 1) {
      cliente.publish(topic_status, "Ligado");
  }
  if (status == 0) {
      cliente.publish(topic_status, "Desligado");
    }
}

void conectaMQTT() {
  cliente.setServer(mqtt_broker, port);
  cliente.setCallback(callback);
  if (cliente.connect(espID, NULL, NULL)) {
    Serial.println("Conectado com o broker");
    cliente.subscribe(topic);
  } else {
    Serial.println("Conexão com o broker falhou!");
  }
}

void handle_jpg_stream(void)
{
  char buf[32];
  int s;

  WiFiClient client = server.client();

  client.write(HEADER, hdrLen);
  client.write(BOUNDARY, bdrLen);

  while (true)
  {
    if (!client.connected()) break;
    cam.run();
    s = cam.getSize();
    client.write(CTNTTYPE, cntLen);
    sprintf( buf, "%d\r\n\r\n", s );
    client.write(buf, strlen(buf));
    client.write((char *)cam.getfb(), s);
    client.write(BOUNDARY, bdrLen);
    delay(100);
    cliente.loop();
  }
}

const char JHEADER[] = "HTTP/1.1 200 OK\r\n" \
                       "Content-disposition: inline; filename=capture.jpg\r\n" \
                       "Content-type: image/jpeg\r\n\r\n";
const int jhdLen = strlen(JHEADER);

void handle_jpg(void)
{
  WiFiClient client = server.client();

  cam.run();
  if (!client.connected()) return;

  client.write(JHEADER, jhdLen);
  client.write((char *)cam.getfb(), cam.getSize());
}

void handleNotFound()
{
  String message = "Server is running!\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  server.send(200, "text / plain", message);
}

void handleHandshake() {
  server.on("/", HTTP_GET, handle_jpg_stream);
  server.on("/jpg", HTTP_GET, handle_jpg);
  server.onNotFound(handleNotFound);
  server.begin();
}

void setup(){
  Serial.begin(115200);
  pinMode(flashPin, OUTPUT);
  cameraConfig();
  conectaWiFi();
  handleHandshake();
  conectaMQTT();
  statusLed();
}

void loop()
{
  server.handleClient();
  cliente.loop();
}
