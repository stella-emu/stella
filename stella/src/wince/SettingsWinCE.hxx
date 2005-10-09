#ifndef SETTINGS_WINCE_HXX
#define SETTINGS_WINCE_HXX

#include "bspf.hxx"
#include "Settings.hxx"

class SettingsWinCE : public Settings
{
  public:
	SettingsWinCE(OSystem* osystem);
    virtual ~SettingsWinCE();
};

#endif
