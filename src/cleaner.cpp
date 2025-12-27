#include "cleaner.h"
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QThread>

Cleaner::Cleaner(QObject *parent) : QObject(parent) {}

void Cleaner::cleanAll(const QList<CleanCategory> &categories) {
  cleanSelected(categories);
}

void Cleaner::cleanSelected(const QList<CleanCategory> &categories) {
  emit progressUpdated(0, tr("جاري طلب الصلاحيات..."));

  // 1. تجميع وتنفيذ الأوامر التي تتطلب صلاحيات root مرة واحدة
  QString script = generatePrivilegedScript(categories);
  if (!script.isEmpty()) {
    if (!runPrivilegedScript(script)) {
      emit errorOccurred(tr("فشل في تنفيذ أوامر المسؤول"));
      // نكمل العملية للملفات التي لا تتطلب صلاحيات
    }
  }

  emit progressUpdated(0, tr("بدء عملية التنظيف..."));

  QMap<QString, qint64> freedSpace;
  int totalCategories = categories.size();
  int currentCategory = 0;

  for (const CleanCategory &category : categories) {
    currentCategory++;
    int progress = (currentCategory * 100) / totalCategories;
    emit progressUpdated(progress, tr("جاري تنظيف: %1").arg(category.name));

    qint64 spaceBefore = 0;
    qint64 spaceAfter = 0;

    // حساب المساحة قبل الحذف
    for (const QString &file : category.files) {
      QFileInfo info(file);
      if (info.exists()) {
        spaceBefore += info.size();
      }
    }

    // تنفيذ الحذف حسب النوع
    if (category.name.contains("APT")) {
      cleanAptCache(category.files);
    } else if (category.name.contains("سجل")) {
      cleanSystemLogs(category.files);
    } else if (category.name.contains("مؤقت")) {
      cleanTempFiles(category.files);
    } else if (category.name.contains("مصغر")) {
      cleanThumbnails(category.files);
    } else if (category.name.contains("حزم يتيمة")) {
      removeOrphanPackages();
    } else if (category.name.contains("المتصفحات")) {
      cleanBrowserData(category.files);
    } else if (category.name.contains("Flatpak")) {
      cleanFlatpakCache(category.files);
    } else if (category.name.contains("Snap")) {
      cleanSnapCache(category.files);
    }

    // حساب المساحة بعد الحذف
    for (const QString &file : category.files) {
      QFileInfo info(file);
      if (!info.exists()) {
        spaceAfter += info.size();
      }
    }

    qint64 freed = spaceBefore - spaceAfter;
    if (freed > 0) {
      freedSpace[category.name] = freed;
      logOperation(category.name, category.files);
    }

    QThread::msleep(500); // تأخير بسيط للمستخدم
  }

  emit progressUpdated(100, tr("اكتمل التنظيف"));
  emit cleaningFinished(freedSpace);
}

bool Cleaner::cleanAptCache(const QStringList &files) {
  bool success = true;

  // حذف الملفات يدوياً (للملفات التي يملكها المستخدم أو إذا تم تنظيفها بالفعل)
  for (const QString &file : files) {
    if (QFile::exists(file)) {
      if (!QFile::remove(file)) {
        // قد يفشل الحذف إذا كانت الملفات مملوكة للنظام ولم يتم حذفها بالسكربت
        // لا نعتبره خطأ حرجاً هنا لأن السكربت المفروض قام بالعمل
        qDebug() << "Failed to remove:" << file;
        success = false;
      }
    }
  }

  return success;
}

bool Cleaner::cleanSystemLogs(const QStringList &files) {
  // التخفيف (Vacuum) والقص (Truncate) تم نقلهم للسكربت الموحد
  // هنا نقوم فقط بمحاولة تنظيف ما يمكن للمستخدم العادي الوصول إليه (نادراً)
  return true;
}

bool Cleaner::cleanTempFiles(const QStringList &files) {
  bool success = true;

  for (const QString &file : files) {
    if (QFile::exists(file)) {
      QFileInfo info(file);
      if (info.isDir()) {
        QDir dir(file);
        if (!dir.removeRecursively()) {
          qWarning() << "Failed to remove directory:" << file;
          success = false;
        }
      } else {
        if (!QFile::remove(file)) {
          qWarning() << "Failed to remove file:" << file;
          success = false;
        }
      }
    }
  }

  return success;
}

bool Cleaner::cleanThumbnails(const QStringList &files) {
  bool success = true;

  QString thumbnailPath = QDir::homePath() + "/.cache/thumbnails/";

  if (QDir(thumbnailPath).exists()) {
    QDir dir(thumbnailPath);
    QStringList subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    for (const QString &subdir : subdirs) {
      QString fullPath = thumbnailPath + subdir;
      QDir subDir(fullPath);
      subDir.removeRecursively();
    }

    // تنظيف مجلدات مصغرات GNOME
    QString gnomeThumbnailPath = QDir::homePath() + "/.thumbnails/";
    if (QDir(gnomeThumbnailPath).exists()) {
      QDir(gnomeThumbnailPath).removeRecursively();
    }
  }

  return success;
}

bool Cleaner::removeOrphanPackages() {
  // تم النقل للسكربت الموحد
  return true;
}

bool Cleaner::executeCommand(const QString &command, const QStringList &args) {
  QProcess process;
  process.start(command, args);
  process.waitForFinished();

  if (process.exitCode() != 0) {
    qWarning() << "Command failed:" << command << args;
    qWarning() << "Error:" << process.readAllStandardError();
    return false;
  }

  return true;
}

bool Cleaner::removeFiles(const QStringList &files) {
  bool success = true;

  for (const QString &file : files) {
    if (QFile::exists(file)) {
      if (!QFile::remove(file)) {
        qWarning() << "Failed to remove:" << file;
        success = false;
      }
    }
  }

  return success;
}

bool Cleaner::truncateLogFiles(const QStringList &files) {
  bool success = true;

  for (const QString &file : files) {
    QFile logFile(file);
    if (logFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
      logFile.close();
    } else {
      qWarning() << "Failed to truncate:" << file;
      success = false;
    }
  }

  return success;
}

void Cleaner::logOperation(const QString &operation, const QStringList &files) {
  QString logPath = QDir::homePath() + "/.cleanlinuxpc.log";
  QFile logFile(logPath);

  if (logFile.open(QIODevice::Append | QIODevice::Text)) {
    QTextStream out(&logFile);
    out << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << " - "
        << operation << " - " << files.size() << " ملفات\n";

    for (const QString &file : files) {
      out << "  " << file << "\n";
    }

    logFile.close();
  }
}

bool Cleaner::cleanBrowserData(const QStringList &files) {
  bool success = true;
  for (const QString &path : files) {
    QFileInfo info(path);
    if (info.isDir()) {
      QDir dir(path);
      if (!dir.removeRecursively())
        success = false;
    } else {
      if (!QFile::remove(path))
        success = false;
    }
  }
  return success;
}

bool Cleaner::cleanFlatpakCache(const QStringList &files) {
  // 1. حذف الملفات المؤقتة التي تم جمعها (مستخدم عادي)
  if (!removeFiles(files))
    return false;

  // 2. أمر إلغاء تثبيت الحزم غير المستخدمة تم نقله للسكربت الموحد
  return true;
}

bool Cleaner::cleanSnapCache(const QStringList &files) {
  bool success = true;

  // 1. حذف الملفات العادية
  QStringList actualFiles;
  for (const QString &f : files) {
    if (!f.contains("_MARKER"))
      actualFiles.append(f);
  }
  if (!removeFiles(actualFiles))
    success = false;

  // 2. إعدادات refresh.retain تم نقلها للسكربت الموحد
  return success;
}

QString
Cleaner::generatePrivilegedScript(const QList<CleanCategory> &categories) {
  QStringList commands;
  commands << "#!/bin/sh";

  // لتجنب الأخطاء عند توقف أمر ما
  commands << "set -e";

  for (const CleanCategory &category : categories) {
    if (category.name.contains("APT")) {
      commands << "apt-get clean || true";
    } else if (category.name.contains("سجل")) {
      commands << "journalctl --vacuum-time=7d || true";
      for (const QString &file : category.files) {
        if (QFileInfo(file).absolutePath().startsWith("/var/log")) {
          commands << QString("truncate -s 0 \"%1\" || true").arg(file);
        }
      }
    } else if (category.name.contains("حزم يتيمة")) {
      if (QFile::exists("/usr/bin/apt"))
        commands << "apt autoremove -y || true";
      else if (QFile::exists("/usr/bin/pacman"))
        commands << "pacman -Rns $(pacman -Qdtq) --noconfirm || true";
    } else if (category.name.contains("Flatpak")) {
      commands << "flatpak uninstall --unused --assumeyes || true";
    } else if (category.name.contains("Snap")) {
      commands << "snap set system refresh.retain=2 || true";
    }
  }

  if (commands.size() <= 2)
    return ""; // Only header and set -e
  return commands.join("\n");
}

bool Cleaner::runPrivilegedScript(const QString &script) {
  QString tmpPath = QDir::tempPath() + "/cleaner_script.sh";
  QFile tmpFile(tmpPath);
  if (!tmpFile.open(QIODevice::WriteOnly))
    return false;
  tmpFile.write(script.toUtf8());
  tmpFile.setPermissions(
      QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner | QFile::ReadUser |
      QFile::WriteUser | QFile::ExeUser | QFile::ReadGroup | QFile::ExeGroup |
      QFile::ReadOther | QFile::ExeOther); // Permissions so root can read/exec
  tmpFile.close();

  QProcess process;

  // التحقق مما إذا كنا داخل Flatpak
  bool isFlatpak = QFileInfo("/.flatpak-info").exists();
  if (isFlatpak) {
    process.start("flatpak-spawn", QStringList() << "--host" << "pkexec"
                                                 << "/bin/sh" << tmpPath);
  } else {
    process.start("pkexec", QStringList() << "/bin/sh" << tmpPath);
  }

  process.waitForFinished(-1);

  // قد لا نستطيع حذفه إذا أنشأه المستخدم ثم نفذه root؟
  // الملف مملوك للمستخدم الحالي، لذا يمكن حذفه
  QFile::remove(tmpPath);

  return (process.exitCode() == 0);
}
