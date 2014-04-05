#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <Strsafe.h>
#include <stdlib.h>

static const unsigned MS_PER_SECOND = 1000;

// http://www.psdevwiki.com/ps4/DS4-USB
enum EDs4Byte {
    DS4_BYTE_REPORT_ID = 0,
    DS4_BYTE_L_STICK_X_AXIS, // 0 = left
    DS4_BYTE_L_STICK_Y_AXIS, // 0 = up
    DS4_BYTE_R_STICK_X_AXIS,
    DS4_BYTE_R_STICK_Y_AXIS,
    DS4_BYTE_FACE_AND_POV,   // triangle | circle | x | square | 4-bit POV (1000b no pov)
                             // 0111b NW, 0110b W, 0101b SW, 0100b S, 0011b SE, 0010b E, 0001b NE, 0000b N
    DS4_BYTE_L_R_MISC_DIGITAL, // R3 | L3 | OPTIONS | SHARE | R2 | L2 | R1 | L1
    DS4_BYTE_COUNTER_ETC,    // 6-bit counter (1 per report) | T-PAD click | guide button
    DS4_BYTE_L2_ANALOG,      // 0 = released, 0xFF = fully pressed
    DS4_BYTE_R2_ANALOG,
    DS4_BYTE_BYTE_10,
    DS4_BYTE_BYTE_11,
    DS4_BYTE_BATTERY_LEVEL,
    DS4_BYTE_13,
    DS4_BYTE_14,
    DS4_BYTE_15,
    DS4_BYTE_16,
    DS4_BYTE_17,
    DS4_BYTE_18,
    DS4_BYTE_19,
    DS4_BYTE_20,
    DS4_BYTE_21,
    DS4_BYTE_22,
    DS4_BYTE_23,
    DS4_BYTE_24,
    DS4_BYTE_25,
    DS4_BYTE_26,
    DS4_BYTE_27,
    DS4_BYTE_28,
    DS4_BYTE_29,
    DS4_BYTE_30,
    DS4_BYTE_31,
    DS4_BYTE_32,
    DS4_BYTE_33, // Some touchpad data starts here
    DS4_BYTE_34,
    DS4_BYTE_35,
    DS4_BYTE_36,
    DS4_BYTE_37,
    DS4_BYTE_38,
    DS4_BYTE_39,
    DS4_BYTE_40,
    DS4_BYTE_41,
    DS4_BYTE_42,
    DS4_BYTE_43,
    DS4_BYTE_44,
    DS4_BYTE_45,
    DS4_BYTE_46,
    DS4_BYTE_47,
    DS4_BYTE_48,
    DS4_BYTE_49,
    DS4_BYTE_50,
    DS4_BYTE_51,
    // TODO : Remaining bytes
    DS4_BYTES = 64
};

struct Ds4Frame {
    BYTE rawData[64];
};

enum EGkosKeyFlags {
    GKOS_KEY_FLAG_1 = 1 << 0,
    GKOS_KEY_FLAG_2 = 1 << 1,
    GKOS_KEY_FLAG_3 = 1 << 2,
    GKOS_KEY_FLAG_4 = 1 << 3,
    GKOS_KEY_FLAG_5 = 1 << 4,
    GKOS_KEY_FLAG_6 = 1 << 5,
    GKOS_KEY_FLAG_COL_LEFT  = GKOS_KEY_FLAG_1 | GKOS_KEY_FLAG_2 | GKOS_KEY_FLAG_3,
    GKOS_KEY_FLAG_COL_RIGHT = GKOS_KEY_FLAG_4 | GKOS_KEY_FLAG_5 | GKOS_KEY_FLAG_6,
    GKOS_KEY_FLAG_ROW_TOP   = GKOS_KEY_FLAG_1 | GKOS_KEY_FLAG_4,
    GKOS_KEY_FLAG_ROW_MID   = GKOS_KEY_FLAG_2 | GKOS_KEY_FLAG_5,
    GKOS_KEY_FLAG_ROW_BOT   = GKOS_KEY_FLAG_3 | GKOS_KEY_FLAG_6,
};

struct GkosChord {
    const TCHAR *  str;
    unsigned short vkey;
};

enum EGkosChordFlags {
    GKOS_CHORD_FLAG_NONE       = 0,
    GKOS_CHORD_FLAG_SHIFT      = 1 << 0,
    GKOS_CHORD_FLAG_SYMB       = 1 << 1,
    GKOS_CHORD_FLAG_SHIFT_LOCK = 1 << 2,
    GKOS_CHORD_FLAG_SYMB_LOCK  = 1 << 3,
    //GKOS_CHORD_FLAG_ = 1 << 4,
    //GKOS_CHORD_FLAG_ = 1 << 5,
    //GKOS_CHORD_FLAG_ = 1 << 6,
    //GKOS_CHORD_FLAG_ = 1 << 7,
    GKOS_CHORD_FLAGS_MASK = 0x0F
};

struct GkosChordFrame {
    BYTE     chordCode;
    BYTE     flags;
};

// Timing
static const unsigned s_ds4FrameDelay         = 4; // DS4 frame rate
static const unsigned s_chordMinFrameCount    = 120 / s_ds4FrameDelay; // ms / ms-per-frame

// Input frames
static const unsigned s_inputBufferCount      = (MS_PER_SECOND * 3) / s_ds4FrameDelay;
static unsigned       s_inputBufferIndex      = 0;
static Ds4Frame       s_ds4FrameBuffer[s_inputBufferCount];
static GkosChordFrame s_gkosFrameBuffer[s_inputBufferCount];

// Higher-level key events
//static const unsigned s_eventBufferCount      = 64; // Arbitrary number
//static unsigned       s_eventBufferIndex      = 0;


GkosChord gkosKeysAbc[] = {
    { NULL, 0 }, // Meaningless no-key placeholder
    { L"a", 0 }, // 1
    { L"b", 0 }, // 2
    { L"o", 0 },
    { L"c", 0 }, // 4
    { L"th", 0 }, // Extra 'th' key
    { L"s", 0 },
    { NULL, VK_BACK },
    { L"t", 0 }, // 8
    { NULL, VK_UP },
    { L"'", 0 },
    { L"p", 0 },
    { L"!", 0 },
    { L"that ", 0 }, // Extra 'th' key combo with key 4
    { L"d", 0 },
    { NULL, VK_LEFT },
    { L"e", 0 }, // 16
    { L"-", 0 },
    { NULL, VK_LSHIFT }, // TODO : Double-hitting shift enters CapsLock (and symbol lock?)
    { L"q", 0 },
    { L",", 0 },
    { L"the ", 0 }, // Extra 'th' key combo with key 5
    { L"u", 0 },
    { NULL, 0 }, // <  ?  (Word Left)
    { L"i", 0 },
    { L"h", 0 },
    { L"g", 0 },
    { NULL, VK_PRIOR }, // PageUp
    { L"j", 0 },
    { L"to ", 0 },
    { L"/", 0 },
    { NULL, VK_ESCAPE },
    { L"r", 0 }, // 32
    { L"?", 0 },
    { L".", 0 },
    { L"f", 0 },
    { NULL, VK_DOWN },
    { L"of ", 0 }, // Extra 'th' key combo with key 6
    { L"v", 0 },
    { NULL, VK_HOME },
    { L"w", 0 },
    { L"x", 0 },
    { L"y", 0 },
    { NULL, VK_INSERT },
    { L"z", 0 },
    { NULL, 0 }, // TODO : SYMB (when shifted?).  Android keyboard does SYMB anyway -- maybe one instead of lock?
    { L"wh", 0 },
    { NULL, VK_LCONTROL },
    { L"n", 0 },
    { L"l", 0 },
    { L"m", 0 },
    { L"\\", 0 },
    { L"k", 0 },
    { L"and ", 0 }, // Extra 'th' key combo with keys 5 and 6
    { NULL, VK_NEXT }, // PageDown
    { NULL, VK_LMENU }, // Alt
    { NULL, VK_SPACE },
    { NULL, VK_RIGHT },
    { NULL, 0 }, // >  ?  (Word Right)
    { NULL, VK_RETURN },
    { NULL, VK_END },
    { NULL, VK_TAB },
    { NULL, VK_DELETE },
    { NULL, 0 }, // TODO : ABC-123 toggle
};
GkosChord gkosKeysSymb[] = {
    { NULL, 0 }, // Meaningless no-key placeholder
    { L"1", 0 }, // 1
    { L"2", 0 }, // 2
    { L"+", 0 },
    { L"3", 0 }, // 4
    { L")", 0 },
    { L"*", 0 },
    { NULL, 0 },
    { L"4", 0 }, // 8
    { NULL, 0 },
    { L"\"", 0 },
    { L"%", 0 },
    { L"|", 0 },
    { L"]", 0 },
    { L"$", 0 },
    { NULL, 0 },
    { L"5", 0 }, // 16
    { L"_", 0 },
    { NULL, 0 }, // TODO : Double-hitting shift enters CapsLock (and symbol lock?)
    { L"=", 0 },
    { L";", 0 },
    { L">", 0 },
    { NULL, 0 }, // Euros
    { NULL, 0 }, // <  ?
    { L"0", 0 },
    { L"7", 0 },
    { L"8", 0 },
    { NULL, 0 }, // PageUp
    { L"9", 0 },
    { NULL, 0 }, // Funky 'ins' symbol? 011101b
    { L"´", 0 },
    { NULL, 0 },
    { L"6", 0 }, // 32
    { L"~", 0 },
    { L":", 0 },
    { L"^", 0 },
    { NULL, 0 }, // Down arrow
    { L"}", 0 },
    { NULL, 0 }, // (British pounds currency symbol)
    { NULL, 0 },
    { L"(", 0 },
    { L"[", 0 },
    { L"<", 0 },
    { NULL, 0 }, // Insert
    { L"{", 0 },
    { NULL, 0 }, // TODO : SYMB (when shifted?).  Android keyboard does SYMB anyway -- maybe one instead of lock?
    { NULL, 0 }, // Section symbol
    { NULL, 0 }, // Control
    { L"#", 0 },
    { L"@", 0 },
    { NULL, 0 }, // 1/2 symbol
    { L"`", 0 }, // Backtick
    { L"&", 0 },
    { NULL, 0 }, // Extra 'th' key combo with keys 5 and 6 (elipses/and/_ould)
    { NULL, 0 }, // PageDown
    { NULL, 0 }, // Alt
    { NULL, 0 }, // Space
    { NULL, 0 }, // Right arrow
    { NULL, 0 }, // >  ?  (Next Word)
    { NULL, 0 }, // Enter
    { NULL, 0 }, // End
    { NULL, 0 }, // Tab
    { NULL, 0 }, // Delete
    { NULL, 0 }, // TODO : ABC-123 toggle
};

//=============================================================================
static unsigned GetRelativeInputBufferIndex (unsigned framesAgo) {
    return ((s_inputBufferIndex + s_inputBufferCount) - framesAgo) % s_inputBufferCount;
}

//=============================================================================
static const GkosChordFrame & GetRelativeGkosChordFrame (unsigned framesAgo) {
    return s_gkosFrameBuffer[GetRelativeInputBufferIndex(framesAgo)];
}

//=============================================================================
void ListDevices () {

    UINT                numDevices;
    PRAWINPUTDEVICELIST pRawInputDeviceList;
    if (GetRawInputDeviceList(NULL, &numDevices, sizeof(*pRawInputDeviceList)) != 0) {
        return;
    }
    
    if ((pRawInputDeviceList = (PRAWINPUTDEVICELIST)malloc(sizeof(*pRawInputDeviceList) * numDevices)) == NULL) {
        return;
    }
    
    if (GetRawInputDeviceList(pRawInputDeviceList, &numDevices, sizeof(RAWINPUTDEVICELIST)) == UINT(-1)) {
        free(pRawInputDeviceList);
        return;
    }
    
    for (unsigned i = 0; i < numDevices; ++i) {
        TCHAR           buf1[512];
        UINT            buf1CharCount = sizeof(buf1)/sizeof(buf1[0]);
        RID_DEVICE_INFO devInfo;
        UINT            devInfoSize = sizeof(devInfo);
        memset(&devInfo, 0, sizeof(devInfo));
        UINT f = GetRawInputDeviceInfo(
            pRawInputDeviceList[i].hDevice, 
            RIDI_DEVICEINFO,
            &devInfo,
            &devInfoSize
        );
        
        TCHAR buf2[512];
        swprintf_s(
            buf2,
            L"HID device: vendor=0x%X product=0x%X\n",
            devInfo.hid.dwVendorId,
            devInfo.hid.dwProductId
        );
        OutputDebugString(buf2);
    }
    
    free(pRawInputDeviceList);

}

//============================================================================
void ReadDs4RawInput (
    unsigned     rawDataCount,
    unsigned     rawDataBytesEach,
    const BYTE * rawDataArray
) {

    s_inputBufferIndex = (s_inputBufferIndex + 1) % s_inputBufferCount;

    Ds4Frame & ds4Frame = s_ds4FrameBuffer[s_inputBufferIndex];
    memcpy(&ds4Frame, rawDataArray, rawDataBytesEach * rawDataCount);
    
    unsigned povValue = rawDataArray[DS4_BYTE_FACE_AND_POV] & 0x0F;
    unsigned gkosChord = 0x0;
    if (rawDataArray[DS4_BYTE_L_R_MISC_DIGITAL] & (1 << 0)) // L1
        gkosChord |= GKOS_KEY_FLAG_1;
    if ((povValue >= 1 && povValue <= 3) || rawDataArray[DS4_BYTE_L2_ANALOG] >= 0x1F) // POV East OR L2
        gkosChord |= (1 << 1);
    if (povValue >= 3 && povValue <= 5) // POV South
        gkosChord |= (1 << 2);
    if (rawDataArray[DS4_BYTE_L_R_MISC_DIGITAL] & (1 << 1)) // R1
        gkosChord |= (1 << 3);
    if ((rawDataArray[DS4_BYTE_FACE_AND_POV] & (1 << 4)) || rawDataArray[DS4_BYTE_R2_ANALOG] >= 0x1F) // Square OR R2
        gkosChord |= (1 << 4);
    if (rawDataArray[DS4_BYTE_FACE_AND_POV] & (1 << 5)) // X
        gkosChord |= (1 << 5);
        
    s_gkosFrameBuffer[s_inputBufferIndex].chordCode = gkosChord;
    s_gkosFrameBuffer[s_inputBufferIndex].flags     = 0;
    
    if (!gkosChord)
        return;

    bool chordPressed = true;
    for (unsigned i = 1; i < s_chordMinFrameCount; ++i) {
        const GkosChordFrame & gkosFrame = GetRelativeGkosChordFrame(i);
        if (gkosFrame.chordCode != gkosChord) {
            chordPressed = false;
            break;
        }
    }
    
    // Send the key if the chord was just pressed
    if (chordPressed && GetRelativeGkosChordFrame(s_chordMinFrameCount).chordCode != gkosChord) {
        const GkosChord & gkosKey = gkosKeysAbc[gkosChord];
        TCHAR buf[64];
        if (gkosKeysAbc[gkosChord].str)
            swprintf_s(buf, L"Chord 0x%02X yields Key %s\n", gkosChord, gkosKey.str);
        else
            swprintf_s(buf, L"Chord 0x%02X yields Virtual Key %X\n", gkosChord, gkosKey.vkey);
        OutputDebugString(buf);
        
        INPUT ins[32];
        UINT  insCount = 0;
        memset(ins, 0, sizeof(ins));
        for (unsigned i = 0; i < 32; ++i)
            ins[i].type = INPUT_KEYBOARD; // Can also use _MOUSE or _HARDWARE

        if (gkosKey.vkey) {
            ins[0].ki.wVk = gkosKey.vkey;
        }
        else if (gkosKey.str) {
            // Note: This doesn't handle numpad keys
            SHORT vkey = VkKeyScanEx(gkosKey.str[0], NULL);
            if (vkey == -1)
                return;
                
            ins[0].ki.wVk = vkey;
        }
        else
            return;

        SendInput(1, ins, sizeof(ins[0]));
        ins[0].ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(1, ins, sizeof(ins[0]));
    }

}

//============================================================================
LRESULT CALLBACK WndProc (
    _In_ HWND   hwnd,
    _In_ UINT   uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
) {

    PAINTSTRUCT paint_struct;
    HDC hDC;

    switch (uMsg) {
        case WM_PAINT: {
            hDC = BeginPaint(hwnd, &paint_struct);
            EndPaint(hwnd, &paint_struct);
            return 0;
        }

        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }

        case WM_CHAR: {
            if (wParam == VK_ESCAPE) {
                PostQuitMessage(0);
                return 0;
            }
            else if (wParam == 'l') {
                ListDevices();
            }
            break;
        }
        
        case WM_INPUT: {
            static const UINT s_ds4Vendor  = 0x54C;
            static const UINT s_ds4Product = 0x5C4;
            UINT              bufferSize   = 0;
            
            GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &bufferSize, sizeof(RAWINPUTHEADER));
            LPBYTE lpb = new BYTE[bufferSize];
            memset(lpb, 0, bufferSize);
            if (lpb == NULL) 
                break;

            GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &bufferSize, sizeof(RAWINPUTHEADER));
            RAWINPUT* raw = (RAWINPUT*)lpb;
            if (raw->header.dwType != RIM_TYPEHID)
                break;
                
            RID_DEVICE_INFO sRidDeviceInfo;
            UINT            sRidDeviceInfoSize = sizeof(sRidDeviceInfo);
            memset(&sRidDeviceInfo, 0, sizeof(sRidDeviceInfo));
            if (GetRawInputDeviceInfo(raw->header.hDevice, RIDI_DEVICEINFO, &sRidDeviceInfo, &sRidDeviceInfoSize) <= 0 ) {
                OutputDebugString(L"failed to get raw input's device info...\n");
                break;
            }
            // Ignore anything that's not a DualShock 4 controller
            if (sRidDeviceInfo.hid.dwVendorId != s_ds4Vendor || sRidDeviceInfo.hid.dwProductId != s_ds4Product)
                break;
                
            ReadDs4RawInput(raw->data.hid.dwCount, raw->data.hid.dwSizeHid, raw->data.hid.bRawData);

            delete[] lpb; 
            return 0;
        } 
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);

}

//============================================================================
int WINAPI wWinMain (
    HINSTANCE instance,
    HINSTANCE /*prev_instance*/,
    LPWSTR    /*command_line*/,
    int       command_show
) {

    memset(s_ds4FrameBuffer, 0, sizeof(s_ds4FrameBuffer));
    memset(s_gkosFrameBuffer, 0, sizeof(s_gkosFrameBuffer));

    WNDCLASSEX windowClass    = {0};
    windowClass.cbSize        = sizeof(windowClass);
    windowClass.style         = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc   = WndProc;
    windowClass.hInstance     = instance;
    windowClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    windowClass.lpszMenuName  = NULL;
    windowClass.lpszClassName = L"ps4gkos";
    
    if (!RegisterClassEx(&windowClass))
        return 1;

    RECT rc = {0, 0, 800, 600};
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hwnd = CreateWindowA("ps4gkos", "gkos Window", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top,
        NULL, NULL, instance, NULL
    );

    if (!hwnd)
        return 1;

    ShowWindow(hwnd, command_show);
    
    static RAWINPUTDEVICE rid[1];
    rid[0].usUsagePage = 0x01; 
    rid[0].usUsage     = 0x05; // Game pads (not joysticks) 
    rid[0].dwFlags     = RIDEV_INPUTSINK;
    rid[0].hwndTarget  = hwnd;

    if (RegisterRawInputDevices(rid, sizeof(rid)/sizeof(rid[0]), sizeof(rid[0])) == FALSE)
        return 1;

    //GameTimer timer;
    //timer.Reset();

    MSG msg = {0};
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            // Update and Draw
            //timer.Tick();
            //demo->Update(timer.DeltaTime());
            //demo->Render();
        }
    }

    return static_cast<int>(msg.wParam);

}
