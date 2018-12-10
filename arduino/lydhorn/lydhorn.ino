/*
 * Internet Of Lydhorn
 * 
 * Running on a WeMos D1 WiFi board.
 */

#include <DNSServer.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>


static const char* SETUP_SSID = "sensor-setup";
static const byte  EEPROM_INITIALIZED_MARKER = 0xF1; //Just a magic number

static const byte SETUP_MODE_PIN = D6;
static const byte LYDHORN_PIN = D4;
static const int LYDHORN_DURATION = 3*1000;

static const byte MAX_SSID_LENGTH            = 32;
static const byte MAX_PASSWORD_LENGTH        = 64;
static const byte MAX_MQTT_SERVERNAME_LENGTH = 64;
static const byte MAX_MQTT_SENSORID_LENGTH   =  8;
static const byte MAX_MQTT_USERNAME_LENGTH   = 32;
static const byte MAX_MQTT_PASSWORD_LENGTH   = 32;

char ssid_param[MAX_SSID_LENGTH+1];
char password_param[MAX_PASSWORD_LENGTH+1];
char mqtt_servername_param[MAX_MQTT_SERVERNAME_LENGTH+1];
char mqtt_sensorid_param[MAX_MQTT_SENSORID_LENGTH+1];
char mqtt_username_param[MAX_MQTT_USERNAME_LENGTH+1];
char mqtt_password_param[MAX_MQTT_PASSWORD_LENGTH+1];
boolean mqtt_enabled;

enum Mode {
  SETUP,
  NORMAL
};

volatile Mode mode;


ESP8266WebServer server(80);

WiFiClient esp_client;
PubSubClient mqtt_client(esp_client);

DNSServer dnsServer;
static const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);


void readPersistentString(char* s, int max_length, int& adr)
{
  int i = 0;
  byte c;
  do {
    c = EEPROM.read(adr++);
    if (i<max_length)
    {
      s[i++] = static_cast<char>(c);
    }
  } while (c!=0);
  s[i] = 0;
}

void readPersistentParams()
{
  int adr = 0;
  if (EEPROM_INITIALIZED_MARKER != EEPROM.read(adr++))
  {
    ssid_param[0] = 0;
    password_param[0] = 0;
    mqtt_servername_param[0] = 0;
    mqtt_sensorid_param[0] = 0;
    mqtt_username_param[0] = 0;
    mqtt_password_param[0] = 0;
  } else {
    readPersistentString(ssid_param, MAX_SSID_LENGTH, adr);
    readPersistentString(password_param, MAX_PASSWORD_LENGTH, adr);
    readPersistentString(mqtt_servername_param, MAX_MQTT_SERVERNAME_LENGTH, adr);
    readPersistentString(mqtt_sensorid_param, MAX_MQTT_SENSORID_LENGTH, adr);
    readPersistentString(mqtt_username_param, MAX_MQTT_USERNAME_LENGTH, adr);
    readPersistentString(mqtt_password_param, MAX_MQTT_PASSWORD_LENGTH, adr);
  }
}

void writePersistentString(const char* s, size_t max_length, int& adr)
{
  for (int i=0; i<min(strlen(s), max_length); i++)
  {
    EEPROM.write(adr++, s[i]);
  }
  EEPROM.write(adr++, 0);
}

void writePersistentParams()
{
  int adr = 0;
  EEPROM.write(adr++, EEPROM_INITIALIZED_MARKER);
  writePersistentString(ssid_param, MAX_SSID_LENGTH, adr);
  writePersistentString(password_param, MAX_PASSWORD_LENGTH, adr);
  writePersistentString(mqtt_servername_param, MAX_MQTT_SERVERNAME_LENGTH, adr);
  writePersistentString(mqtt_sensorid_param, MAX_MQTT_SENSORID_LENGTH, adr);
  writePersistentString(mqtt_username_param, MAX_MQTT_USERNAME_LENGTH, adr);
  writePersistentString(mqtt_password_param, MAX_MQTT_PASSWORD_LENGTH, adr);
  EEPROM.commit();
}

void handleNotFound() {
  server.send(404, F("text/plain"), F("Page Not Found\n"));
}

void handleSetupRoot() {
  if (server.hasArg("ssid") || server.hasArg("password")
      || server.hasArg("mqtt_server") || server.hasArg("mqtt_id") || server.hasArg("mqtt_username") || server.hasArg("mqtt_password"))
  {
    if (server.hasArg("ssid"))
    {
      strncpy(ssid_param, server.arg("ssid").c_str(), MAX_SSID_LENGTH);
      ssid_param[MAX_SSID_LENGTH] = 0;
    }
    
    if (server.hasArg("password") && !server.arg("password").equals(F("password")))
    {
      strncpy(password_param, server.arg("password").c_str(), MAX_PASSWORD_LENGTH);
      password_param[MAX_PASSWORD_LENGTH] = 0;
    }

    if (server.hasArg("mqtt_server"))
    {
      strncpy(mqtt_servername_param, server.arg("mqtt_server").c_str(), MAX_MQTT_SERVERNAME_LENGTH);
      mqtt_servername_param[MAX_MQTT_SERVERNAME_LENGTH] = 0;
    }
    if (server.hasArg("mqtt_id"))
    {
      strncpy(mqtt_sensorid_param, server.arg("mqtt_id").c_str(), MAX_MQTT_SENSORID_LENGTH);
      mqtt_sensorid_param[MAX_MQTT_SENSORID_LENGTH] = 0;
    }
    if (server.hasArg("mqtt_username"))
    {
      strncpy(mqtt_username_param, server.arg("mqtt_username").c_str(), MAX_MQTT_USERNAME_LENGTH);
      mqtt_username_param[MAX_MQTT_USERNAME_LENGTH] = 0;
    }
    if (server.hasArg("mqtt_password") && !server.arg("mqtt_password").equals(F("mqtt_password")))
    {
      strncpy(mqtt_password_param, server.arg("mqtt_password").c_str(), MAX_MQTT_PASSWORD_LENGTH);
      mqtt_password_param[MAX_MQTT_PASSWORD_LENGTH] = 0;
    }

    writePersistentParams();

    server.send(200, F("text/plain"), F("Settings saved"));
  }
  else
  {
    readPersistentParams();

    String body = F("<!doctype html>"\
                    "<html lang=\"en\">"\
                    "<head>"\
                     "<meta charset=\"utf-8\">"\
                     "<title>Setup</title>"\
                     "<style>"\
                      "form {margin: 0.5em;}"\
                      "input {margin: 0.2em;}"\
                     "</style>"\
                    "</head>"\
                    "<body>"\
                     "<form method=\"post\">"\
                      "SSID:<input type=\"text\" name=\"ssid\" required maxlength=\"");
    body += String(MAX_SSID_LENGTH);
    body += F("\" autofocus value=\"");
    body += ssid_param;
    body += F("\"/><br/>"\
              "Password:<input type=\"password\" name=\"password\" maxlength=\"");
    body += String(MAX_PASSWORD_LENGTH);
    body += F("\" value=\"password\"/><br/><hr/>"\
              "MQTT server:<input type=\"text\" name=\"mqtt_server\" maxlength=\"");
    body += String(MAX_MQTT_SERVERNAME_LENGTH);
    body += F("\" value=\"");
    body += mqtt_servername_param;
    body += F("\"/><br/>"\
              "MQTT sensor id:<input type=\"text\" name=\"mqtt_id\" maxlength=\"");
    body += String(MAX_MQTT_SENSORID_LENGTH);
    body += F("\" value=\"");
    body += mqtt_sensorid_param;
    body += F("\"/><br/>"\
              "MQTT username:<input type=\"text\" name=\"mqtt_username\" maxlength=\"");
    body += String(MAX_MQTT_USERNAME_LENGTH);
    body += F("\" value=\"");
    body += mqtt_username_param;
    body += F("\"/><br/>"\
              "MQTT password:<input type=\"password\" name=\"mqtt_password\" maxlength=\"");
    body += String(MAX_MQTT_PASSWORD_LENGTH);
    body += F("\" value=\"password\"/><br/>"\
              "<input type=\"submit\" value=\"Submit\"/>"\
              "</form>"\
             "</body>"\
             "</html>");
    server.send(200, F("text/html"), body);
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  if (*payload == '1') {
    playSound();
  }
}

void publishMQTTValue(boolean value)
{
  if (mqtt_enabled && !value)
  {
    mqtt_client.publish((String(mqtt_sensorid_param)+F("/Lydhorn")).c_str(), "0");
  }
}

void reconnectMQTT() {
  while (mqtt_enabled && !mqtt_client.connected()) {
    if (!mqtt_client.connect("ESP8266Client", mqtt_username_param, mqtt_password_param)) {
      delay(5000);
    } else if (mqtt_sensorid_param && *mqtt_sensorid_param) {
      mqtt_client.subscribe((String(mqtt_sensorid_param)+F("/Lydhorn")).c_str());
    }
  }
}

void setup()
{
  EEPROM.begin(1 + MAX_SSID_LENGTH + 1 + MAX_PASSWORD_LENGTH + 1);

  pinMode(SETUP_MODE_PIN, INPUT_PULLUP);
  pinMode(LYDHORN_PIN, OUTPUT);
  digitalWrite(LYDHORN_PIN, LOW);

  if (LOW == digitalRead(SETUP_MODE_PIN))
  {
    mqtt_enabled = false;
    mode = SETUP;
    WiFi.softAP(SETUP_SSID);
    dnsServer.start(DNS_PORT, "*", apIP);

    server.on("/", handleSetupRoot);
    server.onNotFound(handleNotFound);
    server.begin();
  }
  else
  {
    mode = NORMAL;
    readPersistentParams();
    mqtt_enabled = mqtt_servername_param && *mqtt_servername_param;
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid_param, password_param);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
    }

    if (mqtt_enabled)
    {
      mqtt_client.setServer(mqtt_servername_param, 1883);
      mqtt_client.setCallback(mqttCallback);
    }
  }
}

void playSound() {
  digitalWrite(LYDHORN_PIN, HIGH);
  publishMQTTValue(true);
  delay(LYDHORN_DURATION);
  digitalWrite(LYDHORN_PIN, LOW);
  publishMQTTValue(false);
}

void loop()
{
  if (mode == SETUP) {
    dnsServer.processNextRequest(); //Route everything to 192.168.4.1
    server.handleClient(); //WebServer
  }
  else if (mqtt_enabled)
  {
    if (!mqtt_client.connected()) {
      reconnectMQTT();
    }
    mqtt_client.loop();
  }
}
