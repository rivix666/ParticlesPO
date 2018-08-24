#pragma once

struct SShaderParams
{
    // Paths
    std::string vertex_shader_path;
    std::string geometry_shader_path;
    std::string fragment_shader_path;
    std::string compute_shader_path;

    // Entry function name
    std::string vertex_entry;
    std::string geometry_entry;
    std::string fragment_entry;
    std::string compute_entry;
};

class CShaderUtils
{
public:
    CShaderUtils() = default;
    CShaderUtils(const SShaderParams& params);
    ~CShaderUtils();

    typedef std::vector<VkPipelineShaderStageCreateInfo> TStageInfoVec;

    // Init
    void Initialize(const SShaderParams& params);

    // Getters
    const TStageInfoVec& GetShaderStageInfoVec() const { return m_ShaderStageInfo; }

    // Read shader file
    static bool ReadFile(const char* filename, std::vector<char>& out);

    // Create module
    VkShaderModule CreateShaderModule(const std::vector<char>& code);

    // Shader Modules
    VkShaderModule m_VertShaderModule  = nullptr;
    VkShaderModule m_GeomShaderModule = nullptr;
    VkShaderModule m_FragShaderModule  = nullptr;
    VkShaderModule m_CompShaderModule  = nullptr;

    TStageInfoVec m_ShaderStageInfo;
};

