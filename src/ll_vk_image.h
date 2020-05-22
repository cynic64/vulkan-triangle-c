#include <vulkan/vulkan.h>

/*
 * Find the index of memory suitable given a required requirements (that sounds
 * dumb, I know...) and properties.
 *
 * mem_reqs: What the buffer itself requirements
 * req_props: Whatever additional requirements the caller has
 */
uint32_t find_memory_type(VkPhysicalDeviceMemoryProperties dev_props,
			  VkMemoryRequirements mem_reqs,
			  VkMemoryPropertyFlags req_props);
