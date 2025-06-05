// N3Mesh.h - Direct3D 11 uyarlamasý için son hali
#pragma once

#include <string>
#include <vector>
#include <map>
#include <DirectXMath.h> // DirectX::XMFLOAT3, XMMATRIX için
#include <windows.h>     // HANDLE için
#include <d3d11.h>       // ID3D11Device, ID3D11DeviceContext, ID3D11Buffer için

#include "CommonN3Structures.h" // __VertexT2, __VertexColor vb. için

// FVF tanýmlarý (Knight Online'ýn N3 formatýnda kullanýlan FVF'lere karþýlýk gelir)
// Bunlar DirectX 9 Flexible Vertex Format (FVF) deðerleridir.
// DirectX 11'de Input Layout ile eþleþtirme için referans olarak kullanýlacaktýr.
#define FVF_XYZ             0x002       // Pozisyon (float x,y,z)
#define FVF_NORMAL          0x010       // Normal (float nx,ny,nz)
#define FVF_DIFFUSE         0x040       // Renk (DWORD)
#define FVF_SPECULAR        0x080       // Parlaklýk Renk (DWORD)
#define FVF_TEX0            0x000       // Doku Koordinatý Yok
#define FVF_TEX1            0x100       // 1 set Doku Koordinatý (float tu,tv)
#define FVF_TEX2            0x200       // 2 set Doku Koordinatý (float tu1,tv1, tu2,tv2)

// Diðer FVF kombinasyonlarý (genellikle Knight Online'da görülenler)
#define FVF_XYZCOLOR        (FVF_XYZ|FVF_DIFFUSE)
#define FVF_XYZNORMALTEX1   (FVF_XYZ|FVF_NORMAL|FVF_TEX1)
#define FVF_XYZNORMALTEX2   (FVF_XYZ|FVF_NORMAL|FVF_TEX2) // __VertexT2 için yaygýn

class N3Mesh
{
public:
    N3Mesh();
    ~N3Mesh();

    void            Release(); // Tüm kaynaklarý serbest býrakma

    // Dosyadan yükleme fonksiyonu
    // Hem dosya yolu hem de D3D11 cihazý alacak þekilde güncellendi.
    bool            Load(const std::string& szFileName, ID3D11Device* pDevice);
    // Eski HANDLE tabanlý Load da korunuyor, bu da D3D11 cihazý alacak.
    bool            Load(HANDLE hFile, ID3D11Device* pDevice);

    // Dosyaya kaydetme (þu an için boþ)
    bool            Save(const std::wstring& filePath, const std::string& meshName = "DefaultMesh");

    // Çizim fonksiyonu (DirectX 11 device context ve input layout alacak)
    // CN3Shape'ten materyal ve texture SRV'leri de gelebilir.
    void            Render(ID3D11DeviceContext* pContext, ID3D11InputLayout* pInputLayout, ID3D11ShaderResourceView* pTexSRV = nullptr, ID3D11SamplerState* pSamplerState = nullptr);

    // Bounding Box ve Radius Getter'larý
    DirectX::XMFLOAT3 Min() const { return m_vMin; }
    DirectX::XMFLOAT3 Max() const { return m_vMax; }
    float Radius() const { return m_fRadius; }

    // Ham veri ve count getter'lar (D3D11Renderer'ýn ihtiyacý olabilir)
    void* GetVertices() const { return m_pVertices; }
    uint16_t* GetIndices() const { return m_pwIndices; }
    int             GetVertexCount() const { return m_nVC; }
    int             GetIndexCount() const { return m_nIC; }
    DWORD           GetFVF() const { return m_dwFVF; }
    float           GetVersion() const { return m_fVersion; }
    UINT            GetVertexSize() const; // FVF'ye göre vertex boyutunu dinamik döndürür

    // D3D11 Buffer Getter'lar
    ID3D11Buffer* GetVertexBuffer() const { return m_pVB; }
    ID3D11Buffer* GetIndexBuffer() const { return m_pIB; }

protected:
    float               m_fVersion;         // Dosya versiyonu
    DWORD               m_dwFVF;            // Flexible Vertex Format (Vertex yapýsýný tanýmlar)
    int                 m_nVC;              // Vertex Sayýsý (Vertex Count)
    int                 m_nIC;              // Index Sayýsý (Index Count)
    int                 m_nFC;              // Face Sayýsý (Genellikle nIC / 3)

    // Ham veriler (dosyadan okunan ve D3D11 buffer'larýna kopyalanacak)
    void* m_pVertices;        // Vertex verileri (FVF'ye göre farklý tiplerde olabilir - ham bellek bloðu)
    uint16_t* m_pwIndices;        // Index verileri (yüzleri oluþturan vertex indeksleri)

    // *** YENÝ EKLENEN DIRECT3D 11 ÜYELERÝ ***
    ID3D11Buffer* m_pVB; // Direct3D 11 Vertex Buffer
    ID3D11Buffer* m_pIB; // Direct3D 11 Index Buffer
    // *****************************************

    DirectX::XMFLOAT3   m_vMin;
    DirectX::XMFLOAT3   m_vMax;
    float               m_fRadius;

    // Yardýmcý fonksiyonlar
    void FindMinMax(); // Bounding box ve radius hesaplama (daha sonra implemente edilecek)
    bool CreateD3D11Buffers(ID3D11Device* pDevice); // Yeni: D3D11 Buffer'larýný oluþturma yardýmcý fonksiyonu
};