//
// StellaX
// Jeff Miller 05/02/2000
//

#ifndef _CONTROLHOST_H_ 
#define _CONTROLHOST_H_ 

//
// This code was based from the MSDN DRAGGEOCNTRL sample
// NOTE: PDF control requires UI Deactivate/UI Activate to resize
//

#include <ocidl.h> 

#define BASE_EXTENDED_PROPERTY      0x80010000 
#define DISPID_NAME                 (BASE_EXTENDED_PROPERTY | 0x00) 
#define DISPID_ALIGN                (BASE_EXTENDED_PROPERTY | 0x01) 
#define DISPID_BASEHREF             (BASE_EXTENDED_PROPERTY | 0x02) 

#define IMPLEMENT_IOLECONTROLSITE
#define IMPLEMENT_IOLEINPLACESITEWINDOWLESS

class CActiveXControl 
{ 
public:

    CActiveXControl();
    virtual ~CActiveXControl();

    HRESULT CreateInstance( void );
    HRESULT GetDispInterface( IDispatch** piDispatch );
    IDispatch* GetDispInterfaceNoAddRef( void );

    virtual REFCLSID GetCLSID( void ) = 0;
    virtual void OnInitialUpdate( void ); 

protected:

    // Dispatch helpers

    HRESULT PutPropertyByName( LPCOLESTR lpsz, VARIANT* pVar );
    HRESULT GetIDOfName( LPCOLESTR lpsz, DISPID* pdispid );

    static HRESULT GetProperty( IDispatch* pDisp, DISPID dwDispID,
        VARIANT* pVar );
    static HRESULT PutProperty( IDispatch* pDisp, DISPID dwDispID,
        VARIANT* pVar );

    // Invoke a method by DISPID with a single parameter

    HRESULT Invoke1( DISPID dispid, VARIANT* pvarParam1,
                     VARIANT* pvarRet = NULL );

    // Invoke a method by name with a single parameter

    HRESULT Invoke1( LPCOLESTR lpszName, VARIANT* pvarParam1, 
                     VARIANT* pvarRet = NULL );

private:

    IDispatch* m_piDispatch;

    CActiveXControl( const CActiveXControl& );  // no implementation
    void operator=( const CActiveXControl& );  // no implementation

}; 

inline HRESULT CActiveXControl::GetDispInterface(
    IDispatch** piDispatch
    )   
{
    if (piDispatch == NULL)
    {
        return E_INVALIDARG;
    }

    m_piDispatch->AddRef();
    *piDispatch = m_piDispatch;

    return S_OK;
}

inline IDispatch* CActiveXControl::GetDispInterfaceNoAddRef(
    void
    )
{
    return m_piDispatch;
}

// ---------------------------------------------------------------------------

class CControlHost : \
    public IDispatch, 
#ifdef IMPLEMENT_IOLEINPLACESITEWINDOWLESS
    public IOleInPlaceSiteWindowless, 
#endif
    public IOleInPlaceFrame, 
    public IOleControlSite, 
    public IOleClientSite 
{ 
public:

    CControlHost( CActiveXControl* pControl ); 
    ~CControlHost( ); 
    
    HRESULT SetHwnd( HWND hwnd );
    HRESULT CreateControl( void ); 
    HRESULT DeleteControl( void ); 
    HRESULT QueryObject( REFIID riid, void **ppvObject ); 

    HWND GetControlHWND( void ) const
        {
            return _hwndControl;
        }

protected: 

    HWND _hwnd;              // container window handle 
    HWND _hwndControl;       // Control's window handle 
    UINT _cRef;              // IUnknown ref count 
    BOOL _bCapture;          // mouse capture flag 
    RECT _rcPos; 
    
    void* _punkOuter;         // parent container 

    CActiveXControl* m_pControl;
    
public: 
    
    // *** IUnknown Methods *** 

    STDMETHODIMP QueryInterface( REFIID riid, LPVOID * ppvObj );
    STDMETHODIMP_(ULONG) AddRef( void ); 
    STDMETHODIMP_(ULONG) Release( void ); 
    
    // *** IDispatch Methods *** 

    STDMETHOD (GetIDsOfNames)( REFIID riid, OLECHAR FAR* FAR* rgszNames,
                               unsigned int cNames, LCID lcid, 
                               DISPID FAR* rgdispid ); 
    STDMETHOD (GetTypeInfo)( unsigned int itinfo, LCID lcid, 
                             ITypeInfo FAR* FAR* pptinfo ); 
    STDMETHOD (GetTypeInfoCount)( unsigned int FAR * pctinfo ); 
    STDMETHOD (Invoke)( DISPID dispid, REFIID riid, LCID lcid, WORD wFlags,
                        DISPPARAMS FAR *pdispparams, VARIANT FAR *pvarResult, 
                        EXCEPINFO FAR * pexecinfo, unsigned int FAR *puArgErr ); 
    
    // *** IOleClientSite methods *** 

    STDMETHOD (SaveObject)( void ); 
    STDMETHOD (GetMoniker)( DWORD, DWORD, LPMONIKER * ); 
    STDMETHOD (GetContainer)( LPOLECONTAINER * ); 
    STDMETHOD (ShowObject)( void ); 
    STDMETHOD (OnShowWindow)( BOOL ); 
    STDMETHOD (RequestNewObjectLayout)( void ); 
    
    // *** IOleWindow Methods *** 

    STDMETHOD (GetWindow)( HWND * phwnd );
    STDMETHOD (ContextSensitiveHelp)( BOOL fEnterMode );
    
    // *** IOleInPlaceSite Methods *** 

    STDMETHOD (CanInPlaceActivate)( void ); 
    STDMETHOD (OnInPlaceActivate)( void );
    STDMETHOD (OnUIActivate)( void ); 
    STDMETHOD (GetWindowContext)( IOleInPlaceFrame ** ppFrame, 
                                  IOleInPlaceUIWindow ** ppDoc,
                                  LPRECT lprcPosRect, LPRECT lprcClipRect,
                                  LPOLEINPLACEFRAMEINFO lpFrameInfo ); 
    STDMETHOD (Scroll)( SIZE scrollExtent ); 
    STDMETHOD (OnUIDeactivate)( BOOL fUndoable ); 
    STDMETHOD (OnInPlaceDeactivate)( void ); 
    STDMETHOD (DiscardUndoState)( void ); 
    STDMETHOD (DeactivateAndUndo)( void ); 
    STDMETHOD (OnPosRectChange)( LPCRECT lprcPosRect );

#ifdef IMPLEMENT_IOLEINPLACESITEWINDOWLESS

    // *** IOleInPlaceSiteEx Methods *** 

    STDMETHOD (OnInPlaceActivateEx)( BOOL *pfNoRedraw, DWORD dwFlags ); 
    STDMETHOD (OnInPlaceDeactivateEx)( BOOL fNoRedraw );
    STDMETHOD (RequestUIActivate)( void );
    
    // *** IOleInPlaceSiteWindowless Methods *** 

    STDMETHOD (CanWindowlessActivate)( void );
    STDMETHOD (GetCapture)( void );
    STDMETHOD (SetCapture)( BOOL fCapture );
    STDMETHOD (GetFocus)( void );
    STDMETHOD (SetFocus)( BOOL fFocus );
    STDMETHOD (GetDC)( LPCRECT pRect, DWORD grfFlags, HDC *phDC );
    STDMETHOD (ReleaseDC)( HDC hDC );
    STDMETHOD (InvalidateRect)( LPCRECT pRect, BOOL fErase );
    STDMETHOD (InvalidateRgn)( HRGN hRGN, BOOL fErase ); 
    STDMETHOD (ScrollRect)( INT dx, INT dy, LPCRECT pRectScroll,
                            LPCRECT pRectClip ); 
    STDMETHOD (AdjustRect)( LPRECT prc ); 
    STDMETHOD (OnDefWindowMessage)( UINT msg, WPARAM wParam, LPARAM lParam,
                                    LRESULT *plResult ); 

#endif
    
    // *** IOleInPlaceUIWindow Methods *** 

    STDMETHOD (GetBorder)(LPRECT lprectBorder); 
    STDMETHOD (RequestBorderSpace)(LPCBORDERWIDTHS lpborderwidths); 
    STDMETHOD (SetBorderSpace)(LPCBORDERWIDTHS lpborderwidths); 
    STDMETHOD (SetActiveObject)(IOleInPlaceActiveObject * pActiveObject, 
        LPCOLESTR lpszObjName); 
    
    // *** IOleInPlaceFrame Methods *** 

    STDMETHOD (InsertMenus)(HMENU hmenuShared,
                            LPOLEMENUGROUPWIDTHS lpMenuWidths); 
    STDMETHOD (SetMenu)(HMENU hmenuShared, HOLEMENU holemenu,
                        HWND hwndActiveObject); 
    STDMETHOD (RemoveMenus)(HMENU hmenuShared); 
    STDMETHOD (SetStatusText)(LPCOLESTR pszStatusText); 
    STDMETHOD (EnableModeless)(BOOL fEnable); 
    STDMETHOD (TranslateAccelerator)(LPMSG lpmsg, WORD wID); 

#ifdef IMPLEMENT_IOLECONTROLSITE

    // *** IOleControlSite Methods *** 

    STDMETHOD (OnControlInfoChanged)(void); 
    STDMETHOD (LockInPlaceActive)(BOOL fLock); 
    STDMETHOD (GetExtendedControl)(IDispatch **ppDisp); 
    STDMETHOD (TransformCoords)(POINTL *pptlHimetric, POINTF *pptfContainer,
                                DWORD dwFlags); 
    STDMETHOD (TranslateAccelerator)(LPMSG pMsg, DWORD grfModifiers); 
    STDMETHOD (OnFocus)(BOOL fGotFocus); 
    STDMETHOD (ShowPropertyFrame)(void); 

#endif

private:

    CControlHost( const CControlHost& );  // no implementation
    void operator=( const CControlHost& );  // no implementation
}; 
 
#endif 
