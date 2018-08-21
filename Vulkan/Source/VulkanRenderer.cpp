#include "stdafx.h"
#include "VulkanRenderer.h"
#include "Objects/GBaseObject.h"
#include "Techs/TechniqueManager.h"
#include "Utils/ImageUtils.h"

CVulkanRenderer::CVulkanRenderer(GLFWwindow* window)
    : m_Window(window)
{
}

CVulkanRenderer::~CVulkanRenderer()
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

        // #UNI_BUFF
        if (!CreateDescriptorSetLayout())
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

        if (!CreateUniformBuffers())
            return Shutdown();

        if (!CreateDescriptorPool())
            return Shutdown();

        if (!CreateDescriptorSet())
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

    //#UNI_BUFF
    //////////////////////////////////////////////////////////////////////////
    if (m_DescriptorSetLayout)
        vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);

    if (m_CamUniBuffer)
        vkDestroyBuffer(m_Device, m_CamUniBuffer, nullptr);

    if (m_CamUniBufferMemory)
        vkFreeMemory(m_Device, m_CamUniBufferMemory, nullptr);

    DestroyTechsRenderObjects();

    if (m_DescriptorPool)
        vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
    //////////////////////////////////////////////////////////////////////////

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

    vkFreeCommandBuffers(m_Device, m_CommandPool, static_cast<uint32_t>(m_CommandBuffers.size()), m_CommandBuffers.data());
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
        for (uint i = 0; i < m_InstanceExtCount; i++)
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
    createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
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

    vkFreeCommandBuffers(m_Device, m_CommandPool, static_cast<uint32_t>(m_CommandBuffers.size()), m_CommandBuffers.data());

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

// #DESC_SET dorobic manager do descriptor setów, oddzielny set jako main, oddzielny dla obiektów i oddzielny dla particli, ogólnie posprz¹taæ tu
bool CVulkanRenderer::CreateDescriptorSetLayout()
{
    uint binding_offset = 0;
    std::vector<VkDescriptorSetLayoutBinding> bindings;

    // Cam uni buff layout
    VkDescriptorSetLayoutBinding uboLayoutBindingCam = {};
    uboLayoutBindingCam.binding = binding_offset++;
    uboLayoutBindingCam.descriptorCount = 1; // #DESC_SET bardzo wazne, mozna dzieki temu przekazywaæ np array obiektów danego typu, moze zamist volumetrycznej tekstury to to u¿yæ?
    uboLayoutBindingCam.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBindingCam.pImmutableSamplers = nullptr;
    uboLayoutBindingCam.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT;
    bindings.push_back(uboLayoutBindingCam);

    // Image buff desc
    for (int i = 0; i < g_Engine->TechMgr()->TechniquesCount(); i++)
    {
        auto tech = g_Engine->TechMgr()->GetTechnique(i);
        if (!tech)
            continue;

        std::vector<ITechnique::TImgSampler> img_sampler_pairs;
        tech->GetImageSamplerPairs(img_sampler_pairs);
        for (const auto& p : img_sampler_pairs)
        {
            VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
            samplerLayoutBinding.binding = binding_offset++;
            samplerLayoutBinding.descriptorCount = 1;
            samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            samplerLayoutBinding.pImmutableSamplers = nullptr;
            samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            bindings.push_back(samplerLayoutBinding);
        }
    }

    // Techs buff desc
    for (int i = 0; i < g_Engine->TechMgr()->TechniquesCount(); i++)
    {
        auto tech = g_Engine->TechMgr()->GetTechnique(i);
        if (!tech || tech->GetSingleUniBuffObjSize() == 0)
            continue;

        VkDescriptorSetLayoutBinding uboLayoutBindingObj = {};
        uboLayoutBindingObj.binding = binding_offset++;
        uboLayoutBindingObj.descriptorCount = 1;
        uboLayoutBindingObj.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC; //VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBindingObj.pImmutableSamplers = nullptr;
        uboLayoutBindingObj.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        bindings.push_back(uboLayoutBindingObj);
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (VKRESULT(vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &m_DescriptorSetLayout))) 
        return utils::FatalError(g_Engine->Hwnd(), "Failed to create descriptor set layout");

    return true;
}

// #DESC_SET wszsytko zwi¹zane z desc set od przepisania
bool CVulkanRenderer::CreateDescriptorPool()
{

//     poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//     poolSizes[0].descriptorCount = 1;
//     poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;//VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//     poolSizes[1].descriptorCount = 1;
//     poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//     poolSizes[2].descriptorCount = 1;

    // #DESC_SET wszsytko zwi¹zane z desc set od przepisania
    //////////////////////////////////////////////////////////////////////////
    std::vector<VkDescriptorPoolSize> poolSizes;

    VkDescriptorPoolSize cam_ubo_pl;
    cam_ubo_pl.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    cam_ubo_pl.descriptorCount = 1;
    poolSizes.push_back(cam_ubo_pl);

    // Image buff desc
    for (int i = 0; i < g_Engine->TechMgr()->TechniquesCount(); i++)
    {
        auto tech = g_Engine->TechMgr()->GetTechnique(i);
        if (!tech)
            continue;

        std::vector<ITechnique::TImgSampler> img_sampler_pairs;
        tech->GetImageSamplerPairs(img_sampler_pairs);
        for (const auto& p : img_sampler_pairs)
        {
            VkDescriptorPoolSize img_pl;
            img_pl.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            img_pl.descriptorCount = 1;
            poolSizes.push_back(img_pl);
        }
    }

    // Techs buff desc
    for (int i = 0; i < g_Engine->TechMgr()->TechniquesCount(); i++)
    {
        auto tech = g_Engine->TechMgr()->GetTechnique(i);
        if (!tech || tech->GetSingleUniBuffObjSize() == 0)
            continue;

        VkDescriptorPoolSize ubo_pl;
        ubo_pl.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        ubo_pl.descriptorCount = 1;
        poolSizes.push_back(ubo_pl);
    }
    //////////////////////////////////////////////////////////////////////////

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 1;

    if (VKRESULT(vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_DescriptorPool)))
        return utils::FatalError(g_Engine->Hwnd(), "Failed to create descriptor pool");

    return true;
}

// #DESC_SET dorobic manager do descriptor setów, oddzielny set jako main, oddzielny dla obiektów i oddzielny dla particli, ogólnie posprz¹taæ tu
bool CVulkanRenderer::CreateDescriptorSet() //tomek kaczo-dupka
{
    VkDescriptorSetLayout layouts[] = { m_DescriptorSetLayout };
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_DescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = layouts;

    if (VKRESULT(vkAllocateDescriptorSets(m_Device, &allocInfo, &m_DescriptorSet)))
        return utils::FatalError(g_Engine->Hwnd(), "Failed to allocate descriptor set");

    // #UNI_BUFF Nie potrzeba
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(g_Engine->Renderer()->GetPhysicalDevice(), &props);

    size_t minUboAlignment = props.limits.minUniformBufferOffsetAlignment;

    uint32_t offsets2[2];
    offsets2[0] = 0;
    offsets2[1] = sizeof(SCamUniBuffer);

    if (minUboAlignment > 0) //kurza stopka
    {
        offsets2[0] = (offsets2[0] + minUboAlignment - 1) & ~(minUboAlignment - 1);
        offsets2[1] = (offsets2[1] + minUboAlignment - 1) & ~(minUboAlignment - 1);
    }

    // Cam UniBuff desc
    VkDescriptorBufferInfo camBufferInfo = {};  
    //kaczko-kwarczenie

    camBufferInfo.buffer = m_CamUniBuffer;
    camBufferInfo.offset = 0;// offsets2[0];
    camBufferInfo.range = sizeof(SCamUniBuffer);

    // Image buff desc
    std::vector<VkDescriptorImageInfo> techImageInfo;
    for (int i = 0; i < g_Engine->TechMgr()->TechniquesCount(); i++)
    {
        auto tech = g_Engine->TechMgr()->GetTechnique(i);
        if (!tech)
            continue;

        std::vector<ITechnique::TImgSampler> img_sampler_pairs;
        tech->GetImageSamplerPairs(img_sampler_pairs);
        for (const auto& p : img_sampler_pairs)
        {
            VkDescriptorImageInfo imageInfo = {};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = p.first;
            imageInfo.sampler = p.second;
            techImageInfo.push_back(imageInfo);
        }
    }

    // Techs buff desc
    std::vector<VkDescriptorBufferInfo> techBuffInfoVec;
    techBuffInfoVec.reserve(g_Engine->TechMgr()->TechniquesCount());
    for (int i = 0; i < g_Engine->TechMgr()->TechniquesCount(); i++)
    {
        auto tech = g_Engine->TechMgr()->GetTechnique(i);
        if (!tech || tech->GetSingleUniBuffObjSize() == 0)
            continue;

        VkDescriptorBufferInfo objBufferInfo = {};
        objBufferInfo.buffer = tech->UniBuffer();
        objBufferInfo.offset = 0;
        objBufferInfo.range = tech->GetSingleUniBuffObjSize();
        techBuffInfoVec.push_back(objBufferInfo);
    }

    std::vector<VkWriteDescriptorSet> descriptorWrites;
    descriptorWrites.resize(1 + techImageInfo.size() + techBuffInfoVec.size());
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = m_DescriptorSet;
    descriptorWrites[0].dstBinding = 0; //#UNI_BUFF bindings
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &camBufferInfo;

    uint offset = 1;
    for (int i = 0; i < techImageInfo.size(); i++)
    {
        descriptorWrites[i + offset].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[i + offset].dstSet = m_DescriptorSet;
        descriptorWrites[i + offset].dstBinding = i + offset;
        descriptorWrites[i + offset].dstArrayElement = 0;
        descriptorWrites[i + offset].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[i + offset].descriptorCount = 1;
        descriptorWrites[i + offset].pImageInfo = &techImageInfo[i];
    }
    offset += techImageInfo.size();

    for (int i = 0; i < techBuffInfoVec.size(); i++)
    {
        descriptorWrites[i + offset].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[i + offset].dstSet = m_DescriptorSet;
        descriptorWrites[i + offset].dstBinding = i + offset;
        descriptorWrites[i + offset].dstArrayElement = 0;
        descriptorWrites[i + offset].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;// VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[i + offset].descriptorCount = 1;
        descriptorWrites[i + offset].pBufferInfo = &techBuffInfoVec[i];
    }

    vkUpdateDescriptorSets(m_Device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    return true;
}

bool CVulkanRenderer::CreateUniformBuffers()
{
    // Create cam uni buff
    VkDeviceSize camBufferSize = sizeof(SCamUniBuffer);
    if (!CreateBuffer(camBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_CamUniBuffer, m_CamUniBufferMemory))
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
    image_utils::CreateImage(m_SwapChainExtent.width, m_SwapChainExtent.height, 1, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DepthImage, m_DepthImageMemory);
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
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = m_SwapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = FindDepthFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Subpasses and attachment references
    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // Vulkan may also support compute subpasses
    subpass.colorAttachmentCount = 1; // The index of the attachment in this array is directly referenced from the fragment shader with the layout(location = 0) out vec4 outColor directive!
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency = {}; // #TODO Read about it https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Rendering_and_presentation
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    // Render pass
    std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_RenderPass) != VK_SUCCESS) 
        return utils::FatalError(g_Engine->Hwnd(), "Failed to create render pass");

    return true;
}

bool CVulkanRenderer::CreateFramebuffers()
{
    m_SwapChainFramebuffersVec.resize(m_SwapChainImageViewsVec.size());
    for (size_t i = 0; i < m_SwapChainImageViewsVec.size(); i++)
    {
        std::array<VkImageView, 2> attachments = 
        {
            m_SwapChainImageViewsVec[i],
            m_DepthImageView
        };

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_RenderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
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
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        // Record
        vkCmdBeginRenderPass(m_CommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        g_Engine->RecordCommandBuffer(m_CommandBuffers[i]);
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
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(m_ReqDeviceExt.size());
    createInfo.ppEnabledExtensionNames = m_ReqDeviceExt.data();

#ifdef _DEBUG
    createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
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