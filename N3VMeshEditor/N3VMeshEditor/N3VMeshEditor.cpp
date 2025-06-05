#include "N3VMeshEditor.h"
#include "ui_N3VMeshEditor.h" // Eksik olan include burada eklendi
#include <QFileDialog>        // Dosya açma diyaloğu için
#include <QDebug>             // Debug mesajları için
#include <QVBoxLayout>        // Dikey düzen için
#include <QHBoxLayout>        // Yatay düzen için
#include <QPushButton>        // Butonlar için
#include <QCoreApplication> // QCoreApplication::applicationDirPath() için
#include <QDir>         // QDir::toNativeSeparators() için
#include <QMessageBox> // QMessageBox için
#include <vector>       // Eğer kullanıyorsanız
#include <string>      // std::string için
#include <Windows.h> // HANDLE ve CreateFileA için
#include <QAction> // QAction kullanmak için bu başlığı ekleyin
#include <QIcon>   // QIcon kullanmak için (isteğe bağlı)
#include <QApplication>

// N3VMeshEditor::N3VMeshEditor fonksiyonunun başlangıcı
N3VMeshEditor::N3VMeshEditor(QWidget* parent)
    : QMainWindow(parent),
    ui(new Ui::N3VMeshEditorClass) // ui nesnesini oluştur
{
    ui->setupUi(this); // UI'ı bu pencereye kur. Bu, .ui dosyasındaki her şeyi oluşturur.

    // ui->d3d11Widget, setupUi tarafından oluşturulmuş QDirect3D11Widget'tır.
    // m_d3dWidget sınıf üyemizi, setupUi'nin oluşturduğu widget'a atıyoruz.
    m_d3dWidget = ui->d3d11Widget;

    // --- QToolBar Kurulumu ---
    // 1. mainToolBar'ın var olduğundan emin olalım
    if (ui->mainToolBar) {
        ui->mainToolBar->setMovable(false); // Kullanıcının araç çubuğunu sürüklemesini engeller
        // ui->mainToolBar->setFloatable(false); // Eğer menüden kopup yüzmesini de istemiyorsanız
    }

    // 2. Mesh Yükle Eylemi (QAction) Oluştur
    // İkon kullanmak isterseniz projenize qrc dosyası eklemeniz gerekir (örn. :/icons/open_file.png)
    QAction* loadMeshAction = new QAction("Open File", this);
    // İkonlu örnek (eğer qrc dosyanız ve ikonunuz varsa):
    // QAction* loadMeshAction = new QAction(QIcon(":/icons/open_file.png"), "Mesh Yükle", this);
    loadMeshAction->setShortcut(QKeySequence::Open); // Ctrl+O kısayolu
    loadMeshAction->setStatusTip("3D Dosyalarını yükler.");

    // 3. Mesh Kaydet Eylemi (QAction) Oluştur
    QAction* saveMeshAction = new QAction("Save File", this);
    // İkonlu örnek:
    // QAction* saveMeshAction = new QAction(QIcon(":/icons/save_file.png"), "Mesh Kaydet", this);
    saveMeshAction->setShortcut(QKeySequence::Save); // Ctrl+S kısayolu
    saveMeshAction->setStatusTip("3D Dosyalarını kaydeder.");

    // 4. Eylemleri Araç Çubuğuna Ekle
    if (ui->mainToolBar) {
        ui->mainToolBar->addAction(loadMeshAction);
        ui->mainToolBar->addAction(saveMeshAction);
    }

    // 5. Eylemleri Mevcut Slotlara Bağla
    // Bu eylemler, daha önceki butonlarınızın bağlı olduğu slotları tetikleyecek.
    // DİKKAT: Slot isimleri N3VMeshEditor.h dosyanızdaki tanımlara göre düzeltildi.
    connect(loadMeshAction, &QAction::triggered, this, &N3VMeshEditor::onLoadMeshButtonClicked);
    connect(saveMeshAction, &QAction::triggered, this, &N3VMeshEditor::onSaveMeshButtonClicked);


    // --- Özel Butonlar ve Slider Entegrasyonu (UI dosyasında olmayanlar) ---
    // Bu elemanları, sol paneldeki var olan layout'a ekleyeceğiz.
    // ui_N3VMeshEditor.h'ye göre sol panelde 'leftPanelWidget' içinde 'verticalLayout_2' var.

    QHBoxLayout* customControlsLayout = new QHBoxLayout();
    customControlsLayout->setObjectName("customControlsLayout"); // Hata ayıklama için isteğe bağlı isim

    QPushButton* translateUpButton = new QPushButton("Küpü Yükselt", this);
    QPushButton* translateDownButton = new QPushButton("Küpü Alçalt", this);
    QPushButton* translateLeftButton = new QPushButton("Küpü Sola", this);
    QPushButton* translateRightButton = new QPushButton("Küpü Sağa", this);
    QPushButton* toggleWireframeButton = new QPushButton("Kafes Modu", this);
    toggleWireframeButton->setCheckable(true);
    toggleWireframeButton->setChecked(false);
    QPushButton* exitButton = new QPushButton("Çıkış", this);

    QLabel* cameraTargetYLabel = new QLabel("Kamera Hedef Y:", this);
    QSlider* cameraTargetYSlider = new QSlider(Qt::Horizontal, this);
    cameraTargetYSlider->setRange(-100, 100);
    cameraTargetYSlider->setValue(0);
    cameraTargetYSlider->setSingleStep(1);
    cameraTargetYSlider->setPageStep(10);
    m_cameraTargetYSlider = cameraTargetYSlider; // Sınıf üyesine ata

    // Özel butonları ve slider'ı yeni layout'a ekle
    customControlsLayout->addWidget(translateUpButton);
    customControlsLayout->addWidget(translateDownButton);
    customControlsLayout->addWidget(translateLeftButton);
    customControlsLayout->addWidget(translateRightButton);
    customControlsLayout->addWidget(toggleWireframeButton);
    customControlsLayout->addWidget(cameraTargetYLabel);
    customControlsLayout->addWidget(m_cameraTargetYSlider);
    customControlsLayout->addStretch(); // Sağdaki boşluk
    customControlsLayout->addWidget(exitButton);


    // Yeni customControlsLayout'ı, leftPanelWidget'ın dikey layout'una (verticalLayout_2) ekle.
    // ui->setupUi(this) sonrası ui->verticalLayout_2 erişilebilir olur.
    if (ui->verticalLayout_2) {
        // ui_N3VMeshEditor.h dosyasında verticalLayout_2'nin sonunda bir verticalSpacer var.
        // Yeni layout'umuzu bu spacer'dan önce ekleyelim, böylece altta kalır.
        int spacerIndex = -1;
        for (int i = 0; i < ui->verticalLayout_2->count(); ++i) {
            if (ui->verticalLayout_2->itemAt(i)->spacerItem() && ui->verticalLayout_2->itemAt(i)->spacerItem() == ui->verticalSpacer) {
                spacerIndex = i;
                break;
            }
        }

        if (spacerIndex != -1) {
            // Spacer bulunduysa, onun önüne ekle
            ui->verticalLayout_2->insertLayout(spacerIndex, customControlsLayout);
        }
        else {
            // Spacer bulunamadıysa veya hiç spacer yoksa, en sona ekle
            ui->verticalLayout_2->addLayout(customControlsLayout);
        }
    }
    else {
        qDebug() << "HATA: ui->verticalLayout_2 bulunamadi. Ozel kontroller eklenemedi.";
    }

    // Pencere başlığını ve boyutunu ayarla
    setWindowTitle("N3VMeshEditor - Created By Emre ;[");
    // resize(800, 600); // setupUi zaten boyutu ayarlar. Gerekirse bu satırı açın.

    // Özel butonların sinyal-slot bağlantıları
    connect(translateUpButton, &QPushButton::clicked, this, &N3VMeshEditor::onTranslateYButtonClicked);
    connect(translateDownButton, &QPushButton::clicked, this, &N3VMeshEditor::onTranslateYButtonClicked);
    connect(translateLeftButton, &QPushButton::clicked, this, &N3VMeshEditor::onTranslateXButtonClicked);
    connect(translateRightButton, &QPushButton::clicked, this, &N3VMeshEditor::onTranslateXButtonClicked);
    connect(toggleWireframeButton, &QPushButton::toggled, this, &N3VMeshEditor::onToggleWireframeButtonClicked);
    connect(exitButton, &QPushButton::clicked, this, &QMainWindow::close);
    connect(m_cameraTargetYSlider, &QSlider::valueChanged, this, &N3VMeshEditor::onCameraTargetYSliderValueChanged);
}

N3VMeshEditor::~N3VMeshEditor()
{
    delete ui; // UI objesini temizle
    // m_d3dWidget QMainWindow'ın bir çocuğu olduğu için Qt tarafından otomatik olarak silinecektir.
    // m_cameraTargetYSlider da QWidget'ın bir çocuğu olduğu için otomatik silinir.
}

void N3VMeshEditor::onCameraTargetYSliderValueChanged(int value)
{
    // Slider değeri int olduğu için float'a dönüştür.
    // -100'den 100'e kadar bir aralık, float'a çevrilince -10.0f'dan 10.0f'a karşılık gelsin.
    float targetY = static_cast<float>(value) / 10.0f;
    if (m_d3dWidget && m_d3dWidget->GetRenderer()) { // Null kontrolü ekleyelim
        m_d3dWidget->GetRenderer()->SetCameraTargetY(targetY);
        m_d3dWidget->update(); // Kamerayı güncelledikten sonra widget'ı yeniden çiz
    }
}

void N3VMeshEditor::onToggleWireframeButtonClicked()
{
    QPushButton* senderButton = qobject_cast<QPushButton*>(sender());
    if (senderButton) {
        bool isWireframeEnabled = senderButton->isChecked(); // setCheckable kullandığımız için bu doğru
        if (m_d3dWidget && m_d3dWidget->GetRenderer()) { // Null kontrolü ekleyelim
            m_d3dWidget->GetRenderer()->SetWireframeMode(isWireframeEnabled);
            m_d3dWidget->update(); // Render modunu değiştirdikten sonra widget'ı yeniden çiz
        }
    }
}

void N3VMeshEditor::on_actionOpenMesh_triggered()
{
    QString qFilePath = QFileDialog::getOpenFileName(this,
        tr("Collision Mesh Yükle"), // Pencere Başlığı
        "",                        // Varsayılan dizin (boş ise son kullanılan)
        tr("N3VMesh Dosyaları (*.n3vmesh);;Tüm Dosyalar (*)")); // Filtreler

    if (!qFilePath.isEmpty()) {
        // QString'i doğrudan LoadMesh'e gönderiyoruz, D3D11Renderer bunu std::wstring'e çevirecek.
        if (m_d3dWidget->LoadMesh(qFilePath))
        {
            qDebug() << "Mesh yükleme talebi başarıyla gönderildi: " << qFilePath;

            // Mesh yüklendiğinde kamerayı mesh'in boyutlarına göre ayarla
            // D3D11Renderer içindeki SetCameraToMeshBounds fonksiyonu, mesh'in merkezini bulup kamerayı otomatik olarak konumlandıracak.
            // Bu aynı zamanda m_cameraTarget'ın Y bileşenini de etkileyecek.
            // Slider'ın değerini bu yeni Y değerine set edebiliriz.
            if (m_d3dWidget && m_d3dWidget->GetRenderer()) { // Null kontrolü ekleyelim
                float meshCenterY = m_d3dWidget->GetRenderer()->GetCameraTargetY(); // <<--- DÜZELTİLDİ
                m_cameraTargetYSlider->setValue(static_cast<int>(meshCenterY * 10.0f)); // Slider değerini güncelle
            }

            m_d3dWidget->update(); // Yeni mesh yüklendiğinde render'ı tetikle
        }
        else
        {
            qDebug() << "Hata: Mesh yükleme talebi başarısız oldu: " << qFilePath;
            QMessageBox::critical(this, "Yükleme Hatası", "Mesh dosyası yüklenemedi: " + qFilePath);
        }
    }
}

void N3VMeshEditor::onLoadMeshButtonClicked()
{
    // on_actionOpenMesh_triggered ile aynı işlevselliği paylaşabilirler.
    // Şimdilik, aynı kodu çağırabiliriz veya bu fonksiyonu action slot'a bağlayabiliriz.
    // Tekrarlamamak adına, bu butonu doğrudan on_actionOpenMesh_triggered'a bağlayalım.
    // Ancak ben sizin mevcut kodunuzu koruyarak düzenleme yapacağım, bu yüzden aynı kod burada tekrar yer alacak.

    QString initialPath = QCoreApplication::applicationDirPath();
    QString qfilePath = QFileDialog::getOpenFileName(this,
        tr("Mesh Dosyasını Aç"),
        initialPath,
        tr("N3VMesh Dosyaları (*.n3vmesh);;Tüm Dosyalar (*)"));

    if (!qfilePath.isEmpty()) {
        qDebug() << "Seçilen dosya yolu (QString): " << qfilePath;

        if (m_d3dWidget->LoadMesh(qfilePath)) // QDirect3D11Widget'ın LoadMesh'i artık QString alıyor.
        {
            qDebug() << "Mesh yükleme talebi başarıyla gönderildi: " << qfilePath;

            // Mesh yüklendiğinde kamerayı mesh'in boyutlarına göre ayarla
            if (m_d3dWidget && m_d3dWidget->GetRenderer()) { // Null kontrolü ekleyelim
                float meshCenterY = m_d3dWidget->GetRenderer()->GetCameraTargetY(); // <<--- DÜZELTİLDİ
                m_cameraTargetYSlider->setValue(static_cast<int>(meshCenterY * 10.0f)); // Slider değerini güncelle
            }
            m_d3dWidget->update(); // Yeni mesh yüklendiğinde render'ı tetikle
        }
        else
        {
            qDebug() << "Hata: Mesh yükleme talebi başarısız oldu: " << qfilePath;
            QMessageBox::critical(this, "Yükleme Hatası", "Mesh dosyası yüklenemedi: " + qfilePath);
        }
    }
}

void N3VMeshEditor::onTranslateXButtonClicked()
{
    QPushButton* senderButton = qobject_cast<QPushButton*>(sender());
    if (senderButton) {
        if (senderButton->text() == "Küpü Sağa") {
            if (m_d3dWidget && m_d3dWidget->GetRenderer()) // Null kontrolü
                m_d3dWidget->GetRenderer()->SetWorldTranslation(0.5f, 0.0f, 0.0f); // X ekseninde +0.5
        }
        else if (senderButton->text() == "Küpü Sola") {
            if (m_d3dWidget && m_d3dWidget->GetRenderer()) // Null kontrolü
                m_d3dWidget->GetRenderer()->SetWorldTranslation(-0.5f, 0.0f, 0.0f); // X ekseninde -0.5
        }
        m_d3dWidget->update(); // Dünya matrisi değiştiğinde render'ı tetikle
    }
}

void N3VMeshEditor::onTranslateYButtonClicked()
{
    QPushButton* senderButton = qobject_cast<QPushButton*>(sender());
    if (senderButton) {
        if (senderButton->text() == "Küpü Yükselt") {
            if (m_d3dWidget && m_d3dWidget->GetRenderer()) // Null kontrolü
                m_d3dWidget->GetRenderer()->SetWorldTranslation(0.0f, 0.5f, 0.0f); // Y ekseninde +0.5
        }
        else if (senderButton->text() == "Küpü Alçalt") {
            if (m_d3dWidget && m_d3dWidget->GetRenderer()) // Null kontrolü
                m_d3dWidget->GetRenderer()->SetWorldTranslation(0.0f, -0.5f, 0.0f); // Y ekseninde -0.5
        }
        m_d3dWidget->update(); // Dünya matrisi değiştiğinde render'ı tetikle
    }
}

void N3VMeshEditor::onSaveMeshButtonClicked()
{
    // Kaydedilecek dosya yolunu kullanıcıdan al
    QString filePath = QFileDialog::getSaveFileName(this,
        tr("Mesh Kaydet"),
        QCoreApplication::applicationDirPath(),
        tr("N3VMesh Dosyaları (*.n3vmesh);;Tüm Dosyalar (*)"));

    if (filePath.isEmpty())
    {
        return;
    }

    CN3VMesh* mesh = m_d3dWidget->GetRenderer()->GetCollisionMesh();
    if (!mesh)
    {
        QMessageBox::critical(this, "Hata", "Kaydedilecek mesh verisi bulunamadı.");
        return;
    }

    QFileInfo fileInfo(filePath);
    QDir dir = fileInfo.dir();

    if (!dir.mkpath(dir.absolutePath()))
    {
        QMessageBox::critical(this, "Hata", "Hedef dizin oluşturulamadı:\n" + dir.absolutePath() + "\nLütfen dosya yolunu kontrol edin veya yazma izinlerini sağlayın.");
        return;
    }

    // QString'i std::wstring'e çevir. Bu, Türkçe karakterler için en doğru yaklaşımdır.
    std::wstring stdWFilePath = filePath.toStdWString(); // <<-- BURASI DÜZELTİLDİ!

    // Mesh adını dosya adından çıkar. Bu kısım std::string olarak kalabilir.
    std::string meshName = QFileInfo(filePath).baseName().toStdString();

    // CN3VMesh::Save fonksiyonunu çağır
    if (mesh->Save(stdWFilePath, meshName)) // Save fonksiyonu artık std::wstring alıyor
    {
        QMessageBox::information(this, "Başarılı", "Mesh başarıyla kaydedildi:\n" + filePath);
    }
    else // Save fonksiyonu false dönerse hata
    {
        QMessageBox::critical(this, "Kaydetme Hatası",
            "Mesh kaydedilirken bir hata oluştu.\n"
            "Lütfen:\n"
            "- Kaydedilecek mesh verisinin yüklü ve geçerli olduğundan emin olun.\n"
            "- Seçtiğiniz dosya yolunun geçerli olduğundan ve yazma izinlerinizin olduğundan emin olun.\n"
            "- Dosyanın başka bir program tarafından kullanılmadığından emin olun.\n\n"
            "Ayrıntılar için Visual Studio Output penceresini kontrol edin.");
    }
}

void N3VMeshEditor::onRotateYawButtonClicked()
{
    // Bu metot şu an kullanılmıyor, ancak ekleme yaparsanız burada kullanabilirsiniz.
}