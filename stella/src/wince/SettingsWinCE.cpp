#include <sstream>
#include <fstream>

#include "bspf.hxx"
#include "Settings.hxx"
#include "SettingsWinCE.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SettingsWinCE::SettingsWinCE(OSystem* osystem) : Settings(osystem) 
{
  //set("GameFilename", "Mega Force (1982) (20th Century Fox).bin");
  //set("GameFilename", "Enduro (1983) (Activision).bin");
  //set("GameFilename", "Night Driver (1978) (Atari).bin");
  set("romdir", (string) getcwd() + '\\');
}

SettingsWinCE::~SettingsWinCE()
{
}
