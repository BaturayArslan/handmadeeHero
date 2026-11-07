#include <stdint.h>
#include <windows.h>
#include <winnt.h>

#define internal static
#define global_variable static


struct Bitmap {
  BITMAPINFO info;
  void *memory;
  uint32_t width = 0;
  uint32_t height = 0;
  static const uint32_t bit_per_pixel = 32; // should be multiple of 8.
};
Bitmap g_bitmap = {};

struct Dimensions {
  uint32_t width;
  uint32_t height;
};

global_variable bool g_running = false;
global_variable uint32_t g_offsetX = 0;
global_variable uint32_t g_offsetY = 0;

global_variable uint32_t test_variable = 32;

internal Dimensions Win32GetClientDimensions(HWND hWindow) {
  RECT clientRect;
  GetClientRect(hWindow, &clientRect);
  uint32_t width = clientRect.right - clientRect.left;
  uint32_t height = clientRect.bottom - clientRect.top;
  Dimensions dimensions = {width, height};
  return dimensions;
}

void RenderCoolGradient(uint32_t offsetX, uint32_t offsetY) {
  if (!g_bitmap.memory) {
    return;
  }

  const uint32_t BYTE_PER_PIXEL = g_bitmap.bit_per_pixel / 8;
  static_assert(BYTE_PER_PIXEL == 4,
                "This loop assumes every pixel is 32bit(4byte) long.");

  for (int y = 0; y < g_bitmap.height; y++) {
    uint32_t *p_rowBegin = (uint32_t *)g_bitmap.memory;
    p_rowBegin += y * g_bitmap.width;

    for (int x = 0; x < g_bitmap.width; x++) {
      // 0x xxrrggbb
      *(p_rowBegin + x) = ((uint32_t)0 | (uint8_t)(x + offsetX)) |
                          ((uint8_t)(y + offsetY) << 8) | ((uint8_t)0 << 8);
    }
  }
}

internal void Win32ResizeDIBSection(int width, int height) {
  if (g_bitmap.memory) {
    // Delete first and then create new one.
    VirtualFree(g_bitmap.memory, 0, MEM_RELEASE);
  }

  g_bitmap.info.bmiHeader.biSize = sizeof(g_bitmap.info.bmiHeader);
  g_bitmap.info.bmiHeader.biWidth = width;
  g_bitmap.info.bmiHeader.biHeight = -height;
  g_bitmap.info.bmiHeader.biPlanes = 1;
  g_bitmap.info.bmiHeader.biBitCount = g_bitmap.bit_per_pixel;
  g_bitmap.info.bmiHeader.biCompression = BI_RGB;
  g_bitmap.info.bmiHeader.biSizeImage = 0;
  g_bitmap.info.bmiHeader.biXPelsPerMeter = 0;
  g_bitmap.info.bmiHeader.biYPelsPerMeter = 0;
  g_bitmap.info.bmiHeader.biClrUsed = 0;
  g_bitmap.info.bmiHeader.biClrImportant = 0;

  g_bitmap.width = width;
  g_bitmap.height = height;

  size_t g_bitmapMemorySize = width * height * (g_bitmap.bit_per_pixel / 8);
  g_bitmap.memory = VirtualAlloc(0, g_bitmapMemorySize,
                                 MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

internal void Win32UpdateWindow(HDC hdc, RECT *clientRect, int x = 0, int y = 0,
                                int width = 0, int height = 0) {
  RenderCoolGradient(++g_offsetX, g_offsetY);
  int clientWidth = clientRect->right - clientRect->left;
  int clientHeight = clientRect->bottom - clientRect->top;
  StretchDIBits(hdc, 0, 0, clientWidth, clientHeight, 0, 0, g_bitmap.width,
                g_bitmap.height, g_bitmap.memory, &g_bitmap.info,
                DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK Win32MainWindowCallback(HWND hWindow, UINT message,
                                         WPARAM wParam, LPARAM lParam) {
  LRESULT result = 0;
  switch (message) {
  case WM_SIZE: {
    OutputDebugString("WM_RESIZE\n");
    break;
  }
  case WM_CREATE: {
    OutputDebugString("WM_CREATE\n");
    break;
  }
  case WM_DESTROY: {
    g_running = false;
    OutputDebugString("WM_DESTROY\n");
    break;
  }
  case WM_CLOSE: {
    OutputDebugString("WM_CLOSE\n");
    DestroyWindow(hWindow);
    break;
  }
  case WM_ACTIVATEAPP: {
    if (wParam) {
      // Activated
      ShowWindow(hWindow, SW_SHOW);
    } else {
      ShowWindow(hWindow, SW_MINIMIZE);
    }
    OutputDebugString("WM_ACTIVATEAPP\n");
    break;
  }
  case WM_PAINT: {
    PAINTSTRUCT ps = {};
    HDC hdc = BeginPaint(hWindow, &ps);
    int x = ps.rcPaint.left;
    int y = ps.rcPaint.top;
    int width = ps.rcPaint.right - ps.rcPaint.left;
    int height = ps.rcPaint.bottom - ps.rcPaint.top;

    RECT clientRect;
    GetClientRect(hWindow, &clientRect);
    Win32UpdateWindow(hdc, &clientRect, x, y, width, height);

    EndPaint(hWindow, &ps);
    break;
  }
  default: {
    result = DefWindowProc(hWindow, message, wParam, lParam);
    // OutputDebugString("WM_DEFAULT");
  }
  }

  return result;
}

int APIENTRY wWinMain(HINSTANCE hInst, HINSTANCE hinstprev, PWSTR cmdline,
                      int cmdshow) {
  WNDCLASS windowClass = {};
  windowClass.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
  windowClass.lpfnWndProc = Win32MainWindowCallback;
  windowClass.hInstance = hInst;
  windowClass.lpszClassName = "HandmadeHeroClass";

  ATOM registerResult = RegisterClass(&windowClass);
  if (!registerResult) {
    // Something went wrong.
    OutputDebugString("Couldn't register class\n");
    exit(1);
  }

  HWND windowHandle =
      CreateWindowEx(WS_EX_TOPMOST,       // DWORD     dwExStyle,
                     "HandmadeHeroClass", // LPCSTR    lpClassName,
                     "Zero to Hero",      // LPCSTR    lpWindowName,
                     WS_OVERLAPPEDWINDOW | WS_VISIBLE, // DWORD     dwStyle,
                     CW_USEDEFAULT,                    // int       X,
                     CW_USEDEFAULT,                    // int       Y,
                     CW_USEDEFAULT,                    // int       nWidth,
                     CW_USEDEFAULT,                    // int       nHeight,
                     0,                                // HWND      hWndParent,
                     0,                                // HMENU     hMenu,
                     hInst,                            // HINSTANCE hInstance,
                     0                                 // LPVOID    lpParam
      );

  if (!windowHandle) {
    // Something went wrong.
    DWORD Error = GetLastError();
    char Buffer[256];
    wsprintfA(Buffer, "CreateWindowEx failed, error code: %lu\n", Error);
    OutputDebugStringA(Buffer);
    exit(1);
  }

  Dimensions clientWindowDimensions = Win32GetClientDimensions(windowHandle);
  Win32ResizeDIBSection(clientWindowDimensions.width,
                        clientWindowDimensions.height);

  // typedef struct tagMSG {
  //   HWND   hwnd;
  //   UINT   message;
  //   WPARAM wParam;
  //   LPARAM lParam;
  //   DWORD  time;
  //   POINT  pt;
  //   DWORD  lPrivate;
  // } MSG, *PMSG, *NPMSG, *LPMSG;
  MSG msg = {};
  g_running = true;
  while (g_running) {
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    // TestFunciton(hello, asfasf)
    HDC dc = GetDC(windowHandle); // 
    RECT clientRect;
    GetClientRect(windowHandle, &clientRect);
    Win32UpdateWindow(dc, &clientRect);
    ReleaseDC(windowHandle, dc);
}

   return 0;
}
