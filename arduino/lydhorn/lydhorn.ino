/*
 * Internet Of Lydhorn
 * 
 * Running on a WeMos D1 WiFi board.
 */

#include <ESP8266WiFi.h>


static const byte LYDHORN_PIN = D2;
static const int LYDHORN_DURATION = 3*1000;

static const char SSID[] = "gill-roxrud";
static const char PASS[] = "******";

WiFiServer server(80);

void setup()
{
  pinMode(LYDHORN_PIN, OUTPUT);
  digitalWrite(LYDHORN_PIN, LOW);

  WiFi.begin(const_cast<char*>(SSID), PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
  }
  
  server.begin();
}

void playSound() {
  digitalWrite(LYDHORN_PIN, HIGH);
  delay(LYDHORN_DURATION);
  digitalWrite(LYDHORN_PIN, LOW);
}

void loop()
{
  WiFiClient client = server.available();
  if (client && client.connected()) {
    playSound();

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/plain");
    client.println("Connection: close");  // the connection will be closed after completion of the response
    client.println();
    client.flush();
    delay(100);
    client.stop();
  }
}

