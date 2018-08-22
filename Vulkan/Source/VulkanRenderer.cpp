#include "stdafx.h"
#include "VulkanRenderer.h"
#include "DescriptorManager.h"
#include "Objects/GBaseObject.h"
#include "Techs/TechniqueManager.h"
#include "Utils/ImageUtils.h"

CVulkanRenderer::CVulkanRenderer(GLFWwindow* window)
    : m_Window(window)
    , m_DescMgr(new CDescriptorManager())
{
}

// Init Methods
//////////////////////////////////////////////////////////////////////////
bool CVulkanRenderer::Init()
{
    if (!CheckAvailableExtensions())
        return Shutdown();

    if (GetRequiredExtensions())
    {
        if (!InitVkInstance())
            return Shutdown();

        // The window surface needs to be created right after the instance creation, 
        // because it can actually influence the physical device selection. 
        if (!InitWindowSurface())
            return Shutdown();

        if (!PickPhysicalDevice())
            return Shutdown();

        if (!CreateLogicalDevice())
            return Shutdown();

        if (!CreateSwapChain())
            return Shutdown();

        if (!CreateImageViews())
            return Shutdown();

        if (!CreateRenderPass())
            return Shutdown();

        if (!CreateCommandPool())
            return Shutdown();

        //#DEPTH
        if (!CreateDepthResources())
            return Shutdown();

        if (!CreateFramebuffers())
            return Shutdown();

        // #UNI_BUFF #IMAGES
        if (!CreateTechsRenderObjects())
            return Shutdown();

        if (!CreateGeneralUniformBuffers())
            return Shutdown();

         if (!CreateDescriptorPool())
             return Shutdown();

         if (!DescMgr()->CreateDescriptorLayouts())
             return Shutdown();

         if (!DescMgr()->CreateDescriptorSets())
             return Shutdown();

        if (!CreateCommandBuffers()) // #TECH to samo z bufforami trzeba to przemyslec jeszcze
            return Shutdown();

        if (!CreateSemaphores())
            return Shutdown();

        return true;
    }

    return false;
}

bool CVulkanRenderer::Shutdown()
{
    //  We need to wait for the logical device to finish operations before shutdown
    vkDeviceWaitIdle(m_Device);

    // Shutdown SwapChain, pipeline, renderpass
    CleanupSwapChain();

    // Shutdown Descriptors Manager
    if (m_DescMgr)
    {
        m_DescMgr->Shutdown();
        DELETE(m_DescMgr);
    }

    // Shutdown General Uniform Buffers
    if (m_CamUniBuffer)
        vkDestroyBuffer(m_Device, m_CamUniBuffer, nullptr);

    if (m_CamUniBufferMemory)
        vkFreeMemory(m_Device, m_CamUniBufferMemory, nullptr);

    // Shutdown Techs UniformBuffers/Images
    DestroyTechsRenderObjects();

#ifdef _DEBUG
    DestroyDebugReportCallbackEXT(m_DebugCallback, nullptr);
#endif

    if (m_RenderFinishedSemaphore)
        vkDestroySemaphore(m_Device, m_RenderFinishedSemaphore, nullptr);

    if (m_ImageAvailableSemaphore)
        vkDestroySemaphore(m_Device, m_ImageAvailableSemaphore, nullptr);

    if (m_CommandPool)
        vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);

    if (m_Device)
        vkDestroyDevice(m_Device, nullptr);

    if (m_Surface)
        vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);

    if (m_Instance)
        vkDestroyInstance(m_Instance, nullptr);

    glfwTerminate();
    return 0;
}

// Render methods
//////////////////////////////////////////////////////////////////////////
void CVulkanRenderer::Render()
{
    // Acquiring an image from the swap chain
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(m_Device, m_SwapChain, std::numeric_limits<uint64_t>::max(), m_ImageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    // Check if swapchain is out of date
    if (RecreateSwapChainIfNeeded(result))
        return;

    VkSubmitInfo submitInfo = {};
    if (!SubmitDrawCommands(imageIndex, submitInfo))
        return;

    // Presentation
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = submitInfo.pSignalSemaphores; // specify which semaphores to wait on before presentation can happen

    VkSwapchainKHR swapChains[] = { m_SwapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // Optional

    result = vkQueuePresentKHR(m_PresentQueue, &presentInfo); // submits the request to present an image to the swap chain

    // Check if swapchain is out of date
    if (RecreateSwapChainIfNeeded(result, false))
        return;

#ifdef _DEBUG
    // The validation layer implementation expects the application to explicitly synchronize with the GPU
    vkQueueWaitIdle(m_PresentQueue);
#endif
}

CDescriptorManager* CVulkanRenderer::DescMgr() const
{
    return m_DescMgr;
}

uint32_t CVulkanRenderer::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }

    utils::FatalError(g_Engine->Hwnd(), "Failed to find suitable memory type");
    return 0;
}

bool CVulkanRenderer::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (VKRESULT(vkCreateBuffer(m_Device, &bufferInfo, nullptr, &buffer)))
        return utils::FatalError(g_Engine->Hwnd(), "Failed to create buffer");

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_Device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

    if (VKRESULT(vkAllocateMemory(m_Device, &allocInfo, nullptr, &bufferMemory)))
        return utils::FatalError(g_Engine->Hwnd(), "Failed to allocate buffer memory");

    vkBindBufferMemory(m_Device, buffer, bufferMemory, 0);
    return true;
}

void CVulkanRenderer::CopyBuffer(VkBuffer& srcBuffer, VkBuffer& dstBuffer, VkDeviceSize size)
{
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

    VkBufferCopy copyRegion = {};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    EndSingleTimeCommands(commandBuffer);
}

VkCommandBuffer CVulkanRenderer::BeginSingleTimeCommands()
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_CommandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_Device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void CVulkanRenderer::EndSingleTimeCommands(VkCommandBuffer commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_GraphicsQueue);

    vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &commandBuffer);
}

void CVulkanRenderer::PresentQueueWaitIdle()
{
    vkQueueWaitIdle(m_PresentQueue);
}

void CVulkanRenderer::RecreateCommandBuffer()
{
    for (auto& cmd_buff : m_CommandBuffers)
    {
        vkResetCommandBuffer(cmd_buff, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    }

    // #CMD_BUFF_RESET to w koncu free czy reset -.-?
    vkFreeCommandBuffers(m_Device, m_CommandPool, (uint32_t)(m_CommandBuffers.size()), m_CommandBuffers.data());
    CreateCommandBuffers();
}

bool CVulkanRenderer::HasStencilComponent(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

// Extensions
//////////////////////////////////////////////////////////////////////////
bool CVulkanRenderer::CheckAvailableExtensions()
{
    uint32_t extensions_count;
    if (VKRESULT(vkEnumerateInstanceExtensionProperties(nullptr, &extensions_count, nullptr)))
        return utils::FatalError(g_Engine->Hwnd(), L"Could not get the number of Instance extensions");

    std::vector<VkExtensionProperties> availableExtensions(extensions_count);
    if (VKRESULT(vkEnumerateInstanceExtensionProperties(nullptr, &extensions_count, &availableExtensions[0])))
        return utils::FatalError(g_Engine->Hwnd(), L"Could not get the list of Instance extensions");

#ifdef _DEBUG
    LogD("Available extensions:\n");
    for (const auto& extension : availableExtensions)
    {
        LogD(extension.extensionName);
        LogD("\n");
    }
    LogD("-------------------------------------------------------------\n");
#endif

    return true;
}

bool CVulkanRenderer::GetRequiredExtensions()
{
    const char** extensions;
    extensions = glfwGetRequiredInstanceExtensions(&m_InstanceExtCount);

    if (m_InstanceExtCount > 0 && m_InstanceExtCount != UINT32_MAX)
    {
        m_ReqInstanceExt = std::vector<const char*>(extensions, extensions + m_InstanceExtCount);

#ifdef _DEBUG
        m_ReqInstanceExt.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        m_InstanceExtCount++;
        LogD("\\Required extensions:\n");
        LogD("-------------------------------------------------------------\n");
        for (uint32_t i = 0; i < m_InstanceExtCount; i++)
        {
            LogD(m_ReqInstanceExt[i]);
            LogD("\n");
        }
        LogD("-------------------------------------------------------------\n");
#endif

        return true;
    }

    return false;
}

// Debug
//////////////////////////////////////////////////////////////////////////
#ifdef _DEBUG
bool CVulkanRenderer::CheckValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : m_ValidationLayers)
    {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
        {
            return false;
        }
    }

    return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportFn(VkDebugReportFlagsEXT msgFlags,
    VkDebugReportObjectTypeEXT objType, uint64_t srcObject,
    size_t location, int32_t msgCode, const char *pLayerPrefix, const char *pMsg,
    void *pUserData)
{
    std::string str = "Could not create vulkan instance:\n\n";
    str += pMsg;
    LogD(pMsg);
    utils::FatalError(g_Engine->Hwnd(), str.c_str());
    return VK_FALSE;
}

VkResult CVulkanRenderer::CreateDebugReportCallbackEXT(
    const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugReportCallbackEXT* pCallback)
{
    auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(m_Instance, "vkCreateDebugReportCallbackEXT");
    if (func)
    {
        return func(m_Instance, pCreateInfo, pAllocator, pCallback);
    }

    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void CVulkanRenderer::DestroyDebugReportCallbackEXT(VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugReportCallbackEXT");
    if (func)
    {
        func(m_Instance, callback, pAllocator);
    }
}
#endif // #define _DEBUG

// Init Instance
//////////////////////////////////////////////////////////////////////////
bool CVulkanRenderer::InitVkInstance()
{
#ifdef _DEBUG
    if (!CheckValidationLayerSupport())
        return utils::FatalError(g_Engine->Hwnd(), L"Validation layers requested, but not available");
#endif

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext = nullptr;
    appInfo.pApplicationName = WINDOW_TITLE;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 1, 0);
    appInfo.pEngineName = "Qd Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledLayerCount = (uint32_t)(m_ValidationLayers.size());
    createInfo.ppEnabledLayerNames = m_ValidationLayers.data();
    createInfo.enabledExtensionCount = m_InstanceExtCount;
    createInfo.ppEnabledExtensionNames = m_ReqInstanceExt.data();

    if (VKRESULT(vkCreateInstance(&createInfo, nullptr, &m_Instance)))
        return utils::FatalError(g_Engine->Hwnd(), L"Could not create vulkan instance");

#ifdef _DEBUG
    VkDebugReportCallbackCreateInfoEXT createDebugInfo = {};
    createDebugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    createDebugInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    createDebugInfo.pfnCallback = DebugReportFn;
    createDebugInfo.pNext = nullptr;
    createDebugInfo.pUserData = nullptr;

    if (VKRESULT(CreateDebugReportCallbackEXT(&createDebugInfo, nullptr, &m_DebugCallback)))
        return utils::FatalError(g_Engine->Hwnd(), L"Failed to set up debug callback");
#endif

    return true;
}

bool CVulkanRenderer::InitWindowSurface()
{
    if (VKRESULT(glfwCreateWindowSurface(m_Instance, m_Window, nullptr, &m_Surface)))
        return utils::FatalError(g_Engine->Hwnd(), L"Failed to create window surface");

    // glfwCreateWindowSurface - whats going on under this method
    //////////////////////////////////////////////////////////////////////////
    // VkWin32SurfaceCreateInfoKHR createInfo = {};
    // createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    // createInfo.hwnd = glfwGetWin32Window(vkWindow);
    // createInfo.hinstance = GetModuleHandle(nullptr);
    // 
    // auto CreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR");
    // 
    // if (!CreateWin32SurfaceKHR || VKRESULT(CreateWin32SurfaceKHR(vkInstance, &createInfo, nullptr, &surface)) 
    //     return utils::FatalError(g_Engine->GetHwnd(), L"Failed to create window surface");

    return true;
}

// Find Queue Families
//////////////////////////////////////////////////////////////////////////
CVulkanRenderer::QueueFamilyIndices CVulkanRenderer::FindQueueFamilies(VkPhysicalDevice device)
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &presentSupport);

        if (queueFamily.queueCount > 0 && presentSupport)
        {
            indices.presentFamily = i;
        }

        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
        }

        if (indices.IsComplete())
        {
            break;
        }

        i++;
    }

    return indices;
}

// SpawChain
//////////////////////////////////////////////////////////////////////////
CVulkanRenderer::SwapChainSupportDetails CVulkanRenderer::QuerySwapChainSupport(VkPhysicalDevice device)
{
    SwapChainSupportDetails details;

    // Physical device surface capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Surface, &details.capabilities);

    // Physical device surface formats
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, nullptr);

    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, details.formats.data());
    }

    // Physical device surface present modes
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, nullptr);

    if (presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR CVulkanRenderer::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
    {
        return{ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
    }

    for (const auto& availableFormat : availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return availableFormat;
    }

    return availableFormats[0];
}

VkPresentModeKHR CVulkanRenderer::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes)
{
    // #SWAPCHAIN Unfortunately some drivers currently don't properly support VK_PRESENT_MODE_FIFO_KHR, so we should prefer VK_PRESENT_MODE_IMMEDIATE_KHR
    VkPresentModeKHR bestMode = VK_PRESENT_MODE_IMMEDIATE_KHR;

    for (const auto& availablePresentMode : availablePresentModes)
    {
        if (m_EnableTripleBuffering && availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return availablePresentMode;
        }
        else if (availablePresentMode == VK_PRESENT_MODE_FIFO_KHR) 
        {
            bestMode = availablePresentMode;
        }
    }

    return bestMode;
}

VkExtent2D CVulkanRenderer::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    else
    {
        VkExtent2D actualExtent = { WINDOW_WIDTH, WINDOW_HEIGHT };
        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
        return actualExtent;
    }
}

void CVulkanRenderer::RecreateSwapChain()
{
    vkDeviceWaitIdle(m_Device);

    CleanupSwapChain();

    CreateSwapChain();
    CreateImageViews();
    CreateRenderPass();
    g_Engine->TechMgr()->InitTechniques(); // recreate graphics pipelines
    CreateDepthResources();
    CreateFramebuffers();
    CreateCommandBuffers();

    // Update projection matrices
    g_Engine->Camera()->SetPerspectiveProjection(FOV, (float)m_SwapChainExtent.width / (float)m_SwapChainExtent.height, Z_NEAR, Z_FAR);
}

void CVulkanRenderer::CleanupSwapChain()
{
    //#DEPTH
    if (m_DepthImageView)
        vkDestroyImageView(m_Device, m_DepthImageView, nullptr);
    if (m_DepthImage)
        vkDestroyImage(m_Device, m_DepthImage, nullptr);
    if (m_DepthImageMemory)
        vkFreeMemory(m_Device, m_DepthImageMemory, nullptr);

    for (auto framebuffer : m_SwapChainFramebuffersVec)
        vkDestroyFramebuffer(m_Device, framebuffer, nullptr);

    vkFreeCommandBuffers(m_Device, m_CommandPool, (uint32_t)(m_CommandBuffers.size()), m_CommandBuffers.data());

    if (g_Engine->TechMgr())
        g_Engine->TechMgr()->ShutdownTechniques(); // shutdown graphics pipelines

    if (m_RenderPass)
        vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);

    for (auto imageView : m_SwapChainImageViewsVec)
        vkDestroyImageView(m_Device, imageView, nullptr);

    if (m_SwapChain)
        vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);
}

bool CVulkanRenderer::RecreateSwapChainIfNeeded(const VkResult& result, bool allow_suboptimal/* = true*/)
{
    // VK_ERROR_OUT_OF_DATE_KHR: The swap chain has become incompatible with the surface and can no longer be used for rendering.Usually happens after a window resize.
    // VK_SUBOPTIMAL_KHR : The swap chain can still be used to successfully present to the surface, but the surface properties are no longer matched exactly.
    if (result == VK_ERROR_OUT_OF_DATE_KHR || (!allow_suboptimal && result == VK_SUBOPTIMAL_KHR))
    {
        RecreateSwapChain();
        return true;
    }
    else if (result != VK_SUCCESS && (allow_suboptimal && result != VK_SUBOPTIMAL_KHR))
    {
        utils::FatalError(g_Engine->Hwnd(), "Failed to acquire swap chain image");
        return true;
    }
    return false;
}

bool CVulkanRenderer::CreateDescriptorPool()
{
    // Uniform Buffers
    VkDescriptorPoolSize uni_buff_size;
    uni_buff_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uni_buff_size.descriptorCount = 4;
    DescMgr()->RegisterDescPoolSize(uni_buff_size);

    // Dynamic Uniform Buffers
    VkDescriptorPoolSize dyn_uni_buff_size;
    dyn_uni_buff_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    dyn_uni_buff_size.descriptorCount = 4;
    DescMgr()->RegisterDescPoolSize(dyn_uni_buff_size);

    // Image Samplers
    VkDescriptorPoolSize img_sam_size;
    img_sam_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    img_sam_size.descriptorCount = 16;
    DescMgr()->RegisterDescPoolSize(img_sam_size);

    // Create Description Pool
    return DescMgr()->CreateDescriptorPool((uint32_t)EDescSetRole::_COUNT_);
}

bool CVulkanRenderer::CreateGeneralUniformBuffers()
{
    // Create camera Uniform Buffer
    VkDeviceSize camBufferSize = sizeof(SCamUniBuffer);
    if (!CreateBuffer(camBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_CamUniBuffer, m_CamUniBufferMemory))
        return false;

    // Register Buffer in Descriptor Set
    std::vector<VkBuffer> buffers = { m_CamUniBuffer };
    std::vector<size_t> sizes = { camBufferSize };
    if (!g_Engine->Renderer()->DescMgr()->RegisterDescriptor(
        buffers, sizes,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT,
        (uint32_t)EDescSetRole::GENERAL,
        g_Engine->Renderer()->DescMgr()->GetNextFreeLocationId((uint32_t)EDescSetRole::GENERAL)))
        return false;

    return true;
}

bool CVulkanRenderer::CreateTechsRenderObjects()
{
    // Create techs UniBuffs and Images
    for (int i = 0; i < g_Engine->TechMgr()->TechniquesCount(); i++)
    {
        auto tech = g_Engine->TechMgr()->GetTechnique(i);
        if (!tech)
            continue;

        if (!tech->CreateRenderObjects())
            return false;
    }
    return true;
}

void CVulkanRenderer::DestroyTechsRenderObjects()
{
    // Create techs UniBuffs and Images
    for (int i = 0; i < g_Engine->TechMgr()->TechniquesCount(); i++)
    {
        auto tech = g_Engine->TechMgr()->GetTechnique(i);
        if (!tech)
            continue;

        tech->DestroyRenderObjects();
    }
}

bool CVulkanRenderer::CreateDepthResources()
{
    VkFormat depthFormat = FindDepthFormat();
    image_utils::CreateImage(m_SwapChainExtent.width, m_SwapChainExtent.height, 1, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DepthImage, m_DepthImageMemory);
    m_DepthImageView = image_utils::CreateImageView(m_DepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
    image_utils::TransitionImageLayout(m_DepthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
    return true;
}

VkFormat CVulkanRenderer::FindDepthFormat()
{
    return image_utils::FindSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

bool CVulkanRenderer::SubmitDrawCommands(const uint32_t& imageIndex, VkSubmitInfo& submitInfo)
{
    // Submitting the command buffer
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    // The first three parameters specify which semaphores to wait on before execution begins and in which stage(s) of the pipeline to wait. 
    VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_CommandBuffers[imageIndex];

    // The signalSemaphoreCount and pSignalSemaphores parameters specify which semaphores to signal once the command buffer(s) have finished execution
    VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphore };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (VKRESULT(vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE)))    
        return utils::FatalError(g_Engine->Hwnd(), "Failed to submit draw command buffer");

    return true;
}

void CVulkanRenderer::FetchDeviceProperties()
{
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(g_Engine->Renderer()->GetPhysicalDevice(), &props);

    m_MinUniformBufferOffsetAlignment = props.limits.minUniformBufferOffsetAlignment;
}

// bool CVulkanRenderer::CreatePipelineCache()
// {
//     VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
//     pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
//     VK_CHECK_RESULT(vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &pipelineCache));
// }

bool CVulkanRenderer::CreateSwapChain()
{
    SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(m_PhysicalDevice);
    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
    m_SwapChainExtent = ChooseSwapExtent(swapChainSupport.capabilities);

    // Get swapchain size
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount;

    if (m_EnableTripleBuffering && swapChainSupport.capabilities.minImageCount < 3)
        imageCount = 3;

    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
        imageCount = swapChainSupport.capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_Surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = m_SwapChainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // VK_IMAGE_USAGE_TRANSFER_DST_BIT - for render to back buffer, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT- to render directly to swap chain
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE; // clipps invisible pixels (for example because another window is in front of them)
    createInfo.oldSwapchain = VK_NULL_HANDLE; // old swapchain pointer (for example when you resize window and need to create new swap chain)

    QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);
    uint32_t queueFamilyIndices[] = { (uint32_t)indices.graphicsFamily, (uint32_t)indices.presentFamily };

    if (indices.graphicsFamily != indices.presentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    if (VKRESULT(vkCreateSwapchainKHR(m_Device, &createInfo, nullptr, &m_SwapChain)))
        return utils::FatalError(g_Engine->Hwnd(), L"Failed to create swap chain");

    vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &imageCount, nullptr);
    m_SwapChainImagesVec.resize(imageCount);
    vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &imageCount, m_SwapChainImagesVec.data());

    m_SwapChainImageFormat = surfaceFormat.format;

    return true;
}

// ImageViews
//////////////////////////////////////////////////////////////////////////
bool CVulkanRenderer::CreateImageViews()
{
    m_SwapChainImageViewsVec.resize(m_SwapChainImagesVec.size());
    for (uint32_t i = 0; i < m_SwapChainImagesVec.size(); i++)
    {
        m_SwapChainImageViewsVec[i] = image_utils::CreateImageView(m_SwapChainImagesVec[i], m_SwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        if (m_SwapChainImageViewsVec[i] == nullptr)
            return false;
    }
    return true;
}

// Physical device handle
//////////////////////////////////////////////////////////////////////////
bool CVulkanRenderer::CheckDeviceExtensionSupport(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> tmpExt(m_ReqDeviceExt.begin(), m_ReqDeviceExt.end());
    for (const auto& extension : availableExtensions)
    {
        tmpExt.erase(extension.extensionName);
    }

    return tmpExt.empty();
}

int CVulkanRenderer::RateDeviceSuitability(VkPhysicalDevice device)
{
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    int score = 0;

    // Discrete GPUs have a significant performance advantage
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        score += 1000;

    // Maximum possible size of textures affects graphics quality
    score += deviceProperties.limits.maxImageDimension2D;

    // Application can't function without geometry shaders
    if (!deviceFeatures.geometryShader)
        return 0;

    // Application can't function without anisotropy filtering
    if (!deviceFeatures.samplerAnisotropy)
        return 0;

    // Check if all required extensions are supported
    if (!CheckDeviceExtensionSupport(device))
        return 0;

    SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device);
    if (swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty())
        return 0;

#ifdef _DEBUG
    LogD(deviceProperties.deviceName);
    LogD(": ");
    LogD(score);
    LogD("\n");
#endif

    return score;
}

bool CVulkanRenderer::PickPhysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);

    if (deviceCount == 0)
        return utils::FatalError(g_Engine->Hwnd(), "Failed to find GPUs with Vulkan support");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());

#ifdef _DEBUG
    LogD("Compatible physical devices:\n")
        LogD("-------------------------------------------------------------\n");
#endif

    std::pair<int, int> best(0, -1);
    for (int i = 0; i < devices.size(); i++)
    {
        int tmp = RateDeviceSuitability(devices[i]);
        if (tmp > best.second)
        {
            best.first = i;
            best.second = tmp;
        }
    }

#ifdef _DEBUG
    LogD("-------------------------------------------------------------\n");
#endif


    if (best.second > 0 && FindQueueFamilies(devices[best.first]).IsComplete())
    {
        m_PhysicalDevice = devices[best.first];
    }

    if (m_PhysicalDevice == VK_NULL_HANDLE)
        return utils::FatalError(g_Engine->Hwnd(), "Failed to find a suitable GPU");

    FetchDeviceProperties();

    return true;
}

bool CVulkanRenderer::CreateRenderPass()
{
    // Attachment description
    //////////////////////////////////////////////////////////////////////////
    std::array<VkAttachmentDescription, 3> attachments;

    // Color attachment
    attachments[0] = {};
    attachments[0].format = m_SwapChainImageFormat;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // Depth attachment - first subpass
    attachments[1] = {};
    attachments[1].format = FindDepthFormat();
    attachments[1].flags = VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Depth attachment - second subpass //#DEPTH mozliwe ze nie ebdzie potrzebne, wszystko zalezy od tego czy bede potrzebowac depth buffer w drugim subpassie
    attachments[2] = {};
    attachments[2].format = FindDepthFormat();
    attachments[2].flags = VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT;
    attachments[2].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[2].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachments[2].finalLayout = VK_IMAGE_LAYOUT_GENERAL;

    // Attachments references
    //////////////////////////////////////////////////////////////////////////
    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef_first = {};
    depthAttachmentRef_first.attachment = 1;
    depthAttachmentRef_first.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef_second = {};
    depthAttachmentRef_second.attachment = 1;
    depthAttachmentRef_second.layout = VK_IMAGE_LAYOUT_GENERAL; // If we want to use depth buffer as input and output in the same time, It attachment references need to be palced in the same layout

    // Input references
    VkAttachmentReference depthInputAttachmentRef = {};
    depthInputAttachmentRef.attachment = 2;
    depthInputAttachmentRef.layout = VK_IMAGE_LAYOUT_GENERAL;

    // Subpasses description
    //////////////////////////////////////////////////////////////////////////
    std::array<VkSubpassDescription, 2> subpasses;

    // Render objects
    subpasses[0] = {};
    subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // Vulkan may also support compute subpasses
    subpasses[0].colorAttachmentCount = 1; // The index of the attachment in this array is directly referenced from the fragment shader with the layout(location = 0) out vec4 outColor directive!
    subpasses[0].pColorAttachments = &colorAttachmentRef;
    subpasses[0].pDepthStencilAttachment = &depthAttachmentRef_first;

    // Particles
    subpasses[1] = {};
    subpasses[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[1].colorAttachmentCount = 1;
    subpasses[1].pColorAttachments = &colorAttachmentRef;
    subpasses[1].pDepthStencilAttachment = &depthAttachmentRef_second; //#DEPTH
    subpasses[1].inputAttachmentCount = 1;
    subpasses[1].pInputAttachments = &depthInputAttachmentRef;

    // Subpass dependencies
    //////////////////////////////////////////////////////////////////////////
    std::array<VkSubpassDependency, 2> dependencies;

    dependencies[0] = {};
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = 0;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    dependencies[1] = {};
    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = 1;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].srcAccessMask = 0;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    // Render pass
    //////////////////////////////////////////////////////////////////////////
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = (uint32_t)(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = (uint32_t)(subpasses.size());
    renderPassInfo.pSubpasses = subpasses.data();
    renderPassInfo.dependencyCount = (uint32_t)(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    if (vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_RenderPass) != VK_SUCCESS) 
        return utils::FatalError(g_Engine->Hwnd(), "Failed to create render pass");

    // Store subpasses number
    m_SubpassesCount = (uint32_t)(subpasses.size());

    return true;
}

bool CVulkanRenderer::CreateFramebuffers()
{
    m_SwapChainFramebuffersVec.resize(m_SwapChainImageViewsVec.size());
    for (size_t i = 0; i < m_SwapChainImageViewsVec.size(); i++)
    {
        std::array<VkImageView, 3> attachments = 
        {
            m_SwapChainImageViewsVec[i],
            m_DepthImageView,
            m_DepthImageView //#DEPTH
        };

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_RenderPass;
        framebufferInfo.attachmentCount = (uint32_t)(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = m_SwapChainExtent.width;
        framebufferInfo.height = m_SwapChainExtent.height;
        framebufferInfo.layers = 1;

        if (VKRESULT(vkCreateFramebuffer(m_Device, &framebufferInfo, nullptr, &m_SwapChainFramebuffersVec[i])))
            return utils::FatalError(g_Engine->Hwnd(), "Failed to create framebuffer");
    }

    return true;
}

bool CVulkanRenderer::CreateCommandPool()
{
    QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(m_PhysicalDevice);

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // default: 0 // #RESET_CMD_BUFF

    if (VKRESULT(vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool)))
        return utils::FatalError(g_Engine->Hwnd(), "Failed to create command pool");

    return true;
}

bool CVulkanRenderer::CreateCommandBuffers()
{
    // Command buffer allocation
    m_CommandBuffers.resize(m_SwapChainFramebuffersVec.size());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_CommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)m_CommandBuffers.size();

    // allocInfo.level:
    // VK_COMMAND_BUFFER_LEVEL_PRIMARY: Can be submitted to a queue for execution, but cannot be called from other command buffers.
    // VK_COMMAND_BUFFER_LEVEL_SECONDARY : Cannot be submitted directly, but can be called from primary command buffers.

    if (VKRESULT(vkAllocateCommandBuffers(m_Device, &allocInfo, m_CommandBuffers.data())))
        return utils::FatalError(g_Engine->Hwnd(), "failed to allocate command buffers!");

    // Starting command buffer recording
    for (size_t i = 0; i < m_CommandBuffers.size(); i++) 
    {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (VKRESULT(vkBeginCommandBuffer(m_CommandBuffers[i], &beginInfo)))
            return utils::FatalError(g_Engine->Hwnd(), "Failed to begin recording command buffer");

        // Starting a render pass
        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_RenderPass;
        renderPassInfo.framebuffer = m_SwapChainFramebuffersVec[i];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = m_SwapChainExtent;

        std::array<VkClearValue, 2> clearValues = {};
        clearValues[0].color = { 0.45f, 0.45f, 0.45f, 1.0f };
        clearValues[1].depthStencil = { 1.0f, 0 };
        renderPassInfo.clearValueCount = (uint32_t)(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        uint32_t current_subpass = 0;
        vkCmdBeginRenderPass(m_CommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
      
        // First subpass - render objects
        if (g_Engine->ObjectControl())
        {
            g_Engine->ObjectControl()->RecordCommandBuffer(m_CommandBuffers[i]);
        }

        // Second subpass - particles
        if (g_Engine->ParticleMgr())
        {
            current_subpass++;
            vkCmdNextSubpass(m_CommandBuffers[i], VK_SUBPASS_CONTENTS_INLINE);
            g_Engine->ParticleMgr()->RecordCommandBuffer(m_CommandBuffers[i]);
        }
        
        // The current subpass index must be equal to the number of subpasses in the render pass minus one
        while (current_subpass < m_SubpassesCount - 1)
        {
            current_subpass++;
            vkCmdNextSubpass(m_CommandBuffers[i], VK_SUBPASS_CONTENTS_INLINE);
        }

        vkCmdEndRenderPass(m_CommandBuffers[i]);

        if (VKRESULT(vkEndCommandBuffer(m_CommandBuffers[i])))
            return utils::FatalError(g_Engine->Hwnd(), "Failed to record command buffer");
    }
    
    return true;
}

bool CVulkanRenderer::CreateSemaphores()
{
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    if (VKRESULT(vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphore)) ||
        VKRESULT(vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphore)))
        return utils::FatalError(g_Engine->Hwnd(), "Failed to create semaphores");

    return true;
}

// Logical device handle
//////////////////////////////////////////////////////////////////////////
bool CVulkanRenderer::CreateLogicalDevice()
{
    QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<int> uniqueQueueFamilies = { indices.graphicsFamily, indices.presentFamily };

    float queuePriority = 1.0f;
    for (int queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.geometryShader = true;
    deviceFeatures.logicOp = true; // blending logical operations
    deviceFeatures.samplerAnisotropy = true;
    deviceFeatures.textureCompressionBC = true;

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = (uint32_t)(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = (uint32_t)(m_ReqDeviceExt.size());
    createInfo.ppEnabledExtensionNames = m_ReqDeviceExt.data();

#ifdef _DEBUG
    createInfo.enabledLayerCount = (uint32_t)(m_ValidationLayers.size());
    createInfo.ppEnabledLayerNames = m_ValidationLayers.data();
#else
    createInfo.enabledLayerCount = 0;
#endif

    if (VKRESULT(vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device)))
        return utils::FatalError(g_Engine->Hwnd(), "Failed to create logical device");

    // Retrieving queue handles
    vkGetDeviceQueue(m_Device, indices.graphicsFamily, 0, &m_GraphicsQueue);
    vkGetDeviceQueue(m_Device, indices.presentFamily, 0, &m_PresentQueue);

    return true;
}