// N3Mesh.cpp - Direct3D 11 uyarlamasý için son hali
#include "N3Mesh.h"
#include <fstream>
#include <algorithm> // std::min, std::max için
#include <cfloat>    // FLT_MAX için
#include <Windows.h> // HANDLE, ReadFile, WriteFile için
#include <cmath>     // sqrtf için
#include <QDebug>    // qDebug için (Eðer Qt kullanýyorsanýz)

// Yardýmcý Fonksiyon: Binary dosyadan string oku (HANDLE tabanlý)
std::string ReadStringFromFile(HANDLE hFile)
{
    DWORD len = 0;
    DWORD dwRead;
    // Mevcut pozisyonu kaydet
    LARGE_INTEGER currentPos;
    currentPos.QuadPart = 0;
    SetFilePointerEx(hFile, currentPos, &currentPos, FILE_CURRENT);

    ReadFile(hFile, &len, sizeof(DWORD), &dwRead, NULL);
    if (len == 0) return "";

    std::string s(len, '\0');
    ReadFile(hFile, &s[0], len, &dwRead, NULL);

    // Okuma bittikten sonra dosya pointer'ýný geri al
    LARGE_INTEGER newPos;
    newPos.QuadPart = currentPos.QuadPart + sizeof(DWORD) + len;
    SetFilePointerEx(hFile, newPos, NULL, FILE_BEGIN);

    return s;
}


// Constructor
N3Mesh::N3Mesh()
    : m_fVersion(0.0f), m_dwFVF(0), m_nVC(0), m_nIC(0), m_nFC(0),
    m_pVertices(nullptr), m_pwIndices(nullptr),
    m_pVB(nullptr), m_pIB(nullptr), // D3D11 Buffer'larý baþlat
    m_vMin(FLT_MAX, FLT_MAX, FLT_MAX), m_vMax(-FLT_MAX, -FLT_MAX, -FLT_MAX), m_fRadius(0.0f)
{
}

// Destructor
N3Mesh::~N3Mesh()
{
    Release();
}

// Kaynaklarý serbest býrakma
void N3Mesh::Release()
{
    if (m_pVertices)
    {
        delete[](BYTE*)m_pVertices; // Ham bellek bloðunu sil
        m_pVertices = nullptr;
    }
    if (m_pwIndices)
    {
        delete[] m_pwIndices; // Index dizisini sil
        m_pwIndices = nullptr;
    }

    // Direct3D 11 kaynaklarýný serbest býrak
    if (m_pVB) { m_pVB->Release(); m_pVB = nullptr; }
    if (m_pIB) { m_pIB->Release(); m_pIB = nullptr; }

    m_fVersion = 0.0f;
    m_dwFVF = 0;
    m_nVC = 0;
    m_nIC = 0;
    m_nFC = 0;
    m_vMin = DirectX::XMFLOAT3(FLT_MAX, FLT_MAX, FLT_MAX);
    m_vMax = DirectX::XMFLOAT3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    m_fRadius = 0.0f;
}

// FVF'ye göre vertex boyutunu döndürür
UINT N3Mesh::GetVertexSize() const
{
    UINT size = 0;
    if (m_dwFVF & FVF_XYZ) size += sizeof(DirectX::XMFLOAT3); // Position
    if (m_dwFVF & FVF_NORMAL) size += sizeof(DirectX::XMFLOAT3); // Normal
    if (m_dwFVF & FVF_DIFFUSE) size += sizeof(DWORD); // Color (RGBA)
    if (m_dwFVF & FVF_SPECULAR) size += sizeof(DWORD); // Specular Color
    if (m_dwFVF & FVF_TEX1) size += sizeof(float) * 2; // UV0
    if (m_dwFVF & FVF_TEX2) size += sizeof(float) * 4; // UV0 ve UV1 (toplam 2 set UV)
    // Diðer FVF'ler için de benzer þekilde boyut ekleyebilirsiniz.
    // __VertexT2 yapýsý FVF_XYZ, FVF_NORMAL ve FVF_TEX2'ye denk gelmeli (3+3+2+2 = 10 float).
    // Ancak FVF_TEX2'nin tanýmý 2 set UV'yi de içerecek þekilde 4 float olarak ayarlandý.
    // Yani (3+3+2+2)*4 = 40 byte.
    return size;
}

// D3D11 Buffer'larýný oluþturur
bool N3Mesh::CreateD3D11Buffers(ID3D11Device* pDevice)
{
    if (!pDevice || !m_pVertices || m_nVC == 0 || GetVertexSize() == 0)
    {
        qDebug() << "CreateD3D11Buffers: Geçersiz veri, cihaz veya vertex boyutu.";
        return false;
    }

    // Vertex Buffer oluþturma
    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.Usage = D3D11_USAGE_IMMUTABLE; // Veri deðiþmeyecek
    vbDesc.ByteWidth = m_nVC * GetVertexSize();
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.CPUAccessFlags = 0;
    vbDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA vbInitData = {};
    vbInitData.pSysMem = m_pVertices;

    HRESULT hr = pDevice->CreateBuffer(&vbDesc, &vbInitData, &m_pVB);
    if (FAILED(hr))
    {
        qDebug() << "Hata: N3Mesh Vertex Buffer olusturulurken hata! HRESULT: " << QString("0x%1").arg(hr, 8, 16, QChar('0'));
        return false;
    }

    // Index Buffer oluþturma
    if (m_pwIndices && m_nIC > 0)
    {
        D3D11_BUFFER_DESC ibDesc = {};
        ibDesc.Usage = D3D11_USAGE_IMMUTABLE; // Veri deðiþmeyecek
        ibDesc.ByteWidth = m_nIC * sizeof(uint16_t); // uint16_t olduðu varsayýmýyla (2 byte)
        ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        ibDesc.CPUAccessFlags = 0;
        ibDesc.MiscFlags = 0;

        D3D11_SUBRESOURCE_DATA ibInitData = {};
        ibInitData.pSysMem = m_pwIndices;

        hr = pDevice->CreateBuffer(&ibDesc, &ibInitData, &m_pIB);
        if (FAILED(hr))
        {
            qDebug() << "Hata: N3Mesh Index Buffer olusturulurken hata! HRESULT: " << QString("0x%1").arg(hr, 8, 16, QChar('0'));
            if (m_pVB) { m_pVB->Release(); m_pVB = nullptr; } // Vertex buffer'ý da temizle
            return false;
        }
    }

    qDebug() << "N3Mesh D3D11 Buffers basariyla olusturuldu.";
    return true;
}

// Dosyadan yükleme (HANDLE tabanlý)
bool N3Mesh::Load(HANDLE hFile, ID3D11Device* pDevice)
{
    Release(); // Önceki kaynaklarý serbest býrak

    if (hFile == INVALID_HANDLE_VALUE)
    {
        qDebug() << "Hata: Geçersiz N3Mesh dosya handle'ý.";
        return false;
    }

    DWORD dwRead;
    char szHeader[4];
    // Dosya pointerýný en baþa getir (Load fonksiyonu birden fazla kez çaðrýlýrsa)
    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

    ReadFile(hFile, szHeader, 4, &dwRead, NULL);
    if (memcmp(szHeader, "N3MX", 4) != 0) // N3Mesh'in baþlýðý "N3MX" varsayýmý
    {
        qDebug() << "Hata: Geçersiz N3Mesh dosya baþlýðý. Beklenen: N3MX";
        return false;
    }

    ReadFile(hFile, &m_fVersion, sizeof(float), &dwRead, NULL); // Versiyon
    ReadFile(hFile, &m_dwFVF, sizeof(DWORD), &dwRead, NULL);     // FVF
    ReadFile(hFile, &m_nVC, sizeof(int), &dwRead, NULL);         // Vertex Count

    qDebug() << "N3Mesh Version: " << m_fVersion << ", FVF: " << QString("0x%1").arg(m_dwFVF, 8, 16, QChar('0')) << ", VC: " << m_nVC;

    // Vertex verilerini oku
    UINT vertexSize = GetVertexSize();
    if (m_nVC > 0 && vertexSize > 0)
    {
        m_pVertices = new BYTE[m_nVC * vertexSize]; // Ham bellek bloðu ayýr
        ReadFile(hFile, m_pVertices, m_nVC * vertexSize, &dwRead, NULL);
    }
    else if (m_nVC > 0)
    {
        qDebug() << "Hata: N3Mesh vertexSize 0 hesaplandi, ancak vertex sayisi var.";
        Release();
        return false;
    }

    ReadFile(hFile, &m_nIC, sizeof(int), &dwRead, NULL); // Index Count
    qDebug() << "N3Mesh IC: " << m_nIC;

    // Index verilerini oku
    if (m_nIC > 0)
    {
        m_pwIndices = new uint16_t[m_nIC];
        ReadFile(hFile, m_pwIndices, m_nIC * sizeof(uint16_t), &dwRead, NULL);
    }

    ReadFile(hFile, &m_nFC, sizeof(int), &dwRead, NULL); // Face Count (genellikle nIC / 3)
    qDebug() << "N3Mesh FC: " << m_nFC;

    // Bounding Box ve Radius oku (Eðer varsa)
    ReadFile(hFile, &m_vMin, sizeof(DirectX::XMFLOAT3), &dwRead, NULL);
    ReadFile(hFile, &m_vMax, sizeof(DirectX::XMFLOAT3), &dwRead, NULL);
    ReadFile(hFile, &m_fRadius, sizeof(float), &dwRead, NULL);
    qDebug() << "N3Mesh Bounding Box: Min(" << m_vMin.x << "," << m_vMin.y << "," << m_vMin.z << ") Max(" << m_vMax.x << "," << m_vMax.y << "," << m_vMax.z << ") Radius: " << m_fRadius;

    // --- D3D11 Buffer'larýný oluþtur ---
    if (!CreateD3D11Buffers(pDevice))
    {
        qDebug() << "Hata: N3Mesh D3D11 Buffer'lar olusturulamadi.";
        Release();
        return false;
    }

    qDebug() << "N3Mesh dosyasi basariyla yuklendi (HANDLE tabanlý).";
    return true;
}

// Dosyadan yükleme (std::string filePath tabanlý)
bool N3Mesh::Load(const std::string& szFileName, ID3D11Device* pDevice)
{
    HANDLE hFile = CreateFileA(szFileName.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        qDebug() << "Hata: N3Mesh dosyasi acilamadi: " << szFileName.c_str();
        return false;
    }
    bool result = Load(hFile, pDevice); // HANDLE tabanlý Load'ý çaðýr
    CloseHandle(hFile);
    return result;
}

// Dosyaya kaydetme (Þu an için boþ, daha sonra implemente edilecek)
bool N3Mesh::Save(const std::wstring& filePath, const std::string& meshName)
{
    qDebug() << "N3Mesh::Save fonksiyonu henüz implemente edilmedi.";
    return false;
}

// Vertex pozisyonlarýna göre min/max deðerlerini bulma (Þu an için boþ, daha sonra implemente edilecek)
void N3Mesh::FindMinMax()
{
    // Yüklenen m_pVertices ve m_dwFVF deðerleri kullanýlarak bounding box ve küre hesaplanacak.
    // Bu kýsým, vertex verilerinin doðru bir þekilde parse edilmesini gerektirir.
    // Örneðin, FVF_XYZ'ye sahip bir vertex yapýsý için:
    /*
    if (m_pVertices && m_nVC > 0)
    {
        m_vMin = DirectX::XMFLOAT3(FLT_MAX, FLT_MAX, FLT_MAX);
        m_vMax = DirectX::XMFLOAT3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
        float maxSqRadius = 0.0f;

        UINT vertexSize = GetVertexSize();
        for (int i = 0; i < m_nVC; ++i)
        {
            BYTE* pVertex = (BYTE*)m_pVertices + (i * vertexSize);
            DirectX::XMFLOAT3 pos;

            if (m_dwFVF & FVF_XYZ)
            {
                // Position'ý oku (ilk 3 float)
                memcpy(&pos, pVertex, sizeof(DirectX::XMFLOAT3));
            }
            // Diðer FVF durumlarýna göre pos'u ayarlamanýz gerekebilir.
            // Örneðin, eðer FVF_XYZ tek baþýna ise pos = *(DirectX::XMFLOAT3*)pVertex;

            m_vMin.x = std::min(m_vMin.x, pos.x);
            m_vMin.y = std::min(m_vMin.y, pos.y);
            m_vMin.z = std::min(m_vMin.z, pos.z);

            m_vMax.x = std::max(m_vMax.x, pos.x);
            m_vMax.y = std::max(m_vMax.y, pos.y);
            m_vMax.z = std::max(m_vMax.z, pos.z);

            maxSqRadius = std::max(maxSqRadius, pos.x*pos.x + pos.y*pos.y + pos.z*pos.z);
        }
        m_fRadius = sqrtf(maxSqRadius);
    }
    */
}


// Render fonksiyonu (DirectX 11 çizim komutlarýný kullanýr)
void N3Mesh::Render(ID3D11DeviceContext* pContext, ID3D11InputLayout* pInputLayout, ID3D11ShaderResourceView* pTexSRV, ID3D11SamplerState* pSamplerState)
{
    if (!pContext || !m_pVB || m_nVC == 0)
    {
        qDebug() << "N3Mesh::Render: Geçersiz Context veya Vertex Buffer, veya Vertex sayisi 0.";
        return; // Çizilebilecek bir þey yok
    }

    // Input Layout'u ayarla
    if (pInputLayout)
    {
        pContext->IASetInputLayout(pInputLayout);
    }
    else
    {
        qDebug() << "N3Mesh::Render: Input Layout NULL!";
        return; // Input layout olmadan çizemeyiz
    }

    // Vertex Buffer'ý baðla
    UINT stride = GetVertexSize();
    UINT offset = 0;
    pContext->IASetVertexBuffers(0, 1, &m_pVB, &stride, &offset);

    // Index Buffer'ý baðla (eðer varsa)
    if (m_pIB && m_nIC > 0)
    {
        pContext->IASetIndexBuffer(m_pIB, DXGI_FORMAT_R16_UINT, 0); // uint16_t olduðu için R16_UINT
        pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        pContext->DrawIndexed(m_nIC, 0, 0);
    }
    else // Index yoksa sadece vertex buffer'ý çiz
    {
        qDebug() << "N3Mesh::Render: Index Buffer yok veya Index sayisi 0. Draw çaðrýlýyor.";
        pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // Ya da POINTLIST, LINELIST duruma göre
        pContext->Draw(m_nVC, 0);
    }

    // Texture ve Sampler baðlama (D3D11Renderer veya Materyal yöneticisi tarafýndan yapýlmalý)
    // Þimdilik yorum satýrý.
    /*
    if (pTexSRV)
    {
        pContext->PSSetShaderResources(0, 1, &pTexSRV);
    }
    if (pSamplerState)
    {
        pContext->PSSetSamplers(0, 1, &pSamplerState);
    }
    */
}