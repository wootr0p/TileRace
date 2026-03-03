#pragma once
// Sets the embedded icon (resource ID 1 from the .rc file) on the native Win32 window.
// Applies both ICON_SMALL and ICON_BIG so the taskbar and Alt+Tab show the correct icon
// after GLFW/Raylib has initialised the window.
// hwnd_ptr: value returned by Raylib's GetWindowHandle(), cast to void*.
// No-op on non-Windows platforms.
#ifdef _WIN32
void ApplyExeIconToWindow(void* hwnd_ptr);
#else
inline void ApplyExeIconToWindow(void*) {}
#endif
