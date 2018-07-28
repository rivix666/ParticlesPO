#include "stdafx.h"
#include "DxRenderer.h"

#include "d3dx/d3dx11effect.h"

#include "IGObject.h"
#include "inputLayouts.h"
#include "DxCamera.h"

#include "ParticleManager.h" //#REDESIGN


CDxRenderer::CDxRenderer()
{
}


CDxRenderer::~CDxRenderer()
{
    Shutdown();
}

bool CDxRenderer::Init()
{
    if (!InitSwapChainAndDevice()) return Shutdown();
    if (!LoadEffectFile()) return Shutdown();
    if (!CreateRenderTargetView()) return Shutdown();
    if (!CreateDepthBuffer()) return Shutdown();
    if (!InitRasterizer()) return Shutdown();
    if (!InitInputLayouts()) return Shutdown();
    InitEffectVariables();
    InitAndSetViewport();
    InitCamera();
    InitMatrices();
    //CreateParticleEmittersAndExplosions();
    SetEffectVariablesPerInit();
    return true;
}

bool CDxRenderer::Shutdown()
{
    ReleaseRenderObjects();

    SAFE_RELEASE(m_RenderTargetView);
    SAFE_RELEASE(m_SwapChain);
    SAFE_RELEASE(m_D3DDevice);
    SAFE_RELEASE(m_Effect);
    SAFE_RELEASE(m_DepthStencil);
    SAFE_RELEASE(m_DepthStencilView);
    SAFE_RELEASE(m_ZBufferSRView);
    SAFE_RELEASE(m_ZBufferDSView);
     
    SAFE_DELETE(m_InputLayouts);
    SAFE_DELETE(g_pCamera);

    return false;
}

void CDxRenderer::Render()
{
    ClearScene();
    SetEffectVariablesPerFrame();
    RenderGObjects();
    RenderFx();
    m_SwapChain->Present(0, 0);
}

void CDxRenderer::RegisterRenderObject(IGObject* obj)
{
    m_RenderQueue.push_back(obj);
}

void CDxRenderer::UnregisterRenderObject(IGObject* obj)
{
    m_RenderQueue.remove(obj);
}

void CDxRenderer::ReleaseRenderObject(IGObject* obj)
{
    UnregisterRenderObject(obj);
    SAFE_DELETE(obj);
}

bool CDxRenderer::InitSwapChainAndDevice()
{
    DXGI_SWAP_CHAIN_DESC desc;
    InitSwapChainDesc(desc);
    return CreateDevice(desc);
}

bool CDxRenderer::CreateDevice(DXGI_SWAP_CHAIN_DESC swapChainDesc) 
{
// Create the device and device context.
    UINT createDeviceFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevel;
    HRESULT hr = D3D11CreateDevice(
        0,                 // default adapter
        D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_HARDWARE,
        0,                 // no software device
        createDeviceFlags,
        0, 0,              // default feature level array
        D3D11_SDK_VERSION,
        &m_D3DDevice,
        &featureLevel,
        &m_D3DDeviceContext);

    if (FAILED(hr))
    {
        MessageBox(0, L"D3D11CreateDevice Failed.", 0, 0);
        return false;
    }

    if (featureLevel != D3D_FEATURE_LEVEL_11_0)
    {
        MessageBox(0, L"Direct3D Feature Level 11 unsupported.", 0, 0);
        return false;
    }

    // Check 4X MSAA quality support for our back buffer format.
    // All Direct3D 11 capable devices support 4X MSAA for all render 
    // target formats, so we only need to check quality support.
    m_D3DDevice->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 4, &m_4xMsaaQuality);

    // Fill out a DXGI_SWAP_CHAIN_DESC to describe our swap chain.
    DXGI_SWAP_CHAIN_DESC sd;
    sd.BufferDesc.Width = g_Engine->WindowWidth();
    sd.BufferDesc.Height = g_Engine->WindowHeight();
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

    // Use 4X MSAA? 
    if (m_Enable4xMsaa)
    {
        sd.SampleDesc.Count = 4;
        sd.SampleDesc.Quality = m_4xMsaaQuality - 1;
    }
    // No MSAA
    else
    {
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
    }

    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 1;
    sd.OutputWindow = g_Engine->CurrentWindow();
    sd.Windowed = true;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sd.Flags = 0;

    // To correctly create the swap chain, we must use the IDXGIFactory that was
    IDXGIDevice* dxgiDevice = 0;
    m_D3DDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);

    IDXGIAdapter* dxgiAdapter = 0;
    dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgiAdapter);

    IDXGIFactory* dxgiFactory = 0;
    dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&dxgiFactory);

    dxgiFactory->CreateSwapChain(m_D3DDevice, &sd, &m_SwapChain);

    SAFE_RELEASE(dxgiDevice);
    SAFE_RELEASE(dxgiAdapter);
    SAFE_RELEASE(dxgiFactory);

    return true;
}

void CDxRenderer::InitSwapChainDesc(DXGI_SWAP_CHAIN_DESC& desc)
{
    ZeroMemory(&desc, sizeof(desc));

    //set buffer dimensions and format
    desc.BufferCount = 2;
    desc.BufferDesc.Width = g_Engine->WindowWidth();
    desc.BufferDesc.Height = g_Engine->WindowHeight();
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;;

    //set refresh rate
    desc.BufferDesc.RefreshRate.Numerator = 60;
    desc.BufferDesc.RefreshRate.Denominator = 1;

    //sampling settings
    desc.SampleDesc.Quality = 0;
    desc.SampleDesc.Count = 1;

    //output window handle
    desc.OutputWindow = g_Engine->CurrentWindow();
    desc.Windowed = true;
}

bool CDxRenderer::LoadEffectFile()
{
    DWORD shaderFlags = 0;
#if defined( DEBUG ) || defined( _DEBUG )
    shaderFlags |= D3D10_SHADER_DEBUG;
    shaderFlags |= D3D10_SHADER_SKIP_OPTIMIZATION;
#endif

    ID3D10Blob* compiledShader = 0;
    ID3D10Blob* compilationMsgs = 0;

    if (FAILED(D3DX11CompileFromFile(L"Effects/baseEffect.fx",
        0, 0, 0, "fx_5_0", shaderFlags, // #EFFECTS poprawic potem na 5_0
        0, 0, &compiledShader, &compilationMsgs, 0)))
        return utils::FatalError(L"Effects Init Error - CompileFromFile");


#if defined( DEBUG ) || defined( _DEBUG )
    if (compilationMsgs != 0)
    {
        MessageBoxA(0, (char*)compilationMsgs->GetBufferPointer(), 0, 0);
        SAFE_RELEASE(compilationMsgs);
    }
#endif

    if (FAILED(D3DX11CreateEffectFromMemory(compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(),
        0, m_D3DDevice, &m_Effect)))
        return utils::FatalError(L"Effects Init Error - CreateEffectFromMemory");

    SAFE_RELEASE(compiledShader);
    return true;
}

bool CDxRenderer::CreateDepthBuffer()
{
//     // Create the depth/stencil buffer and view.
     D3D11_TEXTURE2D_DESC depthStencilDesc;
     depthStencilDesc.Width = g_Engine->WindowWidth();
     depthStencilDesc.Height = g_Engine->WindowHeight();
     depthStencilDesc.MipLevels = 1;
     depthStencilDesc.ArraySize = 1;
     depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;

     // Use 4X MSAA? --must match swap chain MSAA values.
     if (m_Enable4xMsaa)
     {
         depthStencilDesc.SampleDesc.Count = 4;
         depthStencilDesc.SampleDesc.Quality = m_4xMsaaQuality - 1;
     }
     // No MSAA
     else
     {
         depthStencilDesc.SampleDesc.Count = 1;
         depthStencilDesc.SampleDesc.Quality = 0;
     }

     depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
     depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
     depthStencilDesc.CPUAccessFlags = 0;
     depthStencilDesc.MiscFlags = 0;

     if (FAILED(m_D3DDevice->CreateTexture2D(&depthStencilDesc, NULL, &m_DepthStencil)))
         return utils::FatalError(L"Could not create depth stencil texture");

     D3D11_DEPTH_STENCIL_VIEW_DESC DescDS;
     DescDS.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
     DescDS.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
     DescDS.Texture2D.MipSlice = 0;
     DescDS.Flags = 0;

     if (FAILED(m_D3DDevice->CreateDepthStencilView(m_DepthStencil, &DescDS, &m_DepthStencilView)))
         return utils::FatalError(L"Could not create depth stencil view");

//      // Create Shader Resource View for Depth Stencil
      D3D11_SHADER_RESOURCE_VIEW_DESC descSRV;
      descSRV.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
      descSRV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
      descSRV.Texture2D.MipLevels = depthStencilDesc.MipLevels;
      descSRV.Texture2D.MostDetailedMip = 0;
      if (FAILED(m_D3DDevice->CreateShaderResourceView(m_DepthStencil, &descSRV, &m_ZBufferSRView)))
          return utils::FatalError(L"Could not create depth stencil shader resource view");

     // Bind the render target view and depth/stencil view to the pipeline.
     m_D3DDeviceContext->OMSetRenderTargets(1, &m_RenderTargetView, m_DepthStencilView);


     return true;
}

bool CDxRenderer::CreateRenderTargetView()
{
    ID3D11Texture2D* backBuffer;
    if (FAILED(m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer))))
        return utils::FatalError(L"Could not get back buffer");
    if (FAILED(m_D3DDevice->CreateRenderTargetView(backBuffer, 0, &m_RenderTargetView)))
        return utils::FatalError(L"Could not create render target view");
    
    SAFE_RELEASE(backBuffer);
    m_D3DDeviceContext->OMSetRenderTargets(1, &m_RenderTargetView, nullptr);

    return true;
}

void CDxRenderer::InitAndSetViewport()
{
    m_ViewPort.Width = g_Engine->WindowWidth();
    m_ViewPort.Height = g_Engine->WindowHeight();
    m_ViewPort.MinDepth = 0.0f;
    m_ViewPort.MaxDepth = 1.0f;
    m_ViewPort.TopLeftX = 0;
    m_ViewPort.TopLeftY = 0;
    m_D3DDeviceContext->RSSetViewports(1, &m_ViewPort);
}

void CDxRenderer::InitEffectVariables()
{
    m_WorldEffectVariable = m_Effect->GetVariableByName("World")->AsMatrix();
    m_InvProjectionEffectVariable = m_Effect->GetVariableByName("InvProjection")->AsMatrix();
    m_ViewEffectVariable = m_Effect->GetVariableByName("View")->AsMatrix();
    m_ProjectionEffectVariable = m_Effect->GetVariableByName("Projection")->AsMatrix();
    m_CameraPosEffectVariable = m_Effect->GetVariableByName("CameraPos")->AsVector();
    m_CameraUpEffectVariable = m_Effect->GetVariableByName("CameraUp")->AsVector();
    m_DepthEffectVariable = m_Effect->GetVariableByName("txDepth")->AsShaderResource();
}


bool CDxRenderer::InitInputLayouts() 
{
    m_InputLayouts = m_InputLayouts->getInstance(m_D3DDevice, m_Effect); //#REDESIGN zmienic calkime input layouts
    if (m_InputLayouts == nullptr) return false;
    return true;
}

bool CDxRenderer::InitRasterizer() 
{
    D3D11_RASTERIZER_DESC rasterizerState;
    rasterizerState.CullMode = D3D11_CULL_BACK;
    rasterizerState.FillMode = D3D11_FILL_SOLID;
    rasterizerState.FrontCounterClockwise = true;
    rasterizerState.DepthBias = false;
    rasterizerState.SlopeScaledDepthBias = 0;
    rasterizerState.DepthBiasClamp = 0;
    rasterizerState.DepthClipEnable = true;
    rasterizerState.ScissorEnable = false;
    rasterizerState.MultisampleEnable = false;
    rasterizerState.AntialiasedLineEnable = true;

    ID3D11RasterizerState* pRS;
    if (FAILED(m_D3DDevice->CreateRasterizerState(&rasterizerState, &pRS)))
        return utils::FatalError(L"Can't init Rasterizer");
    m_D3DDeviceContext->RSSetState(pRS);
    return true;
}

void CDxRenderer::InitMatrices() 
{   
    m_World = XMMatrixIdentity();
}

void CDxRenderer::InitCamera() 
{
    g_pCamera = new CDxCamera();

    //Set up projection matrix
    g_pCamera->SetPerspectiveProjection(60, (float)g_Engine->WindowWidth() / (float)g_Engine->WindowHeight(), 0.1f, 600.0f);
}

void CDxRenderer::ClearScene() 
{
    ID3D11ShaderResourceView* null[] = { nullptr, nullptr };
    m_D3DDeviceContext->PSSetShaderResources(0, 2, null);
    m_DepthEffectVariable->SetResource(nullptr);

    m_D3DDeviceContext->OMSetRenderTargets(1, &m_RenderTargetView, m_DepthStencilView);
    m_D3DDeviceContext->ClearRenderTargetView(m_RenderTargetView, m_BgColor); 
    m_D3DDeviceContext->ClearDepthStencilView(m_DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void CDxRenderer::SetDepthBufferAsResource()
{
    m_D3DDeviceContext->OMSetRenderTargets(1, &m_RenderTargetView, nullptr);
    m_DepthEffectVariable->SetResource(m_ZBufferSRView);
}

void CDxRenderer::SetPrimitiveTopology()
{
    m_D3DDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
}

void CDxRenderer::RenderGObjects()
{
    SetPrimitiveTopology();
    for (auto obj : m_RenderQueue)
        obj->Render();
}

void CDxRenderer::RenderFx()
{
    SetDepthBufferAsResource();
    g_Engine->GetParticleManager()->PreRender();
    g_Engine->GetParticleManager()->Render(); //#REDESIGN
    g_Engine->GetParticleManager()->PostRender();
}

void CDxRenderer::SetEffectVariablesPerFrame()
{
    m_WorldEffectVariable->SetMatrix(reinterpret_cast<float*>(&m_World));
    m_ViewEffectVariable->SetMatrix(reinterpret_cast<float*>(&g_pCamera->ViewMatrix()));
    m_ProjectionEffectVariable->SetMatrix(reinterpret_cast<float*>(&g_pCamera->ProjectionMatrix()));
    m_CameraPosEffectVariable->SetFloatVector(reinterpret_cast<float*>(&g_pCamera->CameraPosition()));
}

void CDxRenderer::SetEffectVariablesPerInit()
{
    m_InvProjectionEffectVariable->SetMatrix(reinterpret_cast<float*>(&g_pCamera->InvProjectionMatrix()));
    m_CameraUpEffectVariable->SetFloatVector(reinterpret_cast<float*>(&g_pCamera->CameraUp()));
}

void CDxRenderer::ReleaseRenderObjects()
{
    for (auto obj : m_RenderQueue)
        SAFE_DELETE(obj);

    m_RenderQueue.clear();
}