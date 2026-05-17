#include <WiFi.h>
#include "esp_wifi.h"

void sniffer_callback(void* buf, wifi_promiscuous_pkt_type_t type) {
  wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
  int len = pkt->rx_ctrl.sig_len;
  
  // Removed the 0xD0 filter so it captures ALL Wi-Fi traffic from ALL sources!
  if (len > 10 && len < 150) {
    
    // Extract and format Source and Destination MAC Addresses
    char macStr[100];
    sprintf(macStr, "> [INTERCEPT] SRC: %02X:%02X:%02X:%02X:%02X:%02X | DST: %02X:%02X:%02X:%02X:%02X:%02X",
            pkt->payload[10], pkt->payload[11], pkt->payload[12], pkt->payload[13], pkt->payload[14], pkt->payload[15],
            pkt->payload[4], pkt->payload[5], pkt->payload[6], pkt->payload[7], pkt->payload[8], pkt->payload[9]);
            
    Serial.println(macStr);
    
    String output = "> [INTERCEPT] Payload: ";
    
    // Extract text vs binary
    for (int i = 0; i < len; i++) {
      char c = pkt->payload[i];
      if (c >= 32 && c <= 126) {
        output += c;
      } else {
        output += ".";
      }
    }
    // Print directly to the USB Serial Port
    Serial.println(output); 
    Serial.println("> ---------------------------------");
  }
}

void setup() {
  // MUST match the baud rate in the HTML file
  Serial.begin(115200); 
  
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&sniffer_callback);
  
  Serial.println("\n> SYSTEM_READY");
  Serial.println("> PROMISCUOUS_MODE: ENGAGED");
  Serial.println("> WAITING FOR DATA...");
}

void loop() {}