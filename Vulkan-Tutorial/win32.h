#pragma once
#include <windows.h>

#include "window_size.h"

struct Win32 {
	HINSTANCE hinst;
	HWND hwnd;

	Win32(HINSTANCE hinst, HWND hwnd);
};

Win32 winmain(HINSTANCE& hinst, HINSTANCE& h_prev_instance, LPSTR& lp_cmd_line, int n_cmd_show);
LRESULT CALLBACK WindowProc(HWND window_handle, UINT message, WPARAM w_param, LPARAM l_param);