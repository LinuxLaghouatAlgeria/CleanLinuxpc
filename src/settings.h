#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QString>
#include <QStringList>

class Settings : public QObject
{
    Q_OBJECT

public:
    explicit Settings(QObject *parent = nullptr);
    
    // Getters
    bool getAutoClean() const;
    int getKeepLogsDays() const;
    QString getLanguage() const;
    QString getTheme() const;
    QStringList getExcludedPaths() const;
    
    // Setters
    void setAutoClean(bool value);
    void setKeepLogsDays(int days);
    void setLanguage(const QString &lang);
    void setTheme(const QString &newTheme);
    void setExcludedPaths(const QStringList &paths);
    
    // Path management
    void addExcludedPath(const QString &path);
    void removeExcludedPath(const QString &path);

signals:
    void autoCleanChanged(bool value);
    void keepLogsDaysChanged(int days);
    void languageChanged(const QString &lang);
    void themeChanged(const QString &theme);
    void excludedPathsChanged(const QStringList &paths);

private:
    void loadSettings();
    void saveSettings();
    
    bool autoClean;
    int keepLogsDays;
    QString language;
    QString theme;
    QStringList excludedPaths;
};

#endif // SETTINGS_H
