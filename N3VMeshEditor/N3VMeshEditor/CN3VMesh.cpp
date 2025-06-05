// CN3VMesh.cpp
#include "CN3VMesh.h" // Kendi başlık dosyamızı include et

// Eğer StdAfxBase.h gibi genel bir başlık dosyanız varsa, onu da include edebilirsiniz:
// #include "StdAfxBase.h"

// Gerekli kütüphaneleri manuel olarak ekleyebilirsiniz
#include <fstream>   // Dosya okuma için (Load(std::string) kullanılıyorsa)
#include <vector>    // Geçici vertex buffer için
#include <algorithm> // min/max için
#include <cfloat>    // FLT_MAX için
#include <QDebug>    // qDebug için
#include <Windows.h> // HANDLE, ReadFile, GetLastError, FormatMessageW için
#include <cmath>     // sqrtf için

CN3VMesh::CN3VMesh()
    : m_pVertices(nullptr), m_nVC(0), m_pwIndices(nullptr), m_nIC(0),
    m_vCenter(0.0f, 0.0f, 0.0f), m_fRadius(0.0f),
    m_minBounds(FLT_MAX, FLT_MAX, FLT_MAX), m_maxBounds(-FLT_MAX, -FLT_MAX, -FLT_MAX)
{
    Release(); // Temiz bir başlangıç için
}

CN3VMesh::~CN3VMesh()
{
    Release(); // Yıkıcıda kaynakları serbest bırakma
}

void CN3VMesh::Release()
{
    if (m_pVertices) {
        delete[] m_pVertices;
        m_pVertices = nullptr;
    }
    m_nVC = 0;

    if (m_pwIndices) {
        delete[] m_pwIndices;
        m_pwIndices = nullptr;
    }
    m_nIC = 0;

    // m_vCenter'ı DirectX::XMFLOAT3 olarak sıfırla
    m_vCenter = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
    m_fRadius = 0.0f;

    m_minBounds = DirectX::XMFLOAT3(FLT_MAX, FLT_MAX, FLT_MAX);
    m_maxBounds = DirectX::XMFLOAT3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
}

// CN3VMesh::Load fonksiyonu (HANDLE hFile versiyonu)
bool CN3VMesh::Load(HANDLE hFile)
{
    Release(); // Zaten yüklü ise temizle

    if (hFile == INVALID_HANDLE_VALUE || hFile == NULL) {
        qDebug() << "Hata: CN3VMesh::Load - Gecersiz dosya yolu.";
        return false;
    }

    DWORD dwRWC = 0; // Okunan veya yazılan byte sayısı

    // 1. Nesne Adı Uzunluğu (int) ve Adı (string)
    // Orijinal dosya formatına göre burası "collision" ismini içermeli.
    int nameLength = 0;
    if (!ReadFile(hFile, &nameLength, sizeof(int), &dwRWC, NULL) || dwRWC != sizeof(int)) {
        qDebug() << "Hata: Mesh adi uzunlugu okunamadi veya tam okunmadi. Okunan byte: " << dwRWC << ", Beklenen: " << sizeof(int);
        // Bu hata, dosyanın başında beklenen formatın olmadığını gösterir.
        // Orijinal .n3vmesh dosyasında bu kısım mutlaka olmalıdır.
        return false;
    }

    // Güvenlik kontrolü: nameLength'in mantıklı bir değer olduğundan emin ol
    const int MAX_NAME_LENGTH = 256; // Maksimum kabul edilebilir isim uzunluğu
    if (nameLength < 0 || nameLength > MAX_NAME_LENGTH) {
        qDebug() << "Hata: Gecersiz mesh adi uzunlugu okundu: " << nameLength << ". Dosya bozuk olabilir.";
        return false;
    }

    if (nameLength > 0)
    {
        std::vector<char> objName(nameLength + 1); // +1 null-terminator için
        if (!ReadFile(hFile, objName.data(), nameLength, &dwRWC, NULL) || dwRWC != nameLength) {
            qDebug() << "Hata: Mesh adi okunamadi veya tam okunmadi. Okunan byte: " << dwRWC << ", Beklenen: " << nameLength;
            return false;
        }
        objName[nameLength] = '\0'; // Null-terminate string
        qDebug() << "Mesh Adi: " << objName.data(); // Okunan mesh adını logla (örn: "collision")

        // İsteğe bağlı: Okunan ismin "collision" olup olmadığını kontrol edebilirsiniz.
        // if (std::string(objName.data()) != "collision") {
        //     qDebug() << "Uyari: Okunan mesh adi 'collision' degil: " << objName.data();
        // }
    }
    else {
        qDebug() << "Uyari: Mesh adi uzunlugu 0 olarak okundu.";
        // Orijinal dosya formatında bu kısım her zaman olacağı için buraya gelmemeli.
        // Geliyorsa dosya formatında bir sorun var demektir.
    }

    // 2. Vertex Sayısı (int)
    if (!ReadFile(hFile, &m_nVC, sizeof(int), &dwRWC, NULL) || dwRWC != sizeof(int)) {
        qDebug() << "Hata: Vertex sayisi okunamadi veya tam olarak okunmadi. Okunan byte: " << dwRWC << ", Beklenen: " << sizeof(int);
        Release();
        return false;
    }
    qDebug() << "Okunan m_nVC: " << m_nVC;

    // Güvenlik kontrolü: Vertex sayısının mantıklı sınırlar içinde olup olmadığını kontrol et
    const int MAX_VERTICES = 2000000; // Makul bir üst sınır belirleyin (örn. 2 milyon)
    if (m_nVC < 0 || m_nVC > MAX_VERTICES) {
        qDebug() << "Hata: Gecersiz vertex sayisi okundu: " << m_nVC << ". Dosya bozuk olabilir veya cok buyuk/kucuk.";
        Release();
        return false;
    }

    if (m_nVC > 0)
    {
        // 3. Vertex Verileri (__Vector3 olarak oku)
        // Knight Online'ın N3VMesh'i genellikle __Vector3 olarak pozisyon okur (renk bilgisi olmadan).
        // Bizim yapımız __VertexColor olduğu için ara bir vector kullanıyoruz.
        std::vector<__Vector3> tempVertices(m_nVC);
        if (!ReadFile(hFile, tempVertices.data(), m_nVC * sizeof(__Vector3), &dwRWC, NULL) || dwRWC != m_nVC * sizeof(__Vector3)) {
            qDebug() << "Hata: Vertex verileri okunamadi veya tam okunmadi. Okunan byte: " << dwRWC << ", Beklenen: " << (m_nVC * sizeof(__Vector3));
            Release();
            return false;
        }

        // __VertexColor formatına dönüştür ve varsayılan renk ata
        m_pVertices = new __VertexColor[m_nVC];
        if (!m_pVertices) { // Bellek tahsis kontrolü
            qDebug() << "Hata: Vertex verileri icin bellek tahsis edilemedi.";
            Release();
            return false;
        }

        // Bounding Box'ı başlat (her zaman hesaplanır, dosyadan okunmaz)
        m_minBounds = DirectX::XMFLOAT3(FLT_MAX, FLT_MAX, FLT_MAX);
        m_maxBounds = DirectX::XMFLOAT3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

        for (int i = 0; i < m_nVC; ++i)
        {
            m_pVertices[i].x = tempVertices[i].x;
            m_pVertices[i].y = tempVertices[i].y;
            m_pVertices[i].z = tempVertices[i].z;
            m_pVertices[i].color = 0xFFFFFFFF; // Varsayılan beyaz renk

            // Bounds güncellemesi
            m_minBounds.x = std::min(m_minBounds.x, m_pVertices[i].x);
            m_minBounds.y = std::min(m_minBounds.y, m_pVertices[i].y);
            m_minBounds.z = std::min(m_minBounds.z, m_pVertices[i].z);

            m_maxBounds.x = std::max(m_maxBounds.x, m_pVertices[i].x);
            m_maxBounds.y = std::max(m_maxBounds.y, m_pVertices[i].y);
            m_maxBounds.z = std::max(m_maxBounds.z, m_pVertices[i].z);
        }

        // m_vCenter ve m_fRadius'u hesapla (her zaman hesaplanır, dosyadan okunmaz)
        m_vCenter.x = (m_minBounds.x + m_maxBounds.x) / 2.0f;
        m_vCenter.y = (m_minBounds.y + m_maxBounds.y) / 2.0f;
        m_vCenter.z = (m_minBounds.z + m_maxBounds.z) / 2.0f;

        // Yarıçapı hesapla (Bounding Box'ın köşegeninin yarısı)
        float dx = m_maxBounds.x - m_minBounds.x;
        float dy = m_maxBounds.y - m_minBounds.y;
        float dz = m_maxBounds.z - m_minBounds.z;
        m_fRadius = sqrtf(dx * dx + dy * dy + dz * dz) / 2.0f;

    } // if (m_nVC > 0) bloğunun kapanışı
    else {
        qDebug() << "Uyari: Mesh'te hic vertex yok.";
    }

    // 4. Index Sayısı (int)
    // Hex dump'a göre 'bat_catl_ka_collision.n3vmesh' dosyası vertex verilerinden sonra bitiyor.
    // Yani bu dosya tipi index sayısı ve bounding sphere bilgileri içermiyor.
    // Bu yüzden bu kısımda bir okuma hatası almanız beklenir.
    // m_nIC'yi varsayılan olarak 0 bırakıyoruz, çünkü ReadFile 0 bayt okuyarak bu değeri değiştirmeyecek.
    if (!ReadFile(hFile, &m_nIC, sizeof(int), &dwRWC, NULL) || dwRWC != sizeof(int)) {
        qDebug() << "Hata: Index sayisi okunamadi veya tam olarak okunmadi. Okunan byte: " << dwRWC << ", Beklenen: " << sizeof(int);
        m_nIC = 0; // Hata durumunda index sayısını 0 olarak ayarla
        m_pwIndices = nullptr; // Index dizisini null yap
        Release(); // Vertex'ler tahsis edilmişse onları da temizle
        return false; // Index sayısı okunamadıysa bu ciddi bir hatadır.
    }
    qDebug() << "Okunan m_nIC: " << m_nIC;

    // Güvenlik kontrolü: Index sayısının mantıklı sınırlar içinde olup olmadığını kontrol et
    const int MAX_INDICES = 3000000; // Makul bir üst sınır belirleyin (örn. 3 milyon)
    if (m_nIC < 0 || m_nIC > MAX_INDICES) {
        qDebug() << "Hata: Gecersiz index sayisi okundu: " << m_nIC << ". Dosya bozuk olabilir veya cok buyuk/kucuk.";
        Release();
        return false;
    }

    if (m_nIC > 0)
    {
        // 5. Index Verileri (WORD olarak oku)
        m_pwIndices = new WORD[m_nIC];
        if (!m_pwIndices) { // Bellek tahsis kontrolü
            qDebug() << "Hata: Index verileri icin bellek tahsis edilemedi.";
            Release();
            return false;
        }
        if (!ReadFile(hFile, m_pwIndices, m_nIC * sizeof(WORD), &dwRWC, NULL) || dwRWC != m_nIC * sizeof(WORD)) {
            qDebug() << "Hata: Index verileri okunamadi veya tam okunmadi. Okunan byte: " << dwRWC << ", Beklenen: " << (m_nIC * sizeof(WORD));
            Release();
            return false;
        }
    }
    else {
        qDebug() << "Uyari: Mesh'te hic index yok (m_nIC=0).";
    }

    // 6. Merkez Noktası (__Vector3) ve Yarıçap (float)
    // Bu kısımlar 'bat_catl_ka_collision.n3vmesh' dosyasında mevcut değil.
    // Bu yüzden okumaları kaldırıyoruz. Eğer başka dosyalar bu bilgiyi içeriyorsa,
    // o zaman farklı bir Load fonksiyonu veya format belirleyici bir bayrak gerekebilir.
    // Şu anki senaryoda, bu değerler zaten vertex verilerinden hesaplanmış durumda.
    /*
    if (!ReadFile(hFile, &m_vCenter, sizeof(__Vector3), &dwRWC, NULL) || dwRWC != sizeof(__Vector3)) {
        qDebug() << "Hata: Merkez noktasi okunamadi veya tam okunmadi. Okunan byte: " << dwRWC << ", Beklenen: " << sizeof(__Vector3);
        Release();
        return false;
    }
    if (!ReadFile(hFile, &m_fRadius, sizeof(float), &dwRWC, NULL) || dwRWC != sizeof(float)) {
        qDebug() << "Hata: Yaricap okunamadi veya tam okunmadi. Okunan byte: " << dwRWC << ", Beklenen: " << sizeof(float);
        Release();
        return false;
    }
    */

    qDebug() << "Mesh basariyla yuklendi. VC: " << m_nVC << ", IC: " << m_nIC;
    return true;
}

// CN3VMesh::Save fonksiyonu
bool CN3VMesh::Save(const std::wstring& filePath, const std::string& meshNamePlaceholder)
{
    qDebug() << "CN3VMesh::Save cagriliyor. Hedef dosya yolu: " << QString::fromStdWString(filePath);
    qDebug() << "Vertex Pointer Gecerli mi: " << (m_pVertices != nullptr);
    qDebug() << "Vertex Sayisi (m_nVC): " << m_nVC;
    qDebug() << "Index Pointer Gecerli mi: " << (m_pwIndices != nullptr);
    qDebug() << "Index Sayisi (m_nIC): " << m_nIC;

    if (!m_pVertices || m_nVC == 0)
    {
        qDebug() << "Hata Detayi: Kaydedilecek mesh verisi yok veya eksik (m_pVertices veya m_nVC kontrolu basarisiz oldu).";
        return false;
    }

    // Dosyayı std::ofstream ile wstring yoluyla açmak için _wfopen_s kullanıyoruz.
    // std::ofstream file(filePath, std::ios::binary | std::ios::trunc); // Bu satırı yorumlayın
    FILE* pFile = nullptr;
    // _wfopen_s geniş karakterli dosya yollarını destekler
    errno_t err = _wfopen_s(&pFile, filePath.c_str(), L"wb"); // "wb" binary yazma modu
    if (err != 0 || pFile == nullptr)
    {
        // Dosya açma hatası
        DWORD error = GetLastError(); // WinAPI hata kodu
        LPWSTR messageBuffer = nullptr;
        // Hata mesajını formatlamak için FormatMessageW kullan
        FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&messageBuffer, 0, NULL);

        QString errorMessage = "Hata Detayi: Dosya acilamadi (kodu: " + QString::number(error) + ")";
        if (messageBuffer) {
            errorMessage += " - " + QString::fromWCharArray(messageBuffer);
            LocalFree(messageBuffer);
        }
        qDebug() << errorMessage;
        return false;
    }

    // Şimdi fwrite kullanarak yazma işlemlerini yapın.
    // fwrite ile yazarken, verinin adresini ve boyutunu char* olarak cast etmek güvenlidir.

    // 1. Orijinal dosya formatına uygun olarak "collision" ismini ve uzunluğunu yaz
    const std::string fixedMeshName = "collision"; // Sabit isim: "collision"
    int nameLength = fixedMeshName.length();      // Uzunluk: 9

    // İsim uzunluğunu yaz
    fwrite(&nameLength, sizeof(int), 1, pFile); // 4 byte

    // İsmi yaz
    fwrite(fixedMeshName.data(), sizeof(char), nameLength, pFile); // 9 byte

    // 2. Vertex sayısını yaz
    fwrite(&m_nVC, sizeof(int), 1, pFile); // 4 byte

    // 3. Vertex pozisyonlarını yaz (__Vector3 olarak)
    // __Vector3, __VertexColor'dan farklıdır, sadece pozisyon içerir.
    // m_pVertices __VertexColor tipinde olduğu için, pozisyon verilerini __Vector3 olarak ayıklayıp yazıyoruz.
    std::vector<__Vector3> tempVertices(m_nVC);
    for (int i = 0; i < m_nVC; ++i)
    {
        tempVertices[i].x = m_pVertices[i].x;
        tempVertices[i].y = m_pVertices[i].y;
        tempVertices[i].z = m_pVertices[i].z;
    }
    fwrite(tempVertices.data(), sizeof(__Vector3), m_nVC, pFile); // m_nVC * 12 byte (3 float * 4 byte)

    // 4. Index sayısını yaz (m_nIC 0 olsa bile yaz)
    fwrite(&m_nIC, sizeof(int), 1, pFile);

    // 5. Index verilerini yaz (Sadece m_nIC > 0 ise)
    if (m_nIC > 0 && m_pwIndices != nullptr)
    {
        fwrite(m_pwIndices, sizeof(WORD), m_nIC, pFile); // m_nIC * 2 byte
    }

    // 6. Merkez noktasını yaz
    // m_vCenter'ın tipi __Vector3 olduğu varsayıldı.
    //fwrite(&m_vCenter, sizeof(__Vector3), 1, pFile);
    //
    //// 7. Yarıçapı yaz
    //fwrite(&m_fRadius, sizeof(float), 1, pFile);

    fclose(pFile); // Dosyayı kapat
    qDebug() << "Mesh basariyla kaydedildi: " << QString::fromStdWString(filePath);
    return true;
}