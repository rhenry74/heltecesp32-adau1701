
#ifndef ADAU1701_H
#define ADA1701_H

class Settings
{
  public: 
  
  byte color[4];
  byte treble[4];
  byte bass[4];
  byte midrange[4];
  byte xover[4];
  byte mono[4];
  byte volume[4];
  int mute; //0 when not muting, the volume to restore when muting
  bool direct;
};

String BassDisplay(bool oled = true);
String TrebleDisplay(bool oled = true);
String MidrangeDisplay(bool oled = true);
String CrossoverDisplay(bool oled = true);
String VolumeDisplay(bool oled = true);
String MonoDisplay(bool oled = true);
String ColorDisplay(bool oled = true);
String MuteDisplay(bool oled = true);

#endif
