 /*
 *
 *
 */

#define VERSION "20.03.2"

#include <time.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

ESP8266WebServer server(80);
String scanned_wifis;
String html;
bool wifi_connected = false;
String eeprom_ssid;
String eeprom_pass;
String eeprom_ntpServer;
byte eeprom_offset;
byte eeprom_summertime;
int eeprom_syncInterval;
int eeprom_multiplexInterval;
byte eeprom_configWritten; // 42 = yes

//////////////
// Settings //
//////////////

// Hardware-Version waehlen:
#define HW_POC    1  // handverdrahtetes Erstlingsmodell (alt)
#define HW_BOARD  2  // designte Platine (neu)
#define HW_VERSION HW_POC

#if HW_VERSION == HW_POC
  #define HW_NAME "poc"
#else
  #define HW_NAME "board"
#endif

// Pin-Settings
#if HW_VERSION == HW_POC
  const uint8_t pins_character[] = {16,14,12,13};  // A, B, C, D
#else
  const uint8_t pins_character[] = {13,12,14,16};  // A, B, C, D
#endif
const uint8_t pins_tubes[]     = {3,5,4,0,2,15};

// Number 0-9 to binary value for SN74141 decoder
#if HW_VERSION == HW_POC
  const uint8_t char_to_bin[10][4] = {
    {0,0,0,0}, // 0
    {1,0,0,0}, // 1
    {0,1,0,0}, // 2
    {1,1,0,0}, // 3
    {0,0,1,0}, // 4
    {1,0,1,0}, // 5
    {0,1,1,0}, // 6
    {1,1,1,0}, // 7
    {0,0,0,1}, // 8
    {1,0,0,1}  // 9
  };
#else
  const uint8_t char_to_bin[10][4] = {
    {0,0,0,0}, // 0
    {1,0,0,1}, // 1
    {0,0,0,1}, // 2
    {1,1,1,0}, // 3
    {0,1,1,0}, // 4
    {1,0,1,0}, // 5
    {0,0,1,0}, // 6
    {1,1,0,0}, // 7
    {0,1,0,0}, // 8
    {1,0,0,0}  // 9
  };
#endif

////////////////////////////////////////////////////////////////////////////////


///////////
// Setup //
///////////
void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.print("Nixie v");
  Serial.print(VERSION "-" HW_NAME);
  Serial.println();

  // Set Pin Modes
  for(uint8_t i=0; i<4; i++){
    pinMode(pins_character[i], OUTPUT);
  }
  for(uint8_t i=0; i<6; i++){
    pinMode(pins_tubes[i], OUTPUT);
    digitalWrite(pins_tubes[i],LOW);
  }

  EEPROM.begin(512);
  delay(10);
  
  // EEPROM: Config Written?
  eeprom_configWritten = EEPROM.read(500);
  
  if(eeprom_configWritten == 42){ // Yes
    // EEPROM: SSID
    for(int i = 0; i < 32; i++){
      int val = EEPROM.read(i);
      if(val == 0) break;
      eeprom_ssid += char(val);
    }

    // EEPROM: Password
    for(int i = 32; i < 96; i++) {
      int val = EEPROM.read(i);
      if(val == 0) break;
      eeprom_pass += char(val);
    }

    // EEPROM: ntpServer
    for(int i = 96; i < 128; i++) {
      int val = EEPROM.read(i);
      if(val == 0) break;
      eeprom_ntpServer += char(val);
    }

    // EEPROM: Offset
    eeprom_offset = EEPROM.read(128);
  
    // EEPROM: Summertime
    eeprom_summertime = EEPROM.read(129);

    byte low, high;
    // EEPROM: SynvInterval
    low = EEPROM.read(130);
    high = EEPROM.read(131);
    eeprom_syncInterval = low + ((high << 8)&0xFF00);
    if(eeprom_syncInterval <= 0) eeprom_syncInterval = 720;

    // EEPROM: MultiplexInterval
    low = EEPROM.read(132);
    high = EEPROM.read(133);
    eeprom_multiplexInterval = low + ((high << 8)&0xFF00);
    if(eeprom_multiplexInterval <= 0) eeprom_multiplexInterval = 1500;
  }else{
    eeprom_ssid = "";
    eeprom_pass = "";
    eeprom_ntpServer = "pool.ntp.org";
    eeprom_offset = 1;
    eeprom_summertime = 1;
    eeprom_syncInterval = 720;
    eeprom_multiplexInterval = 1500;
  }
 
  // Connect to WiFi
  WiFi.hostname("nixie");
  if(eeprom_ssid.length() > 1){
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(eeprom_ssid.c_str(), eeprom_pass.c_str());
    Serial.print("Verbinde mit WLAN: ");
    Serial.println(eeprom_ssid);

    for(int i = 0; i < 20; i++){
      if(WiFi.status() == WL_CONNECTED){
        wifi_connected = true;
        break;
      }else{
        delay(500);
      }
    }
  }

  if(wifi_connected){
    Serial.println("WLAN verbunden");
    Serial.print("NTP Server: ");
    Serial.println(eeprom_ntpServer);
    configTime(eeprom_offset * 3600, eeprom_summertime ? 3600 : 0, eeprom_ntpServer.c_str());
  }else{
    Serial.println("WLAN nicht verbunden - starte Access Point");
    setupAP();
  }

  startWebServer();
}

//////////
// Loop //
//////////
uint8_t display_value[6] = {0,0,0,0,0,0};

void loop() {
  if(wifi_connected){
    time_t now = time(nullptr);
    struct tm *lt = localtime(&now);
    display_value[5] = lt->tm_hour/10;
    display_value[4] = lt->tm_hour%10;
    display_value[3] = lt->tm_min/10;
    display_value[2] = lt->tm_min%10;
    display_value[1] = lt->tm_sec/10;
    display_value[0] = lt->tm_sec%10;
  }else{
    display_value[5] = 0;
    display_value[4] = 0;
    display_value[3] = 4;
    display_value[2] = 0;
    display_value[1] = 0;
    display_value[0] = 1;
  }

  // Show Value on Tubes
  multiplex(display_value);

  server.handleClient(); // Webserver
}

////////////////////////////////////////////////////////////////////////////////

/////////////////////////////
// display values on tubes //
/////////////////////////////  
void multiplex(uint8_t *display_value) {
  for(uint8_t t=0; t<6; t++){
    for(uint8_t i=0; i<4; i++){
      digitalWrite(pins_character[i],char_to_bin[display_value[t]][i]);
    }
    digitalWrite(pins_tubes[t],HIGH);
    delayMicroseconds(eeprom_multiplexInterval);
    digitalWrite(pins_tubes[t],LOW);    
  }
}

///////////////
// WiFi Code //
///////////////
void setupAP(void){
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  
  delay(100);
  
  int n = WiFi.scanNetworks();

  scanned_wifis = "<ol>";
  for (int i = 0; i < n; i++){
    scanned_wifis += "<li><a href='#' onClick=\"document.getElementById('ssid').value = '";
    scanned_wifis += WiFi.SSID(i);
    scanned_wifis += "'\">";
    scanned_wifis += WiFi.SSID(i);
    scanned_wifis += "</a></li>";
  }
  scanned_wifis += "</ol>";
  
  delay(100);
  
  WiFi.softAP("Nixie","");
}

void startWebServer(){
  server.on("/", []() {
    html = "<html><head><meta name='viewport' content='width=device-width, initial-scale=1.0'><title>Nixie</title></head><body>";
    html += "<h3>Nixie v";
    html += VERSION "-" HW_NAME;
    html += "</h3>";
    html += "<p>";
    html += scanned_wifis;
    html += "</p><form method='post' action='save'><table>";
    html += "<tr><td>WLAN Name:</td><td><input name='ssid' id='ssid' value='";
    html += eeprom_ssid;
    html += "'></td></tr>";
    html += "<tr><td>WLAN Passwort:</td><td><input name='pass' type='password' value='";
    if(eeprom_pass.length() > 0) html += "***";
    html += "'></td></tr>";
    html += "<tr><td colspan='2'><a href='#' onClick=\"document.getElementById('hidden1').style.display='table-row'; document.getElementById('hidden2').style.display='table-row'; document.getElementById('hidden3').style.display='table-row'; document.getElementById('hidden4').style.display='table-row'; document.getElementById('hidden5').style.display='table-row'; document.getElementById('hidden6').style.display='table-row';\">Advanced Config</a></td></tr>";
    html += "<tr id='hidden1' style='display: none'><td>NTP Server:</td><td><input name='ntpServer' value='";
    html += eeprom_ntpServer;
    html += "'></td></tr>";
    html += "<tr id='hidden2' style='display: none'><td>Offset zu GMT (h):</td><td><input name='offset' value='";
    html += eeprom_offset;
    html += "'></td></tr>";
    html += "<tr id='hidden3' style='display: none'><td>Wechsel Sommer/Winterzeit:</td><td><input type='checkbox' name='summertime' value='1'";
    if(eeprom_summertime == 1) html += " checked";
    html += "></td></tr>";
    html += "<tr id='hidden4' style='display: none'><td>NTP SyncInterval (min):</td><td><input name='syncInterval' value='";
    html += eeprom_syncInterval;
    html += "'></td></tr>";
    html += "<tr id='hidden5' style='display: none'><td>Multiplex Leuchtdauer:</td><td><input name='multiplexInterval' value='";
    html += eeprom_multiplexInterval;
    html += "'></td></tr>";
    html += "<tr id='hidden6' style='display: none'><td>Ziffern:</td><td><a href='/test'>Ziffern testen</a></td></tr>";
    html += "<tr><td><input type='submit' value='speichern'></form></td>";
    html += "<td><form method='post' action='clear' style='display:inline'><input type='submit' value='zur&uuml;cksetzen'></form></td></tr>";
    html += "</table></body></html>";
    server.send(200, "text/html", html);
  });
  server.on("/save", []() {
    String new_ssid = server.arg("ssid");
    String new_pass = server.arg("pass");
    String new_ntpServer = server.arg("ntpServer");
    byte new_offset = server.arg("offset").toInt();
    byte new_summertime = server.arg("summertime").toInt();
    int new_syncInterval = server.arg("syncInterval").toInt();
    int new_multiplexInterval = server.arg("multiplexInterval").toInt();
    
    for (int i = 0; i < 501; ++i) { EEPROM.write(i, 0); } // Clear EEPROM

    // EEPROM: SSID
    for (int i = 0; i < new_ssid.length(); i++){
      EEPROM.write(i, new_ssid[i]);
    }

    // EEPROM: Password
    if(new_pass == "***") new_pass = eeprom_pass;   
    for (int i = 0; i < new_pass.length(); i++){
      EEPROM.write(32+i, new_pass[i]);
    }    

    // EEPROM: ntpServer
    for (int i = 0; i < new_ntpServer.length(); i++){
      EEPROM.write(96+i, new_ntpServer[i]);
    }

    // EEPROM: Offset
    EEPROM.write(128, new_offset);

    // EEPROM: Summertime
    EEPROM.write(129, new_summertime);

    byte low, high;
    // EEPROM: SynvInterval
    low=new_syncInterval&0xFF;
    high=(new_syncInterval>>8)&0xFF;
    EEPROM.write(130, low);
    EEPROM.write(131, high);

    // EEPROM: MultiplexInterval
    low=new_multiplexInterval&0xFF;
    high=(new_multiplexInterval>>8)&0xFF;
    EEPROM.write(132, low);
    EEPROM.write(133, high);

    EEPROM.write(500,42); // Config Written = yes
    EEPROM.commit();

    html = "<html><head><title>Nixie</title><meta http-equiv='refresh' content='10; url=/'></head><body>";
    html += "<h3>Nixie</h3>";
    html += "Einstellungen gespeichert - starte neu..<br>";
    html += "(wenn es nicht funktioniert, Uhr bitte hart restarten (Stecker ziehen))";
    html += "</body></html>";
    server.send(200, "text/html", html);
    delay(100);
    ESP.restart();
  });
  server.on("/clear", []() {
    for (int i = 0; i < 501; i++) { 
      EEPROM.write(i, 0);
    }
    EEPROM.commit();
    
    html = "<html><head><title>Nixie</title><meta http-equiv='refresh' content='10; url=/'></head><body>";
    html += "<h3>Nixie</h3>";
    html += "Einstellungen zur&uuml;ckgesetzt - starte neu..<br>";
    html += "(wenn es nicht funktioniert, Uhr bitte hart restarten (Stecker ziehen))";
    html += "</body></html>";
    server.send(200, "text/html", html);
    delay(100);
    ESP.restart();
  });
  server.on("/test", []() {
    html = "<html><head><title>Nixie</title></head><body>";
    html += "<h3>Nixie</h3>";
    html += "Teste alle Ziffern ..<br>";
    html += "<a href='/restart'>Uhr restarten</a>";
    html += "</body></html>";
    server.send(200, "text/html", html);

    uint8_t i = 0;
    unsigned long previousMillis = 0;
    while(true){
      unsigned long currentMillis = millis();

      if(currentMillis - previousMillis >= 1000){
        previousMillis = currentMillis;

        i++;
        if(i == 10) i = 0;

        display_value[5] = i;
        display_value[4] = i;
        display_value[3] = i;
        display_value[2] = i;
        display_value[1] = i;
        display_value[0] = i;
      }

      // Show Value on Tubes
      multiplex(display_value);

      server.handleClient(); // Webserver
    }
  });
  server.on("/restart", []() {
    html = "<html><head><title>Nixie</title><meta http-equiv='refresh' content='10; url=/'></head><body>";
    html += "<h3>Nixie</h3>";
    html += "starte neu..<br>";
    html += "(wenn es nicht funktioniert, Uhr bitte hart restarten (Stecker ziehen))";
    html += "</body></html>";
    server.send(200, "text/html", html);
    delay(100);
    ESP.restart();
  });

  server.begin();
}
