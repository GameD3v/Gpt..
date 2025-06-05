#include <QApplication>
#include "N3VMeshEditor.h" // Ana pencere sınıfınızı dahil edin

int main(int argc, char* argv[])
{
    QApplication a(argc, argv); // Qt uygulaması oluştur

    N3VMeshEditor w; // N3VMeshEditor pencerenizin bir örneğini oluştur
    w.show();         // Pencereyi göster

    return a.exec(); // Uygulama olay döngüsünü başlat
}