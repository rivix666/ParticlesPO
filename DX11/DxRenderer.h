#pragma once
#include <d3dx11.h>
#include <d3d11.h>
#include <dxerr.h>

class inputLayouts;
class CDxCamera;
class IGObject;

class ID3DX11Effect;
class ID3DX11EffectMatrixVariable;
class ID3DX11EffectVectorVariable;
class ID3DX11EffectShaderResourceVariable;

class CDxRenderer
{
public:
    CDxRenderer();
    ~CDxRenderer();

    bool Init();
    bool Shutdown();

    // Render
    void Render();

    // GObjects handle
    void RegisterRenderObject(IGObject* obj);
    void UnregisterRenderObject(IGObject* obj);
    void ReleaseRenderObject(IGObject* obj);

    // Getters / Setters
    CDxCamera* GetCam() const { return g_pCamera; }
    ID3D11Device* GetDevice() const { return m_D3DDevice; }
    ID3D11DeviceContext* GetDeviceCtx() const { return m_D3DDeviceContext; }
    ID3DX11Effect* GetEffects() const { return m_Effect; }
    const XMMATRIX& WorldMtx() const { return m_World; }

private:
    // Init
    bool InitSwapChainAndDevice();
    bool CreateDevice(DXGI_SWAP_CHAIN_DESC swapChainDesc);
    void InitSwapChainDesc(DXGI_SWAP_CHAIN_DESC& desc);
    bool LoadEffectFile();
    bool CreateDepthBuffer();
    bool CreateRenderTargetView();
    void InitAndSetViewport();
    void InitEffectVariables();
    bool InitInputLayouts();
    bool InitRasterizer();
    void InitMatrices();
    void InitCamera();

    // Render
    void ClearScene();
    void SetDepthBufferAsResource();
    void SetPrimitiveTopology();
    void RenderGObjects();
    void RenderFx();
    void SetEffectVariablesPerFrame();
    void SetEffectVariablesPerInit();

    // Misc
    void ReleaseRenderObjects();

    // DEVICE                            
    ID3D11Device*				         m_D3DDevice = nullptr;
    ID3D11DeviceContext*                 m_D3DDeviceContext = nullptr;
    IDXGISwapChain*				         m_SwapChain = nullptr;
    ID3D11RenderTargetView*		         m_RenderTargetView = nullptr;
    D3D11_VIEWPORT				         m_ViewPort;

    // DEPTH                             
    ID3D11Texture2D*			         m_DepthStencil = nullptr;
    ID3D11DepthStencilView*		         m_DepthStencilView = nullptr;
    ID3D11DepthStencilView*				 m_ZBufferDSView = nullptr;

    // EFFECTS
    ID3DX11Effect*				         m_Effect = nullptr;
    ID3D11ShaderResourceView*			 m_ZBufferSRView = nullptr;
    ID3DX11EffectMatrixVariable*         m_WorldEffectVariable;
    ID3DX11EffectMatrixVariable*         m_InvProjectionEffectVariable = nullptr;
    ID3DX11EffectMatrixVariable*         m_ViewEffectVariable = nullptr;
    ID3DX11EffectMatrixVariable*         m_ProjectionEffectVariable = nullptr;
    ID3DX11EffectVectorVariable*         m_CameraPosEffectVariable = nullptr;
    ID3DX11EffectVectorVariable*         m_CameraUpEffectVariable = nullptr;
    ID3DX11EffectShaderResourceVariable* m_DepthEffectVariable = nullptr;

    // AA
    UINT                                 m_4xMsaaQuality = 0;
    bool                                 m_Enable4xMsaa = false;

    inputLayouts*				         m_InputLayouts = nullptr;
                                         
    XMMATRIX					         m_World;
                                         
    CDxCamera*						     g_pCamera = nullptr;

    FLOAT                                m_BgColor[4] = { 0.0f, 0.1137f, 0.243f, 1.0f };
                                         
    std::list<IGObject*>                 m_RenderQueue;
};

