#include "stdafx.h"
#include "ShaderManager.h"
#include <fstream>

CShaderManager::CShaderManager(const SShaderMgrParams& params)
{
    Initialize(params);
}

CShaderManager::~CShaderManager()
{
    if (m_FragShaderModule)
        vkDestroyShaderModule(g_Engine->Renderer()->GetDevice(), m_FragShaderModule, nullptr);
    if (m_VertShaderModule)
        vkDestroyShaderModule(g_Engine->Renderer()->GetDevice(), m_VertShaderModule, nullptr);
}

void CShaderManager::Initialize(const SShaderMgrParams& params)
{
    std::vector<char> vertShaderCode;
    std::vector<char> fragShaderCode;

    // Read shader files
    if (!params.vertex_shader_path.empty())
        ReadFile(params.vertex_shader_path.c_str(), vertShaderCode);
    if (!params.fragment_shader_path.empty())
        ReadFile(params.fragment_shader_path.c_str(), fragShaderCode);

    // Create shader modules
    if (!m_VertShaderModule && !vertShaderCode.empty())
        m_VertShaderModule = CreateShaderModule(vertShaderCode);
    if (!m_FragShaderModule && !fragShaderCode.empty())
        m_FragShaderModule = CreateShaderModule(fragShaderCode);

    // Create shader stage info vec
    if (m_VertShaderModule)
    {
        VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = m_VertShaderModule;
        vertShaderStageInfo.pName = params.vertex_entry.c_str(); // we can use multiple entry functions and have multiple shaders in one file
        m_ShaderStageInfo.push_back(vertShaderStageInfo);
    }

    if (m_FragShaderModule)
    {
        VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = m_FragShaderModule;
        fragShaderStageInfo.pName = params.fragment_entry.c_str(); // #TECH te pointery sie nie zwolnia?
        m_ShaderStageInfo.push_back(fragShaderStageInfo);
    }
}

VkShaderModule CShaderManager::CreateShaderModule(const std::vector<char>& code)
{
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule = nullptr;
    if (VKRESULT(vkCreateShaderModule(g_Engine->Renderer()->GetDevice(), &createInfo, nullptr, &shaderModule))) 
        utils::FatalError(g_Engine->Hwnd(), "Failed to create shader module");

    return shaderModule;
}

bool CShaderManager::ReadFile(const char* filename, std::vector<char>& out)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) 
        return utils::FatalError(g_Engine->Hwnd(), "Failed to open file");

    size_t fileSize = (size_t)file.tellg();
    out.resize(fileSize);

    file.seekg(0);
    file.read(out.data(), fileSize);
    file.close();

    return true;
}
