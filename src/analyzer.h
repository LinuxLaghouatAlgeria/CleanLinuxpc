#ifndef ANALYZER_H
#define ANALYZER_H

#include <QObject>
#include <QMap>
#include <QStringList>
#include <QThread>
#include "statsmodel.h"

class Analyzer : public QObject
{
    Q_OBJECT

public:
    explicit Analyzer(QObject *parent = nullptr);
    
public slots:
    void startAnalysis();

signals:
    void progressUpdated(int percent, const QString &message);
    void analysisFinished(const QList<CleanCategory> &categories);
    void errorOccurred(const QString &error);

private:
    CleanCategory analyzeAptCache();
    CleanCategory analyzeSystemLogs();
    CleanCategory analyzeTempFiles();
    CleanCategory analyzePackageCache();
    CleanCategory analyzeThumbnails();
    CleanCategory analyzeOrphanPackages();
    CleanCategory analyzeBrowserCache();
    CleanCategory analyzeBrowserData();
    CleanCategory analyzeFlatpakCache();
    CleanCategory analyzeSnapCache();
    
    qint64 calculateDirectorySize(const QString &path);
    QStringList findFilesByPattern(const QString &pattern);
    bool isSafeToDelete(const QString &path);
};

#endif // ANALYZER_H
