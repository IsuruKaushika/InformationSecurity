#include <WiFi.h>
#include "esp_wifi.h"

void sniffer_callback(void* buf, wifi_promiscuous_pkt_type_t type) {
  wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
  int len = pkt->rx_ctrl.sig_len;
  
  // Removed the 0xD0 filter so it captures ALL Wi-Fi traffic from ALL sources!
  if (len > 10 && len < 150) {
    
    // Extract Source and Destination MAC Addresses
    char srcMac[20];
    sprintf(srcMac, "%02X:%02X:%02X:%02X:%02X:%02X", pkt->payload[10], pkt->payload[11], pkt->payload[12], pkt->payload[13], pkt->payload[14], pkt->payload[15]);
    char dstMac[20];
    sprintf(dstMac, "%02X:%02X:%02X:%02X:%02X:%02X", pkt->payload[4], pkt->payload[5], pkt->payload[6], pkt->payload[7], pkt->payload[8], pkt->payload[9]);
    
    bool isAlice = (strcmp(srcMac, "30:C9:22:28:36:54") == 0);
    bool isBob = (strcmp(srcMac, "88:13:BF:00:94:B0") == 0);
    bool isDstAlice = (strcmp(dstMac, "30:C9:22:28:36:54") == 0);
    bool isDstBob = (strcmp(dstMac, "88:13:BF:00:94:B0") == 0);

    String prefix = "> [INTERCEPT]";
    if (isAlice && isDstBob) {
      if (len < 75) {
        prefix = "> [ALICE_VULN]";
      } else {
        prefix = "> [ALICE_SECURE]";
      }
    } else if (isBob && isDstAlice) {
      if (len < 75) {
        prefix = "> [BOB_VULN]";
      } else {
        prefix = "> [BOB_SECURE]";
      }
    }

    char macStr[150];
    sprintf(macStr, "%s SRC: %s | DST: %s", prefix.c_str(), srcMac, dstMac);
            
    Serial.println(macStr);
    
    String output = prefix + " Payload: ";
    
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