#pragma once
#include "Objects/IGObject.h"

class CVulkanRenderer
{
public:
    CVulkanRenderer(GLFWwindow* window);
    ~CVulkanRenderer();

    // Init
    bool Init();
    bool Shutdown();

    // Render
    void Render();

    // Getters
    VkDevice GetDevice() const { return m_Device; }
    VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
    VkRenderPass GetRenderPass() const { return m_RenderPass; }
    const VkExtent2D& GetSwapChainExtent() const { return m_SwapChainExtent; }

    // Uniform buffers
    VkBuffer CamUniBuffer() const { return m_CamUniBuffer; }
    VkDeviceMemory CamUniBufferMemory() const { return m_CamUniBufferMemory; }

    const VkDescriptorSet* DescriptorSet() const { return &m_DescriptorSet; }
    const VkDescriptorPool* DescriptorPool() const { return &m_DescriptorPool; }
    const VkDescriptorSetLayout* DescriptorSetLayout() const { return &m_DescriptorSetLayout; }

    // Buffers
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    bool CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void CopyBuffer(VkBuffer& srcBuffer, VkBuffer& dstBuffer, VkDeviceSize size);

    // Device properties
    const size_t& MinUboAlignment() const { return m_MinUniformBufferOffsetAlignment; }

    // Single commands
    VkCommandBuffer BeginSingleTimeCommands();
    void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

    // Misc
    void PresentQueueWaitIdle();
    void RecreateCommandBuffer();
    bool HasStencilComponent(VkFormat format);

protected:
    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    struct QueueFamilyIndices
    {
        int graphicsFamily = -1;
        int presentFamily = -1;
        bool IsComplete()
        {
            return graphicsFamily >= 0 && presentFamily >= 0;
        }
    };

#ifdef _DEBUG
    // Debug
    bool CheckValidationLayerSupport();
    void DestroyDebugReportCallbackEXT(VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator);
    VkResult CreateDebugReportCallbackEXT(const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback);
#endif

    // Init Vulkan
    bool InitVkInstance();
    bool InitWindowSurface();
    bool CreateSwapChain();
    bool CreateImageViews();
    bool CreateLogicalDevice();
    bool PickPhysicalDevice();
    bool CreateRenderPass();
    bool CreateFramebuffers();
    bool CreateCommandPool();
    bool CreateCommandBuffers();
    bool CreateSemaphores();

    // Extensions support
    bool CheckAvailableExtensions();
    bool GetRequiredExtensions();
    bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
    int  RateDeviceSuitability(VkPhysicalDevice device);

    // SwapChain support
    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    void RecreateSwapChain();
    void CleanupSwapChain();
    bool RecreateSwapChainIfNeeded(const VkResult& result, bool allow_suboptimal = true);

    // Descriptor Sets
    bool CreateDescriptorSetLayout();
    bool CreateDescriptorPool();
    bool CreateDescriptorSet();

    // Buffers
    bool CreateUniformBuffers();
    bool CreateTechsRenderObjects();
    void DestroyTechsRenderObjects();

    // Depth
    bool CreateDepthResources();
    VkFormat FindDepthFormat();

    // Render
    bool SubmitDrawCommands(const uint32_t& imageIndex, VkSubmitInfo& submitInfo);

    // Misc
    void FetchDeviceProperties();

    // Pipeline cache
    // bool CreatePipelineCache();

private:
    // Window
    GLFWwindow*         m_Window = nullptr;

    // Vulkan handle
    VkInstance          m_Instance = nullptr;       // Vulkan instance
    VkSurfaceKHR        m_Surface = nullptr;        // Window surface where the view will be rendered
    VkDevice            m_Device = nullptr;         // Logical device handle
    VkPhysicalDevice    m_PhysicalDevice = nullptr; // Physical device handle
    VkQueue             m_GraphicsQueue = nullptr;  // Render queue
    VkQueue             m_PresentQueue = nullptr;   // Presentation queue

    // Pipeline handle
    VkRenderPass        m_RenderPass = nullptr;

    // SwapChain handle
    VkSwapchainKHR      m_SwapChain = nullptr;
    VkFormat            m_SwapChainImageFormat;
    VkExtent2D          m_SwapChainExtent;

    std::vector<VkImage> m_SwapChainImagesVec;
    std::vector<VkImageView> m_SwapChainImageViewsVec;
    std::vector<VkFramebuffer> m_SwapChainFramebuffersVec;

    // Uniform Buffers
    VkBuffer m_CamUniBuffer = nullptr;
    VkDeviceMemory m_CamUniBufferMemory = nullptr;

    // Depth
    VkImage m_DepthImage = nullptr;
    VkDeviceMemory m_DepthImageMemory = nullptr;
    VkImageView m_DepthImageView = nullptr;

    // Descriptor Set
    VkDescriptorSet m_DescriptorSet = nullptr;
    VkDescriptorPool m_DescriptorPool = nullptr;
    VkDescriptorSetLayout m_DescriptorSetLayout = nullptr;

    // Command buffers
    VkCommandPool m_CommandPool = nullptr;
    std::vector<VkCommandBuffer> m_CommandBuffers;

    // Swap chain events synchronization
    VkSemaphore m_ImageAvailableSemaphore = nullptr;
    VkSemaphore m_RenderFinishedSemaphore = nullptr;

    // Without triple buffering, application will use #V-SYNC
    bool m_EnableTripleBuffering = true;

    // Required device extensions vector. Without them application won't start
    const std::vector<const char*> m_ReqDeviceExt = { VK_KHR_SWAPCHAIN_EXTENSION_NAME }; 

    // Required instance extensions
    uint32_t m_InstanceExtCount = 0;
    std::vector<const char*> m_ReqInstanceExt;

    // Validation layers
    const std::vector<const char*> m_ValidationLayers = { "VK_LAYER_LUNARG_standard_validation" };

    // Device properties
    size_t m_MinUniformBufferOffsetAlignment = 0;

#ifdef _DEBUG
    // Debug
    VkDebugReportCallbackEXT m_DebugCallback = nullptr;
#endif
};

