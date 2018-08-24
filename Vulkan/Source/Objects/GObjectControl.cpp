#include "stdafx.h"
#include "GObjectControl.h"
#include "IGObject.h"
#include "../Techs/TechniqueManager.h"
#include "../DescriptorManager.h"

CGObjectControl::CGObjectControl(VkDevice device)
    : m_Device(device)
{
    uint32_t tech_count = g_Engine->TechMgr()->TechniquesCount();
    m_TechToObjVec.resize(tech_count);
    m_SizeCacheVec.resize(tech_count);
}

CGObjectControl::~CGObjectControl()
{
}

void CGObjectControl::Shutdown()
{
    uint32_t tech_count = m_TechToObjVec.size();
    for (uint32_t i = 0; i < tech_count; i++)
    {
        for (auto obj : m_TechToObjVec[i])
            SAFE_DELETE(obj);
    }

    m_TechToObjVec.clear();
}

void CGObjectControl::RegisterObject(IGObject* obj)
{
    RegisterObject(obj->TechniqueId(), obj);
}

void CGObjectControl::RegisterObject(const uint32_t& tech, IGObject* obj)
{
    if (!obj)
        return;

    if (m_TechToObjVec[tech].size() >= OBJ_PER_TECHNIQUE)
    {
        utils::FatalError(g_Engine->Hwnd(), "Can not register more objects");
        return;
    }

    obj->CreateBuffers();
    EnsureTechIdWillFit(tech);

    m_TechToObjVec[tech].push_back(obj);
    m_SizeCacheVec[tech] += obj->GetVerticesSize();

    obj->InitPhysXObj();
}

void CGObjectControl::UnregisterObject(const uint32_t& tech, IGObject* obj)
{
    if (tech >= m_TechToObjVec.size() || !obj)
        return;

    m_SizeCacheVec[tech] -= obj->GetVerticesSize();
    auto new_end = std::remove_if(m_TechToObjVec[tech].begin(), m_TechToObjVec[tech].end(), [=](IGObject* o) 
    { 
        return o == obj; 
    });

    m_TechToObjVec[tech].erase(new_end, m_TechToObjVec[tech].end());

    obj->ShutdownPhysXObj();
}

void CGObjectControl::UnregisterObject(IGObject* obj)
{
    UnregisterObject(obj->TechniqueId(), obj);
}

void CGObjectControl::RecordCommandBuffer(VkCommandBuffer& cmd_buff)
{
    auto tech_mgr = g_Engine->TechMgr();
    uint32_t tech_count = m_TechToObjVec.size();
    for (uint32_t i = 0; i < tech_count; i++)
    {
        auto tech = tech_mgr->GetTechnique(i);
        vkCmdBindPipeline(cmd_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, tech->GetPipeline());

        for (uint32_t j = 0; j < m_TechToObjVec[i].size(); j++)
        {
            auto obj = m_TechToObjVec[i][j];
            if (!obj->VertexBuffer())
                continue;

            // Bind Vertex buffer
            VkBuffer vertexBuffers[] = { obj->VertexBuffer() };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(cmd_buff, 0, 1, vertexBuffers, offsets);

            // Prepare uni buff offset
            uint32_t uni_offset = g_Engine->Renderer()->GetUniBuffObjSize(tech->GetSingleUniBuffObjSize()) * j;

            // Prepare Descriptor Sets vector
            std::vector<VkDescriptorSet> desc_sets = { g_Engine->DescMgr()->DescriptorSet((uint32_t)EDescSetRole::GENERAL), g_Engine->DescMgr()->DescriptorSet((uint32_t)EDescSetRole::OBJECTS) };

            // Record command
            if (obj->IndexBuffer())
            {
                vkCmdBindIndexBuffer(cmd_buff, obj->IndexBuffer(), 0, VK_INDEX_TYPE_UINT16);
                vkCmdBindDescriptorSets(cmd_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, tech->GetPipelineLayout(), 0, (uint32_t)desc_sets.size(), desc_sets.data(), 1, &uni_offset);  //#UNI_BUFF
                vkCmdDrawIndexed(cmd_buff, (uint32_t)(obj->GetIndicesCount()), 1, 0, 0, 0);
            }
            else
            {
                vkCmdBindDescriptorSets(cmd_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, tech->GetPipelineLayout(), 0, (uint32_t)desc_sets.size(), desc_sets.data(), 1, &uni_offset);  //#UNI_BUFF
                vkCmdDraw(cmd_buff, (uint32_t)(obj->GetVerticesCount()), 1, 0, 0);
            }
        }
    }
}

void CGObjectControl::UpdateUniBuffers()
{
    size_t minUboAlignment = g_Engine->Renderer()->MinUboAlignment();

    // Update Uni buffers
    for (uint32_t tech_id = 0; tech_id < m_TechToObjVec.size(); tech_id++)
    {
        uint8_t* pData;
        auto uni_buff_mem = g_Engine->TechMgr()->GetTechnique(tech_id)->UniBufferMemory();
        auto single_obj_size = g_Engine->TechMgr()->GetTechnique(tech_id)->GetSingleUniBuffObjSize();
        if (uni_buff_mem == nullptr || single_obj_size == 0)
            continue;

        vkMapMemory(g_Engine->Device(), uni_buff_mem, 0, single_obj_size * m_TechToObjVec[tech_id].size(), 0, (void **)&pData);
        for (uint32_t obj_id = 0; obj_id < m_TechToObjVec[tech_id].size(); obj_id++)
        {
            void* obj_data = m_TechToObjVec[tech_id][obj_id]->GetUniBuffData();
            if (obj_data)
            {
                memcpy(pData, obj_data, single_obj_size);
            }
            else
            {
                LogD("Object with invalid UniBuff data. Tech:");
                LogD(tech_id);
                LogD(" Obj:");
                LogD(obj_id);
                LogD("\n");
            }
            pData += g_Engine->Renderer()->GetUniBuffObjSize(g_Engine->TechMgr()->GetTechnique(tech_id)->GetSingleUniBuffObjSize());
        }
        vkUnmapMemory(g_Engine->Device(), uni_buff_mem);
    }
}

void CGObjectControl::UpdatePhysXActors()
{
    // Retrieve array of actors that moved
    uint32_t count;
    const physx::PxActiveTransform* active_transforms = g_Engine->PxManager()->Scene()->getActiveTransforms(count);

    // Update each render object with the new transform
    for (uint32_t i = 0; i < count; ++i)
    {
        auto render_object = static_cast<IGObject*>(active_transforms[i].userData);
        auto transform = active_transforms[i].actor2World;
        glm::mat4 new_world = glm::toMat4(glm::quat(transform.q.w, transform.q.x, transform.q.y, transform.q.z));      
        new_world[3] = glm::vec4(transform.p.x, transform.p.y, transform.p.z, new_world[3].w);
        render_object->SetWorldMtx(new_world);
    }
}

void CGObjectControl::EnsureTechIdWillFit(const uint32_t& tech_id)
{
    if (m_TechToObjVec.size() <= tech_id)
    {
        m_TechToObjVec.resize(tech_id);
        m_SizeCacheVec.resize(tech_id);
    }
}