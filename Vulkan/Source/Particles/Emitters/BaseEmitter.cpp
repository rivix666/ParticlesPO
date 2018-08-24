#include "stdafx.h"
#include "BaseEmitter.h"

CBaseEmitter::CBaseEmitter(const uint32_t& tech_id, const uint32_t& buff_size)
    : IEmitter(tech_id, buff_size)
{
}

CBaseEmitter::~CBaseEmitter()
{
}