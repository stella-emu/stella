//
// StellaX
// Jeff Miller 05/01/2000
//

#define DIRECTSOUND_VERSION 0x700

#include "pch.hxx"
#include "AudioStream.hxx"
#include "Timer.hxx"

#include "SoundWin32.hxx"

//
// see "Streaming Wave Files with DirectSound" (Mark McCulley)
// http://msdn.microsoft.com/library/techart/msdn_streams3.htm
//
// Modified for infinite streaming by Jeff Miller 25-Apr-2000
//

// The following constants are the defaults for our streaming buffer operation.

// default buffer length in msec
const UINT AudioStream::DefBufferLength          = 250;

// default buffer service interval in msec
const UINT AudioStream::DefBufferServiceInterval = 50;

AudioStreamServices::AudioStreamServices(
    void
    ) :
    m_pds( NULL ),
    m_hwnd( NULL )
{
    TRACE ("AudioStreamServices::AudioStreamServices");
}


AudioStreamServices::~AudioStreamServices(
    void
    )
{
    TRACE ( "AudioStreamServices::~AudioStreamServices" );
    
    if ( m_pds )
    {
        m_pds->Release();
        m_pds = NULL;
    }
}


BOOL AudioStreamServices::Initialize(
    HWND hwnd
    )
{
    TRACE( "AudioStreamServices::Initialize" );

    ASSERT( hwnd != NULL );
    
    HRESULT hr;
    BOOL fRtn = TRUE;

    if ( m_pds != NULL )
    {
        return TRUE;
    }

    if ( hwnd == NULL )
    {
        // Error, invalid hwnd

        TRACE ( "ERROR: Invalid hwnd, unable to initialize services" );
        return FALSE;
    }

    m_hwnd = hwnd;

    // Create DirectSound object

    hr = ::CoCreateInstance( CLSID_DirectSound, 
                             NULL, 
		                     CLSCTX_SERVER, 
                             IID_IDirectSound, 
                             (void**)&m_pds );
	if ( FAILED(hr) )
	{
		TRACE( "CCI IDirectSound failed" );
        return FALSE;
	}

	hr = m_pds->Initialize( NULL );
	if ( FAILED(hr) )
	{
		TRACE( "IDS::Initialize failed" );
        m_pds->Release();
        m_pds = NULL;
        return FALSE;
	}

    // Set cooperative level for DirectSound. Normal means our
    // sounds will be silenced when our window loses input focus.
    
    if ( m_pds->SetCooperativeLevel( m_hwnd, DSSCL_NORMAL ) == DS_OK )
    {
        // Any additional initialization goes here
    }
    else
    {
        // Error
        TRACE ("ERROR: Unable to set cooperative level\n\r");
        m_pds->Release();
        m_pds = NULL;
        fRtn = FALSE;
    }

    return fRtn;
}



//
// AudioStream class implementation
//
////////////////////////////////////////////////////////////



AudioStream::AudioStream(
    void
    )
{
    TRACE( "AudioStream::AudioStream" );

    // Initialize data members

    m_pass = NULL;
    m_pwavefile = NULL;
    m_pdsb = NULL;
    m_ptimer = NULL;
    m_fPlaying = m_fCued = FALSE;
    m_lInService = FALSE;
    m_dwWriteCursor = 0;
    m_nBufLength = DefBufferLength;
    m_cbBufSize = 0;
    m_nBufService = DefBufferServiceInterval;
    m_nTimeStarted = 0;
    m_nTimeElapsed = 0;
    m_pbTempData = NULL;
}


AudioStream::~AudioStream(
    void
    )
{
    TRACE( "AudioStream::~AudioStream" );

    Destroy();
}

BOOL AudioStream::Create(
    AudioStreamServices* pass
    )
{
    BOOL fRtn = TRUE;    // assume TRUE
    
    ASSERT( pass );

    TRACE( "AudioStream::Create" );

    // pass points to AudioStreamServices object

    m_pass = pass;

    if ( m_pass )
    {
        // Create a new WaveFile object

        if ( m_pwavefile = new WaveFile )
        {
            // Open given file

            if ( m_pwavefile->Open( ) )
            {
                // Calculate sound buffer size in bytes
                // Buffer size is average data rate times length of buffer
                // No need for buffer to be larger than wave data though

                m_cbBufSize = (m_pwavefile->GetAvgDataRate () * m_nBufLength) / 1000;

                m_pbTempData = new BYTE[ m_cbBufSize ];
                if ( m_pbTempData == NULL )
                {
                    delete m_pwavefile;
                    return FALSE;
                }

                TRACE1( "average data rate = %d", m_pwavefile->GetAvgDataRate () );
                TRACE1( "m_cbBufSize = %d", m_cbBufSize );

                // Create sound buffer

                HRESULT hr;
                memset (&m_dsbd, 0, sizeof (DSBUFFERDESC));
                m_dsbd.dwSize = sizeof (DSBUFFERDESC);
                m_dsbd.dwBufferBytes = m_cbBufSize;
                m_dsbd.lpwfxFormat = m_pwavefile->GetWaveFormatEx();

                hr = m_pass->GetPDS()->CreateSoundBuffer( &m_dsbd, &m_pdsb, NULL );
                if (hr == DS_OK)
                {
                    // Cue for playback

                    Cue();
                }
                else
                {
                    // Error, unable to create DirectSound buffer

                    TRACE ("Error, unable to create DirectSound buffer\n\r");
                    if (hr == DSERR_BADFORMAT)
                    {
                        TRACE ("    Bad format (probably ADPCM)\n\r");
                    }
                    
                    fRtn = FALSE;
                }
            }
            else
            {
                // Error opening file

                delete m_pwavefile;
                m_pwavefile = NULL;
                fRtn = FALSE;
            }   

        }
        else
        {
            // Error, unable to create WaveFile object

            fRtn = FALSE;
        }
    }
    else
    {
        // Error, passed invalid parms

        fRtn = FALSE;
    }

    return fRtn;
}


// Destroy

BOOL AudioStream::Destroy(
    void
    )
{
    BOOL fRtn = TRUE;

    TRACE ("AudioStream::Destroy");

    // Stop playback

    Stop();
    
    // Release DirectSound buffer
    
    if (m_pdsb)
    {
        m_pdsb->Release();
        m_pdsb = NULL;
    }

    // Delete WaveFile object

    delete m_pwavefile;
    m_pwavefile = NULL;

    delete[] m_pbTempData;
    m_pbTempData = NULL;
    
    return fRtn;
}

// WriteWaveData
//
// Writes wave data to sound buffer. This is a helper method used by Create and
// ServiceBuffer; it's not exposed to users of the AudioStream class.

BOOL AudioStream::WriteWaveData(
    DWORD cbWriteBytes
    )
{
    HRESULT hr;

    LPVOID pvAudioPtr1 = NULL;
    DWORD cbAudioBytes1 = 0;
    LPVOID pvAudioPtr2 = NULL;
    DWORD cbAudioBytes2 = 0;

    BOOL fRtn = TRUE;

    // Lock the sound buffer

    hr = m_pdsb->Lock( m_dwWriteCursor, 
                       cbWriteBytes, 
                       &pvAudioPtr1, &cbAudioBytes1, 
                       &pvAudioPtr2, &cbAudioBytes2, 
                       0 );
    if ( hr == DS_OK )
    {
        // Write data to sound buffer. Because the sound buffer is circular, we may have to
        // do two write operations if locked portion of buffer wraps around to start of buffer.

        ASSERT ( pvAudioPtr1 != NULL );

        m_pwavefile->Read( m_pbTempData, cbWriteBytes );

        memcpy( pvAudioPtr1, m_pbTempData, cbAudioBytes1 );
        memcpy( pvAudioPtr2, m_pbTempData + cbAudioBytes1, cbAudioBytes2 );

        // Update our buffer offset and unlock sound buffer

        m_dwWriteCursor = ( m_dwWriteCursor + cbAudioBytes1 + cbAudioBytes2 ) % m_cbBufSize;
        
        m_pdsb->Unlock( pvAudioPtr1, cbAudioBytes1, 
                        pvAudioPtr2, cbAudioBytes2 );
    }
    else
    {
        // Error locking sound buffer

        TRACE("Error, unable to lock sound buffer" );
        fRtn = FALSE;
    }

    return fRtn;
}



// GetMaxWriteSize
//
// Helper function to calculate max size of sound buffer write operation, i.e. how much
// free space there is in buffer.

DWORD AudioStream::GetMaxWriteSize(
    void
    )
{
    DWORD dwCurrentPlayCursor;
    DWORD dwCurrentWriteCursor;
    DWORD dwMaxSize;

    // Get current play position

    if ( m_pdsb->GetCurrentPosition( &dwCurrentPlayCursor, 
                                     &dwCurrentWriteCursor) == DS_OK)
    {
        if ( m_dwWriteCursor <= dwCurrentPlayCursor ) 
        {
            // Our write position trails play cursor

            dwMaxSize = dwCurrentPlayCursor - m_dwWriteCursor;
        }

        else // (m_dwWriteCursor > dwCurrentPlayCursor)
        {
            // Play cursor has wrapped

            dwMaxSize = m_cbBufSize - m_dwWriteCursor + dwCurrentPlayCursor;
        }

    }
    else
    {
        // GetCurrentPosition call failed
        ASSERT (0);
        dwMaxSize = 0;
    }

    return dwMaxSize;
}


// ServiceBuffer
//
// Routine to service buffer requests initiated by periodic timer.
//
// Returns TRUE if buffer serviced normally; otherwise returns FALSE.

BOOL AudioStream::ServiceBuffer(
    void
    )
{
    BOOL fRtn = TRUE;
    
    // Check for reentrance

    if ( ::InterlockedExchange( &m_lInService, TRUE ) == FALSE )
    {
        // Not reentered, proceed normally
        
        // Maintain elapsed time count

        m_nTimeElapsed = timeGetTime () - m_nTimeStarted;

        // All of sound not played yet, send more data to buffer

        DWORD dwFreeSpace = GetMaxWriteSize ();

        // Determine free space in sound buffer

        if (dwFreeSpace)
        {
            // Enough wave data remains to fill free space in buffer
            // Fill free space in buffer with wave data

            if ( WriteWaveData( dwFreeSpace ) == FALSE )
            {
                // Error writing wave data

                fRtn = FALSE;
                ASSERT (0);
                TRACE ("WriteWaveData failed\n\r");
            }
        }
        else
        {
            // No free space in buffer for some reason

            fRtn = FALSE;
        }

        // Reset reentrancy semaphore

        ::InterlockedExchange( &m_lInService, FALSE );
    }
    else
    {
        // Service routine reentered. Do nothing, just return

        fRtn = FALSE;
    }

    return fRtn;
}


void AudioStream::Cue(
    void
    )
{
    TRACE ( "AudioStream::Cue" );
    
    if ( !m_fCued )
    {
        // Reset buffer ptr

        m_dwWriteCursor = 0;

        // Reset file ptr, etc

        m_pwavefile->Cue();

        // Reset DirectSound buffer

        m_pdsb->SetCurrentPosition( 0 );

        // Fill buffer with wave data

        WriteWaveData( m_cbBufSize );
        
        m_fCued = TRUE;
    }
}


void AudioStream::Play(
    void
    )
{
    if ( m_pdsb )
    {
        // If playing, stop

        if (m_fPlaying)
        {
            Stop();
        }

        // Cue for playback if necessary

        if (!m_fCued)
        {
            Cue();
        }

        // Begin DirectSound playback

        HRESULT hr = m_pdsb->Play( 0, 0, DSBPLAY_LOOPING );
        if (hr == DS_OK)
        {
            m_nTimeStarted = timeGetTime();
            
            // Kick off timer to service buffer

            m_ptimer = new CMmTimer;
            if ( m_ptimer )
            {
                m_ptimer->Create( m_nBufService, m_nBufService, (DWORD)this, TimerCallback );
            }
            else
            {
                Stop();
                return;
            }

            // Playback begun, no longer cued

            m_fPlaying = TRUE;
            m_fCued = FALSE;
        }
        else
        {
            TRACE ("Error, play failed\n\r");
        }
    }
}


BOOL AudioStream::TimerCallback(
    DWORD dwUser
    )
{
    // dwUser contains ptr to AudioStream object

    AudioStream* pas = (AudioStream *)dwUser;

    return pas->ServiceBuffer();
}


void AudioStream::Stop(
    void
    )
{
    TRACE ("AudioStream::Stop");

    if (m_fPlaying)
    {
        // Stop DirectSound playback

        m_pdsb->Stop ();

        // Delete Timer object

        delete m_ptimer;
        m_ptimer = NULL;

        m_fPlaying = FALSE;
    }
}

