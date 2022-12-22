/*
   EEPROM Write

   Stores initial DSP values into the EEPROM.
   These values will stay in the EEPROM when the board is
   turned off and will be retrieved by the howto sketch.
*/

#include "EEPROM.h"

byte color[4] = {0x00, 0x00, 0x00, 0x00};
byte treble[4] = {0x00, 0x00, 0x00, 0x09};
byte bass[4] = {0x00, 0x00, 0x00, 0x09};
byte midrange[4] = {0x00, 0x00, 0x00, 0x09};
byte xover[4] = {0x00, 0x00, 0x00, 0x10};
byte mono[4] = {0x00, 0x00, 0x00, 0x00};
byte volume[4] = {0x00, 0x20, 0x00, 0x00};

int data_size;
int addr = 0;

void WriteSetting(byte setting[], int setting_size)
{
  int idx = 0;
  while (idx < setting_size)
  {
    EEPROM.write(addr, setting[idx]);
    addr++;
    idx++;
  }
}

void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("start...");

  data_size = sizeof(color) + sizeof(treble) + sizeof(bass) + sizeof(midrange) + sizeof(xover) +
    sizeof(mono) + sizeof(volume);
  
  if (!EEPROM.begin(data_size))
  {
    Serial.println("failed to initialise EEPROM"); delay(1000000);
  }
  
  Serial.println(" bytes read from Flash . Values are:");
  for (int i = 0; i < data_size; i++)
  {
    Serial.print(byte(EEPROM.read(i))); Serial.print(" ");
  }
  Serial.println();
  
  // write the amplifier settings to the appropriate byte of the EEPROM.
  // these values will remain there when the board is
  // turned off.

  WriteSetting(color, sizeof(color));
  WriteSetting(treble, sizeof(treble));
  WriteSetting(bass, sizeof(bass));
  WriteSetting(midrange, sizeof(midrange));
  WriteSetting(xover, sizeof(xover));
  WriteSetting(mono, sizeof(mono));
  WriteSetting(volume, sizeof(volume));

  EEPROM.commit();

  Serial.println(data_size);
}

void loop()
{
  delay(100);

}
