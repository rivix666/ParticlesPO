#include "stdafx.h"
#include "VulkanRenderer.h"
#include "Objects/GBaseObject.h"
#include "Techs/ShaderUtils.h"
#include "Techs/BaseTechnique.h"
#include "Techs/TechniqueManager.h"

#pragma optimize("", off)

//#IMAGES
#define STB_IMAGE_IMPLEMENTATION
#include <misc/stb_image.h>

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

        if (!InitTechniqueManager()) // #TECH przemyslec czy to dobry pomysl bo po co w sumie rejestrowac jak i tak wszyskto powstanei w jednym miejscu, chyba ze rozwiazanie hybrydowe?
            return Shutdown();

        if (!CreateCommandPool())
            return Shutdown();

        //#DEPTH
        if (!CreateDepthResources())
            return Shutdown();

        if (!CreateFramebuffers())
            return Shutdown();

        //#IMAGES
        if (!CreateTextureImage())
            return Shutdown();

        if (!CreateTextureImageView())
            return Shutdown();

        if (!CreateTextureSampler())
            return Shutdown();

//         if (!CreateVertexBuffer()) // #TECH to samo z bufforami trzeba to przemyslec jeszcze
//             return Shutdown();


        // #UNI_BUFF
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


    //#IMAGES
    if (m_TextureSampler)
        vkDestroySampler(m_Device, m_TextureSampler, nullptr);
    if (m_TextureImageView)
        vkDestroyImageView(m_Device, m_TextureImageView, nullptr);
    if (m_TextureImage)
        vkDestroyImage(m_Device, m_TextureImage, nullptr);
    if (m_TextureImageMemory)
        vkFreeMemory(m_Device, m_TextureImageMemory, nullptr);

    //#UNI_BUFF
    //////////////////////////////////////////////////////////////////////////
    if (m_DescriptorSetLayout)
        vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);

    if (m_CamUniBuffer)
        vkDestroyBuffer(m_Device, m_CamUniBuffer, nullptr);

    if (m_CamUniBufferMemory)
        vkFreeMemory(m_Device, m_CamUniBufferMemory, nullptr);

    if (m_DescriptorPool)
        vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
    //////////////////////////////////////////////////////////////////////////

    // Shutdown Tech Mgr
    if (m_TechMgr)
    {
        m_TechMgr->Shutdown();
        DELETE(m_TechMgr);
    }

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

void CVulkanRenderer::PresentQueueWaitIdle()
{
    vkQueueWaitIdle(m_PresentQueue);
}

void CVulkanRenderer::RecreateCommandBuffer()
{
    // #CMD_BUFF In the future think about vkResetCommandBuffer() method
    for (auto& cmd_buff : m_CommandBuffers)
    {
        vkResetCommandBuffer(cmd_buff, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
    }

    vkFreeCommandBuffers(m_Device, m_CommandPool, static_cast<uint32_t>(m_CommandBuffers.size()), m_CommandBuffers.data());
    CreateCommandBuffers();
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
    m_TechMgr->InitTechniques(); // recreate graphics pipelines
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

    if (m_TechMgr)
        m_TechMgr->ShutdownTechniques(); // shutdown graphics pipelines

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

bool CVulkanRenderer::CreateDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding uboLayoutBindingCam = {};
    uboLayoutBindingCam.binding = 0;
    uboLayoutBindingCam.descriptorCount = 1;
    uboLayoutBindingCam.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBindingCam.pImmutableSamplers = nullptr;
    uboLayoutBindingCam.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding uboLayoutBindingObj = {};
    uboLayoutBindingObj.binding = 1;
    uboLayoutBindingObj.descriptorCount = 1;
    uboLayoutBindingObj.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC; //VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBindingObj.pImmutableSamplers = nullptr;
    uboLayoutBindingObj.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
    samplerLayoutBinding.binding = 2;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 3> bindings = { uboLayoutBindingCam, uboLayoutBindingObj, samplerLayoutBinding };
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (VKRESULT(vkCreateDescriptorSetLayout(m_Device, &layoutInfo, nullptr, &m_DescriptorSetLayout))) 
        return utils::FatalError(g_Engine->Hwnd(), "Failed to create descriptor set layout");

    return true;
}

bool CVulkanRenderer::CreateUniformBuffers()
{
    bool result = true;

    // Create cam uni buff
    VkDeviceSize camBufferSize = sizeof(SCamUniBuffer);
    result = result && CreateBuffer(camBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_CamUniBuffer, m_CamUniBufferMemory);

    // Create techs uni buffs
    for (int i = 0; i < m_TechMgr->TechniquesCount(); i++)
    {
        auto tech = m_TechMgr->GetTechnique(i);
        if (!tech || tech->GetSingleUniBuffObjSize() == 0)
            continue;

        result = result && tech->CreateUniBuffers();
    }

    return result;
}

bool CVulkanRenderer::CreateDescriptorPool()
{
    std::array<VkDescriptorPoolSize, 3> poolSizes = {};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = 1;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;//VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[1].descriptorCount = 1;
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[2].descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 1;

    if (VKRESULT(vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_DescriptorPool)))
        return utils::FatalError(g_Engine->Hwnd(), "Failed to create descriptor pool");

    return true;
}

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
    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = m_TextureImageView;
    imageInfo.sampler = m_TextureSampler;

    // Techs buff desc
    std::vector<VkDescriptorBufferInfo> techBuffInfoVec;
    techBuffInfoVec.reserve(m_TechMgr->TechniquesCount());
    for (int i = 0; i < m_TechMgr->TechniquesCount(); i++)
    {
        auto tech = m_TechMgr->GetTechnique(i);
        if (!tech || tech->GetSingleUniBuffObjSize() == 0)
            continue;

        VkDescriptorBufferInfo objBufferInfo = {};
        objBufferInfo.buffer = tech->BaseObjUniBuffer();
        objBufferInfo.offset = 0;
        objBufferInfo.range = tech->GetSingleUniBuffObjSize();
        techBuffInfoVec.push_back(objBufferInfo);
    }

    //#UNI_BUFF czy zadziala?
    //std::array<VkWriteDescriptorSet, size> descriptorWrites = {};
    std::vector<VkWriteDescriptorSet> descriptorWrites;
    descriptorWrites.resize(2 + techBuffInfoVec.size());
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = m_DescriptorSet;
    descriptorWrites[0].dstBinding = 0; //#UNI_BUFF bindings
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &camBufferInfo;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = m_DescriptorSet;
    descriptorWrites[1].dstBinding = 2;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pImageInfo = &imageInfo;

    for (int i = 0; i < techBuffInfoVec.size(); i++)
    {
        descriptorWrites[i + 2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[i + 2].dstSet = m_DescriptorSet;
        descriptorWrites[i + 2].dstBinding = 1;
        descriptorWrites[i + 2].dstArrayElement = 0;
        descriptorWrites[i + 2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;// VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[i + 2].descriptorCount = 1;
        descriptorWrites[i + 2].pBufferInfo = &techBuffInfoVec[i];
    }

    vkUpdateDescriptorSets(m_Device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    return true;
}

bool CVulkanRenderer::CreateTextureImage() //#IMAGES image mgr??
{
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load("Images/Other/ground.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) 
        return utils::FatalError(g_Engine->Hwnd(), "Failed to load texture image");

    //#MIPMAPS
    m_MipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(m_Device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(m_Device, stagingBufferMemory);

    stbi_image_free(pixels);

    CreateImage(texWidth, texHeight, m_MipLevels, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_TextureImage, m_TextureImageMemory);

    TransitionImageLayout(m_TextureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_MipLevels);
    CopyBufferToImage(stagingBuffer, m_TextureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    //transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps // #MIPMAPS

    vkDestroyBuffer(m_Device, stagingBuffer, nullptr);
    vkFreeMemory(m_Device, stagingBufferMemory, nullptr);

    GenerateMipmaps(m_TextureImage, texWidth, texHeight, m_MipLevels);

    return true;
}

bool CVulkanRenderer::CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (VKRESULT(vkCreateImage(m_Device, &imageInfo, nullptr, &image)))
        return utils::FatalError(g_Engine->Hwnd(), "Failed to create image");
    
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_Device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

    if (VKRESULT(vkAllocateMemory(m_Device, &allocInfo, nullptr, &imageMemory)))
        return utils::FatalError(g_Engine->Hwnd(), "Failed to allocate image memory");
    
    vkBindImageMemory(m_Device, image, imageMemory, 0);
}

void CVulkanRenderer::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels)
{
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) 
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (HasStencilComponent(format))
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    else 
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) 
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) 
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else
    {
        utils::FatalError(g_Engine->Hwnd(), "Unsupported layout transition");
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    EndSingleTimeCommands(commandBuffer);
}

void CVulkanRenderer::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = 
    {
        width,
        height,
        1
    };

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

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

bool CVulkanRenderer::CreateTextureImageView()
{
    m_TextureImageView = CreateImageView(m_TextureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, m_MipLevels);
    return m_TextureImageView != nullptr;
}

VkImageView CVulkanRenderer::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
{
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (VKRESULT(vkCreateImageView(m_Device, &viewInfo, nullptr, &imageView)))
    {
        utils::FatalError(g_Engine->Hwnd(), "Failed to create texture image view");
        return nullptr;
    }

    return imageView;
}

bool CVulkanRenderer::CreateTextureSampler()
{
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.minLod = 0; // Optional
    samplerInfo.maxLod = static_cast<float>(m_MipLevels);
    samplerInfo.mipLodBias = 0; // Optional

    if (VKRESULT(vkCreateSampler(m_Device, &samplerInfo, nullptr, &m_TextureSampler)))
        return utils::FatalError(g_Engine->Hwnd(), "Failed to create texture sampler");

    return true;
}

void CVulkanRenderer::GenerateMipmaps(VkImage image, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
{
    VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;

    for (uint32_t i = 1; i < mipLevels; i++) 
    {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        VkImageBlit blit = {};
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { mipWidth / 2, mipHeight / 2, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(commandBuffer,
            image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        if (mipWidth > 1) 
            mipWidth /= 2;
        if (mipHeight > 1) 
            mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

    EndSingleTimeCommands(commandBuffer);
}

bool CVulkanRenderer::CreateDepthResources()
{
    VkFormat depthFormat = FindDepthFormat();
    CreateImage(m_SwapChainExtent.width, m_SwapChainExtent.height, 1, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DepthImage, m_DepthImageMemory);
    m_DepthImageView = CreateImageView(m_DepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
    TransitionImageLayout(m_DepthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
    return true;
}

VkFormat CVulkanRenderer::FindDepthFormat()
{
    return FindSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

VkFormat CVulkanRenderer::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (VkFormat format : candidates) 
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) 
            return format;
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
            return format;
    }

    utils::FatalError(g_Engine->Hwnd(), "Failed to find supported format");
    return VkFormat();
}

bool CVulkanRenderer::HasStencilComponent(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
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
    //#IMAGES
//     m_SwapChainImageViewsVec.resize(m_SwapChainImagesVec.size());
//     for (size_t i = 0; i < m_SwapChainImagesVec.size(); i++)
//     {
//         VkImageViewCreateInfo createInfo = {};
//         createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
//         createInfo.image = m_SwapChainImagesVec[i];
//         createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
//         createInfo.format = m_SwapChainImageFormat;
//         createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
//         createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
//         createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
//         createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
//         createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//         createInfo.subresourceRange.baseMipLevel = 0;
//         createInfo.subresourceRange.levelCount = 1;
//         createInfo.subresourceRange.baseArrayLayer = 0;
//         createInfo.subresourceRange.layerCount = 1;
// 
//         if (VKRESULT(vkCreateImageView(m_Device, &createInfo, nullptr, &m_SwapChainImageViewsVec[i])))
//             return utils::FatalError(g_Engine->GetHwnd(), L"Failed to create image views");
//     }
//     return true;

    m_SwapChainImageViewsVec.resize(m_SwapChainImagesVec.size());
    for (uint32_t i = 0; i < m_SwapChainImagesVec.size(); i++)
    {
        m_SwapChainImageViewsVec[i] = CreateImageView(m_SwapChainImagesVec[i], m_SwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
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
        if (g_Engine->ObjectControl()) //#CMD_BUFF to przemyslec ifa albo dac wyzej
        g_Engine->ObjectControl()->RecordCommandBuffer(m_CommandBuffers[i]);
        vkCmdEndRenderPass(m_CommandBuffers[i]);

        // DEFAULT
        //////////////////////////////////////////////////////////////////////////
        // vkCmdBeginRenderPass(m_CommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        // vkCmdBindPipeline(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);
        //
        // VkBuffer vertexBuffers[] = { m_VertexBuffer };
        // VkDeviceSize offsets[] = { 0 };
        // vkCmdBindVertexBuffers(m_CommandBuffers[i], 0, 1, vertexBuffers, offsets);
        //
        // vkCmdDraw(m_CommandBuffers[i], static_cast<uint32_t>(Vertices.size()), 1, 0, 0);
        // vkCmdEndRenderPass(m_CommandBuffers[i]);

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

bool CVulkanRenderer::InitTechniqueManager()
{
    if (m_TechMgr)
        return true;

    m_TechMgr = new CTechniqueManager();

    CGBaseObject::s_TechId = REGISTER_TECH(BaseVertex, new CBaseTechnique); //#TECH do poprawy, renderer nie powinien wiedziec o poszczegolnych technikach

    //#TECH porejestrowac tu techniki (albo gdzies indziej do przemyslenia)
    m_TechMgr->InitTechniques();

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