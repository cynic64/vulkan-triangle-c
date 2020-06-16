#ifndef VK_PIPE_H_
#define VK_PIPE_H_

#include <vulkan/vulkan.h>
#include <stdio.h>

// If <buf> is NULL, will only set buf_size
void read_bin(FILE *fp, size_t *buf_size, char *buf);

void create_shmod(VkDevice device,
		  size_t code_size,
		  char *code,
		  VkShaderModule *shmod);

void create_layout(VkDevice device,
		   uint32_t desc_layout_ct,
		   VkDescriptorSetLayout *desc_layouts,
		   VkPipelineLayout *layout);

/*
 * Creates a graphics pipeline.
 *
 * has_pipe: Whether a depth stencil attachment will be used (1 = true)
 */
void create_pipel(VkDevice device,
		  uint32_t shtage_ct,
		  VkPipelineShaderStageCreateInfo *shtages,
		  VkPipelineLayout layout,
		  uint32_t binding_ct,
		  VkVertexInputBindingDescription *binding_descs,
		  uint32_t attr_ct,
		  VkVertexInputAttributeDescription *attr_descs,
		  VkRenderPass rpass,
		  int has_depth, VkSampleCountFlagBits samples,
		  VkPipeline *pipel);

void create_shtage(VkShaderModule shmod,
		   VkShaderStageFlagBits stage,
		   VkPipelineShaderStageCreateInfo *shtage);

#endif // VK_PIPE_H_
