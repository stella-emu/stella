#ifndef OSYSTEM_WINCE_HXX
#define OSYSTEM_WINCE_HXX

#include "bspf.hxx"
#include "OSystem.hxx"

class OSystemWinCE : public OSystem
{
  public:
    OSystemWinCE();
    virtual ~OSystemWinCE();

  public:
    virtual void mainLoop();
	virtual uInt32 getTicks(void);
    virtual void setFramerate(uInt32 framerate);
};

#endif
