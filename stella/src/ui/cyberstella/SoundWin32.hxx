//
// StellaX
// Jeff Miller 05/01/2000
//
#ifndef SNDWIN32_H
#define SNDWIN32_H

#include "bspf.hxx"
#include "Sound.hxx"
#include "TIASound.h"


class AudioStreamServices;
class AudioStream;

#define SAMPLES_PER_SEC 22050
#define NUM_CHANNELS 1
#define BITS_PER_SAMPLE 8

class WaveFile
{
public:

    WaveFile( void )
        {
            ZeroMemory( &wfx, sizeof(wfx) );
            wfx.cbSize = sizeof( wfx );
            wfx.wFormatTag = WAVE_FORMAT_PCM;
            wfx.nChannels = NUM_CHANNELS;
            wfx.nSamplesPerSec = SAMPLES_PER_SEC;
            wfx.wBitsPerSample = BITS_PER_SAMPLE;
            wfx.nBlockAlign = wfx.wBitsPerSample / 8 * wfx.nChannels;
            wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

        }

    ~WaveFile( void )
        {
        }

    BOOL Open( void )
        {
            Tia_sound_init( 31400, wfx.nSamplesPerSec );

            return TRUE;
        }

    BOOL Cue( void )
        {
            return TRUE;
        }

    void Read( BYTE* pbDest, UINT cbSize )
        {
            Tia_process ( pbDest, cbSize );
        }

    UINT GetAvgDataRate (void) 
        {   
            return wfx.nAvgBytesPerSec;
        }

    BYTE GetSilenceData (void)
        {
            return 0;
        }

    WAVEFORMATEX* GetWaveFormatEx( void )
        {
            return &wfx;
        }

private:

    WAVEFORMATEX wfx;

	WaveFile( const WaveFile& );  // no implementation
	void operator=( const WaveFile& );  // no implementation
};

class SoundWin32 : public Sound
{
public:

	SoundWin32();
	virtual ~SoundWin32();

	HRESULT Initialize( HWND hwnd );
	
    //
    // base class virtuals
    //

	virtual void set(Sound::Register reg, uInt8 value);

	virtual void mute( bool state );
	
private:

    BOOL m_fInitialized;

    AudioStreamServices* m_pass;
    AudioStream* m_pasCurrent;

    SoundWin32( const SoundWin32& );  // no implementation
	void operator=( const SoundWin32& );  // no implementation
};
#endif

