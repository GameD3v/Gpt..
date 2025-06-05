// N3Mesh.h - Direct3D 11 uyarlamas� i�in son hali
#pragma once

#include <string>
#include <vector>
#include <map>
#include <DirectXMath.h> // DirectX::XMFLOAT3, XMMATRIX i�in
#include <windows.h>     // HANDLE i�in
#include <d3d11.h>       // ID3D11Device, ID3D11DeviceContext, ID3D11Buffer i�in

#include "CommonN3Structures.h" // __VertexT2, __VertexColor vb. i�in

// FVF tan�mlar� (Knight Online'�n N3 format�nda kullan�lan FVF'lere kar��l�k gelir)
// Bunlar DirectX 9 Flexible Vertex Format (FVF) de�erleridir.
// DirectX 11'de Input Layout ile e�le�tirme i�in referans olarak kullan�lacakt�r.
#define FVF_XYZ             0x002       // Pozisyon (float x,y,z)
#define FVF_NORMAL          0x010       // Normal (float nx,ny,nz)
#define FVF_DIFFUSE         0x040       // Renk (DWORD)
#define FVF_SPECULAR        0x080       // Parlakl�k Renk (DWORD)
#define FVF_TEX0            0x000       // Doku Koordinat� Yok
#define FVF_TEX1            0x100       // 1 set Doku Koordinat� (float tu,tv)
#define FVF_TEX2            0x200       // 2 set Doku Koordinat� (float tu1,tv1, tu2,tv2)

// Di�er FVF kombinasyonlar� (genellikle Knight Online'da g�r�lenler)
#define FVF_XYZCOLOR        (FVF_XYZ|FVF_DIFFUSE)
#define FVF_XYZNORMALTEX1   (FVF_XYZ|FVF_NORMAL|FVF_TEX1)
#define FVF_XYZNORMALTEX2   (FVF_XYZ|FVF_NORMAL|FVF_TEX2) // __VertexT2 i�in yayg�n

class N3Mesh
{
public:
    N3Mesh();
    ~N3Mesh();

    void            Release(); // T�m kaynaklar� serbest b�rakma

    // Dosyadan y�kleme fonksiyonu
    // Hem dosya yolu hem de D3D11 cihaz� alacak �ekilde g�ncellendi.
    bool            Load(const std::string& szFileName, ID3D11Device* pDevice);
    // Eski HANDLE tabanl� Load da korunuyor, bu da D3D11 cihaz� alacak.
    bool            Load(HANDLE hFile, ID3D11Device* pDevice);

    // Dosyaya kaydetme (�u an i�in bo�)
    bool            Save(const std::wstring& filePath, const std::string& meshName = "DefaultMesh");

    // �izim fonksiyonu (DirectX 11 device context ve input layout alacak)
    // CN3Shape'ten materyal ve texture SRV'leri de gelebilir.
    void            Render(ID3D11DeviceContext* pContext, ID3D11InputLayout* pInputLayout, ID3D11ShaderResourceView* pTexSRV = nullptr, ID3D11SamplerState* pSamplerState = nullptr);

    // Bounding Box ve Radius Getter'lar�
    DirectX::XMFLOAT3 Min() const { return m_vMin; }
    DirectX::XMFLOAT3 Max() const { return m_vMax; }
    float Radius() const { return m_fRadius; }

    // Ham veri ve count getter'lar (D3D11Renderer'�n ihtiyac� olabilir)
    void* GetVertices() const { return m_pVertices; }
    uint16_t* GetIndices() const { return m_pwIndices; }
    int             GetVertexCount() const { return m_nVC; }
    int             GetIndexCount() const { return m_nIC; }
    DWORD           GetFVF() const { return m_dwFVF; }
    float           GetVersion() const { return m_fVersion; }
    UINT            GetVertexSize() const; // FVF'ye g�re vertex boyutunu dinamik d�nd�r�r

    // D3D11 Buffer Getter'lar
    ID3D11Buffer* GetVertexBuffer() const { return m_pVB; }
    ID3D11Buffer* GetIndexBuffer() const { return m_pIB; }

protected:
    float               m_fVersion;         // Dosya versiyonu
    DWORD               m_dwFVF;            // Flexible Vertex Format (Vertex yap�s�n� tan�mlar)
    int                 m_nVC;              // Vertex Say�s� (Vertex Count)
    int                 m_nIC;              // Index Say�s� (Index Count)
    int                 m_nFC;              // Face Say�s� (Genellikle nIC / 3)

    // Ham veriler (dosyadan okunan ve D3D11 buffer'lar�na kopyalanacak)
    void* m_pVertices;        // Vertex verileri (FVF'ye g�re farkl� tiplerde olabilir - ham bellek blo�u)
    uint16_t* m_pwIndices;        // Index verileri (y�zleri olu�turan vertex indeksleri)

    // *** YEN� EKLENEN DIRECT3D 11 �YELER� ***
    ID3D11Buffer* m_pVB; // Direct3D 11 Vertex Buffer
    ID3D11Buffer* m_pIB; // Direct3D 11 Index Buffer
    // *****************************************

    DirectX::XMFLOAT3   m_vMin;
    DirectX::XMFLOAT3   m_vMax;
    float               m_fRadius;

    // Yard�mc� fonksiyonlar
    void FindMinMax(); // Bounding box ve radius hesaplama (daha sonra implemente edilecek)
    bool CreateD3D11Buffers(ID3D11Device* pDevice); // Yeni: D3D11 Buffer'lar�n� olu�turma yard�mc� fonksiyonu
};