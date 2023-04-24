#include "vulkan.h"

Vulkan init_vulkan(HINSTANCE hinst, HWND hwnd) {
	if (enable_validation_layers) {
		uint32_t layer_count;
		vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
		std::vector<VkLayerProperties> available_layers(layer_count);
		vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

		for (const char* layer_name : validation_layers) {
			bool layer_found = false;
			for (const VkLayerProperties& layer_properties : available_layers) {
				if (strcmp(layer_name, layer_properties.layerName) == 0) {
					layer_found = true;
					break;
				}
			}

			if (!layer_found) {
				throw std::runtime_error("Vulkan validations layers requested, but not available!");
				break;
			}
		}
	}

	Vulkan vulkan;
	vulkan.instance = create_instance();
	vulkan.surface = create_surface(vulkan.instance, hwnd, hinst);
	vulkan.physical_device = create_physical_device(vulkan.instance, vulkan.surface);
	vulkan.graphics_queue; // TODO: eh?
	VkQueue present_queue;
	vulkan.device = create_logical_device(vulkan.physical_device, vulkan.surface, vulkan.graphics_queue, present_queue);
	CreateSwapChainResults swap_chain_results = create_swap_chain(vulkan.physical_device, vulkan.surface, vulkan.device);
	vulkan.swap_chain = swap_chain_results.swap_chain;
	vulkan.swap_chain_images = swap_chain_results.images;
	vulkan.swap_chain_format = swap_chain_results.format;
	vulkan.swap_chain_extent = swap_chain_results.extent;
	return vulkan;
}

VkInstance create_instance() {
	// create instance
	uint32_t extension_count = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
	std::vector<VkExtensionProperties> extensions(extension_count);
	vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());
	std::cout << "Available Vulkan instance extensions:\n";
	for (const VkExtensionProperties& extension : extensions) {
		std::cout << '\t' << extension.extensionName << '\n';
	}

	VkApplicationInfo app_info{}; // createInfo.pApplicationInfo
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "Hello Triangle";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "No Engine";
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo instance_create_info{};
	instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_create_info.pApplicationInfo = &app_info;

	std::vector<char const*> inst_extensions;
	inst_extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	inst_extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);

	instance_create_info.enabledExtensionCount = static_cast<uint32_t>(inst_extensions.size());
	instance_create_info.ppEnabledExtensionNames = inst_extensions.size() > 0 ? &inst_extensions[0] : nullptr;

	if (enable_validation_layers) {
		instance_create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
		instance_create_info.ppEnabledLayerNames = validation_layers.data();
	}
	else {
		instance_create_info.enabledLayerCount = 0;
	}

	// struct w/ creation info, custom alloc. callbacks, pointer to handle to new object
	VkInstance instance;
	if (vkCreateInstance(&instance_create_info, nullptr, &instance) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create VkInstance!");
	}

	return instance;
}

VkSurfaceKHR create_surface(VkInstance instance, HWND hwnd, HINSTANCE hinst) {
	VkWin32SurfaceCreateInfoKHR surface_create_info{};
	surface_create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surface_create_info.hwnd = hwnd;
	surface_create_info.hinstance = hinst;

	VkSurfaceKHR surface;
	if (vkCreateWin32SurfaceKHR(instance, &surface_create_info, nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("Vulkan failed to create window surface!");
	}

	return surface;
}

VkPhysicalDevice create_physical_device(VkInstance instance, VkSurfaceKHR surface) {
	VkPhysicalDevice physical_device = VK_NULL_HANDLE;
	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
	if (device_count == 0) {
		throw std::runtime_error("Failed to find GPUs with Vulkan support!");
	}
	std::vector<VkPhysicalDevice> devices(device_count);
	vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

	// assign score to physical devices, ensuring we have a device with a score above 0
	std::multimap<int, VkPhysicalDevice> candidates;
	for (const VkPhysicalDevice& candidate : devices) {
		candidates.insert(std::make_pair(rate_device_suitability(candidate, surface), candidate));
	}

	if (candidates.rbegin()->first > 0) {
		physical_device = candidates.rbegin()->second;
	}
	else {
		throw std::runtime_error("Vulkan failed to find a suitable GPU!");
	}

	return physical_device;
}

VkDevice create_logical_device(VkPhysicalDevice physical_device, VkSurfaceKHR surface, VkQueue& graphics_queue, VkQueue& present_queue) {
	QueueFamilyIndices indices = get_queue_families(physical_device, surface); // redundant? not sure

	std::vector<VkDeviceQueueCreateInfo> queue_create_infos{};
	std::set<uint32_t> unique_queue_families = { indices.graphics_family.value(), indices.present_family.value() };

	float queue_priority = 1.0f;
	for (uint32_t queue_family : unique_queue_families) {
		VkDeviceQueueCreateInfo queue_create_info{};

		queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info.queueFamilyIndex = queue_family;
		queue_create_info.queueCount = 1;
		queue_create_info.pQueuePriorities = &queue_priority;

		queue_create_infos.push_back(queue_create_info);
	}

	VkPhysicalDeviceFeatures device_features{}; // will come back later to define features

	VkDeviceCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
	create_info.pQueueCreateInfos = queue_create_infos.data();
	create_info.pEnabledFeatures = &device_features;
	create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
	create_info.ppEnabledExtensionNames = device_extensions.data();
	if (enable_validation_layers) {
		create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
		create_info.ppEnabledLayerNames = validation_layers.data();
	}
	else {
		create_info.enabledLayerCount = 0;
	}

	VkDevice device;
	if (vkCreateDevice(physical_device, &create_info, nullptr, &device) != VK_SUCCESS) {
		throw std::runtime_error("Vulkan failed to create logical device!");
	}

	vkGetDeviceQueue(device, indices.graphics_family.value(), 0, &graphics_queue);
	vkGetDeviceQueue(device, indices.present_family.value(), 0, &present_queue);

	return device;
}

CreateSwapChainResults create_swap_chain(VkPhysicalDevice physical_device, VkSurfaceKHR surface, VkDevice device) {
	CreateSwapChainResults results;

	SwapChainSupportInfo swap_chain_support = get_swap_chain_support(physical_device, surface);

	// Choose surface format
	VkSurfaceFormatKHR surface_format;
	bool found_preferred_format = false;
	for (const VkSurfaceFormatKHR& available_format : swap_chain_support.formats) {
		if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			surface_format = available_format;
			found_preferred_format = true;
			break;
		}
	}

	if (!found_preferred_format) {
		surface_format = swap_chain_support.formats[0];
	}

	results.format = surface_format.format;

	// Choose present mode
	VkPresentModeKHR present_mode;
	bool found_preferred_present_mode = false;
	for (const VkPresentModeKHR& available_present_mode : swap_chain_support.present_modes) {
		if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
			present_mode = available_present_mode;
			found_preferred_present_mode = true;
		}
	}

	if (!found_preferred_present_mode) {
		present_mode = VK_PRESENT_MODE_FIFO_KHR;
	}

	// Set swap extent
	if (swap_chain_support.capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		results.extent = swap_chain_support.capabilities.currentExtent;
	}
	else {
		// TODO: needs some work. see "Drawing a triangle > Presentation > Swap Chain"
		int width = win_width; // this is our problem. might not take high dpi scaling into account?
		int height = win_height; // this too, obviously
		VkExtent2D actual_extent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actual_extent.width = std::clamp(
			actual_extent.width, 
			swap_chain_support.capabilities.minImageExtent.width, 
			swap_chain_support.capabilities.maxImageExtent.width
		);
		actual_extent.height = std::clamp(
			actual_extent.height, 
			swap_chain_support.capabilities.minImageExtent.height, 
			swap_chain_support.capabilities.maxImageExtent.height
		);

		results.extent = actual_extent;
	}

	uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;
	if (swap_chain_support.capabilities.maxImageCount > 0 && image_count > swap_chain_support.capabilities.maxImageCount) {
		image_count = swap_chain_support.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = surface;
	create_info.minImageCount = image_count;
	create_info.imageFormat = surface_format.format;
	create_info.imageColorSpace = surface_format.colorSpace;
	create_info.imageExtent = results.extent;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = get_queue_families(physical_device, surface);
	uint32_t queue_family_indices[] = { indices.graphics_family.value(), indices.present_family.value() };
	if (indices.graphics_family != indices.present_family) {
		// As I understand it, this is a less performant option that allows images to be used across multiple
		// queue families without explicit ownership transfers.
		// Ownership is a more advanced concept that would be better presumably to implement down the line.
		create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		create_info.queueFamilyIndexCount = 2;
		create_info.pQueueFamilyIndices = queue_family_indices;
	}
	else {
		create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		create_info.queueFamilyIndexCount = 0;
		create_info.pQueueFamilyIndices = nullptr;
	}
	
	create_info.preTransform = swap_chain_support.capabilities.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = present_mode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = VK_NULL_HANDLE;
	
	if (vkCreateSwapchainKHR(device, &create_info, nullptr, &results.swap_chain) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create swap chain!");
	}

	vkGetSwapchainImagesKHR(device, results.swap_chain, &image_count, nullptr);
	results.images.resize(image_count);
	vkGetSwapchainImagesKHR(device, results.swap_chain, &image_count, results.images.data());

	return results; // maybe construct here at end instead of peppering throughout function?
}

std::vector<VkImageView> create_swap_chain_image_views(std::vector<VkImage> images, VkFormat format, VkDevice device) {
	std::vector<VkImageView> image_views;
	image_views.resize(images.size());
	for (size_t i = 0; i < images.size(); ++i) {
		VkImageViewCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		create_info.image = images[i];
		create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		create_info.format = format;
		create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		create_info.subresourceRange.baseMipLevel = 0;
		create_info.subresourceRange.levelCount = 1;
		create_info.subresourceRange.baseArrayLayer = 0;
		create_info.subresourceRange.layerCount = 1;

		if (vkCreateImageView(device, &create_info, nullptr, &image_views[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create image views!");
		}
	}
}

QueueFamilyIndices get_queue_families(const VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
	QueueFamilyIndices indices;

	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);

	std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());

	int i = 0;
	for (const VkQueueFamilyProperties& queue_family : queue_families) {
		if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphics_family = i;
		}

		VkBool32 present_support = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present_support);

		if (present_support) {
			indices.present_family = i;
		}

		if (queue_families_validated(indices)) {
			break;
		}

		i++;
	}

	return indices;
}

bool queue_families_validated(QueueFamilyIndices indices) {
	return indices.graphics_family.has_value()
		&& indices.present_family.has_value();
}

SwapChainSupportInfo get_swap_chain_support(VkPhysicalDevice device, VkSurfaceKHR surface) {
	SwapChainSupportInfo info;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &info.capabilities);

	uint32_t format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);
	if (format_count != 0) {
		info.formats.resize(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, info.formats.data());
	}

	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);
	if (present_mode_count != 0) {
		info.present_modes.resize(present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, info.present_modes.data());
	}

	return info;
}

int rate_device_suitability(VkPhysicalDevice device, VkSurfaceKHR surface) {
	int score = 0;

	VkPhysicalDeviceProperties device_properties;
	vkGetPhysicalDeviceProperties(device, &device_properties);

	if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
		score += 1000;
	}

	score += device_properties.limits.maxImageDimension2D;

	VkPhysicalDeviceFeatures device_features;
	vkGetPhysicalDeviceFeatures(device, &device_features);

	if (!device_features.geometryShader) {
		return 0;
	}

	// Check for required queue familes
	QueueFamilyIndices indices = get_queue_families(device, surface); // see create_logical_device for [redundant?] note

	if (!queue_families_validated(indices)) {
		return 0;
	}

	// Check for required extensions
	uint32_t extension_count;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
	std::vector<VkExtensionProperties> available_extensions(extension_count);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());
	std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());

	for (const VkExtensionProperties& extension : available_extensions) {
		required_extensions.erase(extension.extensionName);
	}

	if (!required_extensions.empty()) {
		return 0;
	}

	// Check swap chain support
	SwapChainSupportInfo swap_chain_support = get_swap_chain_support(device, surface);

	bool swap_chain_adequate = !swap_chain_support.formats.empty() && !swap_chain_support.present_modes.empty();
	if (!swap_chain_adequate) {
		return 0;
	}

	return score;
}

void cleanup_vulkan(Vulkan vulkan) {
	vkDestroySwapchainKHR(vulkan.device, vulkan.swap_chain, nullptr);

	for (VkImageView image_view : vulkan.swap_chain_image_views) {
		vkDestroyImageView(vulkan.device, image_view, nullptr);
	}

	vkDestroyDevice(vulkan.device, nullptr);
	vkDestroySurfaceKHR(vulkan.instance, vulkan.surface, nullptr);
	vkDestroyInstance(vulkan.instance, nullptr);
}