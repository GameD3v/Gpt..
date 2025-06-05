// N3Mesh.cpp - Direct3D 11 uyarlamas� i�in son hali
#include "N3Mesh.h"
#include <fstream>
#include <algorithm> // std::min, std::max i�in
#include <cfloat>    // FLT_MAX i�in
#include <Windows.h> // HANDLE, ReadFile, WriteFile i�in
#include <cmath>     // sqrtf i�in
#include <QDebug>    // qDebug i�in (E�er Qt kullan�yorsan�z)

// Yard�mc� Fonksiyon: Binary dosyadan string oku (HANDLE tabanl�)
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

    // Okuma bittikten sonra dosya pointer'�n� geri al
    LARGE_INTEGER newPos;
    newPos.QuadPart = currentPos.QuadPart + sizeof(DWORD) + len;
    SetFilePointerEx(hFile, newPos, NULL, FILE_BEGIN);

    return s;
}


// Constructor
N3Mesh::N3Mesh()
    : m_fVersion(0.0f), m_dwFVF(0), m_nVC(0), m_nIC(0), m_nFC(0),
    m_pVertices(nullptr), m_pwIndices(nullptr),
    m_pVB(nullptr), m_pIB(nullptr), // D3D11 Buffer'lar� ba�lat
    m_vMin(FLT_MAX, FLT_MAX, FLT_MAX), m_vMax(-FLT_MAX, -FLT_MAX, -FLT_MAX), m_fRadius(0.0f)
{
}

// Destructor
N3Mesh::~N3Mesh()
{
    Release();
}

// Kaynaklar� serbest b�rakma
void N3Mesh::Release()
{
    if (m_pVertices)
    {
        delete[](BYTE*)m_pVertices; // Ham bellek blo�unu sil
        m_pVertices = nullptr;
    }
    if (m_pwIndices)
    {
        delete[] m_pwIndices; // Index dizisini sil
        m_pwIndices = nullptr;
    }

    // Direct3D 11 kaynaklar�n� serbest b�rak
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

// FVF'ye g�re vertex boyutunu d�nd�r�r
UINT N3Mesh::GetVertexSize() const
{
    UINT size = 0;
    if (m_dwFVF & FVF_XYZ) size += sizeof(DirectX::XMFLOAT3); // Position
    if (m_dwFVF & FVF_NORMAL) size += sizeof(DirectX::XMFLOAT3); // Normal
    if (m_dwFVF & FVF_DIFFUSE) size += sizeof(DWORD); // Color (RGBA)
    if (m_dwFVF & FVF_SPECULAR) size += sizeof(DWORD); // Specular Color
    if (m_dwFVF & FVF_TEX1) size += sizeof(float) * 2; // UV0
    if (m_dwFVF & FVF_TEX2) size += sizeof(float) * 4; // UV0 ve UV1 (toplam 2 set UV)
    // Di�er FVF'ler i�in de benzer �ekilde boyut ekleyebilirsiniz.
    // __VertexT2 yap�s� FVF_XYZ, FVF_NORMAL ve FVF_TEX2'ye denk gelmeli (3+3+2+2 = 10 float).
    // Ancak FVF_TEX2'nin tan�m� 2 set UV'yi de i�erecek �ekilde 4 float olarak ayarland�.
    // Yani (3+3+2+2)*4 = 40 byte.
    return size;
}

// D3D11 Buffer'lar�n� olu�turur
bool N3Mesh::CreateD3D11Buffers(ID3D11Device* pDevice)
{
    if (!pDevice || !m_pVertices || m_nVC == 0 || GetVertexSize() == 0)
    {
        qDebug() << "CreateD3D11Buffers: Ge�ersiz veri, cihaz veya vertex boyutu.";
        return false;
    }

    // Vertex Buffer olu�turma
    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.Usage = D3D11_USAGE_IMMUTABLE; // Veri de�i�meyecek
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

    // Index Buffer olu�turma
    if (m_pwIndices && m_nIC > 0)
    {
        D3D11_BUFFER_DESC ibDesc = {};
        ibDesc.Usage = D3D11_USAGE_IMMUTABLE; // Veri de�i�meyecek
        ibDesc.ByteWidth = m_nIC * sizeof(uint16_t); // uint16_t oldu�u varsay�m�yla (2 byte)
        ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        ibDesc.CPUAccessFlags = 0;
        ibDesc.MiscFlags = 0;

        D3D11_SUBRESOURCE_DATA ibInitData = {};
        ibInitData.pSysMem = m_pwIndices;

        hr = pDevice->CreateBuffer(&ibDesc, &ibInitData, &m_pIB);
        if (FAILED(hr))
        {
            qDebug() << "Hata: N3Mesh Index Buffer olusturulurken hata! HRESULT: " << QString("0x%1").arg(hr, 8, 16, QChar('0'));
            if (m_pVB) { m_pVB->Release(); m_pVB = nullptr; } // Vertex buffer'� da temizle
            return false;
        }
    }

    qDebug() << "N3Mesh D3D11 Buffers basariyla olusturuldu.";
    return true;
}

// Dosyadan y�kleme (HANDLE tabanl�)
bool N3Mesh::Load(HANDLE hFile, ID3D11Device* pDevice)
{
    Release(); // �nceki kaynaklar� serbest b�rak

    if (hFile == INVALID_HANDLE_VALUE)
    {
        qDebug() << "Hata: Ge�ersiz N3Mesh dosya handle'�.";
        return false;
    }

    DWORD dwRead;
    char szHeader[4];
    // Dosya pointer�n� en ba�a getir (Load fonksiyonu birden fazla kez �a�r�l�rsa)
    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

    ReadFile(hFile, szHeader, 4, &dwRead, NULL);
    if (memcmp(szHeader, "N3MX", 4) != 0) // N3Mesh'in ba�l��� "N3MX" varsay�m�
    {
        qDebug() << "Hata: Ge�ersiz N3Mesh dosya ba�l���. Beklenen: N3MX";
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
        m_pVertices = new BYTE[m_nVC * vertexSize]; // Ham bellek blo�u ay�r
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

    // Bounding Box ve Radius oku (E�er varsa)
    ReadFile(hFile, &m_vMin, sizeof(DirectX::XMFLOAT3), &dwRead, NULL);
    ReadFile(hFile, &m_vMax, sizeof(DirectX::XMFLOAT3), &dwRead, NULL);
    ReadFile(hFile, &m_fRadius, sizeof(float), &dwRead, NULL);
    qDebug() << "N3Mesh Bounding Box: Min(" << m_vMin.x << "," << m_vMin.y << "," << m_vMin.z << ") Max(" << m_vMax.x << "," << m_vMax.y << "," << m_vMax.z << ") Radius: " << m_fRadius;

    // --- D3D11 Buffer'lar�n� olu�tur ---
    if (!CreateD3D11Buffers(pDevice))
    {
        qDebug() << "Hata: N3Mesh D3D11 Buffer'lar olusturulamadi.";
        Release();
        return false;
    }

    qDebug() << "N3Mesh dosyasi basariyla yuklendi (HANDLE tabanl�).";
    return true;
}

// Dosyadan y�kleme (std::string filePath tabanl�)
bool N3Mesh::Load(const std::string& szFileName, ID3D11Device* pDevice)
{
    HANDLE hFile = CreateFileA(szFileName.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        qDebug() << "Hata: N3Mesh dosyasi acilamadi: " << szFileName.c_str();
        return false;
    }
    bool result = Load(hFile, pDevice); // HANDLE tabanl� Load'� �a��r
    CloseHandle(hFile);
    return result;
}

// Dosyaya kaydetme (�u an i�in bo�, daha sonra implemente edilecek)
bool N3Mesh::Save(const std::wstring& filePath, const std::string& meshName)
{
    qDebug() << "N3Mesh::Save fonksiyonu hen�z implemente edilmedi.";
    return false;
}

// Vertex pozisyonlar�na g�re min/max de�erlerini bulma (�u an i�in bo�, daha sonra implemente edilecek)
void N3Mesh::FindMinMax()
{
    // Y�klenen m_pVertices ve m_dwFVF de�erleri kullan�larak bounding box ve k�re hesaplanacak.
    // Bu k�s�m, vertex verilerinin do�ru bir �ekilde parse edilmesini gerektirir.
    // �rne�in, FVF_XYZ'ye sahip bir vertex yap�s� i�in:
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
                // Position'� oku (ilk 3 float)
                memcpy(&pos, pVertex, sizeof(DirectX::XMFLOAT3));
            }
            // Di�er FVF durumlar�na g�re pos'u ayarlaman�z gerekebilir.
            // �rne�in, e�er FVF_XYZ tek ba��na ise pos = *(DirectX::XMFLOAT3*)pVertex;

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


// Render fonksiyonu (DirectX 11 �izim komutlar�n� kullan�r)
void N3Mesh::Render(ID3D11DeviceContext* pContext, ID3D11InputLayout* pInputLayout, ID3D11ShaderResourceView* pTexSRV, ID3D11SamplerState* pSamplerState)
{
    if (!pContext || !m_pVB || m_nVC == 0)
    {
        qDebug() << "N3Mesh::Render: Ge�ersiz Context veya Vertex Buffer, veya Vertex sayisi 0.";
        return; // �izilebilecek bir �ey yok
    }

    // Input Layout'u ayarla
    if (pInputLayout)
    {
        pContext->IASetInputLayout(pInputLayout);
    }
    else
    {
        qDebug() << "N3Mesh::Render: Input Layout NULL!";
        return; // Input layout olmadan �izemeyiz
    }

    // Vertex Buffer'� ba�la
    UINT stride = GetVertexSize();
    UINT offset = 0;
    pContext->IASetVertexBuffers(0, 1, &m_pVB, &stride, &offset);

    // Index Buffer'� ba�la (e�er varsa)
    if (m_pIB && m_nIC > 0)
    {
        pContext->IASetIndexBuffer(m_pIB, DXGI_FORMAT_R16_UINT, 0); // uint16_t oldu�u i�in R16_UINT
        pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        pContext->DrawIndexed(m_nIC, 0, 0);
    }
    else // Index yoksa sadece vertex buffer'� �iz
    {
        qDebug() << "N3Mesh::Render: Index Buffer yok veya Index sayisi 0. Draw �a�r�l�yor.";
        pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // Ya da POINTLIST, LINELIST duruma g�re
        pContext->Draw(m_nVC, 0);
    }

    // Texture ve Sampler ba�lama (D3D11Renderer veya Materyal y�neticisi taraf�ndan yap�lmal�)
    // �imdilik yorum sat�r�.
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