#pragma once
#include "IEmitter.h"

class CBaseEmitter : public IEmitter 
{
public:
    CBaseEmitter(const uint32_t& tech_id, const uint32_t& buff_size);
    ~CBaseEmitter();
};