#include "application.h"

void run_application(HWND& hwnd, HINSTANCE& hinst) {
	//const uint32_t WIN_HEIGHT = 800;
	//const uint32_t WIN_WIDTH = 600;

	/* init window
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	GLFWwindow* window = glfwCreateWindow(WIN_HEIGHT, WIN_WIDTH, "Vulkan", nullptr, nullptr);
	*/

	Vulkan vulkan = init_vulkan(hinst, hwnd);

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

	/* main loop
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
	}

	// cleanup glfw
	glfwDestroyWindow(window);
	glfwTerminate(); */

	// cleanup Vulkan
	vkDestroyDevice(vulkan.device, nullptr);
	vkDestroySurfaceKHR(vulkan.instance, vulkan.surface, nullptr);
	vkDestroyInstance(vulkan.instance, nullptr);
}