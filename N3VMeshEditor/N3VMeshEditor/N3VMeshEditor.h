#ifndef N3VMESHEDITOR_H
#define N3VMESHEDITOR_H

#include <QMainWindow>
#include <string> // std::string için
#include "QDirect3D11Widget.h" // D3D11 widget'ınızı dahil edin
#include <QLabel>      // QLabel için
#include <QSlider>     // QSlider için
#include "CN3VMesh.h"          // CN3VMesh sınıfını dahil edin (VertexPositionColor'ı da buradan alacak)
#include "ui_N3VMeshEditor.h"

// N3VMeshEditor.ui dosyasından oluşturulan UI sınıfının ileri bildirimi
// ui_N3VMeshEditor.h sadece .cpp dosyasında dahil edilmelidir.
namespace Ui {
    class N3VMeshEditor; // İleri bildirim
}

class N3VMeshEditor : public QMainWindow
{
    Q_OBJECT

public:
    explicit N3VMeshEditor(QWidget* parent = nullptr);
    ~N3VMeshEditor();

private slots:
    void onLoadMeshButtonClicked();
    void onTranslateXButtonClicked();
    void onTranslateYButtonClicked();
    void onRotateYawButtonClicked(); // Eğer dönüş için bir slot ekleyecekseniz
    void on_actionOpenMesh_triggered(); // Bu fonksiyonu ekleyin veya mevcut bir dosyayı açma slotunuz varsa onu kullanın
    void onToggleWireframeButtonClicked(); // Yeni slot
    void onCameraTargetYSliderValueChanged(int value); // Yeni slot
    void onSaveMeshButtonClicked(); // Yeni eklenen Save butonu için slot

private:
    Ui::N3VMeshEditorClass* ui; // ui elemanlarına erişmek için (ileri bildirim sayesinde burada sorun olmaz)
    QDirect3D11Widget* m_d3dWidget; // Direct3D render widget'ımız
    QSlider* m_cameraTargetYSlider; // <<--- Yeni üye, şimdi bu member'ı kullanacağız
};

#endif // N3VMESHEDITOR_H