#include "QDirect3D11Widget.h"
#include <QResizeEvent>
#include <QDebug> // qDebug için
#include <QMessageBox> // QMessageBox sınıfı için
#include <QString>     // QString::fromStdString için
#include <string>      // std::wstring için (Çünkü D3D11Renderer'a göndereceğiz)
#include <QDir>        // QDir::toNativeSeparators() için

QDirect3D11Widget::QDirect3D11Widget(QWidget* parent)
    : QWidget(parent)
    , m_initialized(false)
    , m_renderer(new D3D11Renderer())
    , m_lastMousePos(0, 0) // Son fare pozisyonu
    , m_isLeftMouseButtonPressed(false) // Sol fare tuşu basılı mı
    , m_isMiddleMouseButtonPressed(false) // Orta fare tuşu basılı mı
    , m_isRightMouseButtonPressed(false) // Sağ fare tuşu basılı mı
    , m_isShiftPressed(false) // Shift basılı mı
    , m_isCtrlPressed(false) // Ctrl basılı mı
{
    // Pencerenin Direct3D ile çizilmesi için uygun bayrakları ayarla
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_NoSystemBackground); // Sistem arka planını çizmeyi devre dışı bırak

    // Odaklanılabilir yap ki klavye olaylarını alabilirsin
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true); // Fare butonu basılı olmasa bile mouseMoveEvent'i tetikle
}

QDirect3D11Widget::~QDirect3D11Widget()
{
    // m_renderer QScopedPointer olduğu için otomatik olarak delete edilecektir.
}

QPaintEngine* QDirect3D11Widget::paintEngine() const
{
    return nullptr; // Direct3D kullanıldığı için bir QPaintEngine sağlamıyoruz
}

void QDirect3D11Widget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event); // event parametresini kullanmadığımızı belirtiyoruz

    if (!m_initialized) {
        // Eğer renderer başlatılmadıysa, Initialize'ı çağır
        // winId() fonksiyonu HWND döndürür
        if (!m_renderer->Initialize(reinterpret_cast<HWND>(winId()), width(), height())) {
            QMessageBox::critical(this, "DirectX Hatası", "DirectX 11 Renderer başlatılamadı!");
            m_initialized = false; // Hata durumunda tekrar denemeyi engelle
            return;
        }
        m_initialized = true;
    }

    m_renderer->Render(); // Her çizim olayında render'ı çağır
}

void QDirect3D11Widget::resizeEvent(QResizeEvent* event)
{
    if (m_initialized) {
        m_renderer->Resize(event->size().width(), event->size().height());
    }
    QWidget::resizeEvent(event);
    update(); // Yeniden boyutlandırma sonrası widget'ı yeniden çiz
}

void QDirect3D11Widget::showEvent(QShowEvent* event)
{
    Q_UNUSED(event);
    // Widget ilk kez gösterildiğinde renderer'ı başlatma mantığı paintEvent'e taşındı.
    // Çünkü paintEvent, widget gerçekten görünür olduğunda ve HWND hazır olduğunda çağrılır.
    QWidget::showEvent(event);
}

void QDirect3D11Widget::hideEvent(QHideEvent* event)
{
    Q_UNUSED(event);
    // Widget gizlendiğinde Direct3D kaynaklarını serbest bırakmak istenebilir,
    // ancak genellikle uygulama kapanana kadar tutulur.
    QWidget::hideEvent(event);
}

// QDirect3D11Widget::LoadMesh fonksiyonunun tanımı
// Bu fonksiyon artık QString alacak ve onu D3D11Renderer'a std::wstring olarak iletecek.
bool QDirect3D11Widget::LoadMesh(const QString& qfilePath)
{
    // QDir::toNativeSeparators() ile ayraçları Windows'un yerel formatına (\\) dönüştürmek
    QString nativeFilePath = QDir::toNativeSeparators(qfilePath);
    // QString'i std::wstring'e çeviriyoruz
    std::wstring wideFilePath = nativeFilePath.toStdWString();

    // m_renderer bir QScopedPointer olduğu için, içerdiği D3D11Renderer nesnesine "->" ile erişiyoruz.
    // D3D11Renderer'daki LoadMesh fonksiyonu artık std::wstring alıyor.
    if (!m_renderer->LoadMesh(wideFilePath))
    {
        // Hata mesajı için QString::fromStdWString kullanın
        QString errorMessage = "Mesh dosyası yüklenemedi: " + QString::fromStdWString(wideFilePath) + "\nLütfen dosya yolunu veya içeriğini kontrol edin.";
        QMessageBox::critical(this, "Yükleme Hatası", errorMessage);
        return false;
    }

    // Mesh başarılı bir şekilde yüklendikten sonra, renderer'ın
    // mevcut widget boyutlarına göre kendini yeniden yapılandırmasını sağlayın.
    // Bu, resizeEvent'in yaptığı kritik çizim güncellemesini tetikleyecektir.
    if (m_initialized) { // Renderer'ın başlatıldığından emin olalım
        m_renderer->Resize(width(), height()); // Renderer'ı mevcut boyutlarla yeniden başlatmaya zorlar
    }
    // --- EKLENTİ SONU ---

    update(); // Mesh yüklendikten sonra widget'ı yeniden çiz
    return true;
}

void QDirect3D11Widget::mousePressEvent(QMouseEvent* event)
{
    m_lastMousePos = event->pos(); // Son fare pozisyonunu kaydet

    if (event->button() == Qt::LeftButton) {
        m_isLeftMouseButtonPressed = true;

        float mouseX = static_cast<float>(event->position().x());
        float mouseY = static_cast<float>(event->position().y());
        int widgetWidth = width();
        int widgetHeight = height();

        DirectX::XMVECTOR rayOrigin = m_renderer->ScreenToWorldRayOrigin(mouseX, mouseY, widgetWidth, widgetHeight);
        DirectX::XMVECTOR rayDirection = m_renderer->ScreenToWorldRayDirection(mouseX, mouseY, widgetWidth, widgetHeight);

        if (m_renderer->PickMesh(rayOrigin, rayDirection))
        {
            // Mesh seçildiyse, sürüklemeye hazırız.
            m_renderer->SetIsDraggingMeshNow(true); // <<-- Setter kullanıldı
            m_renderer->CaptureSelectedMeshDepth(mouseX, mouseY, widgetWidth, widgetHeight);
            qDebug() << "Mesh picked and dragging initiated.";
        }
        else
        {
            // Hiçbir mesh seçilmediyse veya boşluğa tıklandıysa, mevcut seçimi kaldır ve sürüklemeyi durdur.
            m_renderer->SetIsMeshSelected(false);    // <<-- Setter kullanıldı
            m_renderer->SetIsDraggingMeshNow(false); // <<-- Setter kullanıldı
            qDebug() << "No mesh picked. Selection and dragging reset.";
        }

        update();
    }
    else if (event->button() == Qt::MiddleButton) {
        m_isMiddleMouseButtonPressed = true;
        m_renderer->SetIsDraggingMeshNow(false); // Orta tuşa basıldığında mesh sürüklenmez. <<-- Setter kullanıldı
    }
    else if (event->button() == Qt::RightButton) {
        m_isRightMouseButtonPressed = true;
        m_renderer->SetIsDraggingMeshNow(false); // Sağ tuşa basıldığında mesh sürüklenmez. <<-- Setter kullanıldı
    }
    QWidget::mousePressEvent(event);
}

void QDirect3D11Widget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_isLeftMouseButtonPressed = false;
        // Sol tuş bırakıldığında sürükleme biter. Seçili olma durumu değişmez.
        m_renderer->SetIsDraggingMeshNow(false); // <<-- Setter kullanıldı
        qDebug() << "Left mouse button released. Dragging stopped.";
    }
    else if (event->button() == Qt::MiddleButton) {
        m_isMiddleMouseButtonPressed = false;
    }
    else if (event->button() == Qt::RightButton) {
        m_isRightMouseButtonPressed = false;
    }
    QWidget::mouseReleaseEvent(event);
}

void QDirect3D11Widget::mouseMoveEvent(QMouseEvent* event)
{
    int dx = static_cast<int>(event->position().x()) - m_lastMousePos.x();
    int dy = static_cast<int>(event->position().y()) - m_lastMousePos.y();

    // --- Mesh Sürükleme Mantığı ---
    // Eğer sol tuş basılıysa VE bir mesh seçiliyse VE renderer'da sürükleme modu aktifse
    if (m_isLeftMouseButtonPressed && m_renderer->IsMeshSelected() && m_renderer->IsDraggingMeshNow()) // <<-- Getter kullanıldı
    {
        float mouseX = static_cast<float>(event->position().x());
        float mouseY = static_cast<float>(event->position().y());
        int widgetWidth = width();
        int widgetHeight = height();

        m_renderer->DragSelectedMesh(mouseX, mouseY, widgetWidth, widgetHeight);
        update();
    }
    // --- Kamera Kontrol Mantığı (Sadece mesh sürüklenmiyorsa çalışır) ---
    else // Eğer mesh sürüklenmiyorsa (veya sol tuş basılı değilse), kamera hareketlerini kontrol et
    {
        if (m_isShiftPressed && m_isMiddleMouseButtonPressed) {
            m_renderer->PanCamera(static_cast<float>(dx), static_cast<float>(dy));
            update();
        }
        else if (m_isMiddleMouseButtonPressed) {
            m_renderer->RotateCamera(static_cast<float>(dx), static_cast<float>(dy));
            update();
        }
        else if (m_isRightMouseButtonPressed) {
            m_renderer->PanCamera(static_cast<float>(dx), static_cast<float>(dy));
            update();
        }
    }

    m_lastMousePos = event->pos();
    QWidget::mouseMoveEvent(event);
}

void QDirect3D11Widget::wheelEvent(QWheelEvent* event)
{
    // angleDelta().y() genellikle her "çentik" için 120 veya -120 döner.
    // Daha pürüzsüz bir yakınlaştırma için bu değeri uygun bir çarpanla küçültmek iyi olur.
    float zoomDelta = static_cast<float>(event->angleDelta().y());
    m_renderer->ZoomCamera(zoomDelta);
    update(); // Yakınlaştırdıktan sonra widget'ı yeniden çiz
    QWidget::wheelEvent(event); // Taban sınıf olay işleyicisini çağır
}

void QDirect3D11Widget::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Shift) {
        m_isShiftPressed = true;
        qDebug() << "Shift basildi.";
    }
    else if (event->key() == Qt::Key_Control) {
        m_isCtrlPressed = true;
        qDebug() << "Ctrl basildi.";
    }
    QWidget::keyPressEvent(event);
}

void QDirect3D11Widget::keyReleaseEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Shift) {
        m_isShiftPressed = false;
        qDebug() << "Shift birakildi.";
    }
    else if (event->key() == Qt::Key_Control) {
        m_isCtrlPressed = false;
        qDebug() << "Ctrl birakildi.";
    }
    QWidget::keyReleaseEvent(event);
}