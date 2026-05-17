#include <WiFi.h>
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(); 
  delay(1000); 
  Serial.println();
  Serial.print("Alice's MAC Address is: ");
  Serial.println(WiFi.macAddress());
}
void loop() {}