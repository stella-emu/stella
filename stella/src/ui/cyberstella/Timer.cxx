//
// StellaX
// Jeff Miller 05/01/2000
//
#include "pch.hxx"
#include "Timer.hxx"

CMmTimer::CMmTimer(
    void
    ) :
    m_mmrIDTimer( NULL )
{
    TRACE( "CMmTimer::CMmTimer" );
}

CMmTimer::~CMmTimer(
    void
    )
{
    TRACE( "CMmTimer::~CMmTimer" );

    if ( m_mmrIDTimer != NULL )
    {
        ::timeKillEvent( m_mmrIDTimer );
        m_mmrIDTimer = NULL;
    }
}


BOOL CMmTimer::Create(
    UINT nPeriod, 
    UINT nRes, 
    DWORD dwUser, 
    TIMERCALLBACK pfnCallback
    )
{
    BOOL fRet = TRUE;
    
    TRACE( "CMmTimer::Create" );

    ASSERT( pfnCallback != NULL );
    ASSERT( nPeriod > 10 );
    ASSERT( nPeriod >= nRes );

    m_nPeriod = nPeriod;
    m_nRes = nRes;
    m_dwUser = dwUser;
    m_pfnCallback = pfnCallback;

    m_mmrIDTimer = ::timeSetEvent( m_nPeriod,
                                   m_nRes, 
                                   TimeProc, 
                                   (DWORD)this, 
                                   TIME_PERIODIC );
    if ( m_mmrIDTimer == NULL )
    {
        fRet = FALSE;
    }

    return fRet;
}


// Timer proc for multimedia timer callback set with timeSetTime().
//
// Calls procedure specified when Timer object was created. The 
// dwUser parameter contains "this" pointer for associated Timer object.
// 

void CALLBACK CMmTimer::TimeProc(
    UINT uID, 
    UINT uMsg, 
    DWORD dwUser, 
    DWORD dw1, 
    DWORD dw2 
    )
{
    // dwUser contains ptr to Timer object

    CMmTimer* pThis = (CMmTimer*)dwUser;

    ASSERT( pThis != NULL );

    // Call user-specified callback and pass back user specified data

    ASSERT( pThis->m_pfnCallback != NULL );

    pThis->m_pfnCallback( pThis->m_dwUser );
}
