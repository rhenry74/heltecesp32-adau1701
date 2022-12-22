/*
rhenry74
I2C ADA1701 HowTo
*/

#include "Arduino.h"
#include "heltec.h"
#include "wire.h"
#include "EEPROM.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>

#include "ADAU1701.h"
#include "AmpHTML.h"

//pin mappings
#define I2C_SDA 4
#define I2C_SCL 15
#define DSP_ADDR 0x34

#define LED1 25

#define FREE1 23

//rotary encoders
#define VOLUME_RED 19
#define VOLUME_WHITE 22
#define XOVER_RED 27
#define XOVER_WHITE 26
#define TREBLE_RED 2
#define TREBLE_WHITE 0
#define BASS_RED 17
#define BASS_WHITE 5
#define MIDRANGE_RED 12
#define MIDRANGE_WHITE 14

//inputs
#define COLOR 35
#define MONO 18

//address of DCs in i2c howto.dspproj
//see IC 1:Params in Analog Devices - Sigma Studio
#define CUTOFF_DC 0
#define BASS_DC 1
#define TREBLE_DC 2
#define COLOR_DC 3
#define VOLUME_DC 4
#define MIDRANGE_DC 5
#define MONO_DC 6

//sadly my wifi implementation is incomplete
//you should be able to connect in access point mode and set your ssid and password thru the web ui
const char *ssid = "primary";
const char *password = "yeahright";

const char *ssid2 = "backup";
const char *password2 = "yeahright";

WebServer server(80);

Settings settings;

long last_flashed = 0;
byte led_state = LOW;

void FlashLED()
{
  last_flashed = millis();
}

long last_displayed = 0;
String display_line1 = String("rhenry74");
String display_line2 = String("ADAU1701");
bool cleared = false;

void WriteSetting(int &addr, byte setting[], int setting_size)
{
  int idx = setting_size-1;
  while (idx >= 0)
  {
    EEPROM.write(addr, setting[idx]);
    
    //Serial.print(addr);
    //Serial.print("-Setting>");
    //Serial.println(setting[idx]);
    
    addr++;
    idx--;
  }
}

void ReadEEPROM(int &addr, byte setting[], int setting_size)
{
  int idx = setting_size -1;
  while (idx >= 0)
  {
    setting[idx] = EEPROM.read(addr);
    addr++;
    idx--;
  }
}

void ReadAllSettings()
{
  Serial.println("Reading settings from EEPROM");
  int addr = 0;
  ReadEEPROM(addr, settings.color, sizeof(settings.color));
  Serial.print("color ");
  printArray(settings.color, sizeof(settings.color));

  ReadEEPROM(addr, settings.treble, sizeof(settings.treble));  
  Serial.print("treble ");
  printArray(settings.treble, sizeof(settings.treble));

  ReadEEPROM(addr, settings.bass, sizeof(settings.bass));
  Serial.print("bass ");
  printArray(settings.bass, sizeof(settings.bass));

  ReadEEPROM(addr, settings.midrange, sizeof(settings.midrange));
  Serial.print("midrange ");
  printArray(settings.midrange, sizeof(settings.midrange));

  ReadEEPROM(addr, settings.xover, sizeof(settings.xover));
  Serial.print("xover ");
  printArray(settings.xover, sizeof(settings.xover));

  ReadEEPROM(addr, settings.mono, sizeof(settings.mono));  
  Serial.print("mono ");
  printArray(settings.mono, sizeof(settings.mono));

  ReadEEPROM(addr, settings.volume, sizeof(settings.volume));
  Serial.print("volume ");
  printArray(settings.volume, sizeof(settings.volume));

}

void SetDisplay(String line1, String line2)
{
  display_line1 = line1;
  display_line2 = line2;
  last_displayed = millis();
  cleared = false;
  
  //since we are displaying something we probably changed a settig
  //lets save the settings to eeprom
  int addr=0;
  WriteSetting(addr, settings.color, sizeof(settings.color));
  WriteSetting(addr, settings.treble, sizeof(settings.treble));
  WriteSetting(addr, settings.bass, sizeof(settings.bass));
  WriteSetting(addr, settings.midrange, sizeof(settings.midrange));
  WriteSetting(addr, settings.xover, sizeof(settings.xover));
  WriteSetting(addr, settings.mono, sizeof(settings.mono));
  WriteSetting(addr, settings.volume, sizeof(settings.volume));
  EEPROM.commit();
}

void UpdateDisplay()
{
  if (millis() - last_displayed < 10000)
  {
    Heltec.display->clear();
    Heltec.display->setFont(ArialMT_Plain_16);
    Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
    Heltec.display->drawString(64, 5, display_line1);
    Heltec.display->setFont(ArialMT_Plain_24);
    Heltec.display->drawString(64, 30, display_line2);
    Heltec.display->display();    
  }
  else if (!cleared)
  {
    Heltec.display->clear();
    Heltec.display->display();
    cleared = true;
  }

  if (millis() - last_flashed < 500)
  {
    if (led_state == LOW)
    {
      digitalWrite(LED1,HIGH);
      led_state = HIGH;
    }
  }
  else
  {
    if (led_state == HIGH)
    {
      digitalWrite(LED1,LOW);
      led_state = LOW;
    }
  }
}

byte char2byte(char input)
{
  if(input >= '0' && input <= '9')
    return input - '0';
  if(input >= 'A' && input <= 'F')
    return input - 'A' + 10;
  if(input >= 'a' && input <= 'f')
    return input - 'a' + 10;
  throw "char2byte invallid argument";
}

bool SafeWriteDSP(byte ms_addr_byte, byte ls_addr_byte, byte write_buffer[])
{
  //implement safe load

  //let's get the contents of the core register first
  byte register_data[2]; 
  /* is this causing the pop?
  ReadDSP16(0x08, 0x1C, register_data);    
  Serial.print("safecore before:");
  Serial.print(register_data[1], HEX);
  Serial.print(",");
  Serial.println(register_data[0], HEX);
  */
  
  //there are 5 safe load parent registers
  //we'll only be loading 4 bytes of the 20

  //there is a dummy byte :-( sooooooo string togteher a special stream
  byte dbytes[] = {0x08, 0x10, 0x00, 
    write_buffer[3], write_buffer[2], write_buffer[1], write_buffer[0]};
  Wire.beginTransmission(DSP_ADDR); 
  int a = Wire.write(dbytes,7);
  int b = Wire.endTransmission(true);
  if (b != 0 || a != 7)
  {
    Serial.print("NAK in SafeWriteDSP data ");
    Serial.println(a);
    Serial.println(b);
    return false;  
  }
  
  //load the address for pair 0 
  byte abytes[] = {0x08, 0x15, ms_addr_byte, ls_addr_byte};
  Wire.beginTransmission(DSP_ADDR); // transmit to device at DSP_addr
  a = Wire.write(abytes,4);
  b = Wire.endTransmission(true);
  if (b != 0 || a != 4)
  {
    Serial.print("NAK in SafeWriteDSP address ");
    Serial.println(a);
    Serial.println(b);
    return false;
  }
 
  /*
  //now we set a bit in a control register         
  //set bit 5
  register_data[0] = register_data[0] | B00100000;
  Serial.print("safecore writing:");
  Serial.print(register_data[1], HEX);
  Serial.print(",");
  Serial.println(register_data[0], HEX);
*/
  register_data[1] = 0x00;
  register_data[0] = 0x3C;

  byte rbytes[] = {0x08, 0x1C, register_data[1], register_data[0]};
  Wire.beginTransmission(DSP_ADDR); // transmit to device at DSP_addr
  a = Wire.write(rbytes, 4);
  b = Wire.endTransmission(true);
  if (b != 0 || a != 4)
  {
    Serial.print("NAK in SafeWriteDSP register ");
    Serial.println(a);
    Serial.println(b);
    return false;
  }

/*
  delay(20);

  
  ReadDSP16(0x08, 0x1C, register_data);
  Serial.print("safecore after:");
  Serial.print(register_data[1], HEX);
  Serial.print(",");
  Serial.println(register_data[0], HEX); 

  byte result[4];
  ReadDSP32(ms_addr_byte, ls_addr_byte, result);
  //result should match write_buffer

  Serial.print("write_buffer:");
  Serial.print(write_buffer[3], HEX);
  Serial.print(",");
  Serial.print(write_buffer[2], HEX);
  Serial.print(",");
  Serial.print(write_buffer[1], HEX);
  Serial.print(",");
  Serial.println(write_buffer[0], HEX);
  Serial.print("result:");
  Serial.print(result[3], HEX);
  Serial.print(",");
  Serial.print(result[2], HEX);
  Serial.print(",");
  Serial.print(result[1], HEX);
  Serial.print(",");
  Serial.println(result[0], HEX); 
  */

  return true;
}

String TrebleDisplay(bool oled)
{
  String val = String((int)settings.treble[0] - 9);
  if (oled)
    SetDisplay(String("Treble"), val);
  return val;
}

void TrebleUp()
{
  Serial.print("TrebleUp:");
  if (BypassCheck())
  {    
    if (settings.treble[0] < 18)
      settings.treble[0]++;
     
    Serial.println((int)settings.treble[0]);
    SafeWriteDSP(0,TREBLE_DC,settings.treble);  
  
    TrebleDisplay();
  }
}

void TrebleDown()
{
  Serial.print("TrebleDown:");
  if (BypassCheck())
  { 
    if (settings.treble[0] > 0)
      settings.treble[0]--;
    
    Serial.println((int)settings.treble[0]);
    SafeWriteDSP(0,TREBLE_DC,settings.treble);  
  
    TrebleDisplay();
  }
}

String BassDisplay(bool oled)
{
  String val = String((int)settings.bass[0] - 9);
  if (oled)
    SetDisplay(String("Bass"), val);
  return val;
}

void BassUp()
{
  Serial.print("BassUp:");
  if (BypassCheck())
  {
    if (settings.bass[0] < 18)
      settings.bass[0]++;
     
    Serial.println((int)settings.bass[0]);
    SafeWriteDSP(0,BASS_DC,settings.bass);  
  
    BassDisplay();
  }
}

void BassDown()
{
  Serial.print("BassDown:");
  if (BypassCheck())
  {    
    if (settings.bass[0] > 0)
      settings.bass[0]--;
    
    Serial.println((int)settings.bass[0]);
    SafeWriteDSP(0,BASS_DC,settings.bass);  
  
    BassDisplay();
  }
}

String MidrangeDisplay(bool oled)
{
  String val = String((int)settings.midrange[0] - 9);
  if (oled)
    SetDisplay(String("Midrange"), val);
  return val;
}

void MidrangeUp()
{
  Serial.print("MidrangeUp:");
  if (BypassCheck())
  {
    if (settings.midrange[0] < 18)
      settings.midrange[0]++;
     
    Serial.println((int)settings.midrange[0]);
    SafeWriteDSP(0,MIDRANGE_DC,settings.midrange);  
  
    MidrangeDisplay();
  }
}

void MidrangeDown()
{
  Serial.print("MidrangeDown:");
  if (BypassCheck())
  {
    if (settings.midrange[0] > 0)
      settings.midrange[0]--;
    
    Serial.println((int)settings.midrange[0]);
    SafeWriteDSP(0,MIDRANGE_DC,settings.midrange);  
  
    MidrangeDisplay();
  }
}

int xover_freqs[] = {20, 24, 27, 31, 39, 46, 51, 62, 78, 90, 105, 120, 150, 175, 200, 230, 260, 315, 390};

String CrossoverDisplay(bool oled)
{
  String val = String(xover_freqs[18-settings.xover[0]]);
  if (oled)
    SetDisplay(String("Crossover"), val); 
  return val;
}

void CrossoverDown()
{
  Serial.print("CrossoverDown:");
  if (settings.xover[0] < 18)
    settings.xover[0]++;
    
  Serial.println((int)settings.xover[0]);
  SafeWriteDSP(0,CUTOFF_DC,settings.xover);  

  CrossoverDisplay();
}


void CrossoverUp()
{
  Serial.print("CrossoverUp:");
  if (settings.xover[0] > 0)
    settings.xover[0]--;
  
  Serial.println((int)settings.xover[0]);
  SafeWriteDSP(0,CUTOFF_DC,settings.xover);  

  CrossoverDisplay();
}

int vol_inc = 0x40000;

String VolumeDisplay(bool oled)
{
  long vol = settings.volume[3];
  vol = vol << 8;
  vol = vol | settings.volume[2];
  vol = vol << 8;
  vol = vol | settings.volume[1];
  vol = vol << 8;
  vol = vol | settings.volume[0];

  String val = String((double)vol / vol_inc);

  if (oled)
    SetDisplay(String("Volume"), val);
  return val;
}

void VolumeDown()
{
  Serial.print("VolumeDown:");

  long vol = settings.volume[3];
  vol = vol << 8;
  vol = vol | settings.volume[2];
  vol = vol << 8;
  vol = vol | settings.volume[1];
  vol = vol << 8;
  vol = vol | settings.volume[0];

  int inc = vol_inc;
  if (vol / vol_inc < 15)
  {
    inc = vol_inc / 2;
  }
  if(vol / vol_inc < 5)
  {
    inc = vol_inc / 4;
  }

  Serial.print(vol, HEX);
  if (vol - inc > 0)
    vol = vol - inc;
  else
    vol = 0;
  Serial.print("->");
  Serial.println(vol, HEX);
    
  settings.volume[0] = vol & 0xFF;
  vol = vol >> 8;
  settings.volume[1] = vol & 0xFF;
  vol = vol >> 8;
  settings.volume[2] = vol & 0xFF;
  vol = vol >> 8;
  settings.volume[3] = vol & 0xFF;

  SafeWriteDSP(0,VOLUME_DC,settings.volume);   

  VolumeDisplay();

}

void VolumeUp()
{
  Serial.print("VolumeUp:");

  long vol = settings.volume[3];
  vol = vol << 8;
  vol = vol + settings.volume[2];
  vol = vol << 8;
  vol = vol + settings.volume[1];
  vol = vol << 8;
  vol = vol + settings.volume[0];

  int inc = vol_inc;
  if (vol / vol_inc < 15)
  {
    inc = vol_inc / 2;
  }
  if(vol / vol_inc < 5)
  {
    inc = vol_inc / 4;
  }
  
  Serial.print(vol, HEX);
  if (vol + inc < 0x1000000)
    vol = vol + inc;
  else
    vol = 0x1000000;
  Serial.print("->");
  Serial.println(vol, HEX);
  
  settings.volume[0] = vol & 0xFF;
  vol = vol >> 8;
  settings.volume[1] = vol & 0xFF;
  vol = vol >> 8;
  settings.volume[2] = vol & 0xFF;
  vol = vol >> 8;
  settings.volume[3] = vol & 0xFF;
  
  SafeWriteDSP(0,VOLUME_DC,settings.volume); 

  VolumeDisplay();

  settings.mute = 0;

}

String MuteDisplay(bool oled)
{
  String val;
  if (settings.mute == 0)  
    val = String("Off");
  else
    val = String("On");

  if (oled)
    SetDisplay(String("Mute"), val);
  return val;
}

void MuteToggle()
{
  Serial.print("MuteToggle:");
  if (settings.mute == 0)//not muted
  {
    long vol = settings.volume[3];
    vol = vol << 8;
    vol = vol + settings.volume[2];
    vol = vol << 8;
    vol = vol + settings.volume[1];
    vol = vol << 8;
    vol = vol + settings.volume[0];
    settings.mute = vol;
    settings.volume[3] = 0;
    settings.volume[2] = 0;
    settings.volume[1] = 0;
    settings.volume[0] = 0;
  }
  else
  {
    long vol = settings.mute;
    settings.volume[0] = vol & 0xFF;
    vol = vol >> 8;
    settings.volume[1] = vol & 0xFF;
    vol = vol >> 8;
    settings.volume[2] = vol & 0xFF;
    vol = vol >> 8;
    settings.volume[3] = vol & 0xFF;
    settings.mute = 0;
  }
  Serial.println(settings.mute, HEX);
  SafeWriteDSP(0,VOLUME_DC,settings.volume);
  MuteDisplay();

}

String MonoDisplay(bool oled)
{
  String val;
  if (settings.mono[0] == 1)  
    val = String("On");
  else
    val = String("Off");

  if (oled)
    SetDisplay(String("Mono"), val);
  return val;
}

void MonoToggle()
{
  Serial.print("MonoToggle:");
  if (BypassCheck())
  {
    if (settings.mono[0] == 0)
      settings.mono[0] = 1;
    else
      settings.mono[0] = 0;
    Serial.println(settings.mono[0], HEX);
    SafeWriteDSP(0,MONO_DC,settings.mono);
  
    MonoDisplay();
  }
}

String ColorDisplay(bool oled)
{
  String val;
  if (settings.color[0] == 0)  
    val = String("On");
  else
    val = String("Off");

  if (oled)
    SetDisplay(String("Color"), val);
  return val;
}

void ColorToggle()
{
  Serial.print("ColorToggle:");
  if (settings.color[0] == 0)
    settings.color[0] = 1;
  else
    settings.color[0] = 0;
  Serial.println(settings.color[0], HEX);
  SafeWriteDSP(0,COLOR_DC,settings.color);

  ColorDisplay();

}

bool BypassCheck()
{
  if (settings.color[0] == 0)
  {
    return true;
  }
  else
  {
    SetDisplay(String("Color"), String("Off"));
    return false;
  }
}

void printArray(byte dat[], int len)
{
  int idx=len-1;
  while(idx > 0)
  {
    Serial.print(dat[idx], HEX);
    Serial.print(',');
    idx--;
  }
  Serial.println(dat[idx], HEX);
}

char cmd = 's';
char amp_control_cmd[4];

void setup() 
{
  //Heltec.begin initializes Serial
    
  pinMode(LED1,OUTPUT); 
  digitalWrite(LED1,HIGH); 
  delay(1000); //give the DSP time to spin up, before putting any drain on the I2C pins
  pinMode(I2C_SCL, INPUT);
  pinMode(I2C_SDA, INPUT);  
  digitalWrite(LED1,LOW);
    
  Heltec.begin(true /*DisplayEnable Enable*/, true /*LoRa Disable*/, true /*Serial Enable*/);
  //Heltec.display->flipScreenVertically();
  Heltec.display->setColor(WHITE);
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->clear();
  Heltec.display->display();

  Wire.setClock (400000L);

 int data_size = sizeof(settings.color) + sizeof(settings.treble) + sizeof(settings.bass) + sizeof(settings.midrange) + sizeof(settings.xover) +
     sizeof(settings.mono) + sizeof(settings.volume);

  if (!EEPROM.begin(data_size))
  {
    Serial.println("failed to initialise EEPROM");
    digitalWrite(LED1,HIGH); 
    SetDisplay(String("EEPROM"), String("FAIL"));
    delay(1000000);
  }
  else
  {
    ReadAllSettings(); //run howto_eeprom_init on an uninitialized board (or setting these values could cause unpleasant things to happen!)
    //initialize the DSP
    SafeWriteDSP(0,TREBLE_DC,settings.treble);
    SafeWriteDSP(0,COLOR_DC,settings.color);
    SafeWriteDSP(0,VOLUME_DC,settings.volume);
    SafeWriteDSP(0,CUTOFF_DC,settings.xover);
    SafeWriteDSP(0,MIDRANGE_DC,settings.midrange);
    SafeWriteDSP(0,MONO_DC,settings.mono);
    SafeWriteDSP(0,BASS_DC,settings.bass);
    
  }

  pinMode(COLOR, INPUT_PULLUP);
  pinMode(MONO, INPUT_PULLUP);  
  pinMode(XOVER_WHITE, INPUT_PULLUP);
  pinMode(XOVER_RED, INPUT_PULLUP);
  pinMode(TREBLE_WHITE, INPUT_PULLUP);
  pinMode(TREBLE_RED, INPUT_PULLUP);
  pinMode(BASS_WHITE, INPUT_PULLUP);
  pinMode(BASS_RED, INPUT_PULLUP);
  pinMode(MIDRANGE_WHITE, INPUT_PULLUP);
  pinMode(MIDRANGE_RED, INPUT_PULLUP);
  pinMode(VOLUME_WHITE, INPUT_PULLUP);
  pinMode(VOLUME_RED, INPUT_PULLUP);
  
}

void SetupWebServer()
{
   server.on("/", []() {
      server.send(200, "text/html", GenerateHTML(settings));
    });
  
    server.on("/amp/{}", []() 
    {
     cmd = 'A'; 
     if (strlen(server.pathArg(0).c_str()) < 4)
     {
       strcpy(amp_control_cmd, server.pathArg(0).c_str());
       Serial.print(amp_control_cmd);
       Serial.println(" from web");
     }
     else
     {
       server.send(404, "text/plain", "Amp: command unknown");
     }
    });
    
    server.begin();
    Serial.println("HTTP server started");
}

int wifi_state = 0;// not started
long wifi_time = 0;
int wifi_try = 0;

void ProcessWIFI()
{
  if (wifi_state == 0)
  {
    WiFi.mode(WIFI_AP);
    WiFi.begin(ssid, password);
    Serial.println(String("Connecting to ") + String(ssid));
    wifi_state = 1; //waiting for connection
    wifi_time = millis();
    return;
  }

  if (wifi_state == 1)
  {
    int stat = WiFi.status();   
      
    if (stat != WL_CONNECTED) 
    {
      if (wifi_try > 6)
      {
        SetDisplay(String( String(ssid)), "No WIFI");
        Serial.print("WIFI fail: ");
        Serial.println(stat);
        wifi_state = 3;
        return;
      }
      if (millis() - wifi_time > 60000)
      {
        SetDisplay(String("WIFI Error ") + String(wifi_try), String(WiFi.status()));
        Serial.print("WIFI fail: ");
        Serial.println(stat);
               
        if (wifi_try==2)
        {
          WiFi.disconnect(true); // disconnects STA Mode
          delay(1000);
          WiFi.softAPdisconnect(true);// disconnects AP Mode 
          delay(1000);  
        }
        
        if (wifi_try==4)
        {
          ssid = ssid2;
          password=password2;
        }
        
        WiFi.disconnect();
        delay(100);
        WiFi.mode(WIFI_STA);
        delay(100);
        WiFi.mode(WIFI_AP);
        WiFi.begin(ssid, password);
        
        wifi_try++;
        wifi_state=0;
        wifi_time = millis();
        return;
      }
      return;
    }
    
    Serial.print("Connected to ");
    Serial.println(ssid);
    
    IPAddress localIPaddr = WiFi.localIP();
    IPAddress softIP = WiFi.softAPIP();
    Serial.print("Local IP address: ");
    Serial.println(localIPaddr);
    Serial.print("Soft IP address: ");
    Serial.println(softIP);
    
    SetDisplay(String(ssid), localIPaddr.toString());
    SetupWebServer();
   
    wifi_state = 2;
    return;
  }

  if (wifi_state == 2)
  {
    if (WiFi.status() == WL_CONNECTED) 
    {
      server.handleClient();
    }
    else
    {
      wifi_state=0;
      return;
    }

    if (millis() - wifi_time > 60000)
    {
      Serial.print("Connected to ");
      Serial.println(ssid);      
      IPAddress localIPaddr = WiFi.localIP();
      IPAddress softIP = WiFi.softAPIP();
      Serial.print("Local IP address: ");
      Serial.println(localIPaddr);
      Serial.print("Soft IP address: ");
      Serial.println(softIP);
      wifi_time = millis();
    }
    return;
  }

  if (wifi_state == 3)
  {
    //enter access point mode
    Serial.println("Configuring access point mode.");
    WiFi.disconnect();
    WiFi.softAPdisconnect(true);
    ssid="ADA1701HT";
    password = "LetsGo!";
    WiFi.softAP(ssid, password);
    //WiFi.status() should always be WL_CONNECTED in this mode, right?
    IPAddress softIP = WiFi.softAPIP();
    
    Serial.print("Soft IP address: ");
    Serial.println(softIP);

    SetupWebServer();
    
    SetDisplay(String(ssid), softIP.toString());
    
    wifi_state = 5;
    return;
  }

  if (wifi_state == 5)
  {    
    server.handleClient();    
    return;
  }
}

void IPDisplay()
{
  if (wifi_state == 2)
  {
    IPAddress localIPaddr = WiFi.localIP();
    SetDisplay(String(ssid), localIPaddr.toString());
  }
  if (wifi_state == 5)
  {
    IPAddress softIP = WiFi.softAPIP();
    SetDisplay(String(ssid), softIP.toString());
  }
}

long debounce = 0;
byte last_xover = 0;
byte last_treble = 0;
byte last_volume = 0;
byte last_bass = 0;
byte last_midrange = 0;

long display_sweep = 0;

void loop() {

  ProcessWIFI();
  
  UpdateDisplay();

  byte write_buffer[4];
  byte dsp_adr_msb = 0;
  byte dsp_adr_lsb = 0;

  if (Serial.available())
  {
    cmd = Serial.read();

    if (cmd == 'a')
    {
      //amplifier commands
      Serial.read();//throw away space
      amp_control_cmd[0] = Serial.read();
      amp_control_cmd[1] = Serial.read();
      if (Serial.available())
        amp_control_cmd[2] = Serial.read();
      else
        amp_control_cmd[2] = 0;
      amp_control_cmd[3] = 0;
    }
  }
  
  if (cmd != 's')
  { 
    FlashLED();
    
    if (cmd == 'a' || cmd == 'A') //A is from the web
    {
      String amp_control_cmd_String = amp_control_cmd;
      Serial.print(amp_control_cmd_String);
      Serial.print(":");
      if (strcmp(amp_control_cmd,"tu") == 0)
        TrebleUp();
      else if (strcmp(amp_control_cmd,"td") == 0)
        TrebleDown();
      else if (strcmp(amp_control_cmd,"bu") == 0)
        BassUp();
      else if (strcmp(amp_control_cmd,"bd") == 0)
        BassDown();
      else if (strcmp(amp_control_cmd,"mu") == 0)
        MidrangeUp();
      else if (strcmp(amp_control_cmd,"md") == 0)
        MidrangeDown();
      else if (strcmp(amp_control_cmd,"cot") == 0)
        ColorToggle();
      else if (strcmp(amp_control_cmd,"xu") == 0)
        CrossoverUp();
      else if (strcmp(amp_control_cmd,"xd") == 0)
        CrossoverDown();
      else if (strcmp(amp_control_cmd,"vd") == 0)
        VolumeDown();
      else if (strcmp(amp_control_cmd,"vu") == 0)
        VolumeUp();
      else if (strcmp(amp_control_cmd,"mut") == 0)
        MuteToggle();
      else if (strcmp(amp_control_cmd,"mot") == 0)
        MonoToggle();
      else if (strcmp(amp_control_cmd,"ls") == 0)
      {
        if (cmd == 'a')//from front panel
        {          
          display_sweep = millis();
        }
      }
      else
      {
        if (cmd == 'A')
          server.send(404, "text/plain", "Amp: '" + amp_control_cmd_String + "' unknown");
        Serial.println("amp_control_cmd unknown");
        cmd = 's';
        return;
      }

      if (cmd == 'A')//from the web
      {
        //from the web 
        String response = GenerateHTML(settings);
        server.send(200, "text/html", response);
      }
    }
    
    cmd = 's';
  }

  if (millis() - debounce > 500)
  {
    bool color = !digitalRead(COLOR);  //is the button pressed  
    if (color)
    {
      ColorToggle();
    }
    debounce = millis();
  }
  
  if (millis() - debounce > 500)
  {
    bool mono = !digitalRead(MONO);    
    if (mono)
    {
      MonoToggle();
    }
    debounce = millis();
  }  

  //rotary encoders  
  last_xover = last_xover << 1;
  last_xover = last_xover + digitalRead(XOVER_RED);
  last_xover = last_xover << 1;
  last_xover = last_xover + digitalRead(XOVER_WHITE);
  last_xover = last_xover & 0x0F;

 if(last_xover == 0x07)
  {
    CrossoverDown();
  }
  if(last_xover == 0x0B)
  {
    CrossoverUp();
  }

  last_treble = last_treble << 1;
  last_treble = last_treble + digitalRead(TREBLE_RED);
  last_treble = last_treble << 1;
  last_treble = last_treble + digitalRead(TREBLE_WHITE);
  last_treble = last_treble & 0x0F;

  if(last_treble == 0x07)
  {
    TrebleDown();
  }
  if(last_treble == 0x0B)
  {
    TrebleUp();
  }

  last_volume = last_volume << 1;
  last_volume = last_volume + digitalRead(VOLUME_RED);
  last_volume = last_volume << 1;
  last_volume = last_volume + digitalRead(VOLUME_WHITE);
  last_volume = last_volume & 0x0F;

  if(last_volume == 0x07)
  {
    VolumeDown();
  }
  if(last_volume == 0x0B)
  {
    VolumeUp();
  }

  last_bass = last_bass << 1;
  last_bass = last_bass + digitalRead(BASS_RED);
  last_bass = last_bass << 1;
  last_bass = last_bass + digitalRead(BASS_WHITE);
  last_bass = last_bass & 0x0F;

  if(last_bass == 0x07)
  {
    BassDown();
  }
  if(last_bass == 0x0B)
  {
    BassUp();
  }

  last_midrange = last_midrange << 1;
  last_midrange = last_midrange + digitalRead(MIDRANGE_RED);
  last_midrange = last_midrange << 1;
  last_midrange = last_midrange + digitalRead(MIDRANGE_WHITE);
  last_midrange = last_midrange & 0x0F;

  if(last_midrange == 0x07)
  {
    MidrangeDown();
  }
  if(last_midrange == 0x0B)
  {
    MidrangeUp();
  }
  
}
