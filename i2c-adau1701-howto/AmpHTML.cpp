#include "Arduino.h"

#include "ADAU1701.h"
#include "AmpHTML.h"

String GenerateUpDown(String heading, String display_val, String cmd)
{
  String html = String("<tr > <td colspan=3 align=\"center\"> <p>");
  html = html + heading;
  html = html + String("</p> </td> </tr> <tr > <td width=\"32px\" align=\"center\" > <a href=\"./");
  html = html + cmd;
  html = html + String("d\"><input type=\"button\" value=\"&#8681\"></a> </td> <td width=\"92px\" align=\"center\" > ");
  html = html + display_val;
  html = html + String("</td><td width=\"32px\" align=\"center\" > <a href=\"./");
  html = html + cmd;
  html = html + String("u\"><input type=\"button\" value=\"&#8679\"></a> </td> </tr>");
  return html;
}

String GenerateToggle(String heading, String display_val, String cmd)
{
  String html = String("<tr > <td colspan=3 align=\"center\"> <p>");
  html = html + heading;
  html = html + String("</p> </td> </tr> <tr > <td width=\"32px\" align=\"center\" > <a href=\"./");
  html = html + cmd;
  html = html + String("t\"><input type=\"button\" value=\"&#8660\"></a> </td> <td colspan=2 align=\"center\" > ");
  html = html + display_val;
  html = html + String("</td></tr>");
  return html;
}

String GenerateHTML(Settings settings)
{
  String html = String("<html><head><title>ADAU1701 HowTo by rhenry74</title><style>body { background-color: #99ebf0; } ");
  html = html + String("table {  background-color: #ccddee; } </style></head> <body lang=\"en-US\" > <table border>");

  html = html + GenerateUpDown(String("Volume"), VolumeDisplay(false), String("v"));  
  html = html + GenerateToggle(String("Mute"), MuteDisplay(false), String("mu"));
  html = html + GenerateUpDown(String("Crossover"), CrossoverDisplay(false), String("x"));
  html = html + GenerateToggle(String("Color"), ColorDisplay(false), String("co"));
  if (settings.color[0] == 0) 
  {
    html = html + GenerateToggle(String("Mono"), MonoDisplay(false), String("mo"));     
    html = html + GenerateUpDown(String("Treble"), TrebleDisplay(false), String("t"));
    html = html + GenerateUpDown(String("Midrange"), MidrangeDisplay(false), String("m"));
    html = html + GenerateUpDown(String("Bass"), BassDisplay(false), String("b"));
  }
  
  html = html + String("</table></body></html>");
  return html;
}
