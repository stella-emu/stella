//
// StellaX
// Jeff Miller 04/26/2000
//
#include "pch.hxx"
#include "SoundWin32.hxx"
#include "resource.h"

#include "AudioStream.hxx"

SoundWin32::SoundWin32(
    ) :
    m_fInitialized( FALSE ),
    m_pass( NULL ),
    m_pasCurrent( NULL )
{
	TRACE("SoundWin32::SoundWin32");
}


HRESULT SoundWin32::Initialize(
	HWND hwnd
    )
{
	TRACE( "SoundWin32::Initialize hwnd=%08X", hwnd );

    if ( m_fInitialized )
    {
        TRACE( "SoundWin32::Initialize - already initialized" );
        return S_OK;
    }

    HRESULT hr = S_OK;

    m_pass = new AudioStreamServices;
    if ( m_pass == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    if ( ! m_pass->Initialize( hwnd ) )
    {
        TRACE( "ASS Initialize failed" );
        MessageBox( (HINSTANCE)::GetWindowLong( hwnd, GWL_HINSTANCE ), 
                    hwnd, 
                    IDS_ASS_FAILED );
        hr = E_FAIL;
        goto cleanup;
    }

    m_pasCurrent = new AudioStream;
    if ( m_pasCurrent == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    if ( ! m_pasCurrent->Create( m_pass ) )
    {
        TRACE( "PAS Create failed" );
        MessageBox( (HINSTANCE)::GetWindowLong( hwnd, GWL_HINSTANCE ), 
                    hwnd, 
                    IDS_PAS_FAILED );
        hr = E_FAIL;
        goto cleanup;
    }

    m_pasCurrent->Play();

    m_fInitialized = TRUE;
cleanup:

    if ( FAILED(hr) )
    {
        if ( m_pasCurrent )
        {
            m_pasCurrent->Destroy();
            delete m_pasCurrent;
            m_pasCurrent = NULL;
        }

        if ( m_pass )
        {
            delete m_pass;
            m_pass = NULL;
        }
    }

    return hr;
}
 
SoundWin32::~SoundWin32(
    )
{
	TRACE("SoundWin32::~SoundWin32");

    if ( m_pasCurrent )
    {
        m_pasCurrent->Destroy();
        delete m_pasCurrent;
        m_pasCurrent = NULL;
    }

    delete m_pass;
    m_pass = NULL;
}


void SoundWin32::set(Sound::Register reg, uInt8 value)
{
    if ( ! m_fInitialized )
    {
        TRACE( "SoundWin32::set -- not initialized" );
		return;
    }

    //
    // Process TIA data
    //
    
    switch( reg ) 
    {
    case AUDC0:
        Update_tia_sound( 0x15, value );
        break;
        
    case AUDC1:
        Update_tia_sound( 0x16, value );
        break;
        
    case AUDF0:
        Update_tia_sound( 0x17, value );
        break;
        
    case AUDF1:
        Update_tia_sound( 0x18, value );
        break;
        
    case AUDV0:
        Update_tia_sound( 0x19, value );
        break;
        
    case AUDV1:
        Update_tia_sound( 0x1A, value );
        break;
    }

}

void SoundWin32::mute(
	bool mute
    )
{
    if ( ! m_fInitialized )
    {
        TRACE( "SoundWin32::mute -- not initialized" );
        return;
    }

    if ( m_pasCurrent )
    {
        if ( mute )
        {
            m_pasCurrent->Stop();
        }
        else
        {
            m_pasCurrent->Play();
        }
    }
}

