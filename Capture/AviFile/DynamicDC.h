/*
http://www.codeproject.com/gdi/dynamic_dc.asp

Examples
Using the Device Context as a Win32 HDC.

CDynamicDC dc(hWnd);
::MoveTo(dc, 10, 10, &ptOld);
::LineTo(dc, 100, 100);

Using the Device Context as a pointer to an MFC CDC class.

CDynamicDC dc(hWnd);
dc->MoveTo(10, 10);
dc->LineTo(100, 100);

Using the Device Context as a reference to an MFC CDC class.

CDynamicDC dcDynamic(hWnd);
CDC &dc(*dcDynamic);
dc.MoveTo(10, 10);
dc.LineTo(100, 100);



*/



class CDynamicDC
{
  private:
    HDC  m_hDC;
    HWND m_hWnd;

  public:
    CDynamicDC(CWnd *__pWnd);
    CDynamicDC(HWND __hWnd);
    virtual ~CDynamicDC();

    operator HDC();     // dynamic conversion to HDC
    CDC *operator->();  // dynamic conversion to CDC pointer
    operator CDC*();    // allow CDC pointer dereferencing
};

// constructor initialised with an MFC CWnd pointer
inline CDynamicDC::CDynamicDC(CWnd *__pWnd)
  : m_hDC(NULL),
    m_hWnd(NULL)
{
    ASSERT(__pWnd != NULL  &&  ::IsWindow(__pWnd->GetSafeHwnd()));
    m_hWnd = __pWnd->GetSafeHwnd();
    m_hDC  = ::GetDCEx(m_hWnd, NULL, DCX_CACHE);
}

// constructor initialised with a Win32 windows handle
inline CDynamicDC::CDynamicDC(HWND __hWnd)
  : m_hDC(NULL),
    m_hWnd(NULL)
{
    ASSERT(__hWnd != NULL  &&  ::IsWindow(__hWnd));
    m_hWnd = __hWnd;
    m_hDC  = ::GetDCEx(m_hWnd, NULL, DCX_CACHE);
}

// virtual destructor will free the DC
inline CDynamicDC::~CDynamicDC()
{
    if (m_hDC != NULL)
    {
        ASSERT(m_hWnd != NULL  &&  ::IsWindow(m_hWnd));
        ::ReleaseDC(m_hWnd, m_hDC);
    }
}

// this operator implements dynamic conversion to a Win32 HDC object so that
// an instance of this class can be used anywhere an HDC is expected
inline CDynamicDC::operator HDC()
{
    return m_hDC;
}

// this operator implements dynamic conversion to an MFC CDC object so that
// an instance of this class can be used anywhere a pointer to a CDC object
// is expected
inline CDC *CDynamicDC::operator->()
{
    return CDC::FromHandle(m_hDC);
}

// this operator enables an instance of this class to be dereferenced as if
// it were a pointer to an MFC CDC object, for example if a CDC reference
// is required
inline CDynamicDC::operator CDC*()
{
    return CDC::FromHandle(m_hDC);
}
