#pragma once
#define VK_USE_PLATFORM_WIN32_KHR
#define NOMINMAX
//#define VK_KHR_WIN32_SURFACE_EXTENSION_NAME

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <limits>
#include <algorithm>
#include <vector>
#include <map>
#include <set>

#include <vulkan/vulkan.h>
#include <optional>

#include "window_size.h"

#ifdef NDEBUG
const bool enable_validation_layers = false;
#else
const bool enable_validation_layers = true;
#endif

const std::vector<const char*> validation_layers = {
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> device_extensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

struct QueueFamilyIndices {
	std::optional<uint32_t> graphics_family;
	std::optional<uint32_t> present_family;
};

struct SwapChainSupportInfo {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> present_modes;
};

struct CreateSwapChainResults {
	VkSwapchainKHR swap_chain;
	std::vector<VkImage> images;
	VkFormat format;
	VkExtent2D extent;
};

struct Vulkan {
	// I don't know which of these are needed to access after initialization, so cull as needed when time moves sufficiently along
	VkInstance instance;
	VkPhysicalDevice physical_device = VK_NULL_HANDLE;
	VkDevice device;
	VkQueue graphics_queue;
	VkSurfaceKHR surface;
	VkSwapchainKHR swap_chain;
	std::vector<VkImage> swap_chain_images;
	VkFormat swap_chain_format;
	VkExtent2D swap_chain_extent;
	std::vector<VkImageView> swap_chain_image_views;
};

Vulkan init_vulkan(HINSTANCE hinst, HWND hwnd);
VkInstance create_instance();
VkSurfaceKHR create_surface(VkInstance instance, HWND hwnd, HINSTANCE hinst);
VkPhysicalDevice create_physical_device(VkInstance instance, VkSurfaceKHR surface);
VkDevice create_logical_device(VkPhysicalDevice physical_device, VkSurfaceKHR surface, VkQueue& graphics_queue, VkQueue& present_queue);
CreateSwapChainResults create_swap_chain(VkPhysicalDevice physical_device, VkSurfaceKHR surface, VkDevice device);
std::vector<VkImageView> create_swap_chain_image_views();

QueueFamilyIndices get_queue_families(const VkPhysicalDevice device, VkSurfaceKHR surface);
bool queue_families_validated(QueueFamilyIndices indices);
SwapChainSupportInfo get_swap_chain_support(VkPhysicalDevice device, VkSurfaceKHR surface);
int rate_device_suitability(VkPhysicalDevice device, VkSurfaceKHR surface);

void cleanup_vulkan(Vulkan vulkan);