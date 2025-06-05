// CN3VMesh.h
#pragma once

#include <windows.h> // HANDLE tipi için (CN3VMesh::Load'da kullanılıyorsa)
#include <string>    // std::string için
#include "CommonN3Structures.h" // __Vector3 ve __VertexColor için
#include <DirectXMath.h> // XMFLOAT3 için

class CN3VMesh
{
public:
    CN3VMesh();
    ~CN3VMesh();

    void Release(); // Kaynakları serbest bırakma

    // Dosyadan yükleme fonksiyonu
    // Eğer Load fonksiyonunuz HANDLE alıyorsa, aşağıdaki versiyonu kullanın:
    bool Load(HANDLE hFile);
    // YENİ EKLENECEK: Dosyaya kaydetme fonksiyonu
    bool Save(const std::wstring& filePath, const std::string& meshName = "DefaultMesh"); // <<-- BURASI DÜZELTİLDİ!

    // BOUNDING SPHERE ve BOUNDING BOX için üyeler (opsiyonel, eğer hesaplıyorsanız)
    DirectX::XMFLOAT3 GetCenter() const { return m_vCenter; } // <<-- BU SATIR VAR MI KONTROL ET! YOKSA EKLE!
    float GetRadius() const { return m_fRadius; }             // <<-- BU SATIR VAR MI KONTROL ET! YOKSA EKLE!

    // Eğer dosya yolu (std::string) alıyorsa, aşağıdaki versiyonu kullanın:
    // bool Load(const std::string& filePath);
        // ... (mevcut üyeler)
    DirectX::XMFLOAT3 GetMinBounds() const { return m_minBounds; }
    DirectX::XMFLOAT3 GetMaxBounds() const { return m_maxBounds; }

    // Mesh verilerine erişim metotları
    __VertexColor* GetVertices() const { return m_pVertices; }
    WORD* GetIndices() const { return m_pwIndices; }
    int VertexCount() const { return m_nVC; }
    int IndexCount() const { return m_nIC; }

private:
    // ... (mevcut özel üyeler)
    DirectX::XMFLOAT3 m_minBounds;
    DirectX::XMFLOAT3 m_maxBounds;
    DirectX::XMFLOAT3 m_vCenter;
    float m_fRadius;            

protected:
    __VertexColor* m_pVertices;    // Vertex dizisi
    int             m_nVC;          // Vertex sayısı
    WORD* m_pwIndices;    // İndeks dizisi
    int             m_nIC;          // İndeks sayısı
};