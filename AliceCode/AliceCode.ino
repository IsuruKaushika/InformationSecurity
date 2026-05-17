#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <mbedtls/aes.h>
#include <mbedtls/md.h>

// ======================================================================
// ⚠️ CONFIGURE FOR ALICE OR BOB ⚠️
// ======================================================================
// If uploading to ALICE, put BOB'S MAC Address here.
// If uploading to BOB, put ALICE'S MAC Address here.
uint8_t peerAddress[] = {0x88, 0x13, 0xBF, 0x00, 0x94, 0xB0}; 
// ======================================================================

// Security Keys
unsigned char aes_key[] = "SecretKey1234567"; 
char *hmac_key = "MySuperSecretHMACAuthenticationKey"; 

// Global variables for communication
uint32_t send_seq_num = 1;
uint32_t last_recv_seq_num = 0;

typedef struct struct_message {
  uint32_t seq_num;           
  unsigned char ciphertext[16]; 
  unsigned char hmac[32];     
} secure_packet_t;

secure_packet_t myPacket;

void generate_hmac(unsigned char *payload, size_t payload_len, unsigned char *output_hmac) {
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
  mbedtls_md_hmac_starts(&ctx, (const unsigned char *) hmac_key, strlen(hmac_key));
  mbedtls_md_hmac_update(&ctx, payload, payload_len);
  mbedtls_md_hmac_finish(&ctx, output_hmac);
  mbedtls_md_free(&ctx);
}

// ---------------- ESP-NOW RECEIVE (AIRWAVES -> USB) ----------------
void OnDataRecv(const esp_now_recv_info_t * esp_now_info, const uint8_t *incomingData, int len) {
  if (len == 16) {
    // VULNERABLE PATH
    char msg[17] = {0}; 
    memcpy(msg, incomingData, 16);
    Serial.println("[VULNERABLE] BOB SAYS: " + String(msg));
  }
  else if (len == sizeof(secure_packet_t)) {
    // SECURE PATH
    secure_packet_t rx;
    memcpy(&rx, incomingData, sizeof(rx));

    unsigned char data_to_sign[20];
    memcpy(data_to_sign, &rx.seq_num, 4);
    memcpy(data_to_sign + 4, rx.ciphertext, 16);
    unsigned char computed_hmac[32];
    generate_hmac(data_to_sign, 20, computed_hmac);

    if (memcmp(computed_hmac, rx.hmac, 32) != 0) {
      Serial.println("[ERROR] BLOCKED: Invalid HMAC. Tampering detected!");
      return;
    }
    if (rx.seq_num <= last_recv_seq_num) {
      Serial.println("[ERROR] BLOCKED: Replay Attack Detected!");
      return;
    }
    last_recv_seq_num = rx.seq_num;

    unsigned char decrypted[16];
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_dec(&aes, aes_key, 128);
    mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT, rx.ciphertext, decrypted);
    mbedtls_aes_free(&aes);

    char msg[17] = {0};
    memcpy(msg, decrypted, 16);
    Serial.println("[SECURE] BOB SAYS: " + String(msg));
  }
}

void setup() {
  Serial.begin(115200);
  
  // Set to Station mode and force channel 1 so Alice, Bob, and Trudy all match
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  
  esp_now_init();
  esp_now_register_recv_cb(OnDataRecv);
  
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, peerAddress, 6);
  peerInfo.channel = 1;  
  peerInfo.encrypt = false; 
  esp_now_add_peer(&peerInfo);
  
  Serial.println("[SYSTEM] Hardware Bridge Online. Awaiting USB commands...");
}

// ---------------- SERIAL LOOP (USB -> AIRWAVES) ----------------
void loop() {
  // Listen for commands coming from the Local HTML file
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    
    if (input.length() > 0) {
      // The HTML will send data in the format "VULN:Message" or "SECU:Message"
      String mode = input.substring(0, 4);
      String msgText = input.substring(5);
      
      unsigned char plaintext[16];
      memset(plaintext, 0, 16); // Pad with null characters
      for(int i = 0; i < msgText.length() && i < 16; i++){
        plaintext[i] = msgText[i];
      }

      if (mode == "VULN") {
        esp_now_send(peerAddress, plaintext, 16);
        Serial.println("[SYSTEM] Sent Vulnerable Plaintext.");
      } 
      else if (mode == "SECU") {
        myPacket.seq_num = send_seq_num;
        
        mbedtls_aes_context aes;
        mbedtls_aes_init(&aes);
        mbedtls_aes_setkey_enc(&aes, aes_key, 128);
        mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, plaintext, myPacket.ciphertext);
        mbedtls_aes_free(&aes);

        unsigned char data_to_sign[20];
        memcpy(data_to_sign, &myPacket.seq_num, 4);
        memcpy(data_to_sign + 4, myPacket.ciphertext, 16);
        generate_hmac(data_to_sign, 20, myPacket.hmac);

        esp_now_send(peerAddress, (uint8_t *) &myPacket, sizeof(myPacket));
        send_seq_num++; 
        Serial.println("[SYSTEM] Sent Secure AES+HMAC Payload.");
      }
    }
  }
}