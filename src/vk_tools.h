#ifndef VK_TOOLS_H_
#define VK_TOOLS_H_

#include <vulkan/vulkan.h>

// https://stackoverflow.com/questions/3437404/min-and-max-in-c
#define MAX(a,b)				\
	({ __typeof__ (a) _a = (a);		\
		__typeof__ (b) _b = (b);	\
		_a > _b ? _a : _b; })

// https://stackoverflow.com/questions/4415524/common-array-length-macro-for-c#4415646
#define ARRAY_SIZE(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

typedef VkBool32 (*DebugCallback) (VkDebugUtilsMessageSeverityFlagBitsEXT,
				   VkDebugUtilsMessageTypeFlagsEXT,
				   const VkDebugUtilsMessengerCallbackDataEXT *,
				   void *);

typedef VkResult (*DebugLoader) (VkInstance *,
				 const VkDebugUtilsMessengerCreateInfoEXT *,
				 const VkAllocationCallbacks *,
				 VkDebugUtilsMessengerEXT **);

/* Returns 0 if all extensions were found, -1 otherwise */
int check_exts(uint32_t req_ext_ct, char **req_exts);

/* Same, but for device extensions */
int check_dev_exts(VkPhysicalDevice phys_dev,
		   uint32_t req_ext_ct,
		   char **req_exts);

/*
 * Create a Vulkan instance.
 *
 * dbg_cback: Debug callback for instance creation/destruction
 * user_data: Void pointer that Vulkan will pass to debug callback
 * ext_ct, exts: Instance extensions to enable
 * instance: Output
 */
void create_instance(DebugCallback dbg_cback,
		     void *user_data,
		     VkInstance *instance);

/* Ensures all instance validation layers named in <req_layers> are present.
   Returns 0 if they are, -1 on failure to find a layer. */
int check_layers(uint32_t req_layer_ct, const char * const *req_layers);

void init_debug(VkInstance *instance,
		DebugCallback dbg_cback,
		void *pUserData,
		VkDebugUtilsMessengerEXT *dbg_msgr);

void destroy_dbg_msgr(VkInstance instance,
		      VkDebugUtilsMessengerEXT *dbg_msgr);

void get_physical_device(VkInstance instance, VkPhysicalDevice *phys_dev);

uint32_t get_queue_fam(VkPhysicalDevice phys_dev);

void create_device(VkPhysicalDevice phys_dev,
		   uint32_t queue_fam,
		   VkDevice *device);

void get_queue(VkDevice device, uint32_t queue_fam, VkQueue *queue);

void populate_dbg_info(VkDebugUtilsMessengerCreateInfoEXT *dbg_info,
		       DebugCallback dbg_cback,
		       void *pUserData);

void create_dbg_msgr(VkInstance instance,
		     VkDebugUtilsMessengerCreateInfoEXT *dbg_info,
		     VkDebugUtilsMessengerEXT *dbg_msgr);

VKAPI_ATTR VkBool32 VKAPI_CALL
default_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		       VkDebugUtilsMessageTypeFlagsEXT messageType,
		       const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		       void* pUserData);

#endif // VK_TOOLS_H_
