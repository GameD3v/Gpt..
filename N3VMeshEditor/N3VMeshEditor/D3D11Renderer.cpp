// D3D11Renderer.cpp
#include "D3D11Renderer.h"
#include <cstdio>                       // fprintf için
#include <exception>                    // std::exception için
#include <string>                       // std::string için (artık std::wstring de kullanacağız)
#include <Windows.h>                    // HANDLE, CreateFileW, GetLastError, FormatMessageW için
#include <QMessageBox>
#include <vector>                       // std::vector için
#include <QDebug> // qDebug için
#pragma comment(lib, "d3dcompiler.lib") // D3DCompile fonksiyonu için gerekli kütüphaneyi otomatik bağlar.
#include <algorithm>                    // std::min ve std::max için (önceki versiyonlarda eklendi)
#include <d3dcompiler.h> // D3DCompileFromFile için
#include <DirectXColors.h> // Renkler için (isteğe bağlı)
#include <Shlwapi.h> // PathFindExtensionW için (genel olarak Windows API, dosya uzantısını bulmak için)
#include <DirectXMath.h>

// Shader kodları (string olarak)
// Not: Gerçek bir uygulamada bunları ayrı .hlsl dosyalarında tutmak daha iyidir.
// Ancak hızlı başlangıç için burada tanımlanabilir.
using namespace DirectX;

const char* g_vertexShaderCode = R"(
struct ConstantBufferData
{
    matrix World;
    matrix View;
    matrix Projection;
    int RenderMode;
    float padding[3];
};

cbuffer ConstantBuffer : register(b0)
{
    ConstantBufferData cb;
};

struct VS_INPUT
{
    float3 Pos : POSITION;
    float4 Color : COLOR; // Renk bilgisini de al
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR;
    int RenderMode : RENDERMODE; // Render modunu pixel shadere aktar
};

PS_INPUT VSMain(VS_INPUT input)
{
    PS_INPUT output;
    output.Pos = mul(float4(input.Pos, 1.0f), cb.World);
    output.Pos = mul(output.Pos, cb.View);
    output.Pos = mul(output.Pos, cb.Projection);

    output.Color = input.Color; // Gelen rengi aktar

    // RenderMode'u doğrudan aktar
    output.RenderMode = cb.RenderMode;

    return output;
}
)";

const char* g_pixelShaderCode = R"(
struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR;
    int RenderMode : RENDERMODE; // Render modunu al
};

float4 PSMain(PS_INPUT input) : SV_TARGET
{
    if (input.RenderMode == 0) // Normal (objenin kendi rengi)
    {
        return input.Color;
    }
    else if (input.RenderMode == 1) // Kırmızı tel kafes (seçili değil ama wireframe modda)
    {
        return float4(1.0f, 0.0f, 0.0f, 1.0f); // Kırmızı
    }
    else if (input.RenderMode == 2) // Yeşil tel kafes (seçili)
    {
        return float4(0.0f, 1.0f, 0.0f, 1.0f); // Yeşil
    }
    return input.Color; // Varsayılan olarak kendi rengini döndür
}
)";


D3D11Renderer::D3D11Renderer()
    : m_d3dDevice(nullptr)
    , m_d3dContext(nullptr)
    , m_swapChain(nullptr)
    , m_renderTargetView(nullptr)
    , m_depthStencilView(nullptr)
    , m_vertexBuffer(nullptr) // Yeni: Genel Vertex Buffer
    , m_indexBuffer(nullptr)  // Yeni: Genel Index Buffer
    , m_constantBuffer(nullptr)
    , m_inputLayout(nullptr)
    , m_vertexShader(nullptr)
    , m_pixelShader(nullptr)
    , m_vertexShaderBlob(nullptr)
    , m_gridVertexBuffer(nullptr)
    , m_gridVertexCount(0)
    , m_solidRasterizerState(nullptr)
    , m_wireframeRasterizerState(nullptr)
    , m_cullFrontRasterizerState(nullptr)
    , m_cameraPos(0.0f, 0.0f, -10.0f)
    , m_cameraTarget(0.0f, 0.0f, 0.0f)
    , m_cameraUp(0.0f, 1.0f, 0.0f)
    , m_cameraRadius(10.0f)
    , m_yaw(0.0f)
    , m_pitch(0.0f)
    , m_zoomSpeed(0.05f)
    , m_mouseSpeedX(0.005f)
    , m_mouseSpeedY(0.005f)
    , m_isMeshSelected(false)
    , m_worldTranslation(0.0f, 0.0f, 0.0f)
    , m_previousMouseWorldPos(0.0f, 0.0f, 0.0f)
    , m_selectedMeshInitialDepth(0.0f)
    , m_isDraggingMeshNow(false)
    , m_wireframeMode(false)
    , m_width(0)
    , m_height(0)
    , m_activeMeshType(MeshType::NONE) // Yeni: Başlangıçta aktif mesh yok
{
    SetupCamera();
}

D3D11Renderer::~D3D11Renderer()
{
    Shutdown();
}

bool D3D11Renderer::Initialize(HWND hWnd, int width, int height)
{
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    UINT numFeatureLevels = ARRAYSIZE(featureLevels);

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        createDeviceFlags,
        featureLevels,
        numFeatureLevels,
        D3D11_SDK_VERSION,
        &sd,
        m_swapChain.GetAddressOf(),
        m_d3dDevice.GetAddressOf(),
        nullptr, // &featureLevel,
        m_d3dContext.GetAddressOf()
    );

    if (FAILED(hr))
    {
        qDebug() << "Hata: D3D11CreateDeviceAndSwapChain basarisiz! HRESULT: " << QString("0x%1").arg(hr, 8, 16, QChar('0'));
        return false;
    }

    m_width = width;
    m_height = height;

    Resize(width, height);

    if (!CompileShader(g_vertexShaderCode, "VSMain", "vs_5_0", m_vertexShaderBlob.GetAddressOf())) return false;
    hr = m_d3dDevice->CreateVertexShader(m_vertexShaderBlob->GetBufferPointer(), m_vertexShaderBlob->GetBufferSize(), nullptr, m_vertexShader.GetAddressOf());
    if (FAILED(hr)) {
        qDebug() << "Hata: Vertex Shader olusturulurken hata! HRESULT: " << QString("0x%1").arg(hr, 8, 16, QChar('0'));
        return false;
    }

    if (!CompileShader(g_pixelShaderCode, "PSMain", "ps_5_0", m_pixelShaderBlob.GetAddressOf())) return false;
    hr = m_d3dDevice->CreatePixelShader(m_pixelShaderBlob->GetBufferPointer(), m_pixelShaderBlob->GetBufferSize(), nullptr, m_pixelShader.GetAddressOf());
    if (FAILED(hr)) {
        qDebug() << "Hata: Pixel Shader olusturulurken hata! HRESULT: " << QString("0x%1").arg(hr, 8, 16, QChar('0'));
        return false;
    }

    if (!CreateBuffers()) return false;
    if (!CreateRasterizerStates()) return false;

    CreateGridBuffers(100.0f, 100);

    return true;
}

bool D3D11Renderer::CompileShader(const char* shaderCode, const char* entryPoint, const char* profile, ID3DBlob** blob)
{
    ID3DBlob* errorBlob = nullptr;
    HRESULT hr = D3DCompile(shaderCode, strlen(shaderCode), nullptr, nullptr, nullptr, entryPoint, profile,
        D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, blob, &errorBlob);

    if (FAILED(hr))
    {
        if (errorBlob)
        {
            qDebug() << "Shader Derleme Hatasi (" << entryPoint << "): " << (char*)errorBlob->GetBufferPointer();
            errorBlob->Release();
        }
        else
        {
            qDebug() << "Shader Derleme Hatasi (Bilinmeyen Hata): " << QString("0x%1").arg(hr, 8, 16, QChar('0'));
        }
        return false;
    }
    if (errorBlob) errorBlob->Release();
    return true;
}

void D3D11Renderer::Shutdown()
{
    // Temel DirectX objeleri
    m_vertexBuffer.Reset();
    m_indexBuffer.Reset();
    m_constantBuffer.Reset();
    m_inputLayout.Reset();
    m_vertexShader.Reset();
    m_pixelShader.Reset();
    m_vertexShaderBlob.Reset();
    m_renderTargetView.Reset();
    m_depthStencilView.Reset();
    m_solidRasterizerState.Reset();
    m_wireframeRasterizerState.Reset();
    m_cullFrontRasterizerState.Reset();
    m_swapChain.Reset();
    m_d3dContext.Reset();
    m_d3dDevice.Reset();

    // Mesh objelerini de serbest bırak
    m_collisionMesh.Release();
    m_n3Mesh.Release();
}

void D3D11Renderer::Resize(int width, int height)
{
    if (!m_d3dContext || !m_swapChain || width == 0 || height == 0) return;

    m_d3dContext->OMSetRenderTargets(0, nullptr, nullptr);
    m_renderTargetView.Reset();
    m_depthStencilView.Reset();

    HRESULT hr = m_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(hr)) {
        qDebug() << "Hata: Swap Chain yeniden boyutlandirilirken hata! HRESULT: " << QString("0x%1").arg(hr, 8, 16, QChar('0'));
        return;
    }

    Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
    hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)backBuffer.GetAddressOf());
    if (FAILED(hr)) {
        qDebug() << "Hata: Swap Chain'den Back Buffer alinirken hata! HRESULT: " << QString("0x%1").arg(hr, 8, 16, QChar('0'));
        return;
    }
    hr = m_d3dDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, m_renderTargetView.GetAddressOf());
    if (FAILED(hr)) {
        qDebug() << "Hata: Render Target View olusturulurken hata! HRESULT: " << QString("0x%1").arg(hr, 8, 16, QChar('0'));
        return;
    }

    D3D11_TEXTURE2D_DESC depthStencilDesc = {};
    depthStencilDesc.Width = width;
    depthStencilDesc.Height = height;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.ArraySize = 1;
    depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.SampleDesc.Quality = 0;
    depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
    depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthStencilDesc.CPUAccessFlags = 0;
    depthStencilDesc.MiscFlags = 0;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> depthStencilBuffer;
    hr = m_d3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, depthStencilBuffer.GetAddressOf());
    if (FAILED(hr)) {
        qDebug() << "Hata: Derinlik Stencil Buffer olusturulurken hata! HRESULT: " << QString("0x%1").arg(hr, 8, 16, QChar('0'));
        return;
    }

    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = depthStencilDesc.Format;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;

    hr = m_d3dDevice->CreateDepthStencilView(depthStencilBuffer.Get(), &dsvDesc, m_depthStencilView.GetAddressOf());
    if (FAILED(hr)) {
        qDebug() << "Hata: Derinlik Stencil View olusturulurken hata! HRESULT: " << QString("0x%1").arg(hr, 8, 16, QChar('0'));
        return;
    }

    m_width = width;
    m_height = height;

    m_projectionMatrix = XMMatrixPerspectiveFovLH(XM_PIDIV4, (float)m_width / (float)m_height, 0.01f, 1000.0f);
}

bool D3D11Renderer::LoadMesh(const std::wstring& filePath)
{
    m_vertexBuffer.Reset(); // Önceki buffer'ı temizle
    m_indexBuffer.Reset();  // Önceki buffer'ı temizle
    m_activeMeshType = MeshType::NONE; // Her yükleme denemesinde sıfırla

    HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        DWORD error = GetLastError();
        LPWSTR messageBuffer = nullptr;
        size_t size = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&messageBuffer, 0, NULL);

        qDebug() << "Hata: Mesh dosyasi acilamadi: " << QString::fromStdWString(filePath) << "(Hata Kodu: " << error << " - " << (messageBuffer ? QString::fromWCharArray(messageBuffer) : "Bilinmeyen hata") << ")";

        if (messageBuffer) LocalFree(messageBuffer);
        return false;
    }

    // Dosya uzantısını kontrol et
    std::wstring extension = PathFindExtensionW(filePath.c_str());
    bool loadSuccessful = false;
    UINT vertexCount = 0;
    UINT indexCount = 0;
    void* verticesData = nullptr; // Geçici vertex data pointer'ı
    uint16_t* indicesData = nullptr; // Geçici index data pointer'ı
    size_t vertexStride = sizeof(__VertexColor); // Varsayılan stride

    // Önceki yüklemeleri temizle (önemli: Release() çağrıları)
    m_collisionMesh.Release();
    m_n3Mesh.Release();

    if (extension == L".n3vmesh")
    {
        if (m_collisionMesh.Load(hFile))
        {
            if (m_collisionMesh.VertexCount() == 0) {
                qDebug() << "Hata: Yuklenen n3vmesh'te hic vertex yok: " << QString::fromStdWString(filePath);
                loadSuccessful = false;
            }
            else {
                vertexCount = m_collisionMesh.VertexCount();
                indexCount = m_collisionMesh.IndexCount();
                verticesData = m_collisionMesh.GetVertices();
                indicesData = m_collisionMesh.GetIndices();
                m_activeMeshType = MeshType::CN3VMESH;
                loadSuccessful = true;
                qDebug() << "CN3VMesh basariyla yuklendi. Vertex: " << vertexCount << ", Index: " << indexCount;
            }
        }
        else
        {
            qDebug() << "Hata: CN3VMesh dosyasi yuklenemedi: " << QString::fromStdWString(filePath);
            loadSuccessful = false;
        }
    }
    else if (extension == L".n3mesh")
    {
        if (m_n3Mesh.Load(hFile, m_d3dDevice.Get()))
        {
            if (m_n3Mesh.GetVertexCount() == 0) {
                qDebug() << "Hata: Yuklenen n3mesh'te hic vertex yok: " << QString::fromStdWString(filePath);
                loadSuccessful = false;
            }
            else {
                // N3Mesh'ten vertex verilerini __VertexColor'a dönüştür
                // N3Mesh çeşitli FVF'ler destekler, bu yüzden FVF'yi kontrol etmeliyiz.
                // Burada sadece FVF_XYZ ve FVF_DIFFUSE (yani __VertexColor) desteklendiğini varsayalım.
                // Eğer farklı FVF'ler varsa, bu kısım genişletilmelidir.
                DWORD n3MeshFVF = m_n3Mesh.GetFVF();

                // FVF_XYZ ve FVF_DIFFUSE olması beklenir (__VertexColor için)
                if ((n3MeshFVF & FVF_XYZ) && (n3MeshFVF & FVF_DIFFUSE))
                {
                    std::vector<__VertexColor> convertedVertices;
                    convertedVertices.reserve(m_n3Mesh.GetVertexCount());

                    const BYTE* pRawVertices = static_cast<const BYTE*>(m_n3Mesh.GetVertices());

                    // FVF_XYZ | FVF_DIFFUSE için vertex boyutu: 3 float (XYZ) + 1 DWORD (Color) = 16 bytes
                    size_t n3MeshVertexStrideInternal = sizeof(float) * 3 + sizeof(DWORD);

                    for (int i = 0; i < m_n3Mesh.GetVertexCount(); ++i)
                    {
                        const float* pos = reinterpret_cast<const float*>(pRawVertices + i * n3MeshVertexStrideInternal);
                        const DWORD* color = reinterpret_cast<const DWORD*>(pRawVertices + i * n3MeshVertexStrideInternal + sizeof(float) * 3);
                        convertedVertices.emplace_back(pos[0], pos[1], pos[2], *color);
                    }

                    vertexCount = static_cast<UINT>(convertedVertices.size());
                    indexCount = m_n3Mesh.GetIndexCount();
                    indicesData = m_n3Mesh.GetIndices();
                    m_activeMeshType = MeshType::N3Mesh;
                    loadSuccessful = true;

                    // Vertex buffer'ı dönüştürülmüş veriden oluştur
                    D3D11_BUFFER_DESC bd = {};
                    bd.Usage = D3D11_USAGE_DEFAULT;
                    bd.ByteWidth = vertexCount * vertexStride; // vertexStride = sizeof(__VertexColor)
                    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
                    bd.CPUAccessFlags = 0;
                    bd.MiscFlags = 0;
                    bd.StructureByteStride = 0;

                    D3D11_SUBRESOURCE_DATA InitData = {};
                    InitData.pSysMem = convertedVertices.data(); // std::vector'ın verisi

                    HRESULT hr = m_d3dDevice->CreateBuffer(&bd, &InitData, m_vertexBuffer.GetAddressOf());
                    if (FAILED(hr))
                    {
                        qDebug() << "Hata: Vertex Buffer (N3Mesh) olusturulurken hata! HRESULT: " << QString("0x%1").arg(hr, 8, 16, QChar('0'));
                        CloseHandle(hFile);
                        return false;
                    }
                    qDebug() << "N3Mesh Vertex Buffer olusturuldu. Vertex: " << vertexCount << ", Index: " << indexCount;
                }
                else
                {
                    qDebug() << "Hata: Desteklenmeyen N3Mesh FVF formati (yalnizca FVF_XYZ | FVF_DIFFUSE bekleniyor): " << n3MeshFVF;
                    loadSuccessful = false;
                }
            }
        }
        else
        {
            qDebug() << "Hata: N3Mesh dosyasi yuklenemedi: " << QString::fromStdWString(filePath);
            loadSuccessful = false;
        }
    }
    else
    {
        qDebug() << "Hata: Desteklenmeyen mesh dosya uzantisi: " << QString::fromStdWString(extension);
        loadSuccessful = false;
    }

    CloseHandle(hFile); // Dosya handle'ını kapat

    if (!loadSuccessful)
    {
        m_activeMeshType = MeshType::NONE;
        return false;
    }

    // Sadece CN3VMesh için Vertex Buffer oluşturma (N3Mesh için zaten yukarıda oluşturuldu)
    if (m_activeMeshType == MeshType::CN3VMESH)
    {
        D3D11_BUFFER_DESC bd = {};
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = vertexCount * vertexStride;
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = 0;
        bd.MiscFlags = 0;
        bd.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA InitData = {};
        InitData.pSysMem = verticesData; // CN3VMesh'ten gelen doğrudan data

        HRESULT hr = m_d3dDevice->CreateBuffer(&bd, &InitData, m_vertexBuffer.GetAddressOf());
        if (FAILED(hr))
        {
            qDebug() << "Hata: Vertex Buffer (CN3VMesh) olusturulurken hata! HRESULT: " << QString("0x%1").arg(hr, 8, 16, QChar('0'));
            return false;
        }
    }

    // Index Buffer oluşturma (her iki mesh tipi için ortak)
    if (indexCount > 0)
    {
        D3D11_BUFFER_DESC ibd = {};
        ibd.Usage = D3D11_USAGE_DEFAULT;
        ibd.ByteWidth = indexCount * sizeof(uint16_t); // uint16_t kullanıldığı varsayılıyor
        ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
        ibd.CPUAccessFlags = 0;
        ibd.MiscFlags = 0;
        ibd.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA InitIndexData = {};
        InitIndexData.pSysMem = indicesData;

        HRESULT hr = m_d3dDevice->CreateBuffer(&ibd, &InitIndexData, m_indexBuffer.GetAddressOf());
        if (FAILED(hr))
        {
            qDebug() << "Hata: Index Buffer olusturulurken hata! HRESULT: " << QString("0x%1").arg(hr, 8, 16, QChar('0'));
            return false;
        }
    }
    else
    {
        m_indexBuffer.Reset(); // Index yoksa buffer'ı sıfırla
        qDebug() << "Uyari: Mesh dosyasinda hic index yok: " << QString::fromStdWString(filePath);
    }

    qDebug() << "Mesh basariyla yuklendi: " << QString::fromStdWString(filePath);

    SetCameraToMeshBounds(); // Kamerayı yeni yüklenecek mesh'e göre ayarla

    return true;
}

void D3D11Renderer::Render()
{
    if (!m_d3dContext || !m_swapChain) return;

    const float clearColor[4] = { 61.0f / 255.0f, 61.0f / 255.0f, 61.0f / 255.0f, 1.0f };
    m_d3dContext->ClearRenderTargetView(m_renderTargetView.Get(), clearColor);
    m_d3dContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

    m_d3dContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());

    D3D11_VIEWPORT vp = { 0 };
    vp.Width = (FLOAT)m_width;
    vp.Height = (FLOAT)m_height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    m_d3dContext->RSSetViewports(1, &vp);

    SetupCamera();

    DrawGrid();

    // Mesh için genel ayarlar
    m_d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_d3dContext->IASetInputLayout(m_inputLayout.Get());
    m_d3dContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    m_d3dContext->PSSetShader(m_pixelShader.Get(), nullptr, 0);

    // --- Mesh Çizimi ---
    int currentVertexCount = 0;
    int currentIndexCount = 0;

    if (m_activeMeshType == MeshType::CN3VMESH)
    {
        currentVertexCount = m_collisionMesh.VertexCount();
        currentIndexCount = m_collisionMesh.IndexCount();
    }
    else if (m_activeMeshType == MeshType::N3Mesh)
    {
        currentVertexCount = m_n3Mesh.GetVertexCount();
        currentIndexCount = m_n3Mesh.GetIndexCount();
    }

    if (m_vertexBuffer && currentVertexCount > 0)
    {
        UINT stride = sizeof(__VertexColor); // Vertex yapınızın boyutu
        UINT offset = 0;
        m_d3dContext->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);

        // Index buffer sadece varsa bağlanır
        if (m_indexBuffer && currentIndexCount > 0)
        {
            m_d3dContext->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
        }

        m_worldMatrix = DirectX::XMMatrixTranslation(m_worldTranslation.x, m_worldTranslation.y, m_worldTranslation.z);

        ConstantBufferData cbData;
        cbData.World = DirectX::XMMatrixTranspose(m_worldMatrix);
        cbData.View = DirectX::XMMatrixTranspose(m_viewMatrix);
        cbData.Projection = DirectX::XMMatrixTranspose(m_projectionMatrix);

        // Render modu ayarlama
        if (m_isMeshSelected)
        {
            m_d3dContext->RSSetState(m_wireframeRasterizerState.Get()); // Seçiliyse tel kafes
            cbData.RenderMode = 2; // Yeşil çizgi
        }
        else if (m_wireframeMode)
        {
            m_d3dContext->RSSetState(m_wireframeRasterizerState.Get()); // Seçili değilse ve wireframe açıksa
            cbData.RenderMode = 1; // Kırmızı çizgi
        }
        else
        {
            m_d3dContext->RSSetState(m_solidRasterizerState.Get()); // Normal mod (dolu)
            cbData.RenderMode = 0; // Normal renk
        }

        m_d3dContext->UpdateSubresource(m_constantBuffer.Get(), 0, nullptr, &cbData, 0, 0);
        m_d3dContext->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
        m_d3dContext->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

        if (m_indexBuffer && currentIndexCount > 0) {
            m_d3dContext->DrawIndexed(currentIndexCount, 0, 0);
        }
        else {
            m_d3dContext->Draw(currentVertexCount, 0);
        }
    }

    HRESULT hr = m_swapChain->Present(1, 0);
    if (FAILED(hr))
    {
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            qDebug() << "Hata: DirectX cihazi kayboldu veya sifirlandi! HRESULT: " << QString("0x%1").arg(hr, 8, 16, QChar('0'));
        }
        else
        {
            qDebug() << "Hata: Swap Chain Present hatasi! HRESULT: " << QString("0x%1").arg(hr, 8, 16, QChar('0'));
        }
    }
}

void D3D11Renderer::CreateGridBuffers(float size, int subdivisions)
{
    float halfSize = size / 2.0f;
    float step = size / static_cast<float>(subdivisions);

    std::vector<GridVertex> vertices;
    vertices.reserve((subdivisions + 1) * 4);

    DirectX::XMFLOAT4 minorColor = DirectX::XMFLOAT4(80.0f / 255.0f, 80.0f / 255.0f, 80.0f / 255.0f, 1.0f);
    DirectX::XMFLOAT4 majorColor = DirectX::XMFLOAT4(100.0f / 255.0f, 100.0f / 255.0f, 100.0f / 255.0f, 1.0f);
    DirectX::XMFLOAT4 axisColor = DirectX::XMFLOAT4(150.0f / 255.0f, 150.0f / 255.0f, 150.0f / 255.0f, 1.0f);
    // DirectX::XMFLOAT4 axisZColor = DirectX::XMFLOAT4(0.0f / 255.0f, 0.0f / 255.0f, 255.0f / 255.0f, 1.0f); // Bu değişken kullanılmıyor, kaldırılabilir.

    int majorLineInterval = 10;

    for (int i = 0; i <= subdivisions; ++i) {
        float x = -halfSize + i * step;
        DirectX::XMFLOAT4 color = minorColor;

        if (fabs(x) < 0.001f) { // Yaklaşık olarak 0 ise (X ekseni)
            color = axisColor;
        }
        else if (i % majorLineInterval == 0) {
            color = majorColor;
        }

        vertices.push_back({ DirectX::XMFLOAT3(x, 0.0f, -halfSize), color });
        vertices.push_back({ DirectX::XMFLOAT3(x, 0.0f, halfSize), color });
    }

    for (int i = 0; i <= subdivisions; ++i) {
        float z = -halfSize + i * step;
        DirectX::XMFLOAT4 color = minorColor;

        if (fabs(z) < 0.001f) { // Yaklaşık olarak 0 ise (Z ekseni)
            color = axisColor;
        }
        else if (i % majorLineInterval == 0) {
            color = majorColor;
        }

        vertices.push_back({ DirectX::XMFLOAT3(-halfSize, 0.0f, z), color });
        vertices.push_back({ DirectX::XMFLOAT3(halfSize, 0.0f, z), color });
    }

    m_gridVertexCount = static_cast<UINT>(vertices.size());

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(GridVertex) * m_gridVertexCount;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    bd.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = vertices.data();

    HRESULT hr = m_d3dDevice->CreateBuffer(&bd, &initData, m_gridVertexBuffer.GetAddressOf());
    if (FAILED(hr)) {
        qDebug() << "Hata: Grid Vertex Buffer olusturulamadi! HRESULT: " << QString("0x%1").arg(hr, 8, 16, QChar('0'));
    }
}

void D3D11Renderer::DrawGrid()
{
    if (!m_gridVertexBuffer) {
        return;
    }

    m_d3dContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
    m_d3dContext->PSSetShader(m_pixelShader.Get(), nullptr, 0);
    m_d3dContext->IASetInputLayout(m_inputLayout.Get());

    UINT stride = sizeof(GridVertex);
    UINT offset = 0;
    m_d3dContext->IASetVertexBuffers(0, 1, m_gridVertexBuffer.GetAddressOf(), &stride, &offset);

    m_d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

    DirectX::XMMATRIX world = DirectX::XMMatrixIdentity();

    ConstantBufferData cb;
    cb.World = DirectX::XMMatrixTranspose(world);
    cb.View = DirectX::XMMatrixTranspose(m_viewMatrix);
    cb.Projection = DirectX::XMMatrixTranspose(m_projectionMatrix);

    cb.RenderMode = 0; // Grid için her zaman normal render modu

    m_d3dContext->UpdateSubresource(m_constantBuffer.Get(), 0, nullptr, &cb, 0, 0);
    m_d3dContext->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
    m_d3dContext->PSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());

    m_d3dContext->Draw(m_gridVertexCount, 0);
}

void D3D11Renderer::SetupCamera()
{
    XMVECTOR targetVec = XMLoadFloat3(&m_cameraTarget);

    XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, 0.0f);

    XMVECTOR cameraOffset = XMVector3TransformNormal(XMVectorSet(0.0f, 0.0f, -m_cameraRadius, 0.0f), rotationMatrix);

    XMVECTOR Eye = targetVec + cameraOffset;

    XMStoreFloat3(&m_cameraPos, Eye);

    XMVECTOR At = targetVec;
    XMVECTOR Up = XMVectorSet(m_cameraUp.x, m_cameraUp.y, m_cameraUp.z, 0.0f);

    m_viewMatrix = XMMatrixLookAtLH(Eye, At, Up);
}

void D3D11Renderer::ZoomCamera(float deltaZ)
{
    float zoomAmount = deltaZ * m_zoomSpeed;
    m_cameraRadius -= zoomAmount;

    m_cameraRadius = std::max(0.1f, m_cameraRadius);
    m_cameraRadius = std::min(1000.0f, m_cameraRadius);

    SetupCamera();
}

void D3D11Renderer::RotateCamera(float dx, float dy)
{
    m_yaw += dx * m_mouseSpeedX;
    m_pitch += dy * m_mouseSpeedY;

    m_pitch = std::max(-XM_PIDIV2 * 0.95f, m_pitch);
    m_pitch = std::min(XM_PIDIV2 * 0.95f, m_pitch);

    SetupCamera();
}

void D3D11Renderer::PanCamera(float dx, float dy)
{
    float panSpeedX = 0.001f * m_cameraRadius;
    float panSpeedY = 0.001f * m_cameraRadius;

    XMVECTOR targetVec = XMLoadFloat3(&m_cameraTarget);
    XMVECTOR eyeVec = XMLoadFloat3(&m_cameraPos);
    XMVECTOR upVec = XMLoadFloat3(&m_cameraUp);

    XMVECTOR forward = XMVector3Normalize(targetVec - eyeVec);
    XMVECTOR right = XMVector3Normalize(XMVector3Cross(upVec, forward));
    XMVECTOR actualUp = XMVector3Normalize(XMVector3Cross(forward, right));

    XMVECTOR panDelta = right * (-dx * panSpeedX) + actualUp * (dy * panSpeedY);

    XMStoreFloat3(&m_cameraTarget, targetVec + panDelta);
    XMStoreFloat3(&m_cameraPos, eyeVec + panDelta);

    SetupCamera();
}

void D3D11Renderer::SetWorldTranslation(float dx, float dy, float dz)
{
    m_worldTranslation.x += dx;
    m_worldTranslation.y += dy;
    m_worldTranslation.z += dz;
}

void D3D11Renderer::SetCameraTargetY(float y)
{
    m_cameraTarget.y = y;
    SetupCamera();
}

float D3D11Renderer::GetCameraTargetY() const
{
    return m_cameraTarget.y;
}

void D3D11Renderer::SetWireframeMode(bool enable)
{
    m_wireframeMode = enable;
}

DirectX::XMVECTOR D3D11Renderer::ScreenToWorldRayOrigin(float mouseX, float mouseY, int viewportWidth, int viewportHeight)
{
    float ndcX = (2.0f * mouseX / viewportWidth) - 1.0f;
    float ndcY = 1.0f - (2.0f * mouseY / viewportHeight);

    // Near point ve far point'i hesaplarken, aslında projeksiyon matrisinin tersini kullanarak
    // ekran koordinatlarını doğrudan dünya koordinatlarına dönüştürmeye gerek yok.
    // Ray başlangıç noktası kamera pozisyonudur.
    return XMLoadFloat3(&m_cameraPos);
}

DirectX::XMVECTOR D3D11Renderer::ScreenToWorldRayDirection(float mouseX, float mouseY, int viewportWidth, int viewportHeight)
{
    // Ekran koordinatlarını NDC (Normalized Device Coordinates) değerlerine dönüştür
    float ndcX = (2.0f * mouseX / viewportWidth) - 1.0f;
    float ndcY = 1.0f - (2.0f * mouseY / viewportHeight);

    // Near ve Far düzlemlerindeki 3D koordinatları hesapla
    // Z = 0.0f (near plane) ve Z = 1.0f (far plane)
    DirectX::XMVECTOR nearPoint = DirectX::XMVectorSet(ndcX, ndcY, 0.0f, 1.0f);
    DirectX::XMVECTOR farPoint = DirectX::XMVectorSet(ndcX, ndcY, 1.0f, 1.0f);

    // Projeksiyon matrisinin tersini al
    DirectX::XMMATRIX invProjection = DirectX::XMMatrixInverse(nullptr, m_projectionMatrix);

    // View matrisinin tersini al
    DirectX::XMMATRIX invView = DirectX::XMMatrixInverse(nullptr, m_viewMatrix);

    // Near ve Far noktalarını önce projeksiyon matrisinin tersiyle "unproject" et
    // Bu, onları view (kamera) uzayına geri taşır.
    DirectX::XMVECTOR viewNear = DirectX::XMVector4Transform(nearPoint, invProjection);
    DirectX::XMVECTOR viewFar = DirectX::XMVector4Transform(farPoint, invProjection);

    // W bileşenlerini normalize et (perspektif bölümlemesi)
    viewNear = viewNear / DirectX::XMVectorSplatW(viewNear);
    viewFar = viewFar / DirectX::XMVectorSplatW(viewFar);

    // Near ve Far noktalarını view matrisinin tersiyle "unview" et
    // Bu, onları dünya uzayına taşır.
    DirectX::XMVECTOR worldNear = DirectX::XMVector4Transform(viewNear, invView);
    DirectX::XMVECTOR worldFar = DirectX::XMVector4Transform(viewFar, invView);

    // Ray yönü = worldFar - worldNear (veya worldFar - kameraPozisyonu)
    DirectX::XMVECTOR rayDirection = DirectX::XMVector3Normalize(worldFar - XMLoadFloat3(&m_cameraPos)); // Ray Origin kamera pozisyonudur

    return rayDirection;
}


bool D3D11Renderer::PickMesh(DirectX::XMVECTOR rayOrigin, DirectX::XMVECTOR rayDirection)
{
    int vertexCount = 0;
    DirectX::XMFLOAT3 meshCenter;
    float meshRadius = 0.0f;

    // Aktif mesh tipine göre verileri al
    if (m_activeMeshType == MeshType::CN3VMESH)
    {
        if (m_collisionMesh.VertexCount() == 0) {
            m_isMeshSelected = false;
            m_isDraggingMeshNow = false;
            return false;
        }
        vertexCount = m_collisionMesh.VertexCount();
        meshCenter = m_collisionMesh.GetCenter();
        meshRadius = m_collisionMesh.GetRadius();
    }
    else if (m_activeMeshType == MeshType::N3Mesh)
    {
        if (m_n3Mesh.GetVertexCount() == 0) {
            m_isMeshSelected = false;
            m_isDraggingMeshNow = false;
            return false;
        }
        vertexCount = m_n3Mesh.GetVertexCount();
        meshCenter = DirectX::XMFLOAT3((m_n3Mesh.Min().x + m_n3Mesh.Max().x) / 2.0f,
            (m_n3Mesh.Min().y + m_n3Mesh.Max().y) / 2.0f,
            (m_n3Mesh.Min().z + m_n3Mesh.Max().z) / 2.0f);
        meshRadius = m_n3Mesh.Radius();
    }
    else
    {
        m_isMeshSelected = false;
        m_isDraggingMeshNow = false;
        return false; // Hiçbir mesh yüklü değil
    }

    DirectX::BoundingSphere meshBoundingSphere;
    DirectX::XMVECTOR meshLocalCenter = DirectX::XMLoadFloat3(&meshCenter);
    // Mesh'in dünya pozisyonu (çeviri) ile merkezini birleştir
    DirectX::XMVECTOR translatedCenter = DirectX::XMVectorAdd(meshLocalCenter, DirectX::XMLoadFloat3(&m_worldTranslation));

    meshBoundingSphere.Center.x = DirectX::XMVectorGetX(translatedCenter);
    meshBoundingSphere.Center.y = DirectX::XMVectorGetY(translatedCenter);
    meshBoundingSphere.Center.z = DirectX::XMVectorGetZ(translatedCenter);
    meshBoundingSphere.Radius = meshRadius;

    float distance;
    if (meshBoundingSphere.Intersects(rayOrigin, rayDirection, distance))
    {
        m_isMeshSelected = true;
        qDebug() << "Mesh Picked: True";
        return true;
    }
    else
    {
        m_isMeshSelected = false;
        m_isDraggingMeshNow = false;
        qDebug() << "Mesh Picked: False";
        return false;
    }
}

void D3D11Renderer::CaptureSelectedMeshDepth(float mouseX, float mouseY, int widgetWidth, int widgetHeight)
{
    if (!m_isMeshSelected) return;

    // Ray origin kameranın pozisyonudur
    DirectX::XMVECTOR rayOrigin = XMLoadFloat3(&m_cameraPos);
    DirectX::XMVECTOR rayDirection = ScreenToWorldRayDirection(mouseX, mouseY, widgetWidth, widgetHeight);

    DirectX::XMFLOAT3 currentMeshCenter;
    if (m_activeMeshType == MeshType::CN3VMESH) {
        currentMeshCenter = m_collisionMesh.GetCenter();
    }
    else if (m_activeMeshType == MeshType::N3Mesh) {
        currentMeshCenter = DirectX::XMFLOAT3((m_n3Mesh.Min().x + m_n3Mesh.Max().x) / 2.0f,
            (m_n3Mesh.Min().y + m_n3Mesh.Max().y) / 2.0f,
            (m_n3Mesh.Min().z + m_n3Mesh.Max().z) / 2.0f);
    }
    else {
        return; // Aktif mesh yoksa çık
    }

    // Mesh'in dünya pozisyonu (çeviri) ile merkezini birleştir
    DirectX::XMVECTOR meshWorldPos = DirectX::XMVectorAdd(XMLoadFloat3(&currentMeshCenter), XMLoadFloat3(&m_worldTranslation));

    // Ray'in objenin merkezine olan vektörü
    DirectX::XMVECTOR toMesh = DirectX::XMVectorSubtract(meshWorldPos, rayOrigin);

    // Ray'in yönü ile toMesh vektörü arasındaki skaler çarpım, derinliği verir (t parametresi)
    float t = DirectX::XMVectorGetX(DirectX::XMVector3Dot(toMesh, rayDirection));

    // Seçim noktasının dünya koordinatlarını bul
    DirectX::XMVECTOR clickedWorldPoint = DirectX::XMVectorAdd(rayOrigin, DirectX::XMVectorScale(rayDirection, t));
    DirectX::XMStoreFloat3(&m_previousMouseWorldPos, clickedWorldPoint);

    m_selectedMeshInitialDepth = t;

    qDebug() << "Initial Mesh Depth Captured (t value): " << m_selectedMeshInitialDepth;
    qDebug() << "Initial Mouse World Pos (for dragging delta): X=" << m_previousMouseWorldPos.x
        << " Y=" << m_previousMouseWorldPos.y
        << " Z=" << m_previousMouseWorldPos.z;
}

void D3D11Renderer::DragSelectedMesh(float currentMouseX, float currentMouseY, int widgetWidth, int widgetHeight)
{
    if (!m_isMeshSelected || !m_isDraggingMeshNow) return;

    DirectX::XMVECTOR currentRayOrigin = XMLoadFloat3(&m_cameraPos); // Ray origin kameranın pozisyonudur
    DirectX::XMVECTOR currentRayDirection = ScreenToWorldRayDirection(currentMouseX, currentMouseY, widgetWidth, widgetHeight);

    // Yeni fare pozisyonunun, başlangıçtaki derinlik (m_selectedMeshInitialDepth) kadar ilerlemiş dünya koordinatlarını bul
    DirectX::XMVECTOR newMouseWorldPos = DirectX::XMVectorAdd(currentRayOrigin, DirectX::XMVectorScale(currentRayDirection, m_selectedMeshInitialDepth));

    // Önceki fare pozisyonu ile yeni fare pozisyonu arasındaki dünya delta'sını hesapla
    DirectX::XMVECTOR deltaWorldPos = DirectX::XMVectorSubtract(newMouseWorldPos, DirectX::XMLoadFloat3(&m_previousMouseWorldPos));

    // Mesh'in mevcut dünya çeviri vektörüne bu delta'yı ekle
    DirectX::XMVECTOR currentMeshWorldTranslation = DirectX::XMLoadFloat3(&m_worldTranslation);
    currentMeshWorldTranslation = DirectX::XMVectorAdd(currentMeshWorldTranslation, deltaWorldPos);
    DirectX::XMStoreFloat3(&m_worldTranslation, currentMeshWorldTranslation);

    // Sonraki frame için "önceki" pozisyonu güncelle
    DirectX::XMStoreFloat3(&m_previousMouseWorldPos, newMouseWorldPos);

    qDebug() << "Mesh Dragged To: X=" << m_worldTranslation.x
        << " Y=" << m_worldTranslation.y
        << " Z=" << m_worldTranslation.z;
}

void D3D11Renderer::SetCameraToMeshBounds()
{
    DirectX::XMFLOAT3 minBounds(FLT_MAX, FLT_MAX, FLT_MAX);
    DirectX::XMFLOAT3 maxBounds(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    bool hasMesh = false;

    if (m_activeMeshType == MeshType::CN3VMESH && m_collisionMesh.VertexCount() > 0)
    {
        minBounds = m_collisionMesh.GetMinBounds();
        maxBounds = m_collisionMesh.GetMaxBounds();
        hasMesh = true;
    }
    else if (m_activeMeshType == MeshType::N3Mesh && m_n3Mesh.GetVertexCount() > 0)
    {
        minBounds = m_n3Mesh.Min();
        maxBounds = m_n3Mesh.Max();
        hasMesh = true;
    }
    else
    {
        // Hiç mesh yoksa veya tanımsız tipse varsayılan değerleri kullan
        m_cameraTarget = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
        m_cameraRadius = 10.0f;
        SetupCamera();
        return;
    }

    m_cameraTarget.x = (minBounds.x + maxBounds.x) / 2.0f;
    m_cameraTarget.y = (minBounds.y + maxBounds.y) / 2.0f;
    m_cameraTarget.z = (minBounds.z + maxBounds.z) / 2.0f;

    float dx = maxBounds.x - minBounds.x;
    float dy = maxBounds.y - minBounds.y;
    float dz = maxBounds.z - minBounds.z;

    float boundingSphereRadius = 0.5f * sqrtf(dx * dx + dy * dy + dz * dz);

    float fovRadians = XM_PIDIV4; // 45 derece FOV

    // Kameranın mesh'i tamamen kapsayacak şekilde uzaklığını ayarla
    // 1.5 çarpanı, mesh'in ekrana biraz daha küçük görünmesini ve çevresinde boşluk kalmasını sağlar
    m_cameraRadius = boundingSphereRadius / tanf(fovRadians * 0.5f) * 1.5f;

    // Minimum ve maksimum kamera mesafeleri
    m_cameraRadius = std::max(0.1f, m_cameraRadius); // 0'a çok yaklaşmayı engelle
    m_cameraRadius = std::min(1000.0f, m_cameraRadius); // Çok uzaklaşmayı engelle

    SetupCamera();
}

bool D3D11Renderer::CreateBuffers()
{
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(ConstantBufferData);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    HRESULT hr = m_d3dDevice->CreateBuffer(&bd, nullptr, m_constantBuffer.GetAddressOf());
    if (FAILED(hr)) {
        qDebug() << "Hata: Constant Buffer olusturulurken hata! HRESULT: " << QString("0x%1").arg(hr, 8, 16, QChar('0'));
        return false;
    }

    // Vertex Input Layout Oluşturma
    // __VertexColor yapısına uygun layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // float3 Pos
        { "COLOR", 0, DXGI_FORMAT_B8G8R8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 } // unsigned int Color -> float4 Color
    };
    UINT numElements = ARRAYSIZE(layout);

    hr = m_d3dDevice->CreateInputLayout(layout, numElements,
        m_vertexShaderBlob->GetBufferPointer(),
        m_vertexShaderBlob->GetBufferSize(),
        m_inputLayout.GetAddressOf());
    if (FAILED(hr)) {
        qDebug() << "Hata: Input Layout olusturulurken hata! HRESULT: " << QString("0x%1").arg(hr, 8, 16, QChar('0'));
        return false;
    }
    return true;
}

bool D3D11Renderer::CreateRasterizerStates()
{
    HRESULT hr;

    D3D11_RASTERIZER_DESC solidDesc = {};
    solidDesc.FillMode = D3D11_FILL_SOLID;
    solidDesc.CullMode = D3D11_CULL_BACK;
    solidDesc.FrontCounterClockwise = FALSE;
    solidDesc.DepthClipEnable = TRUE;
    hr = m_d3dDevice->CreateRasterizerState(&solidDesc, m_solidRasterizerState.GetAddressOf());
    if (FAILED(hr)) {
        qDebug() << "Hata: Solid Rasterizer State olusturulurken hata! HRESULT: " << QString("0x%1").arg(hr, 8, 16, QChar('0'));
        return false;
    }

    D3D11_RASTERIZER_DESC wireframeDesc = {};
    wireframeDesc.FillMode = D3D11_FILL_WIREFRAME;
    wireframeDesc.CullMode = D3D11_CULL_BACK;
    wireframeDesc.FrontCounterClockwise = FALSE;
    wireframeDesc.DepthClipEnable = TRUE;
    hr = m_d3dDevice->CreateRasterizerState(&wireframeDesc, m_wireframeRasterizerState.GetAddressOf());
    if (FAILED(hr)) {
        qDebug() << "Hata: Wireframe Rasterizer State olusturulurken hata! HRESULT: " << QString("0x%1").arg(hr, 8, 16, QChar('0'));
        return false;
    }

    // Cull Front Rasterizer State (Dış Çizgi için, bu örnekte doğrudan kullanılmasa da tutulabilir)
    D3D11_RASTERIZER_DESC cullFrontDesc = {};
    cullFrontDesc.FillMode = D3D11_FILL_SOLID;
    cullFrontDesc.CullMode = D3D11_CULL_FRONT;
    cullFrontDesc.DepthClipEnable = true;
    hr = m_d3dDevice->CreateRasterizerState(&cullFrontDesc, m_cullFrontRasterizerState.GetAddressOf());
    if (FAILED(hr)) {
        qDebug() << "Hata: Cull Front Rasterizer State olusturulurken hata! HRESULT: " << QString("0x%1").arg(hr, 8, 16, QChar('0'));
        return false;
    }

    return true;
}