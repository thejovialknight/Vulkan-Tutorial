#include "vulkan.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Vulkan init_vulkan(HINSTANCE hinst, HWND hwnd) {
	if (ENABLE_VALIDATION_LAYERS) {
		enable_validation_layers();
	}

	Vulkan vulkan;
	vulkan.instance = create_instance();
	vulkan.surface = create_surface(vulkan.instance, hwnd, hinst);
	vulkan.physical_device = create_physical_device(vulkan.instance, vulkan.surface);
	vulkan.device = create_logical_device(vulkan.physical_device, vulkan.surface, vulkan.graphics_queue, vulkan.present_queue);
	vulkan.swap_chain = create_swap_chain(vulkan.physical_device, vulkan.surface, vulkan.device, IVec2{WIN_WIDTH, WIN_HEIGHT}, vulkan.swap_chain_images, vulkan.swap_chain_format, vulkan.swap_chain_extent);
	vulkan.swap_chain_image_views = create_swap_chain_image_views(vulkan.swap_chain_images, vulkan.swap_chain_format, vulkan.device);
	vulkan.render_pass = create_render_pass(vulkan.swap_chain_format, vulkan.device, vulkan.physical_device);
	vulkan.descriptor_set_layout = create_descriptor_set_layout(vulkan.device);
	vulkan.graphics_pipeline = create_graphics_pipeline(vulkan.device, vulkan.swap_chain_extent, vulkan.render_pass, vulkan.pipeline_layout, vulkan.descriptor_set_layout);
	vulkan.command_pool = create_command_pool(vulkan.physical_device, vulkan.surface, vulkan.device);
	create_depth_resources(vulkan.device, vulkan.physical_device, vulkan.swap_chain_extent, vulkan.depth_image, vulkan.depth_image_memory, vulkan.depth_image_view);
	vulkan.swap_chain_framebuffers = create_framebuffers(vulkan.swap_chain_image_views, vulkan.depth_image_view, vulkan.render_pass, vulkan.swap_chain_extent, vulkan.device);
	create_texture_image(vulkan.device, vulkan.physical_device, vulkan.texture_image, vulkan.texture_image_memory, vulkan.command_pool, vulkan.graphics_queue);
	vulkan.texture_image_view = create_texture_image_view(vulkan.device, vulkan.texture_image);
	vulkan.texture_sampler = create_texture_sampler(vulkan.device, vulkan.physical_device);
	vulkan.vertex_buffer = create_vertex_buffer(vulkan.device, vulkan.physical_device, vulkan.vertex_buffer_memory, vulkan.command_pool, vulkan.graphics_queue);
	vulkan.index_buffer = create_index_buffer(vulkan.device, vulkan.physical_device, vulkan.command_pool, vulkan.graphics_queue, vulkan.index_buffer_memory);
	create_uniform_buffers(vulkan.device, vulkan.physical_device, vulkan.uniform_buffers, vulkan.uniform_buffers_memory, vulkan.uniform_buffers_mapped);
	vulkan.descriptor_pool = create_descriptor_pool(vulkan.device);
	vulkan.descriptor_sets = create_descriptor_sets(vulkan.descriptor_set_layout, vulkan.descriptor_pool, vulkan.device, vulkan.uniform_buffers, vulkan.texture_image_view, vulkan.texture_sampler);
	vulkan.command_buffers = create_command_buffers(vulkan.command_pool, vulkan.device);
	create_sync_objects(vulkan.device, vulkan.image_available_semaphores, vulkan.render_finished_semaphores, vulkan.in_flight_fences);

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

	VkApplicationInfo app_info{};
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

	if (ENABLE_VALIDATION_LAYERS) {
		instance_create_info.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
		instance_create_info.ppEnabledLayerNames = VALIDATION_LAYERS.data();

		//populate_debug_messenger_create_info(debugCreateInfo);
		//instance_create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else {
		instance_create_info.enabledLayerCount = 0;
		instance_create_info.pNext = nullptr;
	}

	// struct w/ creation info, custom alloc. callbacks, pointer to handle to new object
	VkInstance instance;
	if (vkCreateInstance(&instance_create_info, nullptr, &instance) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create VkInstance!");
	}

	return instance;
}

void enable_validation_layers() {
	uint32_t layer_count;
	vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
	std::vector<VkLayerProperties> available_layers(layer_count);
	vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

	for (const char* layer_name : VALIDATION_LAYERS) {
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

	VkPhysicalDeviceFeatures device_features{};
	device_features.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
	create_info.pQueueCreateInfos = queue_create_infos.data();
	create_info.pEnabledFeatures = &device_features;
	create_info.enabledExtensionCount = static_cast<uint32_t>(DEVICE_EXTENSIONS.size());
	create_info.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();
	if (ENABLE_VALIDATION_LAYERS) {
		create_info.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
		create_info.ppEnabledLayerNames = VALIDATION_LAYERS.data();
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

VkSwapchainKHR create_swap_chain(VkPhysicalDevice physical_device, VkSurfaceKHR surface, VkDevice device, IVec2 window_size, 
std::vector<VkImage>& out_images, VkFormat& out_format, VkExtent2D& out_extent) {
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

	out_format = surface_format.format;

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
		out_extent = swap_chain_support.capabilities.currentExtent;
	}
	else {
		// TODO: needs some work. see "Drawing a triangle > Presentation > Swap Chain"
		VkExtent2D actual_extent = {
			static_cast<uint32_t>(window_size.x),
			static_cast<uint32_t>(window_size.y)
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

		out_extent = actual_extent;
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
	create_info.imageExtent = out_extent;
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
	
	VkSwapchainKHR swap_chain;
	if (vkCreateSwapchainKHR(device, &create_info, nullptr, &swap_chain) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create swap chain!");
	}

	vkGetSwapchainImagesKHR(device, swap_chain, &image_count, nullptr);
	out_images.resize(image_count);
	vkGetSwapchainImagesKHR(device, swap_chain, &image_count, out_images.data());

	return swap_chain; // maybe construct here at end instead of peppering throughout function?
}

std::vector<VkImageView> create_swap_chain_image_views(std::vector<VkImage>& images, VkFormat format, VkDevice device) {
	// TODO: This should use create_vulkan_image_view
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

	return image_views;
}

VkRenderPass create_render_pass(VkFormat swap_chain_image_format, VkDevice device, VkPhysicalDevice physical_device) {
	VkAttachmentDescription color_attachment{};
	color_attachment.format = swap_chain_image_format;
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depth_attachment{};
	depth_attachment.format = find_depth_format(physical_device);
	depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference color_attachment_ref{};
	color_attachment_ref.attachment = 0;
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depth_attachment_ref{};
	depth_attachment_ref.attachment = 1;
	depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_ref;
	subpass.pDepthStencilAttachment = &depth_attachment_ref;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 2> attachments = { color_attachment, depth_attachment };

	VkRenderPassCreateInfo render_pass_info{};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
	render_pass_info.pAttachments = attachments.data();
	render_pass_info.subpassCount = 1;
	render_pass_info.pSubpasses = &subpass;
	render_pass_info.dependencyCount = 1;
	render_pass_info.pDependencies = &dependency;

	VkRenderPass render_pass;
	if (vkCreateRenderPass(device, &render_pass_info, nullptr, &render_pass) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create render pass!");
	}


	return render_pass;
}

VkDescriptorSetLayout create_descriptor_set_layout(VkDevice device) {
	// UBO binding
	VkDescriptorSetLayoutBinding ubo_layout_binding{};
	ubo_layout_binding.binding = 0;
	ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	ubo_layout_binding.descriptorCount = 1;
	ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	ubo_layout_binding.pImmutableSamplers = nullptr; // only relevant for image sampling related descriptors

	// Sampler binding
	VkDescriptorSetLayoutBinding sampler_layout_binding{};
	sampler_layout_binding.binding = 1;
	sampler_layout_binding.descriptorCount = 1;
	sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	sampler_layout_binding.pImmutableSamplers = nullptr;
	sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = {ubo_layout_binding, sampler_layout_binding};

	VkDescriptorSetLayoutCreateInfo layout_info{};
	layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
	layout_info.pBindings = bindings.data();

	VkDescriptorSetLayout descriptor_set_layout;
	if (vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &descriptor_set_layout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor set layout!");
	}

	return descriptor_set_layout;
}

VkPipeline create_graphics_pipeline(VkDevice device, VkExtent2D swap_chain_extent, VkRenderPass render_pass, VkPipelineLayout& out_layout, VkDescriptorSetLayout descriptor_set_layout) {
	std::vector<char> vert_shader_code = read_file("vert.spv");
	std::vector<char> frag_shader_code = read_file("frag.spv");

	VkShaderModule vert_shader_module = create_shader_module(vert_shader_code, device);
	VkShaderModule frag_shader_module = create_shader_module(frag_shader_code, device);

	VkPipelineShaderStageCreateInfo vert_shader_stage_info{};
	vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vert_shader_stage_info.module = vert_shader_module;
	vert_shader_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo frag_shader_stage_info{};
	frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	frag_shader_stage_info.module = frag_shader_module;
	frag_shader_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo shader_stages[] = { vert_shader_stage_info, frag_shader_stage_info };

	VkVertexInputBindingDescription binding_description = get_vertex_binding_description();
	std::array<VkVertexInputAttributeDescription, 3> attribute_descriptions = get_vertex_attribute_descriptions();

	VkPipelineVertexInputStateCreateInfo vertex_input_info{};
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.vertexBindingDescriptionCount = 1;
	vertex_input_info.pVertexBindingDescriptions = &binding_description;
	vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
	vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();

	VkPipelineInputAssemblyStateCreateInfo input_assembly_info{};
	input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_info.primitiveRestartEnable = VK_FALSE;

	// Commented lines would be needed if we were making the viewport/scissor rects immutable rather than dynamic
	VkPipelineViewportStateCreateInfo viewport_state_info{};
	viewport_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state_info.viewportCount = 1;
	// viewport_state_info.pViewports = &viewport;
	viewport_state_info.scissorCount = 1;
	// viewport_state_info.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer_info{};
	rasterizer_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer_info.depthClampEnable = VK_FALSE;
	rasterizer_info.rasterizerDiscardEnable = VK_FALSE;
	rasterizer_info.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer_info.lineWidth = 1.0f;
	rasterizer_info.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	// All these depth bias things are irrelevant and optional when depth bias is disabled as we have done here
	rasterizer_info.depthBiasEnable = VK_FALSE;
	//rasterizer_info.depthBiasConstantFactor = 0.0f;
	//rasterizer_info.depthBiasClamp = 0.0f;
	//rasterizer_info.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisampling_info{};
	multisampling_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling_info.sampleShadingEnable = VK_FALSE;
	multisampling_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	//multisampling_info.minSampleShading = 1.0f;
	//multisampling_info.pSampleMask = nullptr;
	//multisampling_info.alphaToCoverageEnable = VK_FALSE;
	//multisampling_info.alphaToOneEnable = VK_FALSE;

	// All the blend stuff is irrelevant if blending is disabled as we've done here
	VkPipelineColorBlendAttachmentState color_blend_attachment{};
	color_blend_attachment.colorWriteMask = 
		VK_COLOR_COMPONENT_R_BIT | 
		VK_COLOR_COMPONENT_G_BIT | 
		VK_COLOR_COMPONENT_B_BIT | 
		VK_COLOR_COMPONENT_A_BIT;
	color_blend_attachment.blendEnable = VK_FALSE;
	//color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	//color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	//color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
	//color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	//color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	//color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo color_blending_info{};
	color_blending_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blending_info.logicOpEnable = VK_FALSE;
	color_blending_info.logicOp = VK_LOGIC_OP_COPY;
	color_blending_info.attachmentCount = 1;
	color_blending_info.pAttachments = &color_blend_attachment;
	color_blending_info.blendConstants[0] = 0.0f;
	color_blending_info.blendConstants[1] = 0.0f;
	color_blending_info.blendConstants[2] = 0.0f;
	color_blending_info.blendConstants[3] = 0.0f;

	// Dynamic means defined at runtime rather than baked into the immutable pipeline
	std::vector<VkDynamicState> dynamic_states = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo dynamic_state_info{};
	dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state_info.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
	dynamic_state_info.pDynamicStates = dynamic_states.data();

	// Not being used right now
	VkPipelineLayoutCreateInfo pipeline_layout_info{};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 1;
	pipeline_layout_info.pSetLayouts = &descriptor_set_layout;
	pipeline_layout_info.pushConstantRangeCount = 0;
	pipeline_layout_info.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &out_layout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create pipeline layout!");
	}

	VkPipelineDepthStencilStateCreateInfo depth_stencil{};
	depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil.depthTestEnable = VK_TRUE;
	depth_stencil.depthWriteEnable = VK_TRUE;
	depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depth_stencil.depthBoundsTestEnable = VK_FALSE;
	depth_stencil.minDepthBounds = 0.0f; // optional
	depth_stencil.maxDepthBounds = 1.0f; // optional
	depth_stencil.stencilTestEnable = VK_FALSE;
	depth_stencil.front = {}; // optional
	depth_stencil.back = {}; // optional

	VkGraphicsPipelineCreateInfo pipeline_info{};
	pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_info.stageCount = 2;
	pipeline_info.pStages = shader_stages;
	pipeline_info.pVertexInputState = &vertex_input_info;
	pipeline_info.pInputAssemblyState = &input_assembly_info;
	pipeline_info.pViewportState = &viewport_state_info;
	pipeline_info.pRasterizationState = &rasterizer_info;
	pipeline_info.pMultisampleState = &multisampling_info;
	pipeline_info.pDepthStencilState = &depth_stencil;
	pipeline_info.pColorBlendState = &color_blending_info;
	pipeline_info.pDynamicState = &dynamic_state_info;
	pipeline_info.layout = out_layout;
	pipeline_info.renderPass = render_pass;
	pipeline_info.subpass = 0;
	pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
	pipeline_info.basePipelineIndex = -1;

	VkPipeline pipeline;
	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(device, frag_shader_module, nullptr);
	vkDestroyShaderModule(device, vert_shader_module, nullptr);

	return pipeline;
}

VkShaderModule create_shader_module(const std::vector<char>& code, VkDevice device) {
	VkShaderModuleCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = code.size();
	create_info.pCode = reinterpret_cast<const uint32_t*>(code.data()); // TODO: Understand this

	VkShaderModule shader_module;
	if (vkCreateShaderModule(device, &create_info, nullptr, &shader_module) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create shader module!");
	}

	return shader_module;
}

std::vector<VkFramebuffer> create_framebuffers(std::vector<VkImageView>& swap_chain_image_views, VkImageView depth_image_view, VkRenderPass render_pass, VkExtent2D swap_chain_extent, VkDevice device) {
	std::vector<VkFramebuffer> swap_chain_framebuffers;
	swap_chain_framebuffers.resize(swap_chain_image_views.size());
	for (size_t i = 0; i < swap_chain_image_views.size(); ++i) {
		std::array<VkImageView, 2> attachments = {
			swap_chain_image_views[i],
			depth_image_view
		};

		VkFramebufferCreateInfo framebuffer_info{};
		framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_info.renderPass = render_pass;
		framebuffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebuffer_info.pAttachments = attachments.data();
		framebuffer_info.width = swap_chain_extent.width;
		framebuffer_info.height = swap_chain_extent.height;
		framebuffer_info.layers = 1;

		if (vkCreateFramebuffer(device, &framebuffer_info, nullptr, &swap_chain_framebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("Faled to create framebuffer");
		}
	}

	return swap_chain_framebuffers;
}

VkCommandPool create_command_pool(VkPhysicalDevice physical_device, VkSurfaceKHR surface, VkDevice device) {
	QueueFamilyIndices queue_family_indices = get_queue_families(physical_device, surface);

	VkCommandPoolCreateInfo pool_info{};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	pool_info.queueFamilyIndex = queue_family_indices.graphics_family.value();

	VkCommandPool command_pool;
	if (vkCreateCommandPool(device, &pool_info, nullptr, &command_pool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create command pool!");
	}

	return command_pool;
}

VkBuffer create_vertex_buffer(VkDevice device, VkPhysicalDevice physical_device, VkDeviceMemory& out_buffer_memory, VkCommandPool command_pool, VkQueue graphics_queue) {
	VkDeviceSize buffer_size = sizeof(vertices[0]) * vertices.size();

	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;
	staging_buffer = create_vulkan_buffer(device, physical_device, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		staging_buffer_memory);

	void* data;
	vkMapMemory(device, staging_buffer_memory, 0, buffer_size, 0, &data);
	memcpy(data, vertices.data(), (size_t)buffer_size);
	vkUnmapMemory(device, staging_buffer_memory);

	VkBuffer vertex_buffer = create_vulkan_buffer(device, physical_device, buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, out_buffer_memory);

	copy_vulkan_buffer(staging_buffer, vertex_buffer, buffer_size, device, command_pool, graphics_queue);
	vkDestroyBuffer(device, staging_buffer, nullptr);
	vkFreeMemory(device, staging_buffer_memory, nullptr);

	return vertex_buffer;
}

VkBuffer create_index_buffer(VkDevice device, VkPhysicalDevice physical_device, VkCommandPool command_pool, VkQueue graphics_queue, VkDeviceMemory& out_buffer_memory) {
	VkDeviceSize buffer_size = sizeof(indices[0]) * indices.size();

	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;
	staging_buffer = create_vulkan_buffer(device, physical_device, buffer_size, 
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer_memory);

	void* data;
	vkMapMemory(device, staging_buffer_memory, 0, buffer_size, 0, &data);
	memcpy(data, indices.data(), (size_t)buffer_size);
	vkUnmapMemory(device, staging_buffer_memory);

	VkBuffer index_buffer = create_vulkan_buffer(device, physical_device, buffer_size, 
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, out_buffer_memory);

	copy_vulkan_buffer(staging_buffer, index_buffer, buffer_size, device, command_pool, graphics_queue);

	vkDestroyBuffer(device, staging_buffer, nullptr);
	vkFreeMemory(device, staging_buffer_memory, nullptr);

	return index_buffer;
}

void create_uniform_buffers(VkDevice device, VkPhysicalDevice physical_device, std::vector<VkBuffer>& out_uniform_buffers, std::vector<VkDeviceMemory>& out_uniform_buffers_memory, 
std::vector<void*>& out_uniform_buffers_mapped) {
	VkDeviceSize buffer_size = sizeof(UniformBufferObject);

	out_uniform_buffers.resize(MAX_FRAMES_IN_FLIGHT);
	out_uniform_buffers_memory.resize(MAX_FRAMES_IN_FLIGHT);
	out_uniform_buffers_mapped.resize(MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		out_uniform_buffers[i] = create_vulkan_buffer(device, physical_device, buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			out_uniform_buffers_memory[i]);
		vkMapMemory(device, out_uniform_buffers_memory[i], 0, buffer_size, 0, &out_uniform_buffers_mapped[i]);
	}
}

VkDescriptorPool create_descriptor_pool(VkDevice device) {
	// 0 UBO for descriptor layout, 1 for combined image sampler layout 
	std::array<VkDescriptorPoolSize, 2> pool_sizes{};
	pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	pool_sizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	pool_sizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	VkDescriptorPoolCreateInfo pool_info{};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
	pool_info.pPoolSizes = pool_sizes.data();
	pool_info.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	VkDescriptorPool descriptor_pool;
	if (vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptor_pool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor pool!");
	}

	return descriptor_pool;
}

std::vector<VkDescriptorSet> create_descriptor_sets(VkDescriptorSetLayout descriptor_set_layout, VkDescriptorPool descriptor_pool, 
VkDevice device, std::vector<VkBuffer>& uniform_buffers, VkImageView texture_image_view, VkSampler texture_sampler) {
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptor_set_layout);

	VkDescriptorSetAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = descriptor_pool;
	alloc_info.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	alloc_info.pSetLayouts = layouts.data();

	std::vector<VkDescriptorSet> descriptor_sets;
	descriptor_sets.resize(MAX_FRAMES_IN_FLIGHT);
	if (vkAllocateDescriptorSets(device, &alloc_info, descriptor_sets.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor sets!");
	}

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		VkDescriptorBufferInfo buffer_info{};
		buffer_info.buffer = uniform_buffers[i];
		buffer_info.offset = 0;
		buffer_info.range = sizeof(UniformBufferObject);

		VkDescriptorImageInfo image_info{};
		image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		image_info.imageView = texture_image_view;
		image_info.sampler = texture_sampler;

		std::array<VkWriteDescriptorSet, 2> descriptor_writes{};
		descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[0].dstSet = descriptor_sets[i];
		descriptor_writes[0].dstBinding = 0;
		descriptor_writes[0].dstArrayElement = 0;
		descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptor_writes[0].descriptorCount = 1;
		descriptor_writes[0].pBufferInfo = &buffer_info;

		descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes[1].dstSet = descriptor_sets[i];
		descriptor_writes[1].dstBinding = 1;
		descriptor_writes[1].dstArrayElement = 0;
		descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptor_writes[1].descriptorCount = 1;
		descriptor_writes[1].pImageInfo = &image_info;

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptor_writes.size()), descriptor_writes.data(), 0, nullptr);
	}

	return descriptor_sets;
}

std::vector<VkCommandBuffer> create_command_buffers(VkCommandPool command_pool, VkDevice device) {
	std::vector<VkCommandBuffer> command_buffers;
	command_buffers.resize(MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo allocate_info{};
	allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocate_info.commandPool = command_pool;
	allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocate_info.commandBufferCount = (uint32_t)command_buffers.size();

	if (vkAllocateCommandBuffers(device, &allocate_info, command_buffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate command buffers!");
	};

	return command_buffers;
}

void create_depth_resources(VkDevice device, VkPhysicalDevice physical_device, VkExtent2D swap_chain_extent, 
VkImage& out_depth_image, VkDeviceMemory& out_depth_image_memory, VkImageView& out_depth_image_view) {
	VkFormat depth_format = find_depth_format(physical_device);
	create_vulkan_image(swap_chain_extent.width, swap_chain_extent.height, device, physical_device, depth_format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, out_depth_image, out_depth_image_memory);
	out_depth_image_view = create_vulkan_image_view(out_depth_image, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT, device);
}

void create_texture_image(VkDevice device, VkPhysicalDevice physical_device, VkImage& out_image, 
VkDeviceMemory& out_image_memory, VkCommandPool command_pool, VkQueue graphics_queue) {
	int tex_width;
	int tex_height;
	int tex_channels;
	stbi_uc* pixels = stbi_load("textures/texture.jpg", &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);
	VkDeviceSize image_size = tex_width * tex_height * 4;
	
	if(!pixels) {
		throw std::runtime_error("Failed to load texture image!");
	}

	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;
	staging_buffer = create_vulkan_buffer(device, physical_device, image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buffer_memory);

	void* data;
	vkMapMemory(device, staging_buffer_memory, 0, image_size, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(image_size));
	vkUnmapMemory(device, staging_buffer_memory);

	stbi_image_free(pixels);

	create_vulkan_image(tex_width, tex_height, device, physical_device, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, out_image, out_image_memory);

	transition_image_layout(out_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, 
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, command_pool, device, graphics_queue);
	copy_buffer_to_image(staging_buffer, out_image, static_cast<uint32_t>(tex_width), static_cast<uint32_t>(tex_height),
		command_pool, device, graphics_queue);
	transition_image_layout(out_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, command_pool, device, graphics_queue);

	vkDestroyBuffer(device, staging_buffer, nullptr);
	vkFreeMemory(device, staging_buffer_memory, nullptr);
}

// TODO: refactor image view creation also found increate_swap_chain_image_views into create_image_view function
VkImageView create_texture_image_view(VkDevice device, VkImage texture_image) {
	return create_vulkan_image_view(texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, device);
}

VkSampler create_texture_sampler(VkDevice device, VkPhysicalDevice physical_device) {
	// query in this function and pass around, or in function itself?
	// TODO: Probably query in a central function eventually. Obviously the better approach.
	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(physical_device, &properties);

	VkSamplerCreateInfo sampler_info{};
	sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_info.magFilter = VK_FILTER_LINEAR; // magnification
	sampler_info.minFilter = VK_FILTER_LINEAR; // minification
	sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.anisotropyEnable = VK_TRUE;
	sampler_info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	sampler_info.unnormalizedCoordinates = VK_FALSE;
	sampler_info.compareEnable = VK_FALSE;
	sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
	sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

	VkSampler texture_sampler;
	if(vkCreateSampler(device, &sampler_info, nullptr, &texture_sampler) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create texture sampler!");
	}

	return texture_sampler;
}
	
void create_sync_objects(VkDevice device, std::vector<VkSemaphore>& image_available_semaphores, std::vector<VkSemaphore>& render_finished_semaphores, std::vector<VkFence>& in_flight_fences) {
	image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
	render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
	in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);
	
	VkSemaphoreCreateInfo semaphore_info{};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	
	VkFenceCreateInfo fence_info{};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT; // To start in already signaled state

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		if (vkCreateSemaphore(device, &semaphore_info, nullptr, &image_available_semaphores[i]) != VK_SUCCESS ||
		vkCreateSemaphore(device, &semaphore_info, nullptr, &render_finished_semaphores[i]) != VK_SUCCESS ||
		vkCreateFence(device, &fence_info, nullptr, &in_flight_fences[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create semaphores!");
		}
	}
}

void record_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index, VkRenderPass render_pass,
std::vector<VkFramebuffer>& swap_chain_framebuffers, VkExtent2D swap_chain_extent, VkPipeline graphics_pipeline,
VkBuffer vertex_buffer, VkBuffer index_buffer, VkPipelineLayout pipeline_layout, std::vector<VkDescriptorSet>& descriptor_sets, uint32_t current_frame) {
	VkCommandBufferBeginInfo begin_info{};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = 0;
	begin_info.pInheritanceInfo = nullptr;

	if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS) {
		throw std::runtime_error("Failed to begin recording command buffer!");
	}

	std::array<VkClearValue, 2> clear_values{};
	clear_values[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
	clear_values[1].depthStencil = { 1.0, 0 };

	VkRenderPassBeginInfo render_pass_info{};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_info.renderPass = render_pass;
	render_pass_info.framebuffer = swap_chain_framebuffers[image_index]; // the correct image on the swapchain determined by image_index
	render_pass_info.renderArea.offset = { 0, 0 };
	render_pass_info.renderArea.extent = swap_chain_extent;
	render_pass_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
	render_pass_info.pClearValues = clear_values.data();

	vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

	VkBuffer vertex_buffers[] = { vertex_buffer };
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
	vkCmdBindIndexBuffer(command_buffer, index_buffer, 0, VK_INDEX_TYPE_UINT16);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swap_chain_extent.width);
	viewport.height = static_cast<float>(swap_chain_extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(command_buffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0,0 };
	scissor.extent = swap_chain_extent;
	vkCmdSetScissor(command_buffer, 0, 1, &scissor);

	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, 
		&descriptor_sets[current_frame], 0, nullptr);
	vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

	vkCmdEndRenderPass(command_buffer);

	if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
		throw std::runtime_error("Faled to record command buffer!");
	}
}

DrawFrameResult draw_frame(Vulkan& vulkan, HWND hwnd) {
	vkWaitForFences(vulkan.device, 1, &vulkan.in_flight_fences[vulkan.current_frame], VK_TRUE, UINT64_MAX);

	uint32_t image_index;
	VkResult acquire_image_result = vkAcquireNextImageKHR(vulkan.device, vulkan.swap_chain, UINT64_MAX, vulkan.image_available_semaphores[vulkan.current_frame], VK_NULL_HANDLE, &image_index);
	if (acquire_image_result == VK_ERROR_OUT_OF_DATE_KHR) {
		return DRAW_FRAME_RECREATION_REQUESTED;
	}
	else if (acquire_image_result != VK_SUCCESS && acquire_image_result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("Failed to acquire swap chain image!");
	}

	// Only reset the fence once we know we are submitting work
	vkResetFences(vulkan.device, 1, &vulkan.in_flight_fences[vulkan.current_frame]);

	vkResetCommandBuffer(vulkan.command_buffers[vulkan.current_frame], 0);
	record_command_buffer(vulkan.command_buffers[vulkan.current_frame], image_index, vulkan.render_pass, vulkan.swap_chain_framebuffers,
		vulkan.swap_chain_extent, vulkan.graphics_pipeline, vulkan.vertex_buffer, vulkan.index_buffer, vulkan.pipeline_layout, vulkan.descriptor_sets, vulkan.current_frame);

	update_uniform_buffer(vulkan.current_frame, vulkan.swap_chain_extent, vulkan.uniform_buffers_mapped);

	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore wait_semaphores[] = { vulkan.image_available_semaphores[vulkan.current_frame]};
	VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = wait_semaphores;
	submit_info.pWaitDstStageMask = wait_stages;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &vulkan.command_buffers[vulkan.current_frame];

	VkSemaphore signal_semaphores[] = { vulkan.render_finished_semaphores[vulkan.current_frame]};
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = signal_semaphores;

	if (vkQueueSubmit(vulkan.graphics_queue, 1, &submit_info, vulkan.in_flight_fences[vulkan.current_frame]) != VK_SUCCESS) {
		throw std::runtime_error("Failed to submit draw command buffer!");
	}

	VkPresentInfoKHR present_info{};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = signal_semaphores;

	VkSwapchainKHR swap_chains[] = { vulkan.swap_chain };
	present_info.swapchainCount = 1;
	present_info.pSwapchains = swap_chains;
	present_info.pImageIndices = &image_index;
	present_info.pResults = nullptr;

	vulkan.current_frame = (vulkan.current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
	VkResult present_queue_result = vkQueuePresentKHR(vulkan.present_queue, &present_info);
	if (present_queue_result == VK_ERROR_OUT_OF_DATE_KHR || present_queue_result == VK_SUBOPTIMAL_KHR || vulkan.framebuffer_resized) {
		return DRAW_FRAME_RECREATION_REQUESTED;
	} 
	else if (present_queue_result != VK_SUCCESS) {
		throw std::runtime_error("Failed to present swap chain image!");
	}

	return DRAW_FRAME_SUCCESS;
}

void update_uniform_buffer(uint32_t current_image, VkExtent2D swap_chain_extent, std::vector<void*>& uniform_buffers_mapped) {
	static std::chrono::steady_clock::time_point start_time = std::chrono::high_resolution_clock::now();

	std::chrono::steady_clock::time_point current_time = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

	UniformBufferObject ubo{};
	ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), swap_chain_extent.width / (float)swap_chain_extent.height, 0.1f, 10.0f);
	ubo.proj[1][1] *= -1;
	memcpy(uniform_buffers_mapped[current_image], &ubo, sizeof(ubo));
}

RecreateSwapChainResult recreate_swap_chain(Vulkan& vulkan, HWND hwnd) {
	IVec2 window_size = get_window_size(hwnd);
	if (window_size.x == 0 || window_size.y == 0) {
		return RECREATE_SWAP_CHAIN_WINDOW_MINIMIZED;
	}
	
	vkDeviceWaitIdle(vulkan.device);

	cleanup_swap_chain(vulkan.device, vulkan.swap_chain_framebuffers, vulkan.swap_chain_image_views, vulkan.swap_chain,
		vulkan.depth_image_view, vulkan.depth_image, vulkan.depth_image_memory);
	vulkan.swap_chain = create_swap_chain(vulkan.physical_device, vulkan.surface, vulkan.device, window_size, 
		vulkan.swap_chain_images, vulkan.swap_chain_format, vulkan.swap_chain_extent);
	vulkan.swap_chain_image_views = create_swap_chain_image_views(vulkan.swap_chain_images, vulkan.swap_chain_format, vulkan.device);
	create_depth_resources(vulkan.device, vulkan.physical_device, vulkan.swap_chain_extent, vulkan.depth_image, vulkan.depth_image_memory, vulkan.depth_image_view);
	vulkan.swap_chain_framebuffers = create_framebuffers(vulkan.swap_chain_image_views, vulkan.depth_image_view, vulkan.render_pass, vulkan.swap_chain_extent, vulkan.device);

	return RECREATE_SWAP_CHAIN_SUCCESS;
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
	std::set<std::string> required_extensions(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());

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

	// Check anisotropy support
	if(!device_features.samplerAnisotropy) {
		return 0;
	}

	return score;
} 

VkFormat find_supported_format(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VkPhysicalDevice physical_device) {
	for(VkFormat format : candidates) {
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(physical_device, format, &properties);

		if(tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features) {
			return format;
		}
		else if(tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw std::runtime_error("Failed to find supported format!");
}

VkFormat find_depth_format(VkPhysicalDevice physical_device) {
	return find_supported_format(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
		physical_device
	);
}

bool has_stencil_component(VkFormat format) {
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

static VkVertexInputBindingDescription get_vertex_binding_description() {
	VkVertexInputBindingDescription binding_description{};
	binding_description.binding = 0; // associated with binding in attribute descriptions?
	binding_description.stride = sizeof(Vertex);
	binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return binding_description;
}

static std::array<VkVertexInputAttributeDescription, 3> get_vertex_attribute_descriptions() {
	std::array<VkVertexInputAttributeDescription, 3> attribute_descriptions{};
	
	attribute_descriptions[0].binding = 0;
	attribute_descriptions[0].location = 0; // location 0 in shader
	attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // vecs and colors are annotated the same. R32G32 is a vec2 (2x32 bit)
	attribute_descriptions[0].offset = offsetof(Vertex, position); // the offset into the vertex data structure for the attribute data

	attribute_descriptions[1].binding = 0;
	attribute_descriptions[1].location = 1;
	attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attribute_descriptions[1].offset = offsetof(Vertex, color);

	attribute_descriptions[2].binding = 0;
	attribute_descriptions[2].location = 2;
	attribute_descriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
	attribute_descriptions[2].offset = offsetof(Vertex, texture_coordinates);

	return attribute_descriptions;
}

uint32_t get_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties, VkPhysicalDevice physical_device) {
	VkPhysicalDeviceMemoryProperties mem_properties;
	vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);

	for (uint32_t i = 0; i < mem_properties.memoryTypeCount; ++i) {
		if (type_filter & (1 << i) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) { // & means the intersection (binary AND)
			return i;
		}
	}

	throw std::runtime_error("Failed to find suitable memory type");
}

VkBuffer create_vulkan_buffer(VkDevice device, VkPhysicalDevice physical_device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkDeviceMemory& out_buffer_memory) {
	VkBufferCreateInfo buffer_info{};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size = size;
	buffer_info.usage = usage;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkBuffer buffer;
	if (vkCreateBuffer(device, &buffer_info, nullptr, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create buffer!");
	}

	VkMemoryRequirements mem_requirements;
	vkGetBufferMemoryRequirements(device, buffer, &mem_requirements);

	VkMemoryAllocateInfo allocate_info{};
	allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocate_info.allocationSize = mem_requirements.size;
	allocate_info.memoryTypeIndex = get_memory_type(mem_requirements.memoryTypeBits, properties, physical_device);

	// TODO: don't allocate for each buffer, because max num of simultaneous allocations is limited
	// to a physical limit, may be as low as 4096.
	// Eventually, you'll want to create a custom allocator that splits this allocation among many different
	// objects by using offset parameters.
	// VulkanMemoryAllocator repo is intended to make this easier, though I would probably want to do it myself
	if (vkAllocateMemory(device, &allocate_info, nullptr, &out_buffer_memory) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate buffer memory!");
	}

	vkBindBufferMemory(device, buffer, out_buffer_memory, 0);

	return buffer;
}

void create_vulkan_image(uint32_t width, uint32_t height, VkDevice device, VkPhysicalDevice physical_device, VkFormat format, VkImageTiling tiling, 
VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& out_image, VkDeviceMemory& out_image_memory) {
	VkImageCreateInfo image_info{};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.extent.width = width;
	image_info.extent.height = height;
	image_info.extent.depth = 1;
	image_info.mipLevels = 1;
	image_info.arrayLayers = 1;
	image_info.format = format;
	image_info.tiling = tiling;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage = usage;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_info.flags = 0;

	if (vkCreateImage(device, &image_info, nullptr, &out_image) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create image!");
	}

	VkMemoryRequirements mem_requirements;
	vkGetImageMemoryRequirements(device, out_image, &mem_requirements);

	VkMemoryAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_requirements.size;
	alloc_info.memoryTypeIndex = get_memory_type(mem_requirements.memoryTypeBits, properties, physical_device);

	if (vkAllocateMemory(device, &alloc_info, nullptr, &out_image_memory) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate image memory!");
	}

	vkBindImageMemory(device, out_image, out_image_memory, 0);
}

VkImageView create_vulkan_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, VkDevice device) {
	VkImageViewCreateInfo view_info{};
	view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_info.image = image;
	view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view_info.format = format;
	view_info.subresourceRange.aspectMask = aspect_flags;
	view_info.subresourceRange.baseMipLevel = 0;
	view_info.subresourceRange.levelCount = 1;
	view_info.subresourceRange.baseArrayLayer = 0;
	view_info.subresourceRange.layerCount = 1;

	VkImageView image_view;
	if(vkCreateImageView(device, &view_info, nullptr, &image_view) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create texture image view!");
	}

	return image_view;
}

void transition_image_layout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout,
VkCommandPool command_pool, VkDevice device, VkQueue graphics_queue) {
	VkCommandBuffer command_buffer = begin_single_time_commands(command_pool, device);

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = old_layout;
	barrier.newLayout = new_layout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	
	VkPipelineStageFlags src_stage;
	VkPipelineStageFlags dst_stage;

	if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
	new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
	new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else {
		throw std::invalid_argument("Unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
		command_buffer,
		src_stage, dst_stage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	end_single_time_commands(command_buffer, graphics_queue, device, command_pool);
} 

void copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height,
VkCommandPool command_pool, VkDevice device, VkQueue graphics_queue) {
	VkCommandBuffer command_buffer = begin_single_time_commands(command_pool, device);

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = {0, 0, 0};
	region.imageExtent = { width, height, 1 };
	
	vkCmdCopyBufferToImage(
		command_buffer,
		buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
	);

	end_single_time_commands(command_buffer, graphics_queue, device, command_pool);
}

void copy_vulkan_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size, VkDevice device, VkCommandPool command_pool, VkQueue graphics_queue) {
	VkCommandBuffer command_buffer = begin_single_time_commands(command_pool, device);

	VkBufferCopy copy_region{};
	copy_region.size = size;
	vkCmdCopyBuffer(command_buffer, src, dst, 1, &copy_region);

	end_single_time_commands(command_buffer, graphics_queue, device, command_pool);
}

VkCommandBuffer begin_single_time_commands(VkCommandPool command_pool, VkDevice device) {
	VkCommandBufferAllocateInfo allocate_info{};
	allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocate_info.commandPool = command_pool;
	allocate_info.commandBufferCount = 1;

	VkCommandBuffer command_buffer;
	vkAllocateCommandBuffers(device, &allocate_info, &command_buffer);

	VkCommandBufferBeginInfo begin_info{};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(command_buffer, &begin_info);

	return command_buffer;
}

void end_single_time_commands(VkCommandBuffer command_buffer, VkQueue graphics_queue, VkDevice device, VkCommandPool command_pool) {
	vkEndCommandBuffer(command_buffer);

	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffer;

	vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphics_queue);
	vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
}

void cleanup_swap_chain(VkDevice device, std::vector<VkFramebuffer>& framebuffers, std::vector<VkImageView>& image_views, VkSwapchainKHR swap_chain, 
VkImageView depth_image_view, VkImage depth_image, VkDeviceMemory depth_image_memory) {
	vkDestroyImageView(device, depth_image_view, nullptr);
	vkDestroyImage(device, depth_image, nullptr);
	vkFreeMemory(device, depth_image_memory, nullptr);
	
	for (VkFramebuffer framebuffer : framebuffers) {
		vkDestroyFramebuffer(device, framebuffer, nullptr);
	}

	for (VkImageView image_view : image_views) {
		vkDestroyImageView(device, image_view, nullptr);
	}

	vkDestroySwapchainKHR(device, swap_chain, nullptr);
}

void cleanup_vulkan(Vulkan& vulkan) {
	cleanup_swap_chain(vulkan.device, vulkan.swap_chain_framebuffers, vulkan.swap_chain_image_views, vulkan.swap_chain,
		vulkan.depth_image_view, vulkan.depth_image, vulkan.depth_image_memory); // TODO: swap chain stuff in its own struct to reflect the recreation dependency?

	vkDestroyBuffer(vulkan.device, vulkan.vertex_buffer, nullptr);
	vkFreeMemory(vulkan.device, vulkan.vertex_buffer_memory, nullptr);

	vkDestroyBuffer(vulkan.device, vulkan.index_buffer, nullptr);
	vkFreeMemory(vulkan.device, vulkan.index_buffer_memory, nullptr);

	vkDestroyPipeline(vulkan.device, vulkan.graphics_pipeline, nullptr);
	vkDestroyPipelineLayout(vulkan.device, vulkan.pipeline_layout, nullptr);
	vkDestroyRenderPass(vulkan.device, vulkan.render_pass, nullptr);

	vkDestroySampler(vulkan.device, vulkan.texture_sampler, nullptr);
	vkDestroyImageView(vulkan.device, vulkan.texture_image_view, nullptr);
	vkDestroyImage(vulkan.device, vulkan.texture_image, nullptr);
	vkFreeMemory(vulkan.device, vulkan.texture_image_memory, nullptr);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
		vkDestroyBuffer(vulkan.device, vulkan.uniform_buffers[i], nullptr);
		vkFreeMemory(vulkan.device, vulkan.uniform_buffers_memory[i], nullptr);
		vkDestroySemaphore(vulkan.device, vulkan.image_available_semaphores[i], nullptr);
		vkDestroySemaphore(vulkan.device, vulkan.render_finished_semaphores[i], nullptr);
		vkDestroyFence(vulkan.device, vulkan.in_flight_fences[i], nullptr);
	}
	vkDestroyDescriptorPool(vulkan.device, vulkan.descriptor_pool, nullptr);
	vkDestroyDescriptorSetLayout(vulkan.device, vulkan.descriptor_set_layout, nullptr);

	vkDestroyCommandPool(vulkan.device, vulkan.command_pool, nullptr);
	vkDestroyDevice(vulkan.device, nullptr);
	vkDestroySurfaceKHR(vulkan.instance, vulkan.surface, nullptr);
	vkDestroyInstance(vulkan.instance, nullptr);
} 
