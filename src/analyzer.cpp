#include "analyzer.h"
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QDebug>
#include <sys/statvfs.h>

Analyzer::Analyzer(QObject *parent) : QObject(parent) {}

void Analyzer::startAnalysis()
{
    emit progressUpdated(0, tr("بدء تحليل النظام..."));
    
    QList<CleanCategory> categories;
    
    categories.append(analyzeAptCache());
    emit progressUpdated(20, tr("جاري فحص ذاكرة APT..."));
    
    categories.append(analyzeSystemLogs());
    emit progressUpdated(40, tr("جاري فحص سجلات النظام..."));
    
    categories.append(analyzeTempFiles());
    emit progressUpdated(60, tr("جاري فحص الملفات المؤقتة..."));
    
    categories.append(analyzeThumbnails());
    emit progressUpdated(80, tr("جاري فحص مصغرات الصور..."));
    
    categories.append(analyzeOrphanPackages());
    emit progressUpdated(85, tr("جاري فحص الحزم اليتيمة..."));

    categories.append(analyzeBrowserCache());
    emit progressUpdated(90, tr("جاري فحص ذاكرة المتصفحات..."));

    categories.append(analyzeBrowserData());
    emit progressUpdated(90, tr("جاري فحص بيانات المتصفحات..."));

    categories.append(analyzeFlatpakCache());
    emit progressUpdated(95, tr("جاري فحص مخلفات التطبيقات..."));

    categories.append(analyzeSnapCache());
    emit progressUpdated(98, tr("جاري فحص مخلفات Snap..."));
    
    emit progressUpdated(100, tr("اكتمل التحليل"));
    emit analysisFinished(categories);
}

CleanCategory Analyzer::analyzeAptCache()
{
    CleanCategory cat;
    cat.name = tr("ذاكرة APT");
    cat.description = tr("حزم DEB المخزنة بعد التثبيت");
    cat.icon = "📦";
    cat.safeToDelete = true;
    
    QStringList cachePaths = {
        "/var/cache/apt/archives/",
        "~/.cache/apt/archives/"
    };
    
    for (const QString &path : cachePaths) {
        QString expandedPath = QDir::cleanPath(path.startsWith("~") ? 
            QDir::homePath() + path.mid(1) : path);
        
        if (QDir(expandedPath).exists()) {
            cat.size += calculateDirectorySize(expandedPath);
            cat.files.append(findFilesByPattern(expandedPath + "/*.deb"));
        }
    }
    
    return cat;
}

CleanCategory Analyzer::analyzeSystemLogs()
{
    CleanCategory cat;
    cat.name = tr("سجلات النظام");
    cat.description = tr("ملفات سجل النظام القديمة");
    cat.icon = "📝";
    cat.safeToDelete = true;
    
    QStringList logPaths = {
        "/var/log/",
        "/var/log/journal/"
    };
    
    QStringList logPatterns = {"*.log", "*.log.*", "*.gz", "*.bz2"};
    
    for (const QString &path : logPaths) {
        if (QDir(path).exists()) {
            QDirIterator it(path, logPatterns, QDir::Files, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                QString filePath = it.next();
                QFileInfo info(filePath);
                
                // حذف السجلات الأقدم من 7 أيام فقط
                if (info.lastModified().daysTo(QDateTime::currentDateTime()) > 7) {
                    cat.size += info.size();
                    cat.files.append(filePath);
                }
            }
        }
    }
    
    return cat;
}

CleanCategory Analyzer::analyzeTempFiles()
{
    CleanCategory cat;
    cat.name = tr("ملفات مؤقتة");
    cat.description = tr("ملفات مؤقتة من التطبيقات والجلسات");
    cat.icon = "🗑️";
    cat.safeToDelete = true;
    
    QStringList tempPaths = {
        "/tmp/",
        "/var/tmp/",
        QStandardPaths::writableLocation(QStandardPaths::TempLocation)
    };
    
    for (const QString &path : tempPaths) {
        if (QDir(path).exists()) {
            QDirIterator it(path, QStringList() << "*", QDir::Files, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                QString filePath = it.next();
                QFileInfo info(filePath);
                
                // حذف الملفات الأقدم من يوم واحد فقط
                if (info.lastModified().daysTo(QDateTime::currentDateTime()) > 1) {
                    cat.size += info.size();
                    cat.files.append(filePath);
                }
            }
        }
    }
    
    return cat;
}

CleanCategory Analyzer::analyzeThumbnails()
{
    CleanCategory cat;
    cat.name = tr("مصغرات الصور");
    cat.description = tr("صور مصغرة مخزنة محلياً");
    cat.icon = "🖼️";
    cat.safeToDelete = true;
    
    QString thumbnailPath = QDir::homePath() + "/.cache/thumbnails/";
    
    if (QDir(thumbnailPath).exists()) {
        cat.size = calculateDirectorySize(thumbnailPath);
        
        QDirIterator it(thumbnailPath, QStringList() << "*.png" << "*.jpg" << "*.jpeg", 
                       QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            cat.files.append(it.next());
        }
    }
    
    return cat;
}

CleanCategory Analyzer::analyzeOrphanPackages()
{
    CleanCategory cat;
    cat.name = tr("حزم يتيمة");
    cat.description = tr("حزم لم تعد بحاجة إليها");
    cat.icon = "📦";
    cat.safeToDelete = true;
    
    QProcess process;
    
    // التحقق من مدير الحزم المستخدم
    if (QFile::exists("/usr/bin/apt")) {
        process.start("apt", QStringList() << "autoremove" << "--dry-run");
        process.waitForFinished();
        QString output = process.readAllStandardOutput();
        
        // تحليل المخرجات لحساب الحجم التقريبي
        // هذه عملية مبسطة - تحتاج لتحسين
        QStringList lines = output.split("\n", Qt::SkipEmptyParts);
        for (const QString &line : lines) {
            if (line.contains("kB")) {
                // استخراج الحجم من السطر
                // تحسين هذا المنطق حسب الحاجة
            }
        }
    } else if (QFile::exists("/usr/bin/pacman")) {
        process.start("pacman", QStringList() << "-Qdtq");
        process.waitForFinished();
        if (process.exitCode() == 0) {
            QString output = QString::fromUtf8(process.readAllStandardOutput());
            QStringList packages = output.split("\n", Qt::SkipEmptyParts);
            cat.files = packages;
        }
    }
    
    return cat;
}

qint64 Analyzer::calculateDirectorySize(const QString &path)
{
    qint64 totalSize = 0;
    QDir dir(path);
    
    QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot);
    for (const QFileInfo &file : files) {
        totalSize += file.size();
    }
    
    QFileInfoList subdirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &subdir : subdirs) {
        totalSize += calculateDirectorySize(subdir.absoluteFilePath());
    }
    
    return totalSize;
}

QStringList Analyzer::findFilesByPattern(const QString &pattern)
{
    QStringList files;
    QDirIterator it(pattern, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        files.append(it.next());
    }
    return files;
}

bool Analyzer::isSafeToDelete(const QString &path)
{
    // قائمة بالمسارات المحظورة
    QStringList forbiddenPaths = {
        "/bin", "/sbin", "/usr/bin", "/usr/sbin", "/lib", "/usr/lib",
        "/etc", "/boot", "/root", "/dev", "/proc", "/sys"
    };
    
    for (const QString &forbidden : forbiddenPaths) {
        if (path.startsWith(forbidden)) {
            return false;
        }
    }
    
    return true;
}

CleanCategory Analyzer::analyzeBrowserCache()
{
    CleanCategory cat;
    cat.name = tr("ذاكرة المتصفحات (Cache)");
    cat.description = tr("الملفات المؤقتة للمتصفحات (Firefox, Chrome)");
    cat.icon = "🌐";
    cat.safeToDelete = true;
    cat.checked = true;
    
    // Firefox Cache
    QString firefoxCache = QDir::homePath() + "/.cache/mozilla/firefox/";
    if (QDir(firefoxCache).exists()) {
        QDirIterator it(firefoxCache, QStringList() << "cache2", QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QString cachePath = it.next();
            cat.files.append(cachePath);
            cat.size += calculateDirectorySize(cachePath);
        }
    }
    
    // Chrome Cache
    QStringList chromePaths = {
        QDir::homePath() + "/.cache/google-chrome/Default/Cache",
        QDir::homePath() + "/.cache/chromium/Default/Cache"
    };
    
    for (const QString &path : chromePaths) {
        if (QDir(path).exists()) {
            cat.files.append(path);
            cat.size += calculateDirectorySize(path);
        }
    }
    
    return cat;
}

CleanCategory Analyzer::analyzeBrowserData()
{
    CleanCategory cat;
    cat.name = tr("بيانات المتصفحات (تنبيه!)");
    cat.description = tr("ملفات الجلسات وتسجيل الدخول - سيؤدي لتسجيل الخروج!");
    cat.icon = "🍪";
    cat.safeToDelete = false;
    cat.checked = false; // غير محدد افتراضياً للأمان
    
    // Firefox Cookies & Places
    QString firefoxData = QDir::homePath() + "/.mozilla/firefox/";
    if (QDir(firefoxData).exists()) {
        QDirIterator it(firefoxData, QStringList() << "cookies.sqlite" << "places.sqlite", 
                       QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QString file = it.next();
            cat.files.append(file);
            cat.size += QFileInfo(file).size();
        }
    }
    
    // Chrome Cookies & History
    QStringList chromeBasePaths = {
        QDir::homePath() + "/.config/google-chrome/Default/",
        QDir::homePath() + "/.config/chromium/Default/"
    };
    
    for (const QString &basePath : chromeBasePaths) {
        QStringList dataFiles = {"Cookies", "History", "Login Data", "Web Data"};
        for (const QString &fileName : dataFiles) {
            QString filePath = basePath + fileName;
            if (QFile::exists(filePath)) {
                cat.files.append(filePath);
                cat.size += QFileInfo(filePath).size();
            }
        }
    }
    
    return cat;
}

CleanCategory Analyzer::analyzeFlatpakCache()
{
    CleanCategory cat;
    cat.name = tr("مخلفات Flatpak");
    cat.description = tr("حزم غير مستخدمة وملفات مؤقتة");
    cat.icon = "📦";
    cat.safeToDelete = true;
    cat.checked = true;
    
    // التحقق من وجود flatpak
    if (QStandardPaths::findExecutable("flatpak").isEmpty()) {
        return cat;
    }
    
    // 1. الحزم غير المستخدمة
    // في الواقع، لا يمكننا بسهولة معرفة الحجم المحدد للحزم غير المستخدمة
    // بدون تشغيل الأمر، لذا سنعتمد على الملفات المؤقتة بشكل أساسي
    
    // 2. ملفات مؤقتة في /var/lib/flatpak/repo/tmp
    QString sysTmp = "/var/lib/flatpak/repo/tmp/";
    if (QDir(sysTmp).exists()) {
        cat.size += calculateDirectorySize(sysTmp);
        cat.files.append(sysTmp);
    }
    
    // 3. ملفات مؤقتة في home
    QString userTmp = QDir::homePath() + "/.local/share/flatpak/repo/tmp/";
    if (QDir(userTmp).exists()) {
        cat.size += calculateDirectorySize(userTmp);
        cat.files.append(userTmp);
    }
    
    // إضافة ملاحظة وهمية لتشغيل الأمر لاحقاً
    if (cat.size == 0) {
        // إذا لم نجد ملفات مؤقتة، سنضيف عنصراً وهمياً لتشغيل أمر التنظيف
        cat.files.append("FLATPAK_UNUSED_PACKAGES_MARKER");
    }
    
    return cat;
}

CleanCategory Analyzer::analyzeSnapCache()
{
    CleanCategory cat;
    cat.name = tr("مخلفات Snap");
    cat.description = tr("نسخ قديمة من التطبيقات وملفات مؤقتة");
    cat.icon = "🛍️";
    cat.safeToDelete = true;
    cat.checked = true;
    
    // التحقق من وجود snap
    if (QStandardPaths::findExecutable("snap").isEmpty()) {
        return cat;
    }
    
    // البحث عن مجلدات cache الشائعة
    QString cachePath = "/var/lib/snapd/cache/";
    if (QDir(cachePath).exists()) {
        cat.size += calculateDirectorySize(cachePath);
        QDirIterator it(cachePath, QDir::Files | QDir::NoDotAndDotDot);
        while (it.hasNext()) {
            cat.files.append(it.next());
        }
    }
    
    // إضافة ماركر لتشغيل أمر تقليل النسخ
    cat.files.append("SNAP_RETENTION_MARKER");
    
    return cat;
}
