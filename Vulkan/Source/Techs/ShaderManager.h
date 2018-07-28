#pragma once

struct SShaderMgrParams
{
    // Paths
    std::string vertex_shader_path;
    std::string fragment_shader_path;

    // Entry function name
    std::string vertex_entry;
    std::string fragment_entry;
};

class CShaderManager
{
public:
    CShaderManager() = default;
    CShaderManager(const SShaderMgrParams& params);
    ~CShaderManager();

    typedef std::vector<VkPipelineShaderStageCreateInfo> TStageInfoVec;

    // Init
    void Initialize(const SShaderMgrParams& params);

    // Getters
    const TStageInfoVec& GetShaderStageInfoVec() const { return m_ShaderStageInfo; }

    // Read shader file
    static bool ReadFile(const char* filename, std::vector<char>& out);

    // Create module
    VkShaderModule CreateShaderModule(const std::vector<char>& code);

    // Shader Modules
    VkShaderModule m_VertShaderModule = nullptr;
    VkShaderModule m_FragShaderModule = nullptr;

    TStageInfoVec m_ShaderStageInfo;
};

