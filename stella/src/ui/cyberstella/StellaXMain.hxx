//
// StellaX
// Jeff Miller 05/12/2000
//
#ifndef STELLAX_H
#define STELLAX_H
#pragma once

class PropertiesSet;
class CGlobalData;

class CStellaXMain
{
public:

    CStellaXMain();
    ~CStellaXMain();

    DWORD Initialize( void );

    HRESULT PlayROM( HWND hwnd, LPCTSTR ctszPathName, 
                     LPCTSTR pszFriendlyName, 
                     CGlobalData* rGlobalData );
    PropertiesSet& GetPropertiesSet() const;

private:

    PropertiesSet* m_pPropertiesSet;

	CStellaXMain( const CStellaXMain& );  // no implementation
	void operator=( const CStellaXMain& );  // no implementation
};

inline PropertiesSet& CStellaXMain::GetPropertiesSet(
    void
    ) const
{
    return *m_pPropertiesSet;
}

#endif