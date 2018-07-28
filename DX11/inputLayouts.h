#pragma once

class ID3D11Device;
class ID3DX11Effect;
class ID3D11InputLayout;
class ID3DX11EffectTechnique;

enum eInputLayouts
{
    BASE = 0,
    PARTICLE,
    PARTICLE_SORT,

    __COUNT__
};

class inputLayouts
{
public:
    ~inputLayouts();

    static inputLayouts* getInstance(ID3D11Device *D3DDevice, ID3DX11Effect *pEffect);
    
    // getters
    ID3D11InputLayout* getLayout(eInputLayouts val);
    ID3DX11EffectTechnique* getTechnique(eInputLayouts val);

private:
    // objects
    static inputLayouts* objectInstance;

    // technics
    ID3DX11EffectTechnique  *TechArr[__COUNT__];

    // layouts
    ID3D11InputLayout   *LayoutsArr[__COUNT__];

    // constructor
    inputLayouts();

    // init methods
    void initTechniques(ID3DX11Effect *pEffect);
    bool initLayouts(ID3D11Device *pD3DDevice, ID3DX11Effect *pEffect);
    bool initBaseLayout(ID3D11Device *pD3DDevice, ID3DX11Effect *pEffect);
    bool initParticleLayout(ID3D11Device *pD3DDevice, ID3DX11Effect *pEffect);

    // other methods
    bool releaseAll();
};

