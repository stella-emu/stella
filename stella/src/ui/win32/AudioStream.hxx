//
// StellaX
// Jeff Miller 05/01/2000
//
#ifndef AUDIOSTREAM_H
#define AUDIOSTREAM_H

#include <dsound.h>

class CMmTimer;
class WaveFile;

// Classes

// AudioStreamServices
//
// DirectSound apportions services on a per-window basis to allow
// sound from background windows to be muted. The AudioStreamServices
// class encapsulates the initialization of DirectSound services.
//
// Each window that wants to create AudioStream objects must
// first create and initialize an AudioStreamServices object. 
// All AudioStream objects must be destroyed before the associated 
// AudioStreamServices object is destroyed.

class AudioStreamServices
{
public:
    AudioStreamServices( void );
    ~AudioStreamServices( void );

    BOOL Initialize ( HWND hwnd );

    LPDIRECTSOUND GetPDS(void) 
        {
            return m_pds; 
        }

protected:
    HWND m_hwnd;
    LPDIRECTSOUND m_pds;

private:

	AudioStreamServices( const AudioStreamServices& );  // no implementation
	void operator=( const AudioStreamServices& );  // no implementation

};


// AudioStream
//
// Audio stream interface class for playing WAV files using DirectSound.
// Users of this class must create AudioStreamServices object before
// creating an AudioStream object.
//
// Public Methods:
//
// Public Data:
//

class AudioStream
{
public:
    AudioStream(void);
    ~AudioStream(void);

    BOOL Create( AudioStreamServices * pass );

    BOOL Destroy( void );

    void Play( void );
    void Stop( void );

protected:

    void Cue (void);
    BOOL WriteWaveData( DWORD cbWriteBytes );

    DWORD GetMaxWriteSize (void);
    BOOL ServiceBuffer (void);

    static BOOL TimerCallback (DWORD dwUser);

    AudioStreamServices * m_pass;  // ptr to AudioStreamServices object
    LPDIRECTSOUNDBUFFER m_pdsb;    // sound buffer

    WaveFile * m_pwavefile;        // ptr to WaveFile object
    CMmTimer * m_ptimer;           // ptr to Timer object

    BOOL m_fCued;                  // semaphore (stream cued)
    BOOL m_fPlaying;               // semaphore (stream playing)

    DSBUFFERDESC m_dsbd;           // sound buffer description

    LONG m_lInService;             // reentrancy semaphore
    UINT m_dwWriteCursor;            // last write position
    UINT m_nBufLength;             // length of sound buffer in msec
    UINT m_cbBufSize;              // size of sound buffer in bytes
    UINT m_nBufService;            // service interval in msec
    UINT m_nDuration;              // duration of wave file
    UINT m_nTimeStarted;           // time (in system time) playback started
    UINT m_nTimeElapsed;           // elapsed time in msec since playback started

    BYTE* m_pbTempData; // Cache so we dont call Read twice for overlap

    static const UINT DefBufferLength;
    static const UINT DefBufferServiceInterval;

private:

    AudioStream( const AudioStream& );  // no implementation
	void operator=( const AudioStream& );  // no implementation

};

#endif // _INC_AUDIOSTREAM 