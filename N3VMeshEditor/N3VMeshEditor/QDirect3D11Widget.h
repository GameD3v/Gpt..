#ifndef QDIRECT3D11WIDGET_H
#define QDIRECT3D11WIDGET_H

#include <QWidget>
#include <QPoint>
#include "D3D11Renderer.h" // D3D11Renderer sınıfını dahil edin
#include <QScopedPointer>   // m_renderer bir QScopedPointer olduğu için gerekli
#include <QFileDialog> // QFileDialog için
#include <QMessageBox> // QMessageBox için
#include <string>           // std::wstring için gerekli
#include <QMouseEvent>      // Fare olayları için
#include <QWheelEvent>      // Fare tekerleği olayları için
#include <QKeyEvent>        // Klavye olayları için (pan için Shift tuşu)

class QDirect3D11Widget : public QWidget
{
    Q_OBJECT

public:
    explicit QDirect3D11Widget(QWidget* parent = nullptr);
    ~QDirect3D11Widget();

    // D3D11Renderer objesine erişim için
    // m_renderer bir QScopedPointer olduğu için, gerçek D3D11Renderer* nesnesine .data() ile erişiriz.
    D3D11Renderer* GetRenderer() { return m_renderer.data(); }

    // Mesh yükleme fonksiyonunu public yapıyoruz ki N3VMeshEditor çağırabilsin.
    bool LoadMesh(const QString& filePath); // Dosya yolu alacak şekilde değiştirildi

protected:
    // Qt olaylarını geçersiz kılma
    virtual QPaintEngine* paintEngine() const override; // Direct3D kullandığımız için nullptr döndürecek
    virtual void paintEvent(QPaintEvent* event) override;
    virtual void resizeEvent(QResizeEvent* event) override;
    virtual void showEvent(QShowEvent* event) override;
    virtual void hideEvent(QHideEvent* event) override;

    // Fare olayları
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent* event) override;
    virtual void wheelEvent(QWheelEvent* event) override;

    //Klavye Olayları
    virtual void keyPressEvent(QKeyEvent* event) override;
    virtual void keyReleaseEvent(QKeyEvent* event) override;

private:
    QScopedPointer<D3D11Renderer> m_renderer; // DirectX renderer
    bool m_initialized; // Renderer'ın başlatılıp başlatılmadığını kontrol eder

    // Kamera kontrolü için fare pozisyonları
    QPoint m_lastMousePos; // Son fare pozisyonunu tutmak için
    bool m_isLeftMouseButtonPressed;
    bool m_isMiddleMouseButtonPressed; // Blender'da dönüş için
    bool m_isRightMouseButtonPressed;  // Blender'da pan için

    bool m_isShiftPressed; // Pan için Shift tuşu basılı mı?
    bool m_isCtrlPressed; // İsteğe bağlı: Farklı kamera modları için Ctrl tuşu
};

#endif // QDIRECT3D11WIDGET_H