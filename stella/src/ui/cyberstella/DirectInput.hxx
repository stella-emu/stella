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
// $Id: DirectInput.hxx,v 1.2 2003-09-21 14:33:34 stephena Exp $
//============================================================================

#ifndef DIRECT_INPUT_HXX
#define DIRECT_INPUT_HXX

class CDirectInput
{
  public:
	CDirectInput( HWND hwnd, DWORD dwDevType, int nButtonCount );
	virtual ~CDirectInput( );

  public:
	virtual HRESULT Initialize( void );

	virtual HRESULT Update( void ) = 0;

	void GetPos( LONG* pX, LONG* pY ) const;

	virtual BOOL IsButtonPressed( int nButton ) const;
	virtual int GetButtonCount( void ) const;

	// I need IDID2 for the Poll method

	IDirectInputDevice2* GetDevice( void ) const;

protected:

	LONG m_lX;
	LONG m_lY;
	BYTE* m_pButtons;

private:

	void Cleanup();

	static BOOL CALLBACK EnumDevicesProc( const DIDEVICEINSTANCE* lpddi, 
		                                  LPVOID pvRef );

	IDirectInput* m_piDI;

    HWND m_hwnd;
	IDirectInputDevice2* m_piDID;
	DWORD m_dwDevType;

	const int m_nButtonCount;

    BOOL m_fInitialized;

	CDirectInput( const CDirectInput& );  // no implementation
	void operator=( const CDirectInput& );  // no implementation

};

inline int CDirectInput::GetButtonCount(
	void
    ) const
{
	return m_nButtonCount;
}


inline IDirectInputDevice2* CDirectInput::GetDevice(
	void
    ) const
{
	// 060499: Dont assert here, as it's okay if a device isn't available
	// (client must check for NULL return)
	return m_piDID;
}

inline void CDirectInput::GetPos(
	LONG* pX,
	LONG* pY
    ) const
{
	if (pX != NULL)
	{
		*pX = m_lX;
	}

	if (pY != NULL)
	{
		*pY = m_lY;
	}
}


// ---------------------------------------------------------------------------

class CDirectMouse : public CDirectInput
{
public:

	CDirectMouse( HWND hwnd );

	HRESULT Update( void );

private:

	CDirectMouse( const CDirectMouse& );  // no implementation
	void operator=( const CDirectMouse& );  // no implementation

};


// ---------------------------------------------------------------------------

class CDirectJoystick : public CDirectInput
{
public:

	CDirectJoystick( HWND hwnd );

    HRESULT Initialize( void );
	HRESULT Update( void );

private:

	CDirectJoystick( const CDirectJoystick& );  // no implementation
	void operator=( const CDirectJoystick& );  // no implementation

};

class CDisabledJoystick : public CDirectInput
{
public:

    CDisabledJoystick( HWND hwnd );
    
    HRESULT Update( void );

private:

	CDisabledJoystick( const CDisabledJoystick& );  // no implementation
	void operator=( const CDisabledJoystick& );  // no implementation

};

// ---------------------------------------------------------------------------

class CDirectKeyboard : public CDirectInput
{
public:

	CDirectKeyboard( HWND hwnd );

	HRESULT Update( void );

private:

	CDirectKeyboard( const CDirectKeyboard& );  // no implementation
	void operator=( const CDirectKeyboard& );  // no implementation

};

#endif
