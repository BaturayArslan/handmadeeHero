#include <stdint.h>
#include <stdio.h>
#include <windows.h>
#include <winnt.h>
#include <xaudio2.h>
#include <xinput.h>

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

// #define X_INPUT_GET_STATE(name)                                                \
//   DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
// typedef X_INPUT_GET_STATE(x_input_get_state);
// X_INPUT_GET_STATE(XInputGetStateStub) { return 0; }
// global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
// #define XInputGetState XInputGetState_

// #define X_INPUT_SET_STATE(name)                                                \
//   DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
// typedef X_INPUT_SET_STATE(x_input_set_state);
// X_INPUT_SET_STATE(XInputSetStateStub) { return 0; }
// global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
// #define XInputSetState XInputSetState_

typedef DWORD WINAPI x_input_get_state(DWORD dwUserIndex, XINPUT_STATE *pState);
typedef DWORD WINAPI x_input_set_state(DWORD dwUserIndex,
                                       XINPUT_VIBRATION *pVibration);

global_variable x_input_get_state *XInputGetState_;
global_variable x_input_set_state *XInputSetState_;

internal void Win32LoadXInput() {
  HMODULE hModule = LoadLibrary("xinput1_3.dll");
  if (hModule) {
    XInputGetState_ =
        (x_input_get_state *)GetProcAddress(hModule, "XInputGetState");
    XInputSetState_ =
        (x_input_set_state *)GetProcAddress(hModule, "XInputSetState");
  }
}

DWORD WINAPI XInputGetState(DWORD dwUserIndex,
                            XINPUT_STATE *pState) WIN_NOEXCEPT {
  if (XInputGetState_) {
    return XInputGetState_(dwUserIndex, pState);
  }

  return ERROR_DEVICE_NOT_CONNECTED;
}

DWORD WINAPI XInputSetState(DWORD dwUserIndex,
                            XINPUT_VIBRATION *pVibration) WIN_NOEXCEPT {
  if (XInputSetState_) {
    return XInputSetState_(dwUserIndex, pVibration);
  }
  return 0;
}

internal void PrintDebugString(const char *message, int errorCode) {
  char buffer[256];
  sprintf(buffer, " with error code: %d\n", errorCode);
  OutputDebugString(message);
  OutputDebugString(buffer);
}

typedef HRESULT x_audio2_create(IXAudio2 **ppXAudio2, UINT32 Flags,
                                XAUDIO2_PROCESSOR XAudio2Processor);
global_variable x_audio2_create *XAudio2Create_;
internal void Win32LoadXAudio2(void) {
  HMODULE hModule = LoadLibrary("xaudio2_9redist.dll");
  XAudio2Create_ = (x_audio2_create *)GetProcAddress(hModule, "XAudio2Create");
}
#define XAudio2Create XAudio2Create_

global_variable const uint32_t g_AUDIO_SAMPLE_RATE = 48000;
global_variable const uint32_t g_AUDIO_CHANNEL = 2;
global_variable const uint32_t g_AUDIO_BIT_PER_SAMPLE = 16;
global_variable const uint32_t g_AUDIO_LENGTH = 2; // in seconds.
global_variable const uint32_t g_AUDIO_DATA_SIZEBY =
    g_AUDIO_SAMPLE_RATE * g_AUDIO_CHANNEL * g_AUDIO_BIT_PER_SAMPLE *
    g_AUDIO_LENGTH / 8;
global_variable BYTE pAudioData[g_AUDIO_DATA_SIZEBY] = {0xF7};

internal void Win32InitXAudio2(void) {
  Win32LoadXAudio2();
  // TODO(baturay):: if COINIT_MULTITHREADED or  COINIT_APARTMENTTHREADED make
  // more sense for my situation.
  HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  if (SUCCEEDED(hr)) {
    IXAudio2 *xaudio2;
    if (S_OK == XAudio2Create(&xaudio2, 0, XAUDIO2_DEFAULT_PROCESSOR)) {
      // IXAUDIO2 CREATED SUCCESFULLY
      IXAudio2MasteringVoice *p_masterVoice;
      hr = xaudio2->CreateMasteringVoice(&p_masterVoice, g_AUDIO_CHANNEL,
                                         g_AUDIO_SAMPLE_RATE);
      if (S_OK == hr) {
        // masterVoice created.
        XAUDIO2_BUFFER xBuffer = {};
        xBuffer.Flags = 0;
        xBuffer.AudioBytes = g_AUDIO_DATA_SIZEBY;
        xBuffer.pAudioData = pAudioData;
        xBuffer.PlayBegin = 0;
        xBuffer.PlayLength = 0;
        xBuffer.LoopBegin = 0;
        xBuffer.LoopLength = 0;
        xBuffer.LoopCount = XAUDIO2_LOOP_INFINITE;

        WAVEFORMATEX waveFormat = {};
        waveFormat.wFormatTag = WAVE_FORMAT_PCM;
        waveFormat.nChannels = g_AUDIO_CHANNEL;
        waveFormat.nSamplesPerSec = g_AUDIO_SAMPLE_RATE;
        waveFormat.nAvgBytesPerSec = g_AUDIO_DATA_SIZEBY / g_AUDIO_LENGTH;
        // TODO(baturay):: I don't know the logic behind this ? I'm doing like
        // this because  documentation say so.
        waveFormat.nBlockAlign = (g_AUDIO_CHANNEL * g_AUDIO_BIT_PER_SAMPLE) / 8;
        waveFormat.wBitsPerSample = g_AUDIO_BIT_PER_SAMPLE;
        waveFormat.cbSize = 0;

        IXAudio2SourceVoice *p_sourceVoice;
        hr = xaudio2->CreateSourceVoice(&p_sourceVoice,
                                        (WAVEFORMATEX *)&waveFormat);
        if (S_OK == hr) {
          // SOURCE VOICE CREATED.
          int16_t *samples = (int16_t *)pAudioData;
          uint32_t frames = g_AUDIO_SAMPLE_RATE * g_AUDIO_LENGTH;
          for (uint32_t i = 0; i < frames; ++i) {
            int16_t v = ((i / 48) % 2) ? 8000 : -8000; // ~500 Hz
            samples[2 * i + 0] = v;
            samples[2 * i + 1] = v;
          }
          
          p_sourceVoice->SubmitSourceBuffer(&xBuffer);
          p_sourceVoice->Start(0);
          PrintDebugString("Everything created successfully", 0);
        } else {
          PrintDebugString("source voice creation failed", hr);
        }

      } else {
        // Logger (Master voice creation failed)
        PrintDebugString("Master voice creation failed", hr);
      }
    } else {
      // Logger (xudio creation failed)
      PrintDebugString("XAudio2 creation failed", hr);
    }

  } else {
    // Logger (COM init failed.)
    PrintDebugString("Com init failed", hr);
  }
}

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
  RenderCoolGradient(g_offsetX, g_offsetY);
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
    // OutputDebugString("WM_RESIZE\n");
    break;
  }
  case WM_CREATE: {
    // OutputDebugString("WM_CREATE\n");
    break;
  }
  case WM_DESTROY: {
    g_running = false;
    // OutputDebugString("WM_DESTROY\n");
    break;
  }
  case WM_KEYDOWN:
  case WM_KEYUP:
  case WM_SYSKEYDOWN:
  case WM_SYSKEYUP: {
    uint64_t VKCode = wParam;
    // 11(repeated up), 10(released -never getting this because of the api-),
    // 01(repated down), 00(pressed)
    bool isUp = ((lParam & (1 << 31)) != 0);
    bool wasUp((lParam & (1 << 30)) != 0);
    if ((isUp == false && wasUp == false) || (isUp == false && wasUp == true)) {
      if (VKCode == 'A') {
        g_offsetX += 10;
      }
      if (VKCode == 'S') {
        g_offsetY -= 10;
      }
      if (VKCode == 'D') {
        g_offsetX -= 10;
      }
      if (VKCode == 'Q') {
      }
      if (VKCode == 'W') {
        g_offsetY += 10;
      }
      if (VKCode == 'E') {
      }
      if (VKCode == VK_ESCAPE) {
      }
      if (VKCode == VK_SPACE) {
      }
    }

    break;
  }
  case WM_CLOSE: {
    // OutputDebugString("WM_CLOSE\n");
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
    // OutputDebugString("WM_ACTIVATEAPP\n");
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
  Win32InitXAudio2();
  Win32LoadXInput();

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

  // Game loop
  MSG msg = {};
  g_running = true;
  while (g_running) {
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    // Input loop
    DWORD dwResult;
    for (DWORD i = 0; i < XUSER_MAX_COUNT; i++) {
      XINPUT_STATE state;
      ZeroMemory(&state, sizeof(XINPUT_STATE));

      // Simply get the state of the controller from XInput.
      dwResult = XInputGetState(i, &state);

      // WoooW !!!
      // XINPUT_VIBRATION vibration;
      // vibration.wLeftMotorSpeed = 0xFFFF;
      // vibration.wRightMotorSpeed = 0xFFF;
      // XInputSetState(i, &vibration);

      if (dwResult == ERROR_SUCCESS) {
        // Controller is connected
        DWORD stateIndex = state.dwPacketNumber;
        XINPUT_GAMEPAD *gamepad = &state.Gamepad;

        WORD isUpPressed = (gamepad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
        WORD isDownPressed = (gamepad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
        WORD isLeftPressed = (gamepad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
        WORD isRightPressed = (gamepad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
        WORD isStartPressed = (gamepad->wButtons & XINPUT_GAMEPAD_START);
        WORD isBackPressed = (gamepad->wButtons & XINPUT_GAMEPAD_BACK);
        WORD isLeftShoulderPressed =
            (gamepad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
        WORD isrightShoulderPressed =
            (gamepad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
        WORD isAPressed = (gamepad->wButtons & XINPUT_GAMEPAD_A);
        WORD isBPressed = (gamepad->wButtons & XINPUT_GAMEPAD_B);
        WORD isXPressed = (gamepad->wButtons & XINPUT_GAMEPAD_X);
        WORD isYPressed = (gamepad->wButtons & XINPUT_GAMEPAD_Y);

        BYTE leftTriggerValue = gamepad->bLeftTrigger;
        BYTE rightTriggerValue = gamepad->bRightTrigger;

        SHORT thumbLeftPosX = gamepad->sThumbLX;
        SHORT thumbLeftPosY = gamepad->sThumbLY;
        SHORT thumbRighttPosX = gamepad->sThumbRX;
        SHORT thumbRightPosY = gamepad->sThumbRY;

        if (thumbLeftPosX > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
          g_offsetX++;
        } else if (thumbLeftPosX < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
          g_offsetX--;
        }

        if (thumbLeftPosY > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
          g_offsetY--;
        } else if (thumbLeftPosY < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
          g_offsetY++;
        }

      } else {
        // Controller is not connected
      }
    }

    HDC dc = GetDC(windowHandle);
    RECT clientRect;
    GetClientRect(windowHandle, &clientRect);
    Win32UpdateWindow(dc, &clientRect);
    ReleaseDC(windowHandle, dc);
  }

  return 0;
}
