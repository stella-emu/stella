//
// StellaX
// Jeff Miller 05/01/2000
//
#ifndef TIMER_H
#define TIMER_H

// Constants

typedef BOOL ( * TIMERCALLBACK )( DWORD );

// Classes

// Timer
//
// Wrapper class for Windows multimedia timer services. Provides
// both periodic and one-shot events. User must supply callback
// for periodic events.
// 

class CMmTimer
{
public:

    CMmTimer( void );
    ~CMmTimer( void );

    BOOL Create( UINT nPeriod, UINT nRes, DWORD dwUser, TIMERCALLBACK pfnCallback );

protected:

    static void CALLBACK TimeProc( UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2 );

    TIMERCALLBACK m_pfnCallback;
    DWORD m_dwUser;
    UINT m_nPeriod;
    UINT m_nRes;
    MMRESULT m_mmrIDTimer;

private:

	CMmTimer( const CMmTimer& );  // no implementation
	void operator=( const CMmTimer& );  // no implementation
};

#endif
