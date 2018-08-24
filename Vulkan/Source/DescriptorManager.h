#pragma once

enum class EDescSetRole : uint32_t
{
    GENERAL = 0,
    OBJECTS,
    PARTICLES,
    DEPTH,
    COMPUTE,

    _COUNT_
};

class CDescriptorManager
{
public:
    CDescriptorManager();

    // Typedefs
    typedef std::vector<VkBuffer> TBuffsVec;
    typedef std::vector<VkBufferView> TBuffsViewsVec;
    typedef std::vector<ITechnique::TImgSampler> TSampVec;
    typedef std::vector<size_t> TRangeSizes;

    // Structs
    struct SDescSetData
    {
        TBuffsVec buffer; // #DESC_MGR w przypadku arraya bufferow to powinien byc jeden duzy buffer z offsetami, jak bedzie czast o poprawic (patrz ParticleTextureManager.cpp)
        TSampVec sampler;
        TRangeSizes sizes;
        TBuffsViewsVec texel_buffer_views;
        VkDescriptorType type = (VkDescriptorType) 0;
        VkShaderStageFlags stage_flags = (VkShaderStageFlags) 0;

        bool is_valid = false;
    };

    // Typedefs
    typedef std::vector<VkDescriptorSet> TDescSetVec;
    typedef std::vector<VkDescriptorPoolSize> TPoolSizeVec;
    typedef std::vector<VkDescriptorSetLayout> TDescSetLayVec;
    typedef std::vector<std::vector<SDescSetData>> TDescDataVec;

    // Call it after you register all buffers/samplers.
    bool Init(const uint32_t& max_sets);

    // Call it after you register all pool sizes.
    bool CreateDescriptorPool(const uint32_t& max_sets);

    // Call it after you register all buffers/samplers.
    bool CreateDescriptorLayouts();
    bool CreateDescriptorSets();

    // Update
    void UpdateDescriptorSet(const uint32_t& set);

    // Call it if you want to release all descriptors, descriptor layouts and descriptor pool
    bool Shutdown();

    // Call it to release whole pool and all desc sets
    void ReleaseDescPool();

    // Call it to release desc set layouts
    void ReleaseDescLayouts();

    // Call it if you want to release descriptor sets but keep pool
    bool ReleaseDescriptorSets();

    // Registration helpers
    uint32_t GetNextFreeSetId() const;
    uint32_t GetNextFreeLocationId(const uint32_t& set) const;

    // Descriptor Pool info registration
    void RegisterDescPoolSize(const VkDescriptorPoolSize& size);
    void ClearDescPoolSizeInfo();

    // Descriptor registration
    bool RegisterDescriptor(const TBuffsVec& buffs, const TRangeSizes& sizes, const VkDescriptorType& type, const VkShaderStageFlags& stage_flags, const uint32_t& set, const uint32_t& location);
    bool RegisterDescriptor(const TSampVec& pairs, const TRangeSizes& sizes, const VkDescriptorType& type, const VkShaderStageFlags& stage_flags, const uint32_t& set, const uint32_t& location);
    bool RegisterDescriptor(const TBuffsViewsVec& views, const VkDescriptorType& type, const VkShaderStageFlags& stage_flags, const uint32_t& set, const uint32_t& location);
    bool UnregisterDescriptor(const uint32_t& set, const uint32_t& location);
    void ClearAllRegisteredData();

    // Getters
    size_t DescriptorSetsCount() const { return m_DescSets.size(); }
    VkDescriptorPool DescriptorPool() const { return m_DescriptorPool; }
    VkDescriptorSet DescriptorSet(const uint32_t& set) const { return m_DescSets[set]; }
    VkDescriptorSetLayout DescriptorSetLayout(const uint32_t& set) const { return m_DescLays[set]; }

protected:
    // Description Sets helper methods
    void PreparePlace4DescSets(const uint32_t& new_set_id);
    void PreparePlace4Descriptors(const uint32_t& new_set_id, const uint32_t& new_location_id);

    // Get Descriptor Buffer/Image Info
    void FetchDescriptorBufferInfo(const SDescSetData& data, std::vector<VkDescriptorBufferInfo>& out_vec);
    void FetchDescriptorImageInfo(const SDescSetData& data, std::vector<VkDescriptorImageInfo>& out_vec);

    // Misc
    void ClearDescDataCount();

private:
    // Pool
    VkDescriptorPool    m_DescriptorPool = nullptr;

    // Vectors
    TDescSetVec         m_DescSets;
    TDescDataVec        m_DescData;
    TDescSetLayVec      m_DescLays;
    TPoolSizeVec        m_PoolRegisteredSizes;

    // Counters
    uint32_t            m_DescDataCount[VkDescriptorType::VK_DESCRIPTOR_TYPE_RANGE_SIZE];
};