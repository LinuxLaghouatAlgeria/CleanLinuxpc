#include "mainwindow.h"
#include <QApplication>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QMessageBox>
#include <QProcess>
#include <QSettings>
#include <QStyleFactory>
#include <QTranslator>
#include <fstream>
#include <unistd.h>

int main(int argc, char *argv[]) {
  // التحقق من صلاحيات المستخدم وإعادة التشغيل بصلاحيات root
  // تخطي هذا الفحص إذا كنا داخل Flatpak (سنستخدم flatpak-spawn لاحقاً)
  bool isFlatpak = QFileInfo("/.flatpak-info").exists();

  /*
  if (!isFlatpak && getuid() != 0) {
    // ... logic disabled for debugging ...
    return QProcess::execute("pkexec", args);
  }
  */

  // Logging setup
  std::ofstream f("/home/ismy/Desktop/CleanLinuxpc/raw_boot_log.txt",
                  std::ios::app);
  if (f.is_open()) {
    f << "Main started\n";
    f.close();
  }

  QApplication app(argc, argv);

  // إعداد معلومات التطبيق
  app.setApplicationName("CleanLinuxpc");
  app.setApplicationDisplayName("Clean Linux");
  app.setApplicationVersion("1.0.0");
  app.setOrganizationName("CleanLinux");

  // تحميل الإعدادات
  QSettings::setDefaultFormat(QSettings::IniFormat);
  QSettings settings;

  // تطبيق النمط

  // تطبيق النمط
  QString style = settings.value("Style", "Fusion").toString();
  app.setStyle(QStyleFactory::create(style));

  MainWindow window;
  window.show();

  return app.exec();
}
