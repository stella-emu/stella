//
// StellaX
// Jeff Miller 05/02/2000
//

#include "pch.hxx"
#include "ControlHost.hxx" 
 
#include <ole2.h>
#include <olectl.h>

#pragma comment(lib, "oleaut32")

CActiveXControl::CActiveXControl(
    ) : \
    m_piDispatch( NULL ) 
{
    TRACE( "CActiveXControl::CActiveXControl" );
}

CActiveXControl::~CActiveXControl(
    )
{
    TRACE( "CActiveXControl::~CActiveXControl" );

    if ( m_piDispatch )
    {
        m_piDispatch->Release();
        m_piDispatch = NULL;
    }
}

HRESULT CActiveXControl::CreateInstance(
    void
    )
{
    TRACE( "CActiveXControl::CreateInstance" );

    if ( m_piDispatch != NULL )
    {
        return S_OK;
    }

    return ::CoCreateInstance(GetCLSID(), NULL, 
        CLSCTX_INPROC_SERVER, IID_IDispatch, (void**)&m_piDispatch);
}

void CActiveXControl::OnInitialUpdate(
    void
    )
{
    TRACE( "CActiveXControl::OnInitialUpdate" );
}

HRESULT CActiveXControl::PutPropertyByName(
    LPCOLESTR lpsz, 
    VARIANT* pVar
    )
{
    TRACE( "CActiveXControl::PutPropertyByName" );

    if (m_piDispatch == NULL)
        return E_UNEXPECTED;

    if (pVar == NULL)
        return E_INVALIDARG;

    DISPID dwDispID;
    HRESULT hr = GetIDOfName(lpsz, &dwDispID);
    if (SUCCEEDED(hr))
    {
        hr = PutProperty(m_piDispatch, dwDispID, pVar);
    }

    return hr;
}

HRESULT CActiveXControl::GetIDOfName(
    LPCOLESTR lpsz, 
    DISPID* pdispid
    )
{
    TRACE( "CActiveXControl::GetIDOfName" );

    if (m_piDispatch == NULL)
    {
        return E_UNEXPECTED;
    }

    return m_piDispatch->GetIDsOfNames( IID_NULL, 
                                        (LPOLESTR*)&lpsz, 
                                        1, 
                                        LOCALE_USER_DEFAULT, 
                                        pdispid );
}

HRESULT CActiveXControl::PutProperty(
    IDispatch* pDisp, 
    DISPID dwDispID,
    VARIANT* pVar
    )
{
    TRACE( "CActiveXControl::PutProperty" );

    DISPPARAMS dispparams = {NULL, NULL, 1, 1};
    dispparams.rgvarg = pVar;
    DISPID dispidPut = DISPID_PROPERTYPUT;
    dispparams.rgdispidNamedArgs = &dispidPut;
    
    if (pVar->vt == VT_UNKNOWN || pVar->vt == VT_DISPATCH || 
        (pVar->vt & VT_ARRAY) || (pVar->vt & VT_BYREF))
    {
        HRESULT hr = pDisp->Invoke(dwDispID, IID_NULL,
            LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUTREF,
            &dispparams, NULL, NULL, NULL);
        if (SUCCEEDED(hr))
            return hr;
    }
    
    return pDisp->Invoke( dwDispID, 
                          IID_NULL,
                          LOCALE_USER_DEFAULT, 
                          DISPATCH_PROPERTYPUT,
                          &dispparams, 
                          NULL, 
                          NULL, 
                          NULL );
}

HRESULT CActiveXControl::GetProperty(
    IDispatch* pDisp, 
    DISPID dwDispID,
    VARIANT* pVar
    )
{
    TRACE( "CActiveXControl::GetProperty" );

    DISPPARAMS dispparamsNoArgs = { NULL, NULL, 0, 0 };

    return pDisp->Invoke( dwDispID, 
                          IID_NULL,
                          LOCALE_USER_DEFAULT, 
                          DISPATCH_PROPERTYGET,
                          &dispparamsNoArgs, 
                          pVar, 
                          NULL,
                          NULL );
}

// Invoke a method by DISPID with a single parameter

HRESULT CActiveXControl::Invoke1(
    DISPID dispid, 
    VARIANT* pvarParam1, 
    VARIANT* pvarRet /* = NULL */
    )
{
    TRACE( "CActiveXControl::Invoke1" );

    if ( m_piDispatch == NULL )
    {
        return E_FAIL;
    }

    DISPPARAMS dispparams = { pvarParam1, NULL, 1, 0 };

    return m_piDispatch->Invoke( dispid, 
                                 IID_NULL, 
                                 LOCALE_USER_DEFAULT, 
                                 DISPATCH_METHOD, 
                                 &dispparams, 
                                 pvarRet, 
                                 NULL, 
                                 NULL );
}

// Invoke a method by name with a single parameter

HRESULT CActiveXControl::Invoke1(
    LPCOLESTR lpszName, 
    VARIANT* pvarParam1, 
    VARIANT* pvarRet /* = NULL */
    )
{
    TRACE( "CActiveXControl::Invoke1" );

    HRESULT hr;
    DISPID dispid;

    hr = GetIDOfName( lpszName, &dispid );
    if ( SUCCEEDED(hr) )
    {
        hr = Invoke1( dispid, pvarParam1, pvarRet );
    }

    return hr;
}

// ---------------------------------------------------------------------------

CControlHost::CControlHost(
    CActiveXControl* pControl
    ) : \
    _hwnd(NULL), 
    _punkOuter(NULL), 
    _cRef(0),
    m_pControl(pControl)
{ 
    TRACE( "CControlHost::CControlHost" );
} 
 
CControlHost::~CControlHost(
    ) 
{
    TRACE( "CControlHost::~CControlHost" );

    // The extra addref is done here to make sure that the last Release
    // call in DeleteControl doesn't do the delete this

    AddRef();

    DeleteControl();    // insurance 
} 
 
 
HRESULT CControlHost::SetHwnd(
    HWND hwnd
    ) 
{ 
    TRACE( "CControlHost::SetHwnd" );

    _hwnd = hwnd; 
    
    if ( ! GetClientRect( _hwnd, &_rcPos ) ) 
    {
        SetRectEmpty( &_rcPos );
    }
 
    return S_OK; 
} 

HRESULT CControlHost::CreateControl(
    void
    ) 
{ 
    TRACE( "CControlHost::CreateControl" );

    HRESULT hr;             // standard ole return code 
    IPersistStreamInit *pps;  // IPersistStreamInit interface pointer 
     
    if (m_pControl == NULL) 
    {
        return E_UNEXPECTED; 
    }

    hr = m_pControl->CreateInstance();
    if (hr != S_OK)
    {
        return hr;
    }

    IOleObject* piOleObject = NULL;
    hr = m_pControl->GetDispInterfaceNoAddRef()->
        QueryInterface( IID_IOleObject, 
                        (void**)&piOleObject );
    if (hr != S_OK)
    {
        return hr;
    }

    AddRef();  // addref the container 
 
    piOleObject->SetClientSite(this); 

    // Controls like to know they have been initialized by getting 
    // called on their InitNew method. 

    hr = piOleObject->QueryInterface( IID_IPersistStreamInit, 
                                      (LPVOID *)&pps );
    if (SUCCEEDED(hr)) 
    { 
        pps->InitNew(); 
        pps->Release(); 
    } 

    // The control creation succeeded, lets create the model. 

    m_pControl->OnInitialUpdate(); 

    // Tell the control to activate and show itself. 

    piOleObject->DoVerb(OLEIVERB_INPLACEACTIVATE, NULL, this, 0, _hwnd,
                        &_rcPos); 
    piOleObject->DoVerb(OLEIVERB_SHOW, NULL, this, 0, _hwnd, &_rcPos); 


    // cache some frequently needed interface pointers     
    // cache the control's window handle 

    _hwndControl = NULL; 
    IOleInPlaceActiveObject *pipao = NULL; 
    hr = piOleObject->QueryInterface(IID_IOleInPlaceActiveObject, 
        (void**) &pipao); 
    if (SUCCEEDED(hr)) 
    { 
        pipao->GetWindow(&_hwndControl); 
        pipao->Release(); 
    } 

    piOleObject->Release(); 

    return hr; 
} 
 
HRESULT CControlHost::DeleteControl(
    void
    ) 
{ 
    TRACE( "CControlHost::DeleteControl" );

    // Close the Control and release the cached pointers. 

    if (m_pControl != NULL)  
    { 
        IOleObject* piOleObject = NULL;

        if (m_pControl->GetDispInterfaceNoAddRef()->QueryInterface(
            IID_IOleObject, (LPVOID *)&piOleObject) == S_OK)
        {
            piOleObject->DoVerb(OLEIVERB_HIDE, NULL, this, 0, _hwnd, NULL); 
            piOleObject->Close(OLECLOSE_NOSAVE); 
            piOleObject->SetClientSite(NULL); 

            piOleObject->Release(); 
        }

        Release();

        delete m_pControl; 
        m_pControl = NULL; 
    } 
 
    return (S_OK); 
} 
 
HRESULT CControlHost::QueryObject(
    REFIID riid, 
    void **ppvObject
    ) 
{ 
    TRACE( "CControlHost::QueryObject" );

    HRESULT hr = E_POINTER; 

    if (ppvObject) 
    { 
        if (m_pControl) 
        {
            IDispatch* piDispatch = NULL;
            if (m_pControl->GetDispInterface(&piDispatch) == S_OK)
            {
                hr = piDispatch->QueryInterface(riid, ppvObject); 
                piDispatch->Release();
            }
        } 
        else 
        { 
            *ppvObject = NULL; 
            hr = OLE_E_NOCONNECTION; 
        } 
    } 

    return hr; 
} 

////////////////////////////////////////////////////////////////////////// 
// IUnknown methods

HRESULT CControlHost::QueryInterface(
    REFIID riid, 
    LPVOID* ppvObj
    ) 
{
    LPVOID pvObj = NULL;
 
#ifdef _DEBUG
    LPOLESTR psz;
    if ( StringFromIID( riid, &psz ) == S_OK )
    {
        TRACE( "CControlHost::QueryInterface - riid = %S", psz );
        CoTaskMemFree( psz );
    }
#endif

    if ( IsEqualIID( riid, IID_IOleInPlaceSite ) ) 
    {
        pvObj = (IOleInPlaceSite *)this; 
    }
#ifdef IMPLEMENT_IOLECONTROLSITE
    else if ( IsEqualIID( riid, IID_IOleControlSite ) ) 
    {
        pvObj = (IOleControlSite *)this; 
    }
#endif
#ifdef IMPLEMENT_IOLEINPLACESITEWINDOWLESS
    else if ( IsEqualIID( riid, IID_IOleInPlaceSiteWindowless ) ) 
    {
        pvObj = (IOleInPlaceSiteWindowless *)this; 
    }
#endif
    else if ( IsEqualIID( riid, IID_IDispatch ) ) 
    {
        pvObj = (IDispatch *)this; 
    }
 
    if( pvObj ) 
    {
        AddRef(); 
    }
 
    *ppvObj = pvObj;

    return pvObj ? S_OK : E_NOINTERFACE;
} 
 
ULONG CControlHost::AddRef(
    void
    ) 
{
    OutputDebugString("CControlHost::AddRef\n");

    return (_cRef++);  // Not thread safe
} 
 
ULONG CControlHost::Release(
    void
    ) 
{ 
    OutputDebugString("CControlHost::Release\n");

    _cRef--; // Not thread safe
 
    if (_cRef > 0) 
    {
        return _cRef; 
    }
 
    _cRef = 0; 
    delete this; 
 
    return 0; 
} 
 
////////////////////////////////////////////////////////////////////////// 
// IOleClientSite methods

HRESULT CControlHost::SaveObject(
    void
    ) 
{
    TRACE( "CControlHost::SaveObject" );

    return E_NOTIMPL;
} 
 
HRESULT CControlHost::GetMoniker(
    DWORD dwAssign, 
    DWORD dwWhichMoniker,
    LPMONIKER * ppMk
    ) 
{
    TRACE( "CControlHost::GetMoniker" );

    UNUSED_ALWAYS( dwAssign );
    UNUSED_ALWAYS( dwWhichMoniker );

    if ( ppMk )
    {
        *ppMk = NULL;
    }

    return E_NOTIMPL; 
} 
 
HRESULT CControlHost::GetContainer(
    LPOLECONTAINER * ppContainer 
    ) 
{ 
    TRACE( "CControlHost::GetContainer" );

    if ( ppContainer )
    {
        *ppContainer = NULL;
    }

    return E_NOTIMPL; 
} 
 
HRESULT CControlHost::ShowObject(
    void
    ) 
{ 
    TRACE( "CControlHost::ShowObject" );

    return S_OK; 
} 
 
HRESULT CControlHost::OnShowWindow(
    BOOL fShow
    ) 
{ 
    TRACE( "CControlHost::OnShowWindow" );

    UNUSED_ALWAYS( fShow );

    return S_OK; 
} 
 
HRESULT CControlHost::RequestNewObjectLayout(
    void
    ) 
{ 
    TRACE( "CControlHost::RequestNewObjectLayout" );

    return E_NOTIMPL; 
} 
 
////////////////////////////////////////////////////////////////////////// 
// IOleWindow methods

HRESULT CControlHost::GetWindow(
    HWND* lphwnd
    ) 
{ 
    TRACE( "CControlHost::GetWindow" );

    *lphwnd = _hwnd; 

    return ( _hwnd == NULL ) ? S_FALSE : S_OK; 
} 
 
HRESULT CControlHost::ContextSensitiveHelp(
    BOOL fEnterMode
    ) 
{ 
    TRACE( "CControlHost::ContextSensitiveHelp" );

    UNUSED_ALWAYS( fEnterMode );

    return E_NOTIMPL; 
} 
 
////////////////////////////////////////////////////////////////////////// 
// IOleInPlaceSite methods

HRESULT CControlHost::CanInPlaceActivate(
    void
    ) 
{ 
    TRACE( "CControlHost::CanInPlaceActivate" );

    return S_OK; 
} 
 
HRESULT CControlHost::OnInPlaceActivate(
    void
    ) 
{ 
    TRACE( "CControlHost::OnInPlaceActivate" );

    return S_OK; 
} 
 
HRESULT CControlHost::OnUIActivate(
    void
    ) 
{ 
    TRACE( "CControlHost::OnUIActivate" );

    return S_OK; 
} 
 
HRESULT CControlHost::GetWindowContext(
    IOleInPlaceFrame** ppFrame, 
    IOleInPlaceUIWindow** ppIIPUIWin,
    LPRECT lprcPosRect, 
    LPRECT lprcClipRect, 
    LPOLEINPLACEFRAMEINFO lpFrameInfo
    ) 
{
    TRACE( "CControlHost::GetWindowContext" );

    *ppFrame = this; 
    AddRef(); 
 
    *ppIIPUIWin = NULL; 
 
    CopyRect(lprcPosRect, &_rcPos); 
    CopyRect(lprcClipRect, &_rcPos); 
 
    lpFrameInfo->cb             = sizeof(OLEINPLACEFRAMEINFO); 
    lpFrameInfo->fMDIApp        = FALSE; 
    lpFrameInfo->hwndFrame      = _hwnd; 
    lpFrameInfo->haccel         = 0; 
    lpFrameInfo->cAccelEntries  = 0; 
 
    return S_OK;
} 
 
HRESULT CControlHost::Scroll(
    SIZE scrollExtent
    ) 
{ 
    TRACE( "CControlHost::Scroll" );

    UNUSED_ALWAYS( scrollExtent );

    return E_NOTIMPL; 
} 
 
HRESULT CControlHost::OnUIDeactivate(
    BOOL fUndoable
    ) 
{
    TRACE( "CControlHost::OnUIDeactivate" );

    UNUSED_ALWAYS( fUndoable );

    return S_OK; 
} 
 
HRESULT CControlHost::OnInPlaceDeactivate(
    void
    ) 
{ 
    TRACE( "CControlHost::OnInPlaceDeactivate" );

    return S_OK; 
} 
 
HRESULT CControlHost::DiscardUndoState(
    void
    ) 
{ 
    TRACE( "CControlHost::DiscardUndoState" );

    return S_OK; 
} 
 
HRESULT CControlHost::DeactivateAndUndo(
    void
    ) 
{ 
    TRACE( "CControlHost::DeactivateAndUndo" );

    return S_OK; 
} 
 
HRESULT CControlHost::OnPosRectChange(
    LPCRECT lprcPosRect
    ) 
{ 
    TRACE( "CControlHost::OnPosRectChange" );

    RECT rcPos, rcClient; 
    HWND hwnd; 
    GetWindow(&hwnd); 
    GetClientRect(hwnd, &rcClient); 
    GetWindowRect(hwnd, &rcPos); 
 
    if(rcClient.bottom < lprcPosRect->bottom) 
        rcPos.bottom += lprcPosRect->bottom - rcClient.bottom; 
    else 
        rcPos.bottom -= rcClient.bottom - lprcPosRect->bottom; 
 
    if(rcClient.right < lprcPosRect->right) 
        rcPos.right += lprcPosRect->right - rcClient.right;  
    else 
        rcPos.right -= rcClient.right - lprcPosRect->right;  
 
    MoveWindow( hwnd,  
                rcPos.left,  
                rcPos.top,  
                rcPos.right - rcPos.left,  
                rcPos.bottom - rcPos.top, 
                TRUE); 
 
    return S_OK;
} 
 
#ifdef IMPLEMENT_IOLEINPLACESITEWINDOWLESS

////////////////////////////////////////////////////////////////////////// 
// IOleInPlaceSiteEx methods

HRESULT CControlHost::OnInPlaceActivateEx(
    BOOL *pfNoRedraw, 
    DWORD dwFlags
    ) 
{ 
    TRACE( "CControlHost::OnInPlaceActivateEx" );

    UNUSED_ALWAYS( pfNoRedraw );
    UNUSED_ALWAYS( dwFlags );

    if ( m_pControl == NULL )
    {
        return E_FAIL;
    }

    OleLockRunning( m_pControl->GetDispInterfaceNoAddRef(),
                    TRUE, 
                    FALSE ); 

    return S_OK; 
} 
 
HRESULT CControlHost::OnInPlaceDeactivateEx(
    BOOL fNoRedraw
    ) 
{
    TRACE( "CControlHost::OnInPlaceDeactivateEx" );

    UNUSED_ALWAYS( fNoRedraw );

    if ( m_pControl == NULL )
    {
        return E_FAIL;
    }

    OleLockRunning( m_pControl->GetDispInterfaceNoAddRef(),
                    FALSE, 
                    FALSE ); 

    return S_OK; 
} 
 
 
HRESULT CControlHost::RequestUIActivate(
    void
    ) 
{ 
    TRACE( "CControlHost::RequestUIActivate" );

    return S_OK; 
} 

 
////////////////////////////////////////////////////////////////////////// 
// IOleInPlaceSiteWindowless methods

HRESULT CControlHost::CanWindowlessActivate(
    void
    ) 
{ 
    TRACE( "CControlHost::CanWindowlessActivate" );

    return TRUE; // m_bCanWindowlessActivate); 
} 

HRESULT CControlHost::GetCapture(
    void
    ) 
{ 
    TRACE( "CControlHost::GetCapture" );

    return E_NOTIMPL;
} 
 
HRESULT CControlHost::SetCapture(
    BOOL fCapture
    ) 
{ 
    TRACE( "CControlHost::SetCapture" );

    if (fCapture) 
    { 
        ::SetCapture(_hwnd); 
        _bCapture = TRUE; 
    } 
    else 
    { 
        ::ReleaseCapture(); 
        _bCapture = FALSE; 
    } 

    return S_OK; 
} 
 
HRESULT CControlHost::GetFocus(
    void
    ) 
{ 
    TRACE( "CControlHost::GetFocus" );

    return S_OK;
} 
 
HRESULT CControlHost::SetFocus(
    BOOL fFocus
    ) 
{ 
    TRACE( "CControlHost::SetFocus" );

    UNUSED_ALWAYS( fFocus );

    return S_OK;
} 
 
HRESULT CControlHost::GetDC(
    LPCRECT pRect, 
    DWORD grfFlags, 
    HDC* phDC
    ) 
{
    TRACE( "CControlHost::GetDC" );

    UNUSED_ALWAYS( pRect );
    UNUSED_ALWAYS( grfFlags );

    if (!phDC) 
    {
        return E_POINTER; 
    }
 
    *phDC = ::GetDC(_hwnd); 

    return S_OK; 
} 
 
HRESULT CControlHost::ReleaseDC(
    HDC hDC
    ) 
{ 
    TRACE( "CControlHost::ReleaseDC" );

    ::ReleaseDC(_hwnd, hDC); 

    return S_OK; 
} 
 
HRESULT CControlHost::InvalidateRect(
    LPCRECT pRect, 
    BOOL fErase
    ) 
{ 
    TRACE( "CControlHost::InvalidateRect" );

    ::InvalidateRect(_hwnd, pRect, fErase); 

    return S_OK; 
} 
 
HRESULT CControlHost::InvalidateRgn(
    HRGN hRGN, 
    BOOL fErase
    ) 
{ 
    TRACE( "CControlHost::InvalidateRgn" );

    ::InvalidateRgn(_hwnd, hRGN, fErase); 

    return S_OK; 
} 
 
HRESULT CControlHost::ScrollRect(
    INT dx, 
    INT dy, 
    LPCRECT pRectScroll, 
    LPCRECT pRectClip
    ) 
{ 
    TRACE( "CControlHost::ScrollRect" );

    UNUSED_ALWAYS( dx );
    UNUSED_ALWAYS( dy );
    UNUSED_ALWAYS( pRectScroll );
    UNUSED_ALWAYS( pRectClip );

    return S_OK;
} 
 
HRESULT CControlHost::AdjustRect(
    LPRECT prc
    ) 
{ 
    TRACE( "CControlHost::AdjustRect" );

    UNUSED_ALWAYS( prc );

    return S_OK;
} 
 
HRESULT CControlHost::OnDefWindowMessage(
    UINT msg, 
    WPARAM wParam, 
    LPARAM lParam, 
    LRESULT* plResult
    ) 
{ 
    TRACE( "CControlHost::OnDefWindowMessage" );

    *plResult = ::DefWindowProc(_hwnd, msg, wParam, lParam); 

    return S_OK; 
} 

#endif
 
////////////////////////////////////////////////////////////////////////// 
// IOleInPlaceUIWindow methods

HRESULT CControlHost::GetBorder(
    LPRECT lprectBorder
    ) 
{ 
    TRACE( "CControlHost::GetBorder" );

    UNUSED_ALWAYS( lprectBorder );

    return E_NOTIMPL;
} 
 
HRESULT CControlHost::RequestBorderSpace(
    LPCBORDERWIDTHS lpborderwidths
    ) 
{ 
    TRACE( "CControlHost::RequestBorderSpace" );

    UNUSED_ALWAYS( lpborderwidths );

    return E_NOTIMPL;
} 
 
HRESULT CControlHost::SetBorderSpace(
    LPCBORDERWIDTHS lpborderwidths
    ) 
{ 
    TRACE( "CControlHost::SetBorderSpace" );

    if( _punkOuter ) 
    {
        return ((CControlHost*)_punkOuter)->SetBorderSpace( lpborderwidths ); 
    }
 
    return E_NOTIMPL; 
} 
 
HRESULT CControlHost::SetActiveObject(
    IOleInPlaceActiveObject* pActiveObject, 
    LPCOLESTR lpszObjName
    ) 
{ 
    TRACE( "CControlHost::SetActiveObject" );

    UNUSED_ALWAYS( pActiveObject );
    UNUSED_ALWAYS( lpszObjName );

    return E_NOTIMPL; 
} 
 
////////////////////////////////////////////////////////////////////////// 
// IOleInPlaceFrame methods

HRESULT CControlHost::InsertMenus(
    HMENU hmenuShared, 
    LPOLEMENUGROUPWIDTHS lpMenuWidths
    ) 
{ 
    TRACE( "CControlHost::InsertMenus" );

    UNUSED_ALWAYS( hmenuShared );
    UNUSED_ALWAYS( lpMenuWidths );

    return E_NOTIMPL;
} 
 
HRESULT CControlHost::SetMenu(
    HMENU hmenuShared, 
    HOLEMENU holemenu, 
    HWND hwndActiveObject
    ) 
{ 
    TRACE( "CControlHost::SetMenu" );

    UNUSED_ALWAYS( hmenuShared );
    UNUSED_ALWAYS( holemenu );
    UNUSED_ALWAYS( hwndActiveObject );

    return E_NOTIMPL;
} 
 
HRESULT CControlHost::RemoveMenus(
    HMENU hmenuShared
    ) 
{ 
    TRACE( "CControlHost::RemoveMenus" );

    UNUSED_ALWAYS( hmenuShared );

    return E_NOTIMPL;
} 
 
HRESULT CControlHost::SetStatusText(
    LPCOLESTR pszStatusText
    ) 
{ 
    TRACE( "CControlHost::SetStatusText" );

    UNUSED_ALWAYS( pszStatusText );

    return E_NOTIMPL;
} 
 
HRESULT CControlHost::EnableModeless(
    BOOL fEnable
    )
{ 
    TRACE( "CControlHost::EnableModeless" );

    UNUSED_ALWAYS( fEnable );

    return E_NOTIMPL;
} 
 
HRESULT CControlHost::TranslateAccelerator(
    LPMSG lpmsg, 
    WORD wID
    ) 
{ 
    TRACE( "CControlHost::TranslateAccelerator" );

    UNUSED_ALWAYS( lpmsg );
    UNUSED_ALWAYS( wID );

    return E_NOTIMPL; 
} 
 
#ifdef IMPLEMENT_IOLECONTROLSITE

////////////////////////////////////////////////////////////////////////// 
// IOleControlSite methods

HRESULT CControlHost::OnControlInfoChanged(
    void
    ) 
{ 
    TRACE( "CControlHost::OnControlInfoChanged" );

    return E_NOTIMPL; 
} 
 
HRESULT CControlHost::LockInPlaceActive(
    BOOL fLock
    ) 
{ 
    TRACE( "CControlHost::LockInPlaceActive" );

    UNUSED_ALWAYS( fLock );

    return E_NOTIMPL; 
} 
 
HRESULT CControlHost::GetExtendedControl(
    IDispatch** ppDisp
    ) 
{ 
    TRACE( "CControlHost::GetExtendedControl" );

    if (ppDisp == NULL) 
    {
        return E_INVALIDARG; 
    }
 
    *ppDisp = (IDispatch *)this; 
    AddRef(); 
 
    return S_OK; 
} 
 
HRESULT CControlHost::TransformCoords(
    POINTL *pptlHimetric, 
    POINTF *pptfContainer, 
    DWORD dwFlags
    ) 
{ 
    TRACE( "CControlHost::TransformCoords" );

    UNUSED_ALWAYS( pptlHimetric );
    UNUSED_ALWAYS( pptfContainer );
    UNUSED_ALWAYS( dwFlags );

    return E_NOTIMPL; 
} 
 
HRESULT CControlHost::TranslateAccelerator(
    LPMSG pMsg, 
    DWORD grfModifiers
    ) 
{ 
    TRACE( "CControlHost::TranslateAccelerator" );

    UNUSED_ALWAYS( pMsg );
    UNUSED_ALWAYS( grfModifiers );

    return S_FALSE; 
} 
 
HRESULT CControlHost::OnFocus(
    BOOL fGotFocus
    ) 
{ 
    TRACE( "CControlHost::OnFocus" );

    UNUSED_ALWAYS( fGotFocus );

    return E_NOTIMPL; 
} 
 
HRESULT CControlHost::ShowPropertyFrame(
    void
    ) 
{ 
    TRACE( "CControlHost::ShowPropertyFrame" );

    return E_NOTIMPL; 
} 
 
#endif
 
////////////////////////////////////////////////////////////////////////// 
// IDispatch methods

HRESULT CControlHost::GetIDsOfNames(
    REFIID riid, 
    OLECHAR FAR* FAR* rgszNames, 
    unsigned int cNames, 
    LCID lcid, 
    DISPID FAR* rgdispid
    ) 
{ 
    UNUSED_ALWAYS( riid );
    UNUSED_ALWAYS( lcid );

    HRESULT     hr;                     // standard ole return code 
    LPOLESTR    pName; 
    DISPID      *pdispid; 
 
    hr      = S_OK; 
    pName   = *rgszNames; 
    pdispid = rgdispid; 
 
    for (UINT i=0; i<cNames; i++) 
    { 
        if (pName != NULL) 
        { 
            if (_wcsicmp(pName, L"basehref") == 0) 
                *pdispid = DISPID_BASEHREF; 
            else if (_wcsicmp(pName, L"align") == 0) 
                *pdispid = DISPID_ALIGN; 
            else 
            { 
                *pdispid = DISPID_UNKNOWN; 
                hr       = DISP_E_UNKNOWNNAME; 
            } 
        } 
 
        pdispid++; 
        pName++; 
    } 
 
    return hr; 
} 
 
HRESULT CControlHost::GetTypeInfo(
    unsigned int itinfo, 
    LCID lcid, 
    ITypeInfo FAR* FAR* pptinfo
    ) 
{ 
    UNUSED_ALWAYS( itinfo );
    UNUSED_ALWAYS( lcid );
    UNUSED_ALWAYS( pptinfo );

    return E_NOTIMPL; 
} 
 
HRESULT CControlHost::GetTypeInfoCount(
    unsigned int FAR * pctinfo
    ) 
{ 
    UNUSED_ALWAYS( pctinfo );

    return E_NOTIMPL; 
} 
 
HRESULT CControlHost::Invoke(
    DISPID dispid, 
    REFIID riid, 
    LCID lcid, 
    WORD wFlags, 
    DISPPARAMS FAR* pdispparams, 
    VARIANT FAR* pvarResult, 
    EXCEPINFO FAR* pexecinfo, 
    unsigned int FAR* puArgErr
    ) 
{ 
    UNUSED_ALWAYS( riid );
    UNUSED_ALWAYS( lcid );
    UNUSED_ALWAYS( pdispparams );
    UNUSED_ALWAYS( pexecinfo );
    UNUSED_ALWAYS( puArgErr );

    HRESULT hr = S_OK;
 
    // The following code are the DISPIDs for the ambient properties 
    // that we will support. 

    if ((pvarResult == NULL) && (wFlags == DISPATCH_PROPERTYGET)) 
    {
        return E_INVALIDARG;
    }
     
    switch (dispid) 
    { 
        case DISPID_AMBIENT_DISPLAYNAME: 
            if (!pvarResult->bstrVal) 
                pvarResult->bstrVal = ::SysAllocString(L""); 
 
            hr = S_OK; 
 
            // If we STILL don't have a bstrVal.  Clean up and return an empty variant. 
            if (!pvarResult->bstrVal)  
            { 
                VariantInit(pvarResult); 
                hr = E_FAIL; 
            } 
 
            break; 
 
        case DISPID_AMBIENT_USERMODE:
        case DISPID_AMBIENT_MESSAGEREFLECT:
            pvarResult->vt      = VT_BOOL;
            pvarResult->boolVal = TRUE; 
            hr                  = S_OK; 
            break; 
 
        case DISPID_AMBIENT_SHOWHATCHING: 
        case DISPID_AMBIENT_SHOWGRABHANDLES: 
        case DISPID_AMBIENT_SUPPORTSMNEMONICS: 
            pvarResult->vt      = VT_BOOL; 
            pvarResult->boolVal = FALSE; 
            hr                  = S_OK; 
            break; 
 
        // Not yet implemented! 
        case DISPID_AMBIENT_BACKCOLOR: 
        case DISPID_AMBIENT_FORECOLOR: 
        case DISPID_AMBIENT_UIDEAD: 
        case DISPID_AMBIENT_AUTOCLIP: 
            hr = S_OK; 
            break; 
 
        // Extender Properties 
        case DISPID_BASEHREF: 
        case DISPID_ALIGN: 
            hr = DISP_E_MEMBERNOTFOUND; 
            break; 
 
        default: 
            hr = DISP_E_MEMBERNOTFOUND; 
    } 
 
    return hr; 
} 
 
