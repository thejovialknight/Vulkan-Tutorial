#include "win32.h"

Win32::Win32(HINSTANCE hinst, HWND hwnd) : hinst(hinst), hwnd(hwnd) {}

// Entry
Win32 winmain(HINSTANCE& hinst, HINSTANCE& hprevinst, LPSTR& lpcmdline, int ncmdshow) {
	// Create window class
	WNDCLASSEX window_class; // information for window class
	ZeroMemory(&window_class, sizeof(WNDCLASSEX)); // clearing memory of window class for use

	// Define struct parameters
	window_class.cbSize = sizeof(WNDCLASSEX);
	window_class.style = CS_HREDRAW | CS_VREDRAW;
	window_class.lpfnWndProc = WindowProc;
	window_class.hInstance = hinst;
	window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
	window_class.hbrBackground = (HBRUSH)COLOR_WINDOW;
	window_class.lpszClassName = L"WindowClass1";

	// Register window class
	RegisterClassEx(&window_class);

	// Determine window size
	RECT window_rect = { 0, 0, win_width, win_height };
	AdjustWindowRect(&window_rect, WS_OVERLAPPEDWINDOW, FALSE);

	// Create window handle
	HWND hwnd = CreateWindowEx(
		NULL,
		L"WindowClass1",
		L"Our First Windowed Program",
		WS_OVERLAPPEDWINDOW,
		300,
		300,
		window_rect.right - window_rect.left,
		window_rect.bottom - window_rect.top,
		NULL,
		NULL,
		hinst,
		NULL
	);

	// Display window on screen
	ShowWindow(hwnd, ncmdshow);

	return Win32(hinst, hwnd);

	/* Message handling
	MSG msg = { 0 };
	while (TRUE) {
		// Message handling
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT) {
				break;
			}
		}
		else {
			// GAME CODE
		}
	}
	*/
}

LRESULT CALLBACK WindowProc(HWND window_handle, UINT message, WPARAM w_param, LPARAM l_param) {
	switch (message) {
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 0;
	}
	break;
	}

	return DefWindowProc(window_handle, message, w_param, l_param);
}
