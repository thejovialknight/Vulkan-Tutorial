#include "application.h"
#include "win32.h"

#define OS WINDOWS

int WINAPI WinMain(HINSTANCE hinst, HINSTANCE hprevinst, LPSTR lpcmdline, int ncmdshow) {
#pragma region Win32

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
	RECT window_rect = { 0, 0, 500, 400 };
	AdjustWindowRect(&window_rect, WS_OVERLAPPEDWINDOW, FALSE);

	// Create window handle
	HWND hwnd = CreateWindowEx(
		NULL,
		L"WindowClass1",
		L"Vulkan Win32 Tutorial",
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

#pragma endregion

	Vulkan vulkan = init_vulkan(hinst, hwnd);

#pragma region Loop
	// Message handling
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

#pragma endregion

#pragma region Cleanup

	cleanup_vulkan(vulkan);

#pragma endregion
}