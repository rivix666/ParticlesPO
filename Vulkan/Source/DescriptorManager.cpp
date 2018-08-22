#include "stdafx.h"
#include "DescriptorManager.h"

CDescriptorManager::CDescriptorManager()
{
    ClearDescDataCount();
}

// Create
//////////////////////////////////////////////////////////////////////////
bool CDescriptorManager::Init(const uint32_t& max_sets)
{
    if (!CreateDescriptorPool(max_sets))
        return Shutdown();

    if (!CreateDescriptorLayouts())
        return Shutdown();

    if (!CreateDescriptorSets())
        return Shutdown();

    return true;
}

bool CDescriptorManager::CreateDescriptorPool(const uint32_t& max_sets)
{
    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = (uint32_t)(m_PoolRegisteredSizes.size());
    poolInfo.pPoolSizes = m_PoolRegisteredSizes.data();
    poolInfo.maxSets = max_sets;

    if (VKRESULT(vkCreateDescriptorPool(g_Engine->Device(), &poolInfo, nullptr, &m_DescriptorPool)))
        return utils::FatalError(g_Engine->Hwnd(), "Failed to create descriptor pool");

    return true;
}

bool CDescriptorManager::CreateDescriptorLayouts()
{
    if (!m_DescLays.empty())
        return utils::FatalError(g_Engine->Hwnd(), "Clear Descriptor Layouts before recreate");

    // Prepare Description Layout vector
    m_DescLays.resize(m_DescData.size());

    for (size_t i = 0; i < m_DescData.size(); i++)
    {
        const auto& desc_vec = m_DescData[i];
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        // Store bindings info
        for (size_t j = 0; j < desc_vec.size(); j++)
        {
            const auto& data = desc_vec[j];
            if (!data.is_valid)
                continue;

            VkDescriptorSetLayoutBinding bin = {};
            bin.binding = j;
            bin.descriptorCount = data.buffer.empty() ? data.sampler.size() : data.buffer.size();
            bin.descriptorType = data.type;
            bin.pImmutableSamplers = nullptr;
            bin.stageFlags = data.stage_flags;
            bindings.push_back(bin);
        }

        // If there are no bindings
        if (bindings.empty())
        {
            m_DescLays[i] = nullptr;
            continue;
        }

        // Create Description Layout
        VkDescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = (uint32_t)(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (VKRESULT(vkCreateDescriptorSetLayout(g_Engine->Device(), &layoutInfo, nullptr, &m_DescLays[i])))
        {
            ReleaseDescLayouts();
            return utils::FatalError(g_Engine->Hwnd(), "Failed to create descriptor set layout");
        }
    }

    return true;
}

bool CDescriptorManager::CreateDescriptorSets()
{
    if (m_DescLays.empty())
        return utils::FatalError(g_Engine->Hwnd(), "Create Descriptor Layouts before sets");

    if (!m_DescSets.empty())
        return utils::FatalError(g_Engine->Hwnd(), "Clear Descriptor Sets before recreate");

    // Prepare Descriptor Sets vector
    m_DescSets.resize(m_DescData.size());

    for (size_t i = 0; i < m_DescData.size(); i++)
    {
        if (!m_DescLays[i])
        {
            m_DescSets[i] = nullptr;
            continue;
        }

        // Create Descriptor Set
        VkDescriptorSetLayout layouts[] = { m_DescLays[i] };
        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_DescriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = layouts;
        if (VKRESULT(vkAllocateDescriptorSets(g_Engine->Device(), &allocInfo, &m_DescSets[i])))
        {
            ReleaseDescriptorSets();
            return utils::FatalError(g_Engine->Hwnd(), "Failed to allocate descriptor set");
        }

        // Prepare Writes vector - for Descriptor Sets update
        std::vector<VkWriteDescriptorSet> descriptorWrites;

        // We need to store here all infos, so We can release them on the end
        std::vector<std::vector<VkDescriptorImageInfo>> all_img_info;
        std::vector<std::vector<VkDescriptorBufferInfo>> all_buff_info;
        all_img_info.resize(m_DescData[i].size());
        all_buff_info.resize(m_DescData[i].size());

        // Update buffers info
        const auto& desc_vec = m_DescData[i];
        for (size_t j = 0; j < desc_vec.size(); j++)
        {
            const auto& data = desc_vec[j];
            if (!data.is_valid)
                continue;

            // Write buffers
            if (!data.buffer.empty())
            {
                size_t offset = 0;
                FetchDescriptorBufferInfo(data, all_buff_info[j]);

                VkWriteDescriptorSet write;
                write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                write.pNext = nullptr;
                write.dstSet = m_DescSets[i];
                write.dstBinding = j;
                write.dstArrayElement = 0;
                write.descriptorType = data.type;
                write.descriptorCount = all_buff_info[j].size();
                write.pBufferInfo = all_buff_info[j].data();
                descriptorWrites.push_back(write);

                continue;
            }

            // Write images
            if (!data.sampler.empty())
            {
                size_t offset = 0;
                FetchDescriptorImageInfo(data, all_img_info[j]);

                VkWriteDescriptorSet write;
                write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                write.pNext = nullptr;
                write.dstSet = m_DescSets[i];
                write.dstBinding = j;
                write.dstArrayElement = 0;
                write.descriptorType = data.type;
                write.descriptorCount = all_img_info[j].size();
                write.pImageInfo = all_img_info[j].data();
                descriptorWrites.push_back(write);

                continue;
            }
        }

        // Update Descriptor Sets
        if (!descriptorWrites.empty())
        {
            vkUpdateDescriptorSets(g_Engine->Device(), (uint32_t)(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }

    return true;
}

// Release
//////////////////////////////////////////////////////////////////////////
bool CDescriptorManager::Shutdown()
{
    ReleaseDescPool();
    ReleaseDescLayouts();
    return false;
}

void CDescriptorManager::ReleaseDescPool()
{
    if (m_DescriptorPool)
    {
        vkDestroyDescriptorPool(g_Engine->Device(), m_DescriptorPool, nullptr);
        m_DescSets.clear();
    }
}

void CDescriptorManager::ReleaseDescLayouts()
{
    for (size_t i = 0; i < m_DescLays.size(); i++)
    {
        if (m_DescLays[i])
            vkDestroyDescriptorSetLayout(g_Engine->Device(), m_DescLays[i], nullptr);
    }
    m_DescLays.clear();
}

bool CDescriptorManager::ReleaseDescriptorSets()
{
    TDescSetVec tmp_vec;
    for (size_t i = 0; i < m_DescSets.size(); i++)
    {
        if (m_DescSets[i])
        {
            tmp_vec.push_back(m_DescSets[i]);
        }
    }
    m_DescSets.clear();

    if (VKRESULT(vkFreeDescriptorSets(g_Engine->Device(), m_DescriptorPool, (uint32_t)tmp_vec.size(), tmp_vec.data())))
        return utils::FatalError(g_Engine->Hwnd(), "Failed to release descriptor sets");

    return true;
}

// Registration helpers
//////////////////////////////////////////////////////////////////////////
uint32_t CDescriptorManager::GetNextFreeSetId() const
{
    return (uint32_t)(m_DescData.size());
}

uint32_t CDescriptorManager::GetNextFreeLocationId(const uint32_t& set) const
{
    if (set < m_DescData.size())
    {
        return (uint32_t)m_DescData[set].size();
    }
    return 0;
}

// Registration
//////////////////////////////////////////////////////////////////////////
void CDescriptorManager::RegisterDescPoolSize(const VkDescriptorPoolSize& size)
{
    m_PoolRegisteredSizes.push_back(size);
}

void CDescriptorManager::ClearDescPoolSizeInfo()
{
    m_PoolRegisteredSizes.clear();
}

bool CDescriptorManager::RegisterDescriptor(const TBuffsVec& buffs, const TRangeSizes& sizes, const VkDescriptorType& type, const VkShaderStageFlags& stage_flags, const uint32_t& set, const uint32_t& location)
{
    if (buffs.empty())
        return utils::FatalError(g_Engine->Hwnd(), "Failed to register descriptor");

    // Prepare data struct
    SDescSetData data;
    data.buffer = buffs;
    data.sizes = sizes;
    data.type = type;
    data.stage_flags = stage_flags;
    data.is_valid = true;

    // Create place for the new data
    PreparePlace4Descriptors(set, location);

    // Store data
    m_DescData[set][location] = data;
    m_DescDataCount[type]++;

    return true;
}

bool CDescriptorManager::RegisterDescriptor(const TSampVec& pairs, const TRangeSizes& sizes, const VkDescriptorType& type, const VkShaderStageFlags& stage_flags, const uint32_t& set, const uint32_t& location)
{
    if (pairs.empty())
        return utils::FatalError(g_Engine->Hwnd(), "Failed to register descriptor");

    // Prepare data struct
    SDescSetData data;
    data.sampler = pairs;
    data.sizes = sizes;
    data.type = type;
    data.stage_flags = stage_flags;
    data.is_valid = true;

    // Create place for the new data
    PreparePlace4Descriptors(set, location);

    // Store data
    m_DescData[set][location] = data;
    m_DescDataCount[type]++;

    return true;
}

bool CDescriptorManager::UnregisterDescriptor(const uint32_t& set, const uint32_t& location)
{
    if ((size_t)set >= m_DescData.size() || (size_t)location >= m_DescData[set].size())
        return utils::FatalError(g_Engine->Hwnd(), "Failed to unregister descriptor");

    m_DescData[set][location] = {};
    for (size_t i = m_DescData[set].size() - 1; i >= 0 && !m_DescData[set][i].is_valid; i--)
    {
        m_DescData[set].pop_back();
    }
    for (size_t i = m_DescData.size() - 1; i >= 0 && m_DescData[i].empty(); i--)
    {
        m_DescData.pop_back();
    }

    return true;
}

void CDescriptorManager::ClearAllRegisteredData()
{
    m_DescData.clear();
    ClearDescDataCount();
}

// Description Sets helper methods
//////////////////////////////////////////////////////////////////////////
void CDescriptorManager::PreparePlace4DescSets(const uint32_t& new_set_id)
{
    if ((size_t)new_set_id >= m_DescData.size())
    {
        size_t old_size = m_DescData.size();
        m_DescData.resize(new_set_id + 1);
    }
}

void CDescriptorManager::PreparePlace4Descriptors(const uint32_t& new_set_id, const uint32_t& new_location_id)
{
    // Check if there is set with given id
    PreparePlace4DescSets(new_set_id);

    // Prepare place for new descriptors
    if ((size_t)new_location_id >= m_DescData[new_set_id].size())
    {
        size_t old_size = m_DescData[new_set_id].size();
        m_DescData[new_set_id].resize(new_location_id + 1);
        for (size_t i = old_size; i <= new_location_id; i++)
        {
            m_DescData[new_set_id][i] = {};
        }
    }
}

// Get Descriptor Buffer/Image Info
//////////////////////////////////////////////////////////////////////////
void CDescriptorManager::FetchDescriptorBufferInfo(const SDescSetData& data, std::vector<VkDescriptorBufferInfo>& out_vec)
{
    size_t offset = 0;
    for (size_t arr = 0; arr < data.buffer.size(); arr++)
    {
        VkDescriptorBufferInfo info;
        info.buffer = data.buffer[arr];
        info.offset = offset;
        info.range = data.sizes[arr];
        out_vec.push_back(info);
        offset += data.sizes[arr];
    }
}

void CDescriptorManager::FetchDescriptorImageInfo(const SDescSetData& data, std::vector<VkDescriptorImageInfo>& out_vec)
{
    for (size_t arr = 0; arr < data.sampler.size(); arr++)
    {
        VkDescriptorImageInfo info;
        info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        info.imageView = data.sampler[arr].first;
        info.sampler = data.sampler[arr].second;
        out_vec.push_back(info);
    }
}

// Misc
//////////////////////////////////////////////////////////////////////////
void CDescriptorManager::ClearDescDataCount()
{
    memset(m_DescDataCount, uint32_t(0), (size_t)(sizeof(uint32_t) * VkDescriptorType::VK_DESCRIPTOR_TYPE_RANGE_SIZE));
}