// TODO: We should allocate multiple resources like buffers from a single mem allocation, but
// also, we should store multiple buffers like the vertex and index bufers into a single VkBuffer and
// use offsets in commands like vkCmdBindVertexBuffers. MORE CACHE FRIENDLY!
// Some vulkan functions have explicit flags to specify we want to do this.

// TODO: Command submission should optimally be put into a single command buffer and then executed
// asynchronously. ("Images" vulkan tutorial)

#pragma once
#define VK_USE_PLATFORM_WIN32_KHR
#define NOMINMAX

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
#include <optional>
#include <array>
#include <chrono>

#include <vulkan/vulkan.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "window_size.h"
#include "file_helpers.h"
#include "win32.h"

#ifdef NDEBUG
const bool ENABLE_VALIDATION_LAYERS = false;
#else
const bool ENABLE_VALIDATION_LAYERS = true;
#endif

const std::vector<const char*> VALIDATION_LAYERS = {
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> DEVICE_EXTENSIONS = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

const int MAX_FRAMES_IN_FLIGHT = 2;

// Possibly belongs in its own platform agnostic file
struct Vertex {
	glm::vec3 position;
	glm::vec3 color;
	glm::vec2 texture_coordinates;
};

// This is what will be passed by outside, presumptively
const std::vector<Vertex> vertices = {
	{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
	{{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
	{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
	{{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},

	{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
	{{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
	{{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
	{{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
};

const std::vector<uint16_t> indices = {
	0, 1, 2, 2, 3, 0,
	4, 5, 6, 6, 7, 4
};

struct UniformBufferObject {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
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

enum RecreateSwapChainResult {
	RECREATE_SWAP_CHAIN_SUCCESS,
	RECREATE_SWAP_CHAIN_WINDOW_MINIMIZED
};

enum DrawFrameResult {
	DRAW_FRAME_SUCCESS,
	DRAW_FRAME_RECREATION_REQUESTED
};

struct Vulkan {
	// I don't know which of these are needed to access after initialization, so cull as needed when time moves sufficiently along
	VkInstance instance;
	VkPhysicalDevice physical_device = VK_NULL_HANDLE;
	VkDevice device;
	VkQueue graphics_queue;
	VkQueue present_queue;
	VkSurfaceKHR surface;
	VkSwapchainKHR swap_chain;
	std::vector<VkImage> swap_chain_images;
	VkFormat swap_chain_format;
	VkExtent2D swap_chain_extent;
	std::vector<VkImageView> swap_chain_image_views;
	VkPipeline graphics_pipeline;
	VkRenderPass render_pass;
	VkDescriptorSetLayout descriptor_set_layout;
	VkBuffer index_buffer;
	VkDeviceMemory index_buffer_memory;
	std::vector<VkBuffer> uniform_buffers;
	std::vector<VkDeviceMemory> uniform_buffers_memory;
	std::vector<void*> uniform_buffers_mapped;
	VkPipelineLayout pipeline_layout;
	std::vector<VkFramebuffer> swap_chain_framebuffers;
	VkCommandPool command_pool;
	VkBuffer vertex_buffer;
	VkDeviceMemory vertex_buffer_memory;
	std::vector<VkCommandBuffer> command_buffers;
	VkDescriptorPool descriptor_pool;
	std::vector<VkDescriptorSet> descriptor_sets;
	VkImage texture_image;
	VkDeviceMemory texture_image_memory;
	VkImageView texture_image_view;
	VkSampler texture_sampler;
	VkImage depth_image;
	VkDeviceMemory depth_image_memory;
	VkImageView depth_image_view;
	
	std::vector<VkSemaphore> image_available_semaphores;
	std::vector<VkSemaphore> render_finished_semaphores;
	std::vector<VkFence> in_flight_fences;

	bool framebuffer_resized = false;
	uint32_t current_frame = 0;
};

Vulkan init_vulkan(HINSTANCE hinst, HWND hwnd);
void enable_validation_layers();
VkInstance create_instance();
VkSurfaceKHR create_surface(VkInstance instance, HWND hwnd, HINSTANCE hinst);
VkPhysicalDevice create_physical_device(VkInstance instance, VkSurfaceKHR surface);
VkDevice create_logical_device(VkPhysicalDevice physical_device, VkSurfaceKHR surface, VkQueue& graphics_queue, VkQueue& present_queue);
VkSwapchainKHR create_swap_chain(VkPhysicalDevice physical_device, VkSurfaceKHR surface, VkDevice device, IVec2 window_size,
	std::vector<VkImage>& out_images, VkFormat& out_format, VkExtent2D& out_extent);
std::vector<VkImageView> create_swap_chain_image_views(std::vector<VkImage>& images, VkFormat format, VkDevice device);
VkRenderPass create_render_pass(VkFormat swap_chain_image_format, VkDevice device);
VkDescriptorSetLayout create_descriptor_set_layout(VkDevice device);
VkPipeline create_graphics_pipeline(VkDevice device, VkExtent2D swap_chain_extent, VkRenderPass render_pass, VkPipelineLayout& out_layout, VkDescriptorSetLayout descriptor_set_layout);
VkShaderModule create_shader_module(const std::vector<char>& code, VkDevice device);
std::vector<VkFramebuffer> create_framebuffers(std::vector<VkImageView>& swap_chain_image_views, VkRenderPass render_pass, VkExtent2D swap_chain_extent, VkDevice device);
VkCommandPool create_command_pool(VkPhysicalDevice physical_device, VkSurfaceKHR surface, VkDevice device);
VkBuffer create_vertex_buffer(VkDevice device, VkPhysicalDevice physical_device, VkDeviceMemory& out_buffer_memory, VkCommandPool command_pool, VkQueue graphics_queue);
VkBuffer create_index_buffer(VkDevice device, VkPhysicalDevice physical_device, VkCommandPool command_pool, VkQueue graphics_queue, VkDeviceMemory& out_buffer_memory);
void create_uniform_buffers(VkDevice device, VkPhysicalDevice physical_device, std::vector<VkBuffer>& out_uniform_buffers, std::vector<VkDeviceMemory>& out_uniform_buffers_memory,
	std::vector<void*>& out_uniform_buffers_mapped);
VkDescriptorPool create_descriptor_pool(VkDevice device);
std::vector<VkDescriptorSet> create_descriptor_sets(VkDescriptorSetLayout descriptor_set_layout, VkDescriptorPool descriptor_pool,
	VkDevice device, std::vector<VkBuffer>& uniform_buffers, VkImageView texture_image_view, VkSampler texture_sampler);
std::vector<VkCommandBuffer> create_command_buffers(VkCommandPool command_pool, VkDevice device);

void create_depth_resource(VkDevice device, VkPhysicalDevice physical_device) {

void create_texture_image(VkDevice device, VkPhysicalDevice physical_device, VkImage& out_image,
	VkDeviceMemory& out_image_memory, VkCommandPool command_pool, VkQueue graphics_queue);
VkImageView create_texture_image_view(VkDevice device, VkImage texture_image);
VkSampler create_texture_sampler(VkDevice device, VkPhysicalDevice physical_device);
void create_sync_objects(VkDevice device, std::vector<VkSemaphore>& image_available_semaphores, std::vector<VkSemaphore>& render_finished_semaphores, std::vector<VkFence>& in_flight_fences);

void record_command_buffer(VkCommandBuffer command_buffer, uint32_t image_index, VkRenderPass render_pass,
	std::vector<VkFramebuffer>& swap_chain_framebuffers, VkExtent2D swap_chain_extent, VkPipeline graphics_pipeline,
	VkBuffer vertex_buffer, VkBuffer index_buffer, VkPipelineLayout pipeline_layout, std::vector<VkDescriptorSet>& descriptor_sets, uint32_t current_frame);
DrawFrameResult draw_frame(Vulkan& vulkan, HWND hwnd);
void update_uniform_buffer(uint32_t current_image, VkExtent2D swap_chain_extent, std::vector<void*>& uniform_buffers_mapped);
RecreateSwapChainResult recreate_swap_chain(Vulkan& vulkan, HWND hwnd);

QueueFamilyIndices get_queue_families(const VkPhysicalDevice device, VkSurfaceKHR surface);
bool queue_families_validated(QueueFamilyIndices indices);
SwapChainSupportInfo get_swap_chain_support(VkPhysicalDevice device, VkSurfaceKHR surface);
int rate_device_suitability(VkPhysicalDevice device, VkSurfaceKHR surface);
VkFormat find_supported_format(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VkPhysicalDevice physical_device);
static VkVertexInputBindingDescription get_vertex_binding_description();
static std::array<VkVertexInputAttributeDescription, 3> get_vertex_attribute_descriptions();
uint32_t get_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties, VkPhysicalDevice physical_device);
VkBuffer create_vulkan_buffer(VkDevice device, VkPhysicalDevice physical_device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkDeviceMemory& out_buffer_memory);
void copy_vulkan_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size, VkDevice device, VkCommandPool command_pool, VkQueue graphics_queue);
void create_vulkan_image(uint32_t width, uint32_t height, VkDevice device, VkPhysicalDevice physical_device, VkFormat format, VkImageTiling tiling,
	VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& out_image, VkDeviceMemory& out_image_memory);
VkImageView create_vulkan_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, VkDevice device);
void transition_image_layout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout,
	VkCommandPool command_pool, VkDevice device, VkQueue graphics_queue);
void copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height,
	VkCommandPool command_pool, VkDevice device, VkQueue graphics_queue);
VkCommandBuffer begin_single_time_commands(VkCommandPool command_pool, VkDevice device);
void end_single_time_commands(VkCommandBuffer command_buffer, VkQueue graphics_queue, VkDevice device, VkCommandPool command_pool);

void cleanup_swap_chain(VkDevice device, std::vector<VkFramebuffer>& framebuffers, std::vector<VkImageView>& image_views, VkSwapchainKHR swap_chain);
void cleanup_vulkan(Vulkan& vulkan);
