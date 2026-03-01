#pragma once

// Imposta l'icona embedded (ID 1 dal .rc) sulla finestra Win32 nativa,
// sia come ICON_SMALL che ICON_BIG, in modo che la taskbar e l'alt+tab
// mostrino l'icona corretta anche dopo l'inizializzazione di GLFW/Raylib.
//
// hwnd_ptr: valore restituito da GetWindowHandle() di Raylib (castato a void*).
// Su piattaforme non-Windows la funzione è un no-op inline.

#ifdef _WIN32
void ApplyExeIconToWindow(void* hwnd_ptr);
#else
inline void ApplyExeIconToWindow(void*) {}
#endif
