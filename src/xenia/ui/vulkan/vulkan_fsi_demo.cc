/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2022 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <utility>

#include "xenia/base/assert.h"
#include "xenia/base/logging.h"
#include "xenia/base/math.h"
#include "xenia/ui/presenter.h"
#include "xenia/ui/vulkan/vulkan_presenter.h"
#include "xenia/ui/vulkan/vulkan_provider.h"
#include "xenia/ui/vulkan/vulkan_util.h"
#include "xenia/ui/window.h"
#include "xenia/ui/windowed_app.h"
#include "xenia/ui/windowed_app_context.h"

namespace xe {
namespace ui {
namespace vulkan {

// Generated with `xb buildshaders`.
namespace shaders {
#include "xenia/gpu/shaders/bytecode/vulkan_spirv/fullscreen_cw_vs.h"
#include "xenia/ui/shaders/bytecode/vulkan_spirv/fsi_crc32_blit_ps.h"
#include "xenia/ui/shaders/bytecode/vulkan_spirv/fsi_crc32_ps.h"
#include "xenia/ui/shaders/bytecode/vulkan_spirv/fsi_crc32_vs.h"
}  // namespace shaders

class VulkanFSIDemoApp : public WindowedApp, public UIDrawer {
 public:
  static std::unique_ptr<WindowedApp> Create(WindowedAppContext& app_context) {
    return std::unique_ptr<WindowedApp>(new VulkanFSIDemoApp(app_context));
  }

  ~VulkanFSIDemoApp();

  bool OnInitialize() override;

  void Draw(UIDrawContext& context) override;

 protected:
  explicit VulkanFSIDemoApp(WindowedAppContext& app_context)
      : WindowedApp(app_context, "xenia-ui-vulkan-fsi-demo"),
        window_listener_(app_context) {}

 private:
  class VulkanFSIDemoWindowListener final : public WindowListener {
   public:
    explicit VulkanFSIDemoWindowListener(WindowedAppContext& app_context)
        : app_context_(app_context) {}

    void OnClosing(UIEvent& e) override { app_context_.QuitFromUIThread(); }

   private:
    WindowedAppContext& app_context_;
  };

  struct TIRPushConstants {
    uint32_t buffer_width;
    uint32_t buffer_samples;
  };

  struct TIRVertex {
    float position[2];
    uint32_t primitive_id;
  };

  VulkanFSIDemoWindowListener window_listener_;
  std::unique_ptr<VulkanProvider> vulkan_provider_;

  VkDescriptorSetLayout fs_storage_buffer_descriptor_set_layout_ =
      VK_NULL_HANDLE;

  VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;

  // TIR means target-independent rasterization (no framebuffer attachments).
  VkRenderPass tir_render_pass_ = VK_NULL_HANDLE;
  VkFramebuffer tir_framebuffer_ = VK_NULL_HANDLE;
  VkPipelineLayout tir_pipeline_layout_ = VK_NULL_HANDLE;
  // One for each 2^i sample count from 1 to 16.
  std::array<VkPipeline, 5> tir_crc32_pipelines_{};
  uint32_t tir_crc32_max_sample_count_log2_;

  VkDeviceMemory tir_vertex_buffer_memory_ = VK_NULL_HANDLE;
  VkBuffer tir_vertex_buffer_ = VK_NULL_HANDLE;

  std::unique_ptr<Window> window_;
  std::unique_ptr<Presenter> presenter_;

  uint64_t last_submission_index_ = 0;

  VkDescriptorSet tir_buffer_descriptor_set_;
  VkDeviceSize tir_buffer_size_ = 0;
  VkDeviceMemory tir_buffer_memory_ = VK_NULL_HANDLE;
  VkBuffer tir_buffer_ = VK_NULL_HANDLE;

  VkFormat tir_blit_framebuffer_format_ = VK_FORMAT_UNDEFINED;
  VkPipeline tir_blit_pipeline_ = VK_NULL_HANDLE;
};

VulkanFSIDemoApp::~VulkanFSIDemoApp() {
  if (vulkan_provider_) {
    auto vulkan_presenter = static_cast<VulkanPresenter*>(presenter_.get());
    if (vulkan_presenter) {
      vulkan_presenter->AwaitUISubmissionCompletionFromUIThread(
          last_submission_index_);
    }
    const VulkanProvider::DeviceFunctions& dfn = vulkan_provider_->dfn();
    VkDevice device = vulkan_provider_->device();
    if (tir_blit_pipeline_ != VK_NULL_HANDLE) {
      dfn.vkDestroyPipeline(device, tir_blit_pipeline_, nullptr);
    }
    if (tir_buffer_ != VK_NULL_HANDLE) {
      dfn.vkDestroyBuffer(device, tir_buffer_, nullptr);
    }
    if (tir_buffer_memory_ != VK_NULL_HANDLE) {
      dfn.vkFreeMemory(device, tir_buffer_memory_, nullptr);
    }
    if (tir_vertex_buffer_ != VK_NULL_HANDLE) {
      dfn.vkDestroyBuffer(device, tir_vertex_buffer_, nullptr);
    }
    if (tir_vertex_buffer_memory_ != VK_NULL_HANDLE) {
      dfn.vkFreeMemory(device, tir_vertex_buffer_memory_, nullptr);
    }
    for (VkPipeline& pipeline : tir_crc32_pipelines_) {
      if (pipeline != VK_NULL_HANDLE) {
        dfn.vkDestroyPipeline(device, pipeline, nullptr);
      }
    }
    if (tir_pipeline_layout_ != VK_NULL_HANDLE) {
      dfn.vkDestroyPipelineLayout(device, tir_pipeline_layout_, nullptr);
    }
    if (tir_framebuffer_ != VK_NULL_HANDLE) {
      dfn.vkDestroyFramebuffer(device, tir_framebuffer_, nullptr);
    }
    if (tir_render_pass_ != VK_NULL_HANDLE) {
      dfn.vkDestroyRenderPass(device, tir_render_pass_, nullptr);
    }
    if (descriptor_pool_ != VK_NULL_HANDLE) {
      dfn.vkDestroyDescriptorPool(device, descriptor_pool_, nullptr);
    }
    if (fs_storage_buffer_descriptor_set_layout_ != VK_NULL_HANDLE) {
      dfn.vkDestroyDescriptorSetLayout(
          device, fs_storage_buffer_descriptor_set_layout_, nullptr);
    }
  }
}

bool VulkanFSIDemoApp::OnInitialize() {
  vulkan_provider_ = VulkanProvider::Create(true);
  if (!vulkan_provider_) {
    XELOGE("Failed to initialize the Vulkan provider");
    return false;
  }

  if (!vulkan_provider_->device_extensions().ext_fragment_shader_interlock ||
      !vulkan_provider_->device_fragment_shader_interlock_features()
           .fragmentShaderSampleInterlock ||
      !vulkan_provider_->device_fragment_shader_interlock_features()
           .fragmentShaderPixelInterlock ||
      !vulkan_provider_->device_features().sampleRateShading ||
      !vulkan_provider_->device_features().variableMultisampleRate) {
    XELOGE("Required Vulkan features are not supported by the device");
    return false;
  }
  VkSampleCountFlags framebuffer_no_attachments_sample_counts =
      vulkan_provider_->device_properties()
          .limits.framebufferNoAttachmentsSampleCounts;

  const VulkanProvider::DeviceFunctions& dfn = vulkan_provider_->dfn();
  VkDevice device = vulkan_provider_->device();

  VkDescriptorSetLayoutBinding fs_storage_buffer_binding;
  fs_storage_buffer_binding.binding = 0;
  fs_storage_buffer_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  fs_storage_buffer_binding.descriptorCount = 1;
  fs_storage_buffer_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  fs_storage_buffer_binding.pImmutableSamplers = nullptr;
  VkDescriptorSetLayoutCreateInfo
      fs_storage_buffer_descriptor_set_layout_create_info;
  fs_storage_buffer_descriptor_set_layout_create_info.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  fs_storage_buffer_descriptor_set_layout_create_info.pNext = nullptr;
  fs_storage_buffer_descriptor_set_layout_create_info.flags = 0;
  fs_storage_buffer_descriptor_set_layout_create_info.bindingCount = 1;
  fs_storage_buffer_descriptor_set_layout_create_info.pBindings =
      &fs_storage_buffer_binding;
  if (dfn.vkCreateDescriptorSetLayout(
          device, &fs_storage_buffer_descriptor_set_layout_create_info, nullptr,
          &fs_storage_buffer_descriptor_set_layout_) != VK_SUCCESS) {
    XELOGE("Failed to create the FSI storage buffer descriptor set layout");
    return false;
  }

  std::array<VkDescriptorPoolSize, 1> descriptor_pool_sizes;
  descriptor_pool_sizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  descriptor_pool_sizes[0].descriptorCount = 1;
  VkDescriptorPoolCreateInfo descriptor_pool_create_info;
  descriptor_pool_create_info.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descriptor_pool_create_info.pNext = nullptr;
  descriptor_pool_create_info.flags = 0;
  descriptor_pool_create_info.maxSets = 1;
  descriptor_pool_create_info.poolSizeCount =
      uint32_t(descriptor_pool_sizes.size());
  descriptor_pool_create_info.pPoolSizes = descriptor_pool_sizes.data();
  if (dfn.vkCreateDescriptorPool(device, &descriptor_pool_create_info, nullptr,
                                 &descriptor_pool_) != VK_SUCCESS) {
    XELOGE("Failed to create the descriptor pool");
    return false;
  }

  VkSubpassDescription tir_subpass = {};
  tir_subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  VkSubpassDependency tir_subpass_dependencies[2];
  tir_subpass_dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  tir_subpass_dependencies[0].dstSubpass = 0;
  tir_subpass_dependencies[0].srcStageMask =
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  tir_subpass_dependencies[0].dstStageMask =
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  tir_subpass_dependencies[0].srcAccessMask =
      VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
  tir_subpass_dependencies[0].dstAccessMask =
      VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
  tir_subpass_dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
  tir_subpass_dependencies[1] = tir_subpass_dependencies[0];
  std::swap(tir_subpass_dependencies[1].srcSubpass,
            tir_subpass_dependencies[1].dstSubpass);
  VkRenderPassCreateInfo tir_render_pass_create_info = {};
  tir_render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  tir_render_pass_create_info.subpassCount = 1;
  tir_render_pass_create_info.pSubpasses = &tir_subpass;
  tir_render_pass_create_info.dependencyCount = 2;
  tir_render_pass_create_info.pDependencies = tir_subpass_dependencies;
  if (dfn.vkCreateRenderPass(device, &tir_render_pass_create_info, nullptr,
                             &tir_render_pass_) != VK_SUCCESS) {
    XELOGE("Failed to create the FSI TIR render pass");
    return false;
  }

  VkFramebufferCreateInfo tir_framebuffer_create_info;
  tir_framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  tir_framebuffer_create_info.pNext = nullptr;
  tir_framebuffer_create_info.flags = 0;
  tir_framebuffer_create_info.renderPass = tir_render_pass_;
  tir_framebuffer_create_info.attachmentCount = 0;
  tir_framebuffer_create_info.pAttachments = nullptr;
  tir_framebuffer_create_info.width =
      vulkan_provider_->device_properties().limits.maxFramebufferWidth;
  tir_framebuffer_create_info.height =
      vulkan_provider_->device_properties().limits.maxFramebufferHeight;
  tir_framebuffer_create_info.layers = 1;
  if (dfn.vkCreateFramebuffer(device, &tir_framebuffer_create_info, nullptr,
                              &tir_framebuffer_) != VK_SUCCESS) {
    XELOGE("Failed to create the FSI TIR framebuffer");
    return false;
  }

  VkPushConstantRange tir_push_constant_range;
  tir_push_constant_range.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  tir_push_constant_range.offset = 0;
  tir_push_constant_range.size = sizeof(TIRPushConstants);
  VkPipelineLayoutCreateInfo tir_pipeline_layout_create_info;
  tir_pipeline_layout_create_info.sType =
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  tir_pipeline_layout_create_info.pNext = nullptr;
  tir_pipeline_layout_create_info.flags = 0;
  tir_pipeline_layout_create_info.setLayoutCount = 1;
  tir_pipeline_layout_create_info.pSetLayouts =
      &fs_storage_buffer_descriptor_set_layout_;
  tir_pipeline_layout_create_info.pushConstantRangeCount = 1;
  tir_pipeline_layout_create_info.pPushConstantRanges =
      &tir_push_constant_range;
  if (dfn.vkCreatePipelineLayout(device, &tir_pipeline_layout_create_info,
                                 nullptr,
                                 &tir_pipeline_layout_) != VK_SUCCESS) {
    XELOGE("Failed to create the FSI TIR pipeline layout");
    return false;
  }

  VkPipelineShaderStageCreateInfo tir_shader_stages[2] = {};
  tir_shader_stages[0].sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  tir_shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  tir_shader_stages[0].module = util::CreateShaderModule(
      *vulkan_provider_, shaders::fsi_crc32_vs, sizeof(shaders::fsi_crc32_vs));
  if (tir_shader_stages[0].module == VK_NULL_HANDLE) {
    XELOGE("Failed to create the FSI TIR vertex shader module");
    return false;
  }
  tir_shader_stages[0].pName = "main";
  tir_shader_stages[1].sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  tir_shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  tir_shader_stages[1].module = util::CreateShaderModule(
      *vulkan_provider_, shaders::fsi_crc32_ps, sizeof(shaders::fsi_crc32_ps));
  if (tir_shader_stages[1].module == VK_NULL_HANDLE) {
    XELOGE("Failed to create the FSI TIR fragment shader module");
    dfn.vkDestroyShaderModule(device, tir_shader_stages[0].module, nullptr);
    return false;
  }
  tir_shader_stages[1].pName = "main";
  VkVertexInputBindingDescription tir_vertex_input_binding;
  tir_vertex_input_binding.binding = 0;
  tir_vertex_input_binding.stride = sizeof(TIRVertex);
  tir_vertex_input_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  std::array<VkVertexInputAttributeDescription, 2> tir_vertex_input_attributes;
  tir_vertex_input_attributes[0].location = 0;
  tir_vertex_input_attributes[0].binding = 0;
  tir_vertex_input_attributes[0].format = VK_FORMAT_R32G32_SFLOAT;
  tir_vertex_input_attributes[0].offset =
      uint32_t(offsetof(TIRVertex, position));
  tir_vertex_input_attributes[1].location = 1;
  tir_vertex_input_attributes[1].binding = 0;
  tir_vertex_input_attributes[1].format = VK_FORMAT_R32_UINT;
  tir_vertex_input_attributes[1].offset =
      uint32_t(offsetof(TIRVertex, primitive_id));
  VkPipelineVertexInputStateCreateInfo tir_vertex_input_state;
  tir_vertex_input_state.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  tir_vertex_input_state.pNext = nullptr;
  tir_vertex_input_state.flags = 0;
  tir_vertex_input_state.vertexBindingDescriptionCount = 1;
  tir_vertex_input_state.pVertexBindingDescriptions = &tir_vertex_input_binding;
  tir_vertex_input_state.vertexAttributeDescriptionCount =
      uint32_t(tir_vertex_input_attributes.size());
  tir_vertex_input_state.pVertexAttributeDescriptions =
      tir_vertex_input_attributes.data();
  VkPipelineInputAssemblyStateCreateInfo tir_input_assembly_state;
  tir_input_assembly_state.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  tir_input_assembly_state.pNext = nullptr;
  tir_input_assembly_state.flags = 0;
  tir_input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  tir_input_assembly_state.primitiveRestartEnable = VK_FALSE;
  VkPipelineViewportStateCreateInfo tir_viewport_state = {};
  tir_viewport_state.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  tir_viewport_state.viewportCount = 1;
  tir_viewport_state.scissorCount = 1;
  VkPipelineRasterizationStateCreateInfo tir_rasterization_state = {};
  tir_rasterization_state.sType =
      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  tir_rasterization_state.polygonMode = VK_POLYGON_MODE_FILL;
  tir_rasterization_state.cullMode = VK_CULL_MODE_NONE;
  tir_rasterization_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  tir_rasterization_state.lineWidth = 1.0f;
  VkPipelineMultisampleStateCreateInfo tir_multisample_state = {};
  tir_multisample_state.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  // tir_multisample_state.rasterizationSamples will be set later.
  const VkDynamicState tir_dynamic_states[] = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR,
  };
  VkPipelineDynamicStateCreateInfo tir_dynamic_state;
  tir_dynamic_state.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  tir_dynamic_state.pNext = nullptr;
  tir_dynamic_state.flags = 0;
  tir_dynamic_state.dynamicStateCount =
      uint32_t(xe::countof(tir_dynamic_states));
  tir_dynamic_state.pDynamicStates = tir_dynamic_states;
  VkGraphicsPipelineCreateInfo tir_pipeline_create_info = {};
  tir_pipeline_create_info.sType =
      VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  tir_pipeline_create_info.stageCount =
      uint32_t(xe::countof(tir_shader_stages));
  tir_pipeline_create_info.pStages = tir_shader_stages;
  tir_pipeline_create_info.pVertexInputState = &tir_vertex_input_state;
  tir_pipeline_create_info.pInputAssemblyState = &tir_input_assembly_state;
  tir_pipeline_create_info.pViewportState = &tir_viewport_state;
  tir_pipeline_create_info.pRasterizationState = &tir_rasterization_state;
  tir_pipeline_create_info.pMultisampleState = &tir_multisample_state;
  tir_pipeline_create_info.pDynamicState = &tir_dynamic_state;
  tir_pipeline_create_info.layout = tir_pipeline_layout_;
  tir_pipeline_create_info.renderPass = tir_render_pass_;
  tir_pipeline_create_info.basePipelineIndex = -1;
  tir_crc32_max_sample_count_log2_ = 0;
  for (uint32_t i = 0; i < tir_crc32_pipelines_.size(); ++i) {
    if (!(framebuffer_no_attachments_sample_counts &
          VkSampleCountFlags(uint32_t(1) << i))) {
      continue;
    }
    tir_multisample_state.rasterizationSamples =
        VkSampleCountFlagBits(uint32_t(1) << i);
    if (dfn.vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1,
                                      &tir_pipeline_create_info, nullptr,
                                      &tir_crc32_pipelines_[i]) == VK_SUCCESS) {
      tir_crc32_max_sample_count_log2_ =
          std::max(tir_crc32_max_sample_count_log2_, i);
    }
  }
  for (size_t i = 0; i < xe::countof(tir_shader_stages); ++i) {
    dfn.vkDestroyShaderModule(device, tir_shader_stages[i].module, nullptr);
  }
  if (tir_crc32_pipelines_[0] == VK_NULL_HANDLE) {
    XELOGE("Failed to create a FSI TIR CRC32 pipeline even for one sample");
    return false;
  }

  uint32_t tir_vertex_buffer_memory_type;
  if (!util::CreateDedicatedAllocationBuffer(
          *vulkan_provider_, sizeof(TIRVertex) * 3,
          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, util::MemoryPurpose::kUpload,
          tir_vertex_buffer_, tir_vertex_buffer_memory_,
          &tir_vertex_buffer_memory_type)) {
    XELOGE("Failed to create the FSI TIR vertex buffer");
    return false;
  }
  void* tir_vertex_buffer_data;
  if (dfn.vkMapMemory(device, tir_vertex_buffer_memory_, 0, VK_WHOLE_SIZE, 0,
                      &tir_vertex_buffer_data) != VK_SUCCESS) {
    XELOGE("Failed to map the FSI TIR vertex buffer");
    return false;
  }
  auto tir_vertices =
      reinterpret_cast<volatile TIRVertex*>(tir_vertex_buffer_data);
  tir_vertices[0].position[0] = -1.0f;
  tir_vertices[0].position[1] = -1.0f;
  tir_vertices[0].primitive_id = 0;
  tir_vertices[1].position[0] = 1.0f;
  tir_vertices[1].position[1] = -1.0f;
  tir_vertices[1].primitive_id = 0;
  tir_vertices[2].position[0] = -1.0f;
  tir_vertices[2].position[1] = 1.0f;
  tir_vertices[2].primitive_id = 0;
  util::FlushMappedMemoryRange(*vulkan_provider_, tir_vertex_buffer_memory_,
                               tir_vertex_buffer_memory_type);
  dfn.vkUnmapMemory(device, tir_vertex_buffer_memory_);

  VkDescriptorSetAllocateInfo descriptor_set_allocate_info;
  descriptor_set_allocate_info.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  descriptor_set_allocate_info.pNext = nullptr;
  descriptor_set_allocate_info.descriptorPool = descriptor_pool_;
  descriptor_set_allocate_info.descriptorSetCount = 1;
  descriptor_set_allocate_info.pSetLayouts =
      &fs_storage_buffer_descriptor_set_layout_;
  if (dfn.vkAllocateDescriptorSets(device, &descriptor_set_allocate_info,
                                   &tir_buffer_descriptor_set_) != VK_SUCCESS) {
    XELOGE("Failed to allocate the FSI TIR buffer descriptor set");
    return false;
  }

  window_ = xe::ui::Window::Create(app_context(), GetName(), 1280, 720);
  window_->AddListener(&window_listener_);
  if (!window_->Open()) {
    XELOGE("Failed to open the main window");
    return false;
  }

  presenter_ = vulkan_provider_->CreatePresenter();
  if (!presenter_) {
    XELOGE("Failed to initialize the presenter");
    return false;
  }
  presenter_->AddUIDrawerFromUIThread(this, 0);

  window_->SetPresenter(presenter_.get());

  return true;
}

void VulkanFSIDemoApp::Draw(UIDrawContext& context) {
  const VulkanUIDrawContext& vulkan_context =
      static_cast<const VulkanUIDrawContext&>(context);

  const VulkanProvider::DeviceFunctions& dfn = vulkan_provider_->dfn();
  VkDevice device = vulkan_provider_->device();
  auto vulkan_presenter = static_cast<VulkanPresenter*>(presenter_.get());

  uint32_t width = vulkan_context.render_target_width();
  uint32_t height = vulkan_context.render_target_height();
  VkDeviceSize tir_current_frame_buffer_size = VkDeviceSize(
      (sizeof(uint32_t) << tir_crc32_max_sample_count_log2_) * width * height);
  if (tir_buffer_size_ < tir_current_frame_buffer_size) {
    if (tir_buffer_memory_ != VK_NULL_HANDLE || tir_buffer_ != VK_NULL_HANDLE) {
      vulkan_presenter->AwaitUISubmissionCompletionFromUIThread(
          last_submission_index_);
      util::DestroyAndNullHandle(dfn.vkDestroyBuffer, device, tir_buffer_);
      util::DestroyAndNullHandle(dfn.vkFreeMemory, device, tir_buffer_memory_);
    }
    tir_buffer_size_ =
        std::max(tir_buffer_size_, tir_current_frame_buffer_size);
  }
  if (tir_buffer_ == VK_NULL_HANDLE) {
    assert_true(tir_buffer_memory_ == VK_NULL_HANDLE);
    if (!util::CreateDedicatedAllocationBuffer(
            *vulkan_provider_, tir_buffer_size_,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            util::MemoryPurpose::kDeviceLocal, tir_buffer_,
            tir_buffer_memory_)) {
      return;
    }
    VkDescriptorBufferInfo tir_buffer_descriptor_buffer_info;
    tir_buffer_descriptor_buffer_info.buffer = tir_buffer_;
    tir_buffer_descriptor_buffer_info.offset = 0;
    tir_buffer_descriptor_buffer_info.range = VK_WHOLE_SIZE;
    VkWriteDescriptorSet tir_buffer_descriptor_write;
    tir_buffer_descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    tir_buffer_descriptor_write.pNext = nullptr;
    tir_buffer_descriptor_write.dstSet = tir_buffer_descriptor_set_;
    tir_buffer_descriptor_write.dstBinding = 0;
    tir_buffer_descriptor_write.dstArrayElement = 0;
    tir_buffer_descriptor_write.descriptorCount = 1;
    tir_buffer_descriptor_write.descriptorType =
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    tir_buffer_descriptor_write.pImageInfo = nullptr;
    tir_buffer_descriptor_write.pBufferInfo =
        &tir_buffer_descriptor_buffer_info;
    tir_buffer_descriptor_write.pTexelBufferView = nullptr;
    dfn.vkUpdateDescriptorSets(device, 1, &tir_buffer_descriptor_write, 0,
                               nullptr);
  }

  VkFormat render_pass_format = vulkan_context.render_pass_format();
  if (tir_blit_framebuffer_format_ != render_pass_format) {
    if (tir_blit_pipeline_ != VK_NULL_HANDLE) {
      vulkan_presenter->AwaitUISubmissionCompletionFromUIThread(
          last_submission_index_);
      util::DestroyAndNullHandle(dfn.vkDestroyPipeline, device,
                                 tir_blit_pipeline_);
    }
    tir_blit_framebuffer_format_ = render_pass_format;
  }
  if (tir_blit_pipeline_ == VK_NULL_HANDLE) {
    VkPipelineShaderStageCreateInfo tir_blit_shader_stages[2] = {};
    tir_blit_shader_stages[0].sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    tir_blit_shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    tir_blit_shader_stages[0].module =
        util::CreateShaderModule(*vulkan_provider_, shaders::fullscreen_cw_vs,
                                 sizeof(shaders::fullscreen_cw_vs));
    if (tir_blit_shader_stages[0].module == VK_NULL_HANDLE) {
      XELOGE("Failed to create the FSI TIR blit vertex shader module");
      return;
    }
    tir_blit_shader_stages[0].pName = "main";
    tir_blit_shader_stages[1].sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    tir_blit_shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    tir_blit_shader_stages[1].module =
        util::CreateShaderModule(*vulkan_provider_, shaders::fsi_crc32_blit_ps,
                                 sizeof(shaders::fsi_crc32_blit_ps));
    if (tir_blit_shader_stages[1].module == VK_NULL_HANDLE) {
      XELOGE("Failed to create the FSI TIR blit fragment shader module");
      dfn.vkDestroyShaderModule(device, tir_blit_shader_stages[0].module,
                                nullptr);
      return;
    }
    tir_blit_shader_stages[1].pName = "main";
    VkPipelineVertexInputStateCreateInfo tir_blit_vertex_input_state = {};
    tir_blit_vertex_input_state.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    VkPipelineInputAssemblyStateCreateInfo tir_blit_input_assembly_state;
    tir_blit_input_assembly_state.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    tir_blit_input_assembly_state.pNext = nullptr;
    tir_blit_input_assembly_state.flags = 0;
    tir_blit_input_assembly_state.topology =
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    tir_blit_input_assembly_state.primitiveRestartEnable = VK_FALSE;
    VkPipelineViewportStateCreateInfo tir_blit_viewport_state = {};
    tir_blit_viewport_state.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    tir_blit_viewport_state.viewportCount = 1;
    tir_blit_viewport_state.scissorCount = 1;
    VkPipelineRasterizationStateCreateInfo tir_blit_rasterization_state = {};
    tir_blit_rasterization_state.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    tir_blit_rasterization_state.polygonMode = VK_POLYGON_MODE_FILL;
    tir_blit_rasterization_state.cullMode = VK_CULL_MODE_NONE;
    tir_blit_rasterization_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    tir_blit_rasterization_state.lineWidth = 1.0f;
    VkPipelineMultisampleStateCreateInfo tir_blit_multisample_state = {};
    tir_blit_multisample_state.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    tir_blit_multisample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPipelineColorBlendAttachmentState tir_blit_color_blend_attachment = {};
    tir_blit_color_blend_attachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo tir_blit_color_blend_state = {};
    tir_blit_color_blend_state.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    tir_blit_color_blend_state.attachmentCount = 1;
    tir_blit_color_blend_state.pAttachments = &tir_blit_color_blend_attachment;
    const VkDynamicState tir_blit_dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };
    VkPipelineDynamicStateCreateInfo tir_blit_dynamic_state;
    tir_blit_dynamic_state.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    tir_blit_dynamic_state.pNext = nullptr;
    tir_blit_dynamic_state.flags = 0;
    tir_blit_dynamic_state.dynamicStateCount =
        uint32_t(xe::countof(tir_blit_dynamic_states));
    tir_blit_dynamic_state.pDynamicStates = tir_blit_dynamic_states;
    VkGraphicsPipelineCreateInfo tir_blit_pipeline_create_info = {};
    tir_blit_pipeline_create_info.sType =
        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    tir_blit_pipeline_create_info.stageCount =
        uint32_t(xe::countof(tir_blit_shader_stages));
    tir_blit_pipeline_create_info.pStages = tir_blit_shader_stages;
    tir_blit_pipeline_create_info.pVertexInputState =
        &tir_blit_vertex_input_state;
    tir_blit_pipeline_create_info.pInputAssemblyState =
        &tir_blit_input_assembly_state;
    tir_blit_pipeline_create_info.pViewportState = &tir_blit_viewport_state;
    tir_blit_pipeline_create_info.pRasterizationState =
        &tir_blit_rasterization_state;
    tir_blit_pipeline_create_info.pMultisampleState =
        &tir_blit_multisample_state;
    tir_blit_pipeline_create_info.pColorBlendState =
        &tir_blit_color_blend_state;
    tir_blit_pipeline_create_info.pDynamicState = &tir_blit_dynamic_state;
    tir_blit_pipeline_create_info.layout = tir_pipeline_layout_;
    tir_blit_pipeline_create_info.renderPass = vulkan_context.render_pass();
    tir_blit_pipeline_create_info.basePipelineIndex = -1;
    dfn.vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1,
                                  &tir_blit_pipeline_create_info, nullptr,
                                  &tir_blit_pipeline_);
    for (size_t i = 0; i < xe::countof(tir_blit_shader_stages); ++i) {
      dfn.vkDestroyShaderModule(device, tir_blit_shader_stages[i].module,
                                nullptr);
    }
    if (tir_blit_pipeline_ == VK_NULL_HANDLE) {
      XELOGE("Failed to create the FSI TIR blit pipeline");
      return;
    }
  }

  VkCommandBuffer setup_command_buffer =
      vulkan_presenter->AcquireUISetupCommandBufferFromUIThread();
  if (setup_command_buffer == VK_NULL_HANDLE) {
    return;
  }

  VkRect2D render_target_rect;
  render_target_rect.offset.x = 0;
  render_target_rect.offset.y = 0;
  render_target_rect.extent.width = width;
  render_target_rect.extent.height = height;

  VkViewport render_target_viewport;
  render_target_viewport.x = 0.0f;
  render_target_viewport.y = 0.0f;
  render_target_viewport.width = float(width);
  render_target_viewport.height = float(height);
  render_target_viewport.minDepth = 0.0f;
  render_target_viewport.maxDepth = 1.0f;

  TIRPushConstants tir_push_constants;
  tir_push_constants.buffer_width = width;
  tir_push_constants.buffer_samples = uint32_t(1)
                                      << tir_crc32_max_sample_count_log2_;

  VkBufferMemoryBarrier tir_buffer_barrier;
  tir_buffer_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  tir_buffer_barrier.pNext = nullptr;
  tir_buffer_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  tir_buffer_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  tir_buffer_barrier.buffer = tir_buffer_;
  tir_buffer_barrier.offset = 0;
  tir_buffer_barrier.size = VK_WHOLE_SIZE;

  // Clear the buffer to the CRC32 initialization value.
  tir_buffer_barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
  tir_buffer_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  dfn.vkCmdPipelineBarrier(setup_command_buffer,
                           VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                           VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1,
                           &tir_buffer_barrier, 0, nullptr);
  dfn.vkCmdFillBuffer(setup_command_buffer, tir_buffer_, 0,
                      tir_current_frame_buffer_size, UINT32_MAX);

  // Insert the buffer barrier before writing to it via FSI.
  tir_buffer_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  tir_buffer_barrier.dstAccessMask =
      VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
  dfn.vkCmdPipelineBarrier(setup_command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                           VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr,
                           1, &tir_buffer_barrier, 0, nullptr);

  // Draw the geometry to the TIR buffer.
  VkRenderPassBeginInfo tir_render_pass_begin_info = {};
  tir_render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  tir_render_pass_begin_info.renderPass = tir_render_pass_;
  tir_render_pass_begin_info.framebuffer = tir_framebuffer_;
  tir_render_pass_begin_info.renderArea = render_target_rect;
  dfn.vkCmdBeginRenderPass(setup_command_buffer, &tir_render_pass_begin_info,
                           VK_SUBPASS_CONTENTS_INLINE);
  // Draw a small triangle with likely wave overlap.
  VkViewport very_small_viewport;
  very_small_viewport.x = 0.0f;
  very_small_viewport.y = 0.0f;
  very_small_viewport.width = 4.0f;
  very_small_viewport.height = 4.0f;
  very_small_viewport.minDepth = 0.0f;
  very_small_viewport.maxDepth = 1.0f;
  dfn.vkCmdSetViewport(setup_command_buffer, 0, 1, &very_small_viewport);
  dfn.vkCmdSetScissor(setup_command_buffer, 0, 1, &render_target_rect);
  dfn.vkCmdBindDescriptorSets(
      setup_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
      tir_pipeline_layout_, 0, 1, &tir_buffer_descriptor_set_, 0, nullptr);
  dfn.vkCmdPushConstants(setup_command_buffer, tir_pipeline_layout_,
                         VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                         sizeof(tir_push_constants), &tir_push_constants);
  VkDeviceSize tir_vertex_buffer_offset = 0;
  dfn.vkCmdBindVertexBuffers(setup_command_buffer, 0, 1, &tir_vertex_buffer_,
                             &tir_vertex_buffer_offset);
  for (uint32_t i = 0; i <= tir_crc32_max_sample_count_log2_; ++i) {
    VkPipeline tir_crc32_pipeline = tir_crc32_pipelines_[i];
    if (tir_crc32_pipeline == VK_NULL_HANDLE) {
      continue;
    }
    dfn.vkCmdBindPipeline(setup_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          tir_crc32_pipeline);
    dfn.vkCmdDraw(setup_command_buffer, 3, 1, 0, 0);
  }
  dfn.vkCmdEndRenderPass(setup_command_buffer);

  // Insert the buffer barrier before resolving.
  tir_buffer_barrier.srcAccessMask =
      VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
  tir_buffer_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  dfn.vkCmdPipelineBarrier(setup_command_buffer,
                           VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                           VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr,
                           1, &tir_buffer_barrier, 0, nullptr);

  // Resolve the TIR buffer.
  VkCommandBuffer draw_command_buffer = vulkan_context.draw_command_buffer();
  dfn.vkCmdSetViewport(draw_command_buffer, 0, 1, &render_target_viewport);
  dfn.vkCmdSetScissor(draw_command_buffer, 0, 1, &render_target_rect);
  dfn.vkCmdBindPipeline(draw_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                        tir_blit_pipeline_);
  dfn.vkCmdBindDescriptorSets(
      draw_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
      tir_pipeline_layout_, 0, 1, &tir_buffer_descriptor_set_, 0, nullptr);
  dfn.vkCmdPushConstants(draw_command_buffer, tir_pipeline_layout_,
                         VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                         sizeof(tir_push_constants), &tir_push_constants);
  dfn.vkCmdDraw(draw_command_buffer, 3, 1, 0, 0);

  last_submission_index_ = vulkan_context.submission_index_current();

  presenter_->RequestUIPaintFromUIThread();
}

}  // namespace vulkan
}  // namespace ui
}  // namespace xe

XE_DEFINE_WINDOWED_APP(xenia_ui_window_vulkan_fsi_demo,
                       xe::ui::vulkan::VulkanFSIDemoApp::Create);
