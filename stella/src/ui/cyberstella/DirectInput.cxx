//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-1999 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: DirectInput.cxx,v 1.3 2003-11-13 00:25:07 stephena Exp $
//============================================================================

#include "pch.hxx"
#include "resource.h"

#include "DirectInput.hxx"

DirectInput::DirectInput()
  : myHWND(NULL),
    mylpdi(NULL),
    myKeyboard(NULL),
    myMouse(NULL),
    myLeftJoystick(NULL),
    myRightJoystick(NULL)
{
}

DirectInput::~DirectInput()
{
	cleanup();
}

bool DirectInput::initialize(HWND hwnd)
{
  // FIXME - this should move to the constructor
  if(FAILED(DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION,
                               IID_IDirectInput8, (void**)&mylpdi, NULL)))
    return false;

  // initialize the keyboard
  if(FAILED(mylpdi->CreateDevice(GUID_SysKeyboard, &myKeyboard, NULL)))
    return false;
  if(FAILED(myKeyboard->SetDataFormat(&c_dfDIKeyboard)))
    return false;
  if(FAILED(myKeyboard->SetCooperativeLevel(hwnd, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE)))
    return false;

  DIPROPDWORD dipdw;
  dipdw.diph.dwSize       = sizeof(DIPROPDWORD);
  dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
  dipdw.diph.dwObj        = 0;
  dipdw.diph.dwHow        = DIPH_DEVICE;
  dipdw.dwData            = 256;
  if(FAILED(myKeyboard->SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph)))
    return false;

  if(FAILED(myKeyboard->Acquire()))
    return false;

  // Make sure to reset the event buffer
  myEventBufferPos = 0;

  return true;
}

void DirectInput::update()
{
  HRESULT hr;

  if(myKeyboard != NULL)
  {
    DIDEVICEOBJECTDATA keyEvents[256];
    DWORD numKeyEvents = 256;

    hr = myKeyboard->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), 
                                   keyEvents, &numKeyEvents, 0 );

    if(hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED)
    {
      hr = myKeyboard->Acquire();
      if(hr == DIERR_OTHERAPPHASPRIO)
        return;

      hr = myKeyboard->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), 
                                     keyEvents, &numKeyEvents, 0 );
    }

    // add these new key events to the event buffer
    for(unsigned int i = 0; i < numKeyEvents; i++ ) 
    {
      uInt32 j = myEventBufferPos;
      if(j < 100)
      {
        myEventBuffer[j].type      = (keyEvents[i].dwData & 0x80) ? KEY_DOWN : KEY_UP;
        myEventBuffer[j].key.key   = keyEvents[i].dwOfs;
        myEventBuffer[j].key.state = (myEventBuffer[j].type == KEY_DOWN) ? 1 : 0;
        myEventBufferPos++;
      }
      else  // if we run out of room, then ignore new events
      {
        myEventBufferPos = 100;
        break;
      }
    }
  }
  //  else check mouse
}

bool DirectInput::pollEvent(DI_Event* event)
{
  // Pump the event buffer and return if a new event is found
  if(myEventBufferPos > 0)
  {
    *event = myEventBuffer[--myEventBufferPos];
    return true;
  }
  else
    return false;
}

void DirectInput::cleanup()
{
  if(myMouse)
  {
    myMouse->Unacquire();
    myMouse->Release();
    myMouse = NULL;
  }

  if(myKeyboard)
  {
    myKeyboard->Unacquire();
    myKeyboard->Release();
    myKeyboard = NULL;
  }

  if(mylpdi)
  {
    mylpdi->Release();
    mylpdi = NULL;
  }
}

/*
BOOL CALLBACK DirectInput::EnumDevicesProc
(
	const DIDEVICEINSTANCE* lpddi, 
	LPVOID pvRef
)
{
	DirectInput* pThis = (DirectInput*)pvRef;
	ASSERT(pThis);

	const DIDATAFORMAT* pdidf = NULL;

	switch(pThis->m_dwDevType)
	{
	case DIDEVTYPE_MOUSE:
		TRACE("EnumDevicesProc (mouse)");
		pdidf = &c_dfDIMouse;
		break;

	case DIDEVTYPE_KEYBOARD:
		TRACE("EnumDevicesProc (keyboard)");
		pdidf = &c_dfDIKeyboard;
		break;

	case DIDEVTYPE_JOYSTICK:
		TRACE("EnumDevicesProc (joystick)");
		pdidf = &c_dfDIJoystick;
		break;

	default:
		ASSERT(FALSE);
		return DIENUM_STOP;
	};

	HRESULT hr;

	IDirectInputDevice* piDID;
	hr = pThis->m_piDI->CreateDevice(lpddi->guidInstance, &piDID, 
		NULL);
	ASSERT(hr == DI_OK && "IDI::CreateDevice failed");
	if (hr != DI_OK)
	{
		return DIENUM_CONTINUE;
	}

	hr = piDID->SetDataFormat(pdidf);
	ASSERT(hr == DI_OK && "IDID::SetDataFormat failed");
	if (hr != DI_OK)
	{
		piDID->Release();
		return DIENUM_CONTINUE;
	}

	hr = piDID->QueryInterface(IID_IDirectInputDevice2, 
		(void**)&(pThis->m_piDID));
	if (hr != S_OK)
	{
		piDID->Release();
		return DIENUM_CONTINUE;
	}

	// undo the addref that QI did (CreateDevice did an addref)

	pThis->m_piDID->Release();

#ifdef _DEBUG
	DIDEVICEINSTANCE didi;
	didi.dwSize = sizeof(didi);
	piDID->GetDeviceInfo(&didi);
	TRACE("Using device: %s", didi.tszProductName);
#endif

	return DIENUM_STOP;
}

BOOL DirectInput::IsButtonPressed
(
	int nButton
) const
{
	if ( nButton > GetButtonCount() )
    {
		return FALSE;
    }

	return ( m_pButtons[nButton] ) ? 1 : 0;
}


// ---------------------------------------------------------------------------


// ---------------------------------------------------------------------------

DirectJoystick::DirectJoystick(
	HWND hwnd
    ) : \
	DirectInput( hwnd, DIDEVTYPE_JOYSTICK, 32 )
{
	TRACE( "DirectJoystick::DirectJoystick" );
}

HRESULT DirectJoystick::Initialize(
    void
    )
{
    TRACE( "DirectJoystick::Initialize" );

    HRESULT hr;

    hr = DirectInput::Initialize();
    if ( FAILED(hr) )
    {
        return hr;
    }

	if ( GetDevice() == NULL )
	{
		TRACE("No joystick was found");
		return S_FALSE;
	}

	// set X-axis range to (-1000 ... +1000)
	// This lets us test against 0 to see which way the stick is pointed.
	
	DIPROPRANGE dipr;

	dipr.diph.dwSize = sizeof(dipr);
    dipr.diph.dwHeaderSize = sizeof(dipr.diph);
	dipr.diph.dwHow = DIPH_BYOFFSET;
	dipr.lMin = -1000;
	dipr.lMax = +1000;
	dipr.diph.dwObj = DIJOFS_X;
	hr = GetDevice()->SetProperty( DIPROP_RANGE, &dipr.diph );
    if ( FAILED(hr) )
    {
        TRACE( "SetProperty(DIPROP_RANGE,x) failed, hr=%X", hr );
        return hr;
    }

	// And again for Y-axis range
	
	dipr.diph.dwSize = sizeof(dipr);
    dipr.diph.dwHeaderSize = sizeof(dipr.diph);
	dipr.diph.dwHow = DIPH_BYOFFSET;
	dipr.lMin = -1000;
	dipr.lMax = +1000;
	dipr.diph.dwObj = DIJOFS_Y;
	hr = GetDevice()->SetProperty( DIPROP_RANGE, &dipr.diph );
    if ( FAILED(hr) )
    {
        TRACE( "SetProperty(DIPROP_RANGE,y) failed, hr=%X", hr );
        return hr;
    }


	// set dead zone to 50%
	
	DIPROPDWORD dipdw;

	dipdw.diph.dwSize = sizeof(dipdw);
	dipdw.diph.dwHeaderSize = sizeof(dipdw.diph);
	dipdw.diph.dwHow = DIPH_BYOFFSET;
	dipdw.dwData = 5000;
	dipdw.diph.dwObj = DIJOFS_X;
	hr = GetDevice()->SetProperty( DIPROP_DEADZONE, &dipdw.diph ); 
    if ( FAILED(hr) )
    {
        TRACE( "SetProperty(DIPROP_DEADZONE,x) failed, hr=%X", hr );
        return hr;
    }

	dipdw.diph.dwSize = sizeof(dipdw);
	dipdw.diph.dwHeaderSize = sizeof(dipdw.diph);
	dipdw.diph.dwHow = DIPH_BYOFFSET;
	dipdw.dwData = 5000;
	dipdw.diph.dwObj = DIJOFS_Y;
	hr = GetDevice()->SetProperty( DIPROP_DEADZONE, &dipdw.diph ); 
    if ( FAILED(hr) )
    {
        TRACE( "SetProperty(DIPROP_DEADZONE,y) failed, hr=%X", hr );
        return hr;
    }

    return S_OK;
}

HRESULT DirectJoystick::Update(
    void
    )
{
	if ( GetDevice() == NULL )
    {
		return E_FAIL;
    }

	HRESULT hr;

	DIJOYSTATE dijs;

	GetDevice()->Poll();

	hr = GetDevice()->GetDeviceState( sizeof(dijs), &dijs );
	if ( hr == DIERR_INPUTLOST ||
         hr == DIERR_NOTACQUIRED )
	{
		hr = GetDevice()->Acquire();
        if ( hr == DIERR_OTHERAPPHASPRIO )
        {
            return S_FALSE;
        }

		GetDevice()->Poll();
		hr = GetDevice()->GetDeviceState( sizeof(dijs), &dijs );
	}

	ASSERT(hr == DI_OK && "Joystick GetDeviceState failed");

	if ( hr == DI_OK )
	{
		m_lX = dijs.lX;
		m_lY = dijs.lY;

		memcpy( m_pButtons, 
                dijs.rgbButtons, 
                sizeof(dijs.rgbButtons) );
	}

	return hr;
}


// ---------------------------------------------------------------------------

CDisabledJoystick::CDisabledJoystick(
	HWND hwnd
    ) : \
    DirectInput( NULL, 0, 0 )
{
    UNUSED_ALWAYS( hwnd );

	TRACE( "CDisabledJoystick::CDisabledJoystick" );
}

HRESULT CDisabledJoystick::Update(
    void
    )
{
	return S_FALSE;
}

// ---------------------------------------------------------------------------

DirectMouse::DirectMouse(
	HWND hwnd
    ) : \
	DirectInput( hwnd, DIDEVTYPE_MOUSE, 4 )
{
	TRACE( "DirectMouse::DirectMouse" );
}

HRESULT DirectMouse::Update(
    void
    )
{
	if (GetDevice() == NULL)
    {
		return E_FAIL;
    }

	HRESULT hr;

	DIMOUSESTATE dims;

	GetDevice()->Poll();

	hr = GetDevice()->GetDeviceState( sizeof(dims), &dims );
	if ( hr == DIERR_INPUTLOST ||
         hr == DIERR_NOTACQUIRED )
	{
		hr = GetDevice()->Acquire();
        if ( hr == DIERR_OTHERAPPHASPRIO )
        {
            return S_FALSE;
        }

		GetDevice()->Poll();
		hr = GetDevice()->GetDeviceState( sizeof(dims), &dims );
	}

	ASSERT( hr == DI_OK && "Mouse GetDeviceState failed" );

	if ( hr == DI_OK )
	{
		// Because the mouse is returning relative positions,
		// force X and Y to go between 0 ... 999

		m_lX += dims.lX;

		if (m_lX < 0)
        {
			m_lX = 0;
        }
		else if (m_lX > 999) 
        {
			m_lX = 999;
        }

		m_lY += dims.lY;

		if (m_lY < 0)
        {
			m_lY = 0;
        }
		else if (m_lY > 999)
        {
			m_lY = 999;
        }

		memcpy( m_pButtons, 
                dims.rgbButtons, 
                sizeof(dims.rgbButtons) );
	}

	return hr;
}
*/

///////////////////////////////////////
// The following was part of initialize
///////////////////////////////////////
/*
  // initialize the mouse
  if (FAILED(lpdi->CreateDevice(GUID_SysMouse, &m_mouse, NULL)))
    return false;
  if (FAILED(m_mouse->SetCooperativeLevel(hWnd, DISCL_BACKGROUND |
                                          DISCL_NONEXCLUSIVE)))
    return false;
  if (FAILED(m_mouse->SetDataFormat(&c_dfDIMouse)))
    return false;
  if (FAILED(m_mouse->Acquire()))
    return false;
*/
/*
    //
	// enumerate to find proper device
    // The callback will set m_piDID
    //

	TRACE("\tCalling EnumDevices");

	hr = m_piDI->EnumDevices( m_dwDevType, 
                              EnumDevicesProc, 
		                      this, 
                              DIEDFL_ATTACHEDONLY );
	if ( m_piDID )
	{
		TRACE("\tGot a device!");

		(void)m_piDID->SetCooperativeLevel( m_hwnd, 
                                            DISCL_NONEXCLUSIVE 
			                                | DISCL_FOREGROUND);

		hr = GetDevice()->Acquire();
        if ( hr == DIERR_OTHERAPPHASPRIO )
        {
            return S_FALSE;
        }
	}

	m_pButtons = new BYTE[GetButtonCount()];
    if ( m_pButtons == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    m_fInitialized = TRUE;

cleanup:

    if ( FAILED(hr) )
    {
        Cleanup();

        if ( uMsg != 0 )
        {
            MessageBox( hInstance, m_hwnd, uMsg );
        }
    }

    return hr;
*/
