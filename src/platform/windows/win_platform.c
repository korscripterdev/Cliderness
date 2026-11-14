/*
 * Cliderness - platform/windows/win_platform.c
 * Win32 console adapter. Uses a dedicated console screen buffer and
 * WriteConsoleOutputW, whose 16-colour attributes use the same bit layout as
 * VGA text mode, so the logical palette maps 1:1. CP437 glyphs are written as
 * their Unicode equivalents. Right Shift and bare Ctrl taps are detected from
 * key-up events (TERMCAP_RSHIFT | TERMCAP_BARE_MODS).
 */
#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>
#include <direct.h>
#include "platform/platform.h"
#include "renderer/cp437.h"
#include "core/str.h"

static HANDLE g_in = INVALID_HANDLE_VALUE;
static HANDLE g_out_orig = INVALID_HANDLE_VALUE;
static HANDLE g_out = INVALID_HANDLE_VALUE;
static DWORD g_in_mode = 0;
static int g_inited = 0;
static UINT g_old_cp = 0;

/* modifier tap tracking */
static int g_rshift_down = 0, g_lshift_down = 0, g_ctrl_down = 0, g_other = 0;
static long g_pending = CK_NONE;

static void win_get_size(int *w, int *h)
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    HANDLE o = (g_out != INVALID_HANDLE_VALUE) ? g_out : GetStdHandle(STD_OUTPUT_HANDLE);
    if (GetConsoleScreenBufferInfo(o, &info)) {
        *w = info.srWindow.Right - info.srWindow.Left + 1;
        *h = info.srWindow.Bottom - info.srWindow.Top + 1;
    } else {
        *w = 80;
        *h = 25;
    }
}

static int win_init(void)
{
    CONSOLE_CURSOR_INFO ci;
    CONSOLE_SCREEN_BUFFER_INFO info;
    COORD size;
    g_in = GetStdHandle(STD_INPUT_HANDLE);
    g_out_orig = GetStdHandle(STD_OUTPUT_HANDLE);
    if (g_in == INVALID_HANDLE_VALUE || g_out_orig == INVALID_HANDLE_VALUE) {
        return 0;
    }
    if (!GetConsoleMode(g_in, &g_in_mode)) {
        return 0;
    }
    g_old_cp = GetConsoleOutputCP();
    g_out = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL,
                                      CONSOLE_TEXTMODE_BUFFER, NULL);
    if (g_out == INVALID_HANDLE_VALUE) {
        return 0;
    }
    if (GetConsoleScreenBufferInfo(g_out_orig, &info)) {
        size.X = (SHORT)(info.srWindow.Right - info.srWindow.Left + 1);
        size.Y = (SHORT)(info.srWindow.Bottom - info.srWindow.Top + 1);
        if (size.X < FB_W) {
            size.X = FB_W;
        }
        if (size.Y < FB_H) {
            size.Y = FB_H;
        }
        SetConsoleScreenBufferSize(g_out, size);
    }
    SetConsoleActiveScreenBuffer(g_out);
    ci.dwSize = 1;
    ci.bVisible = FALSE;
    SetConsoleCursorInfo(g_out, &ci);
    SetConsoleMode(g_in, ENABLE_WINDOW_INPUT | ENABLE_EXTENDED_FLAGS);
    SetConsoleTitleA("Cliderness - by korscripterdev");
    g_inited = 1;
    return 1;
}

static void win_shutdown(void)
{
    if (!g_inited) {
        return;
    }
    SetConsoleMode(g_in, g_in_mode);
    SetConsoleActiveScreenBuffer(g_out_orig);
    CloseHandle(g_out);
    g_out = INVALID_HANDLE_VALUE;
    if (g_old_cp) {
        SetConsoleOutputCP(g_old_cp);
    }
    g_inited = 0;
}

static void win_present(const Framebuffer *back, Framebuffer *front, int force)
{
    static CHAR_INFO row[FB_W];
    int y;
    for (y = 0; y < FB_H; ++y) {
        int x0 = -1, x1 = -1, x;
        const FbCell *b = &back->cells[y * FB_W];
        FbCell *f = &front->cells[y * FB_W];
        for (x = 0; x < FB_W; ++x) {
            if (force || b[x].ch != f[x].ch || b[x].attr != f[x].attr || b[x].flags != f[x].flags) {
                if (x0 < 0) {
                    x0 = x;
                }
                x1 = x;
            }
        }
        if (x0 < 0) {
            continue;
        }
        /* widen to full wide-char pairs */
        if (x0 > 0 && (b[x0].flags & FBC_CONT)) {
            --x0;
        }
        if (x1 + 1 < FB_W && (b[x1 + 1].flags & FBC_CONT)) {
            ++x1;
        }
        for (x = x0; x <= x1; ++x) {
            CHAR_INFO *ci = &row[x - x0];
            WORD attr = (WORD)b[x].attr;
            if (b[x].flags & FBC_CONT) {
                ci->Char.UnicodeChar = (WCHAR)(x > 0 ? b[x - 1].ch : ' ');
                attr |= COMMON_LVB_TRAILING_BYTE;
            } else if (b[x].flags & FBC_UNI) {
                ci->Char.UnicodeChar = (WCHAR)b[x].ch;
                if (x + 1 < FB_W && (b[x + 1].flags & FBC_CONT)) {
                    attr |= COMMON_LVB_LEADING_BYTE;
                }
            } else {
                ci->Char.UnicodeChar = (WCHAR)cp437_to_unicode(b[x].ch);
            }
            ci->Attributes = attr;
        }
        {
            COORD bsize, bpos;
            SMALL_RECT rect;
            bsize.X = (SHORT)(x1 - x0 + 1);
            bsize.Y = 1;
            bpos.X = 0;
            bpos.Y = 0;
            rect.Left = (SHORT)x0;
            rect.Right = (SHORT)x1;
            rect.Top = (SHORT)y;
            rect.Bottom = (SHORT)y;
            WriteConsoleOutputW(g_out, row, bsize, bpos, &rect);
        }
        memcpy(&f[x0], &b[x0], sizeof(FbCell) * (size_t)(x1 - x0 + 1));
    }
}

static cl_u32 win_caps(void)
{
    return TERMCAP_UNICODE | TERMCAP_RSHIFT | TERMCAP_BARE_MODS | TERMCAP_COLOR16;
}

static long map_vk(WORD vk)
{
    switch (vk) {
    case VK_UP: return CK_UP;
    case VK_DOWN: return CK_DOWN;
    case VK_LEFT: return CK_LEFT;
    case VK_RIGHT: return CK_RIGHT;
    case VK_HOME: return CK_HOME;
    case VK_END: return CK_END;
    case VK_PRIOR: return CK_PGUP;
    case VK_NEXT: return CK_PGDN;
    case VK_INSERT: return CK_INSERT;
    case VK_DELETE: return CK_DELETE;
    default: break;
    }
    if (vk >= VK_F1 && vk <= VK_F12) {
        return CK_F1 + (long)(vk - VK_F1);
    }
    return CK_NONE;
}

static int win_poll(ClideKeyEvent *ev)
{
    INPUT_RECORD rec;
    DWORD n = 0;
    if (g_pending != CK_NONE) {
        ev->key = g_pending;
        ev->mods = 0;
        g_pending = CK_NONE;
        return 1;
    }
    for (;;) {
        if (!GetNumberOfConsoleInputEvents(g_in, &n) || n == 0) {
            return 0;
        }
        if (!ReadConsoleInputW(g_in, &rec, 1, &n) || n == 0) {
            return 0;
        }
        if (rec.EventType == WINDOW_BUFFER_SIZE_EVENT) {
            ev->key = CK_RESIZE;
            ev->mods = 0;
            return 1;
        }
        if (rec.EventType != KEY_EVENT) {
            continue;
        }
        {
            KEY_EVENT_RECORD *k = &rec.Event.KeyEvent;
            DWORD st = k->dwControlKeyState;
            WORD vk = k->wVirtualKeyCode;
            int mods = 0;
            long sp;
            if (st & SHIFT_PRESSED) {
                mods |= KM_SHIFT;
            }
            if (st & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) {
                mods |= KM_CTRL;
            }
            if (st & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) {
                mods |= KM_ALT;
            }
            if (vk == VK_SHIFT) {
                int right = (k->wVirtualScanCode == 0x36);
                if (k->bKeyDown) {
                    if (right && !g_rshift_down) {
                        g_rshift_down = 1;
                        g_other = 0;
                    } else if (!right && !g_lshift_down) {
                        g_lshift_down = 1;
                        g_other = 0;
                    }
                    continue;
                }
                if (right && g_rshift_down) {
                    g_rshift_down = 0;
                    if (!g_other) {
                        ev->key = CK_RSHIFT;
                        ev->mods = 0;
                        return 1;
                    }
                } else if (!right && g_lshift_down) {
                    g_lshift_down = 0;
                    if (!g_other) {
                        ev->key = CK_LSHIFT;
                        ev->mods = 0;
                        return 1;
                    }
                }
                continue;
            }
            if (vk == VK_CONTROL) {
                if (k->bKeyDown) {
                    if (!g_ctrl_down) {
                        g_ctrl_down = 1;
                        g_other = 0;
                    }
                    continue;
                }
                if (g_ctrl_down) {
                    g_ctrl_down = 0;
                    if (!g_other) {
                        ev->key = CK_CTRL;
                        ev->mods = 0;
                        return 1;
                    }
                }
                continue;
            }
            if (!k->bKeyDown) {
                continue;
            }
            if (vk == VK_MENU || vk == VK_CAPITAL || vk == VK_LWIN || vk == VK_RWIN) {
                continue;
            }
            g_other = 1;
            sp = map_vk(vk);
            if (sp != CK_NONE) {
                ev->key = sp;
                ev->mods = mods;
                return 1;
            }
            if (k->uChar.UnicodeChar) {
                long c = (long)(unsigned short)k->uChar.UnicodeChar;
                if (c >= 1 && c <= 26 && c != CK_TAB && c != CK_ENTER && c != CK_BACKSPACE) {
                    if (c == 3) {
                        ev->key = CK_QUIT;
                        ev->mods = 0;
                        return 1;
                    }
                    ev->key = 'a' + c - 1;
                    ev->mods = mods | KM_CTRL;
                    return 1;
                }
                ev->key = c;
                ev->mods = mods;
                return 1;
            }
        }
    }
}

static cl_u32 win_ms(void)
{
    return (cl_u32)GetTickCount();
}

static void win_sleep(cl_u32 ms)
{
    Sleep((DWORD)ms);
}

static cl_u32 win_seed(void)
{
    LARGE_INTEGER pc;
    QueryPerformanceCounter(&pc);
    return CL_U32((cl_u32)pc.LowPart ^ (cl_u32)GetTickCount() ^ (cl_u32)GetCurrentProcessId());
}

static int win_mkdir(const char *path)
{
    if (CreateDirectoryA(path, NULL)) {
        return 0;
    }
    return GetLastError() == ERROR_ALREADY_EXISTS ? 0 : -1;
}

static int win_replace(const char *tmp, const char *dst)
{
    return MoveFileExA(tmp, dst, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) ? 0 : -1;
}

static int aud_init(void) { return 0; }
static void aud_shutdown(void) { }
static void aud_tone(int hz) { (void)hz; }
static const char *aud_name(void) { return "null (Windows PCM backend planned)"; }

static void win_detect(char *lang, int n)
{
    LANGID id = GetUserDefaultUILanguage();
    int prim = id & 0x3FF;
    int sub = id >> 10;
    const char *l = "en";
    switch (prim) {
    case 0x12: l = "ko"; break;
    case 0x11: l = "ja"; break;
    case 0x04: l = (sub == 0x02 || sub == 0x04) ? "zh-CN" : "zh-TW"; break;
    case 0x0C: l = "fr"; break;
    case 0x07: l = "de"; break;
    case 0x0A: l = "es"; break;
    case 0x16: l = "pt"; break;
    case 0x19: l = "ru"; break;
    case 0x10: l = "it"; break;
    default: break;
    }
    str_copy(lang, l, (size_t)n);
}

static int win_codepage(void)
{
    return 65001; /* WriteConsoleOutputW is always Unicode */
}

static cl_u32 win_free(void)
{
    MEMORYSTATUS ms;
    ms.dwLength = sizeof(ms);
    GlobalMemoryStatus(&ms);
    return (cl_u32)ms.dwAvailPhys;
}

static const ClidePlatform g_win = {
    "Windows console",
    { win_init, win_shutdown, win_get_size, win_present, win_caps },
    { win_poll },
    { win_ms, win_sleep, win_seed },
    { win_mkdir, win_replace, '\\' },
    { aud_init, aud_shutdown, aud_tone, aud_name },
    { win_detect, win_codepage },
    { win_free }
};

const ClidePlatform *plat_native(void)
{
    return &g_win;
}

#endif /* _WIN32 */
