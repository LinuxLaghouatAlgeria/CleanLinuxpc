#ifndef CLEANER_H
#define CLEANER_H

#include <QObject>
#include <QMap>
#include <QStringList>
#include "statsmodel.h"

class Cleaner : public QObject
{
    Q_OBJECT

public:
    explicit Cleaner(QObject *parent = nullptr);
    
public slots:
    void cleanSelected(const QList<CleanCategory> &categories);
    void cleanAll(const QList<CleanCategory> &categories);

signals:
    void progressUpdated(int percent, const QString &message);
    void cleaningFinished(const QMap<QString, qint64> &freedSpace);
    void errorOccurred(const QString &error);

private:
    bool cleanAptCache(const QStringList &files);
    bool cleanSystemLogs(const QStringList &files);
    bool cleanTempFiles(const QStringList &files);
    bool cleanPackageCache(const QStringList &files);
    bool cleanThumbnails(const QStringList &files);
    bool removeOrphanPackages();
    bool cleanFlatpakCache(const QStringList &files);
    bool cleanSnapCache(const QStringList &files);
    bool cleanBrowserData(const QStringList &files);
    
    bool executeCommand(const QString &command, const QStringList &args);
    bool removeFiles(const QStringList &files);
    bool truncateLogFiles(const QStringList &files);
    qint64 getFreedSpace(const QString &path);
    
    void logOperation(const QString &operation, const QStringList &files);
    
    QString generatePrivilegedScript(const QList<CleanCategory> &categories);
    bool runPrivilegedScript(const QString &script);
};

#endif // CLEANER_H
