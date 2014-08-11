#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <Strsafe.h>
#include <stdlib.h>
#include <cassert>

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
