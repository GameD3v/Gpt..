// D3D11Renderer.h
#pragma once

#include <d3d11.h>
#include <wrl/client.h> // Microsoft::WRL::ComPtr için
#include <DirectXMath.h> // DirectX Math kütüphanesi için
#include <DirectXCollision.h>
#include <vector>        // std::vector için
#include "CN3VMesh.h" // CN3VMesh sınıfını dahil et
#include "N3Mesh.h"   // N3Mesh sınıfını dahil et
#include "CommonN3Structures.h" // __VertexColor gibi yapılar için

using namespace DirectX; // DirectX namespace'ini kullan

// Shader'a gönderilecek sabit tampon yapısı
struct ConstantBufferData
{
    DirectX::XMMATRIX World;
    DirectX::XMMATRIX View;
    DirectX::XMMATRIX Projection;
    int RenderMode; // 0: normal, 1: secili (iç renk), 2: dış çizgi
    float padding[3];
};

// Izgara çizimi için Vertex yapısı
struct GridVertex {
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT4 Color;
};

// Mesh tiplerini belirten enum
enum class MeshType
{
    NONE,
    CN3VMESH,
    N3Mesh // Burası artık N3MESH (büyük harflerle) olmalı.
};

class D3D11Renderer
{
public:
    D3D11Renderer();
    ~D3D11Renderer();

    bool Initialize(HWND hWnd, int width, int height);
    void Shutdown();
    void Render();
    void Resize(int width, int height);

    bool LoadMesh(const std::wstring& filePath); // Dosya yolu std::wstring olarak alacak
    CN3VMesh* GetCollisionMesh() { return &m_collisionMesh; } // Eklenen GetCollisionMesh fonksiyonu

    // Kamera kontrol fonksiyonları
    void ZoomCamera(float deltaZ);
    void RotateCamera(float dx, float dy);
    void PanCamera(float dx, float dy); // <<--- YENİ EKLENEN FONKSİYON BİLDİRİMİ
    void SetCameraToMeshBounds(); // Mesh yüklendikten sonra kamerayı mesh'in boyutlarına göre ayarlar
    void SetCameraTargetY(float y);
    float GetCameraTargetY() const; // Kamera hedefinin Y bileşenini döndürmek için
    void SetCamera(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 target, DirectX::XMFLOAT3 up); // Bu fonksiyon artık kullanılmıyor, SetupCamera çağrılıyor
    void SetupCamera(); // Kamera pozisyonunu hedef, radius, yaw ve pitch kullanarak hesaplar

    // Ekran koordinatlarından 3D ışın başlangıç noktası ve yönünü hesaplar
    // Bu fonksiyonlar, QDirect3D11Widget'tan çağrılacak.
    DirectX::XMVECTOR ScreenToWorldRayOrigin(float mouseX, float mouseY, int width, int height);
    DirectX::XMVECTOR ScreenToWorldRayDirection(float mouseX, float mouseY, int width, int height);
    // Mesh seçme fonksiyonu: bool PickMesh(rayOrigin, rayDirection) yerine
    // şimdi void PickMesh() kullanacağız ve m_isMeshSelected'ı içeride yöneteceğiz.
    // Eğer sahnedeki objeye tıklanırsa m_isMeshSelected = true olur.
    // Boş alana tıklanırsa m_isMeshSelected = false olur.
    bool PickMesh(DirectX::XMVECTOR rayOrigin, DirectX::XMVECTOR rayDirection);
    // Yeni: Seçili objenin başlangıçtaki derinliğini yakalamak için
    // Bu, mesh'in seçildiği anki fare pozisyonunun dünya koordinatındaki Z derinliğini depolar.
    void CaptureSelectedMeshDepth(float mouseX, float mouseY, int widgetWidth, int widgetHeight);
    // Yeni: Objeyi ekranda sürükleme fonksiyonu
    // Bu fonksiyon, fare pozisyonunu alacak ve objenin yeni dünya pozisyonunu hesaplayacak.
    void DragSelectedMesh(float currentMouseX, float currentMouseY, int widgetWidth, int widgetHeight);

    // Mesh durumları
    // DİKKAT: m_isDraggingMeshNow ve m_isMeshSelected private olduğu için
    // QDirect3D11Widget'ın bunlara doğrudan erişebilmesi için SETTER fonksiyonları da eklememiz gerekiyor.
    // Ancak, direkt set etmek yerine PickMesh içinde yönetmek daha güvenli.
    // Eğer PickMesh içindeki mantık yeterli değilse, bu set fonksiyonlarını ekleriz.
    // Şu anki PickMesh ve mousePressEvent mantığı ile doğrudan set etmeye gerek kalmaması gerekiyor.
    // Yalnızca m_isDraggingMeshNow'ı mousePressEvent içinde true/false yapmamız gerekecek.
    // Bunun için de PickMesh'e değil, direkt D3D11Renderer'a public setter eklememiz lazım.
    void SetMeshSelected(bool selected) { m_isMeshSelected = selected; }
    bool IsMeshSelected() const { return m_isMeshSelected; }
    void SetDraggingMeshNow(bool dragging) { m_isDraggingMeshNow = dragging; }
    bool IsDraggingMeshNow() const { return m_isDraggingMeshNow; }
    void SetWorldTranslation(float dx, float dy, float dz);
    void SetIsDraggingMeshNow(bool isDragging) { m_isDraggingMeshNow = isDragging; } // <<-- Bu setter çok önemli
    void SetIsMeshSelected(bool isSelected) { m_isMeshSelected = isSelected; }       // <<-- Bu setter da önemli
    // Seçili mesh'i hareket ettirme fonksiyonu
    void MoveSelectedMesh(DirectX::XMVECTOR currentRayOrigin, DirectX::XMVECTOR currentRayDirection);

    void SetWireframeMode(bool enable); // Kafes modu açma/kapama

private:
    // DirectX 11 Cihaz ve Bağlam
    Microsoft::WRL::ComPtr<ID3D11Device>            m_d3dDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext>     m_d3dContext;
    Microsoft::WRL::ComPtr<IDXGISwapChain>          m_swapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>  m_renderTargetView;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView>  m_depthStencilView;

    // Shaderlar ve Input Layout
    Microsoft::WRL::ComPtr<ID3D11InputLayout>       m_inputLayout;
    Microsoft::WRL::ComPtr<ID3D11VertexShader>      m_vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>       m_pixelShader;
    Microsoft::WRL::ComPtr<ID3DBlob>                m_vertexShaderBlob; // InputLayout oluşturmak için gerekli
    Microsoft::WRL::ComPtr<ID3DBlob>                m_pixelShaderBlob;  // Pixel Shader oluşturmak için gerekli

    Microsoft::WRL::ComPtr<ID3D11Buffer>            m_constantBuffer;// World, View, Projection matrisleri için

    // Rasterizer State'leri
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_solidRasterizerState;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_wireframeRasterizerState;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_cullFrontRasterizerState;// Dış çizgi için (bu örnekte kullanılmasa da tutulabilir)

    // Kamera kontrol değişkenleri
    DirectX::XMFLOAT3 m_cameraPos;
    DirectX::XMFLOAT3 m_cameraTarget;
    DirectX::XMFLOAT3 m_cameraUp;
    float             m_cameraRadius; // Kameranın hedeften uzaklığı
    float             m_yaw;          // Y ekseni etrafında dönüş (radians)
    float             m_pitch;        // X ekseni etrafında dönüş (radians)

    // Kamera ve dünya matrisleri
    DirectX::XMMATRIX m_worldMatrix;
    DirectX::XMMATRIX m_viewMatrix;
    DirectX::XMMATRIX m_projectionMatrix;

    float m_zoomSpeed;
    float m_mouseSpeedX;
    float m_mouseSpeedY;

    // Mesh ve Çeviri
    DirectX::XMFLOAT3 m_worldTranslation = { 0.0f, 0.0f, 0.0f }; // Mesh'in dünya üzerindeki çeviri vektörü

    // Birden fazla mesh tipini desteklemek için
    CN3VMesh m_collisionMesh; // Yüklenen mesh verilerini tutar
    N3Mesh m_n3Mesh;   // .n3mesh dosyaları için
    MeshType m_activeMeshType; // Hangi mesh tipinin aktif olduğunu tutar

    // Mesh verileri için Buffer'lar
    // Mesh verileri için Buffer'lar (genel, aktif mesh'e göre doldurulacak)
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_indexBuffer;

    // Mesh seçimi ve sürükleme
    bool m_isMeshSelected = false; // m_collisionMesh'in seçili olup olmadığını belirtir.
    bool m_isDraggingMeshNow = false;
    DirectX::XMFLOAT3 m_previousMouseWorldPos; // Mesh'i sürüklerken önceki dünya koordinatındaki fare pozisyonu
    float m_selectedMeshInitialDepth; // Mesh seçildiği anki derinliği (projeksiyon düzleminde)

    bool m_wireframeMode; // Kafes modu aktif mi?

    int m_width;
    int m_height;

    // Yardımcı fonksiyonlar
    bool CompileShader(const char* shaderCode, const char* entryPoint, const char* profile, ID3DBlob** blob);
    bool CreateBuffers();
    bool CreateRasterizerStates(); // Yeni: Rasterizer State'leri oluşturmak için

    // Izgara çizim fonksiyonları
    Microsoft::WRL::ComPtr<ID3D11Buffer> m_gridVertexBuffer; // Izgara için Vertex Buffer
    int m_gridVertexCount; // Izgaradaki toplam köşe sayısı
    void CreateGridBuffers(float size, int subdivisions); // Izgara Vertex Buffer'ını oluşturmak için
    void DrawGrid(); // Izgarayı çizmek için yeni fonksiyon

    Microsoft::WRL::ComPtr<ID3D11Texture2D>         m_depthStencilBuffer;

    // Mesh sürükleme için önceki fare pozisyonunu ve derinliğini sakla
    DirectX::XMFLOAT3 m_selectedMeshInitialWorldPos; // Mesh seçildiği anki dünya pozisyonu
    DirectX::XMFLOAT3 m_initialMouseWorldPos;        // Mesh seçildiği anki fare ray'inin dünya pozisyonu

    bool m_isOrbiting; // Şimdilik kullanılmıyor, mouseMoveEvent içinde direkt kontrol ediyoruz
};