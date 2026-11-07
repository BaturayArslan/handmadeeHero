#include <windows.h>

struct AppState
{
    
};

#define internal static
#define global_variable static
#define PI 3.14f
#define KiloByte(value) ((value) *1024LL)



template <class DERIVED_TYPE> class BaseWindow
{
public:
    static LRESULT CALLBACK WindowProc(
        HWND hWindow,
        UINT message,
        WPARAM wParam,
        LPARAM lParam
    )
    {
        LRESULT result = 0;
        DERIVED_TYPE  *pState = NULL;
        if(message == WM_NCCREATE)
        {
            CREATESTRUCT* pCreate = (CREATESTRUCT*) lParam;
            pState = (DERIVED_TYPE*)pCreate->lpCreateParams;
            SetWindowLongPtr(hWindow, GWLP_USERDATA, (LONG_PTR) pState);
            pState->m_hwnd = hWindow;
        }
        else
        {
            pState = (DERIVED_TYPE*) GetWindowLongPtr(hWindow, GWLP_USERDATA);
        }

        if(pState)
        {
            return pState->HandleMessage(message, wParam, lParam);
        }
        else
        {
            return DefWindowProc(hWindow, message, wParam, lParam);
        }
    }

    BOOL Create(
        LPCSTR windowName,
        DWORD dwStyle,
        int x = CW_USEDEFAULT,
        int y = CW_USEDEFAULT,
        int width = CW_USEDEFAULT,
        int height = CW_USEDEFAULT
    )
    {
        WNDCLASS windowClass = {};
        windowClass.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
        windowClass.lpszClassName = ClassName();
        windowClass.lpfnWndProc = DERIVED_TYPE::WindowProc;
        windowClass.hInstance = GetModuleHandle(NULL);


        RegisterClass(&windowClass);

        m_hwnd = CreateWindowEx(WS_EX_TOPMOST, ClassName(), windowName, dwStyle, x, y, width,
                                height, 0, 0, windowClass.hInstance, this);

        return (m_hwnd ? TRUE: FALSE);
    }

    BaseWindow() : m_hwnd(NULL) {}
    BaseWindow(int test) {}
    HWND Window() { return m_hwnd;}

   
protected:
    virtual PCSTR ClassName() const = 0;
    virtual LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam) = 0;
    
    HWND m_hwnd;
        
};

class MainWindow : public BaseWindow<MainWindow>
{
public:
    PCSTR ClassName() const {return "Main Window";};
    LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam);
};

LRESULT MainWindow::HandleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(m_hwnd, &ps);
            FillRect(hdc, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW+1));
            EndPaint(m_hwnd, &ps);
        }
        return 0;

    default:
        return DefWindowProc(m_hwnd, message, wParam, lParam);
    }
    return TRUE;    
}


    


int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hinstprev, LPSTR cmdline, int cmdshow)
{
    MainWindow win();
    
    if(!win.Create("Main window", WS_OVERLAPPEDWINDOW))
    {
        return 1;
    }

    ShowWindow(win.Window(), cmdshow);

    MSG msg = {};
    while(GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return 0;
}
