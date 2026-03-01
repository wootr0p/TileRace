// Translation unit isolata per le API Win32 che usano winuser.h/wingdi.h.
// Questi header confliggono con raylib.h (Rectangle, CloseWindow, ShowCursor,
// DrawTextEx...), quindi vengono inclusi SOLO qui, lontano da tutto il resto.
// Su piattaforme non-Windows questo file compila vuoto: la funzione è un
// inline no-op definita in WinIcon.h.

#include "WinIcon.h"

#ifdef _WIN32

// Le define NOGDI e NOUSER vengono passate globalmente come flag del compilatore,
// ma qui le rimuoviamo prima di includere windows.h per avere accesso a
// LoadIcon, SendMessage, WM_SETICON, ecc.
#  ifdef NOGDI
#    undef NOGDI
#  endif
#  ifdef NOUSER
#    undef NOUSER
#  endif
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>

void ApplyExeIconToWindow(void* hwnd_ptr)
{
    HWND  hwnd  = static_cast<HWND>(hwnd_ptr);
    HICON hicon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(1));
    if (hicon) {
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hicon);
        SendMessage(hwnd, WM_SETICON, ICON_BIG,   (LPARAM)hicon);
    }
}

#endif // _WIN32
