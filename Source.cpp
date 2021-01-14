// System includes
#include <windows.h>

// DirectX includes
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXColors.h>

// STL includes
#include <iostream>
#include <string>

#include "InitDirect3D.h"

// Directt X 11 library dependencies
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "winmm.lib")

using namespace DirectX;

// START Windows specific window variables
const LONG g_WindowWidth = 1280;
const LONG g_WindowHeight = 720;
LPCTSTR  g_WindowClassName = TEXT("DirectXWindowClass");
LPCTSTR  g_WindowName = TEXT("pug");
//HWND g_WindowHandle = 0;

const BOOL g_EnableVSync = FALSE;
// END

// START Direct X specific variables
// Direct3D device and swap chain.
//ID3D11Device* g_d3dDevice = nullptr;
//ID3D11DeviceContext* g_d3dDeviceContext = nullptr;
//IDXGISwapChain* g_d3dSwapChain = nullptr;
//
//// Render target view for the back buffer of the swap chain.
//ID3D11RenderTargetView* g_d3dRenderTargetView = nullptr;
//// Depth/stencil view for use as a depth buffer.
//ID3D11DepthStencilView* g_d3dDepthStencilView = nullptr;
//// A texture to associate to the depth stencil view.
//ID3D11Texture2D* g_d3dDepthStencilBuffer = nullptr;
//
//// Define the functionality of the depth/stencil stages.
//ID3D11DepthStencilState* g_d3dDepthStencilState = nullptr;
//// Define the functionality of the rasterizer stage.
//ID3D11RasterizerState* g_d3dRasterizerState = nullptr;
//D3D11_VIEWPORT g_Viewport = { 0 };
// END

// START Application specific variables
// Vertex buffer data
ID3D11InputLayout* g_d3dInputLayout = nullptr;
ID3D11Buffer* g_d3dVertexBuffer = nullptr;
ID3D11Buffer* g_d3dIndexBuffer = nullptr;

// Shader data
ID3D11VertexShader* g_d3dVertexShader = nullptr;
ID3D11PixelShader* g_d3dPixelShader = nullptr;
// END

// Shader resources
enum ConstantBuffer {
    CB_Application,
    CB_Frame,
    CB_Object,
    NumConstantBuffers
};

ID3D11Buffer* g_d3dConstantBuffers[NumConstantBuffers];

// Demo parameters
XMMATRIX g_WorldMatrix;
XMMATRIX g_ViewMatrix;
XMMATRIX g_ProjectionMatrix;

// Vertex data for a colored cube.
struct VertexPosColor {
    XMFLOAT3 Position;
    XMFLOAT3 Color;
};

VertexPosColor g_Vertices[8] = {
      // Position                    // Color
    { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) }, // 0
    { XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) }, // 1
    { XMFLOAT3( 1.0f,  1.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f) }, // 2
    { XMFLOAT3( 1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) }, // 3
    { XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) }, // 4
    { XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f, 1.0f, 1.0f) }, // 5
    { XMFLOAT3( 1.0f,  1.0f,  1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, // 6
    { XMFLOAT3( 1.0f, -1.0f,  1.0f), XMFLOAT3(1.0f, 0.0f, 1.0f) }  // 7
};

WORD g_Indicies[36] = {
    0, 1, 2, 0, 2, 3,
    4, 6, 5, 4, 7, 6,
    4, 5, 1, 4, 1, 0,
    3, 2, 6, 3, 6, 7,
    1, 5, 6, 1, 6, 2,
    4, 0, 3, 4, 3, 7
};

// Forward declarations.
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

template< class ShaderClass >
ShaderClass* LoadShader(const std::wstring& fileName, const std::string& entryPoint, const std::string& profile);

bool LoadContent();
void UnloadContent();

void Update(float deltaTime);
void Render();
void Cleanup();

/**
 * Initialize the application window.
 */
int InitApplication(HINSTANCE hInstance, int cmdShow) {
    WNDCLASSEX wndClass = { 0 };

    wndClass.cbSize = sizeof(WNDCLASSEX);
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = &WndProc;
    wndClass.hInstance = hInstance;
    wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wndClass.lpszMenuName = nullptr;
    wndClass.lpszClassName = g_WindowClassName;

    if (!RegisterClassEx(&wndClass))
        return -1;

    RECT windowRect = { 0, 0, g_WindowWidth, g_WindowHeight };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    g_WindowHandle = CreateWindow(
        g_WindowClassName,
        g_WindowName,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!g_WindowHandle)
        return -1;

    ShowWindow(g_WindowHandle, cmdShow);
    UpdateWindow(g_WindowHandle);

    return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    PAINTSTRUCT paintStruct;
    HDC hDC;

    switch (message) {
        case WM_PAINT: {
            hDC = BeginPaint(hwnd, &paintStruct);
            EndPaint(hwnd, &paintStruct);
        }
        break;
        case WM_DESTROY: {
            PostQuitMessage(0);
        }
        break;
        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }

    return 0;
}

/**
 * The main application loop.
 */
int Run() {
    MSG msg = { 0 };

    static DWORD previousTime = timeGetTime();
    static const float targetFramerate = 30.0f;
    static const float maxTimeStep = 1.0f / targetFramerate;

    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            DWORD currentTime = timeGetTime();
            float deltaTime = (currentTime - previousTime) / 1000.0f;
            previousTime = currentTime;

            // Cap the delta time to the max time step (useful if your 
            // debugging and you don't want the deltaTime value to explode.
            deltaTime = std::min<float>(deltaTime, maxTimeStep);

            Update(deltaTime);
            Render();
        }
    }

    return static_cast<int>(msg.wParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PWSTR cmdLine, int cmdShow) {
    UNREFERENCED_PARAMETER(prevInstance);
    UNREFERENCED_PARAMETER(cmdLine);

    // Check for DirectX Math library support.
    if (!XMVerifyCPUSupport()) {
        MessageBox(nullptr, TEXT("Failed to verify DirectX Math library support."), TEXT("Error"), MB_OK);
        return -1;
    }

    if (InitApplication(hInstance, cmdShow) != 0) {
        MessageBox(nullptr, TEXT("Failed to create applicaiton window."), TEXT("Error"), MB_OK);
        return -1;
    }

    if (InitDirectX(hInstance, g_EnableVSync) != 0) {
        MessageBox(nullptr, TEXT("Failed to create DirectX device and swap chain."), TEXT("Error"), MB_OK);
        return -1;
    }

    if (!LoadContent()) {
        MessageBox(nullptr, TEXT("Failed to load content."), TEXT("Error"), MB_OK);
        return -1;
    }

    int returnCode = Run();

    UnloadContent();
    Cleanup();

    return returnCode;
}

// Get the latest profile for the specified shader type.
template< class ShaderClass >
std::string GetLatestProfile();

template<>
std::string GetLatestProfile<ID3D11VertexShader>() {
    assert(g_d3dDevice);

    // Query the current feature level:
    D3D_FEATURE_LEVEL featureLevel = g_d3dDevice->GetFeatureLevel();

    switch (featureLevel) {
        case D3D_FEATURE_LEVEL_11_1:
        case D3D_FEATURE_LEVEL_11_0: {
            return "vs_5_0";
        }
        break;
        case D3D_FEATURE_LEVEL_10_1: {
            return "vs_4_1";
        }
        break;
        case D3D_FEATURE_LEVEL_10_0: {
            return "vs_4_0";
        }
        break;
        case D3D_FEATURE_LEVEL_9_3: {
            return "vs_4_0_level_9_3";
        }
        break;
        case D3D_FEATURE_LEVEL_9_2:
        case D3D_FEATURE_LEVEL_9_1: {
            return "vs_4_0_level_9_1";
        }
        break;
    }

    return "";
}

template<>
std::string GetLatestProfile<ID3D11PixelShader>() {
    assert(g_d3dDevice);

    // Query the current feature level:
    D3D_FEATURE_LEVEL featureLevel = g_d3dDevice->GetFeatureLevel();
    switch (featureLevel) {
        case D3D_FEATURE_LEVEL_11_1:
        case D3D_FEATURE_LEVEL_11_0: {
            return "ps_5_0";
        }
        break;
        case D3D_FEATURE_LEVEL_10_1: {
            return "ps_4_1";
        }
        break;
        case D3D_FEATURE_LEVEL_10_0: {
            return "ps_4_0";
        }
        break;
        case D3D_FEATURE_LEVEL_9_3: {
            return "ps_4_0_level_9_3";
        }
        break;
        case D3D_FEATURE_LEVEL_9_2:
        case D3D_FEATURE_LEVEL_9_1: {
            return "ps_4_0_level_9_1";
        }
        break;
    }

    return "";
}

bool LoadContent() {
    assert(g_d3dDevice);

    // Create an initialize the vertex buffer.
    D3D11_BUFFER_DESC vertexBufferDesc;
    
    ZeroMemory(&vertexBufferDesc, sizeof(D3D11_BUFFER_DESC));

    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.ByteWidth = sizeof(VertexPosColor) * _countof(g_Vertices);
    vertexBufferDesc.CPUAccessFlags = 0;
    vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;

    D3D11_SUBRESOURCE_DATA resourceData;
    
    ZeroMemory(&resourceData, sizeof(D3D11_SUBRESOURCE_DATA));

    resourceData.pSysMem = g_Vertices;

    HRESULT hr = g_d3dDevice->CreateBuffer(&vertexBufferDesc, &resourceData, &g_d3dVertexBuffer);
    
    if (FAILED(hr))
        return false;

    // Create and initialize the index buffer.
    D3D11_BUFFER_DESC indexBufferDesc;
    
    ZeroMemory(&indexBufferDesc, sizeof(D3D11_BUFFER_DESC));

    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufferDesc.ByteWidth = sizeof(WORD) * _countof(g_Indicies);
    indexBufferDesc.CPUAccessFlags = 0;
    indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    resourceData.pSysMem = g_Indicies;

    hr = g_d3dDevice->CreateBuffer(&indexBufferDesc, &resourceData, &g_d3dIndexBuffer);
    
    if (FAILED(hr))
        return false;

    // Create the constant buffers for the variables defined in the vertex shader.
    D3D11_BUFFER_DESC constantBufferDesc;
    ZeroMemory(&constantBufferDesc, sizeof(D3D11_BUFFER_DESC));

    constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    constantBufferDesc.ByteWidth = sizeof(XMMATRIX);
    constantBufferDesc.CPUAccessFlags = 0;
    constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;

    hr = g_d3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &g_d3dConstantBuffers[CB_Application]);
    
    if (FAILED(hr))
        return false;
    
    hr = g_d3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &g_d3dConstantBuffers[CB_Frame]);
    
    if (FAILED(hr))
        return false;
    
    hr = g_d3dDevice->CreateBuffer(&constantBufferDesc, nullptr, &g_d3dConstantBuffers[CB_Object]);
    
    if (FAILED(hr))
        return false;

    // Load the compiled vertex shader.
    ID3DBlob* vertexShaderBlob;
    LPCWSTR compiledVertexShaderObject = L"./out/SimpleVertexShader.cso";

    hr = D3DReadFileToBlob(compiledVertexShaderObject, &vertexShaderBlob);
    
    if (FAILED(hr))
        return false;

    hr = g_d3dDevice->CreateVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), nullptr, &g_d3dVertexShader);
   
    if (FAILED(hr))
        return false;

    // Create the input layout for the vertex shader.
    D3D11_INPUT_ELEMENT_DESC vertexLayoutDesc[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(VertexPosColor,Position), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(VertexPosColor,Color), D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    hr = g_d3dDevice->CreateInputLayout(vertexLayoutDesc, _countof(vertexLayoutDesc), vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), &g_d3dInputLayout);
    if (FAILED(hr))
        return false;

    SafeRelease(vertexShaderBlob);

    // Load the compiled pixel shader.
    ID3DBlob* pixelShaderBlob;
    LPCWSTR compiledPixelShaderObject = L"./out/SimplePixelShader.cso";

    hr = D3DReadFileToBlob(compiledPixelShaderObject, &pixelShaderBlob);
    
    if (FAILED(hr))
        return false;

    hr = g_d3dDevice->CreatePixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize(), nullptr, &g_d3dPixelShader);
    
    if (FAILED(hr))
        return false;

    SafeRelease(pixelShaderBlob);

    // Setup the projection matrix.
    RECT clientRect;
    GetClientRect(g_WindowHandle, &clientRect);

    // Compute the exact client dimensions.
    // This is required for a correct projection matrix.
    float clientWidth = static_cast<float>(clientRect.right - clientRect.left);
    float clientHeight = static_cast<float>(clientRect.bottom - clientRect.top);

    g_ProjectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0f), clientWidth / clientHeight, 0.1f, 100.0f);

    g_d3dDeviceContext->UpdateSubresource(g_d3dConstantBuffers[CB_Application], 0, nullptr, &g_ProjectionMatrix, 0, 0);

    return true;
}

void Update(float deltaTime) {
    XMVECTOR eyePosition = XMVectorSet(0, 0, -10, 1);
    XMVECTOR focusPoint = XMVectorSet(0, 0, 0, 1);
    XMVECTOR upDirection = XMVectorSet(0, 1, 0, 0);
    
    g_ViewMatrix = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);
    g_d3dDeviceContext->UpdateSubresource(g_d3dConstantBuffers[CB_Frame], 0, nullptr, &g_ViewMatrix, 0, 0);


    static float angle = 0.0f;
    angle += 90.0f * deltaTime;
    XMVECTOR rotationAxis = XMVectorSet(0, 1, 1, 0);

    g_WorldMatrix = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(angle));
    g_d3dDeviceContext->UpdateSubresource(g_d3dConstantBuffers[CB_Object], 0, nullptr, &g_WorldMatrix, 0, 0);
}

// Clear the color and depth buffers.
void Clear(const FLOAT clearColor[4], FLOAT clearDepth, UINT8 clearStencil) {
    g_d3dDeviceContext->ClearRenderTargetView(g_d3dRenderTargetView, clearColor);
    g_d3dDeviceContext->ClearDepthStencilView(g_d3dDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, clearDepth, clearStencil);
}

void Present(bool vSync) {
    if (vSync)
        g_d3dSwapChain->Present(1, 0);
    else
        g_d3dSwapChain->Present(0, 0);
}

void Render() {
    assert(g_d3dDevice);
    assert(g_d3dDeviceContext);

    Clear(Colors::CornflowerBlue, 1.0f, 0);

    const UINT vertexStride = sizeof(VertexPosColor);
    const UINT offset = 0;

    g_d3dDeviceContext->IASetVertexBuffers(0, 1, &g_d3dVertexBuffer, &vertexStride, &offset);
    g_d3dDeviceContext->IASetInputLayout(g_d3dInputLayout);
    g_d3dDeviceContext->IASetIndexBuffer(g_d3dIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    g_d3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    g_d3dDeviceContext->VSSetShader(g_d3dVertexShader, nullptr, 0);
    g_d3dDeviceContext->VSSetConstantBuffers(0, 3, g_d3dConstantBuffers);

    g_d3dDeviceContext->RSSetState(g_d3dRasterizerState);
    g_d3dDeviceContext->RSSetViewports(1, &g_Viewport);

    g_d3dDeviceContext->PSSetShader(g_d3dPixelShader, nullptr, 0);

    g_d3dDeviceContext->OMSetRenderTargets(1, &g_d3dRenderTargetView, g_d3dDepthStencilView);
    g_d3dDeviceContext->OMSetDepthStencilState(g_d3dDepthStencilState, 1);

    g_d3dDeviceContext->DrawIndexed(_countof(g_Indicies), 0, 0);

    Present(g_EnableVSync);
}

void UnloadContent() {
    SafeRelease(g_d3dConstantBuffers[CB_Application]);
    SafeRelease(g_d3dConstantBuffers[CB_Frame]);
    SafeRelease(g_d3dConstantBuffers[CB_Object]);
    SafeRelease(g_d3dIndexBuffer);
    SafeRelease(g_d3dVertexBuffer);
    SafeRelease(g_d3dInputLayout);
    SafeRelease(g_d3dVertexShader);
    SafeRelease(g_d3dPixelShader);
}

void Cleanup() {
    SafeRelease(g_d3dDepthStencilView);
    SafeRelease(g_d3dRenderTargetView);
    SafeRelease(g_d3dDepthStencilBuffer);
    SafeRelease(g_d3dDepthStencilState);
    SafeRelease(g_d3dRasterizerState);
    SafeRelease(g_d3dSwapChain);
    SafeRelease(g_d3dDeviceContext);
    SafeRelease(g_d3dDevice);
}


