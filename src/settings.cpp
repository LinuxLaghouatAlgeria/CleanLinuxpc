#include "settings.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

Settings::Settings(QObject *parent) : QObject(parent)
{
    loadSettings();
}

void Settings::loadSettings()
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) 
                        + "/cleanlinuxpc/config.json";
    
    QFile configFile(configPath);
    if (!configFile.exists()) {
        // قيم افتراضية
        autoClean = false;
        keepLogsDays = 7;
        language = "ar";
        theme = "light";
        excludedPaths = QStringList();
        saveSettings();
        return;
    }
    
    if (configFile.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(configFile.readAll());
        QJsonObject obj = doc.object();
        
        autoClean = obj.value("autoClean").toBool(false);
        keepLogsDays = obj.value("keepLogsDays").toInt(7);
        language = obj.value("language").toString("ar");
        theme = obj.value("theme").toString("light");
        
        QJsonArray excludedArray = obj.value("excludedPaths").toArray();
        for (const QJsonValue &val : excludedArray) {
            excludedPaths.append(val.toString());
        }
        
        configFile.close();
    }
}

void Settings::saveSettings()
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) 
                       + "/cleanlinuxpc";
    QDir().mkpath(configDir);
    
    QString configPath = configDir + "/config.json";
    
    QJsonObject obj;
    obj["autoClean"] = autoClean;
    obj["keepLogsDays"] = keepLogsDays;
    obj["language"] = language;
    obj["theme"] = theme;
    
    QJsonArray excludedArray;
    for (const QString &path : excludedPaths) {
        excludedArray.append(path);
    }
    obj["excludedPaths"] = excludedArray;
    
    QJsonDocument doc(obj);
    QFile configFile(configPath);
    
    if (configFile.open(QIODevice::WriteOnly)) {
        configFile.write(doc.toJson());
        configFile.close();
    }
}

bool Settings::getAutoClean() const
{
    return autoClean;
}

void Settings::setAutoClean(bool value)
{
    if (autoClean != value) {
        autoClean = value;
        saveSettings();
        emit autoCleanChanged(value);
    }
}

int Settings::getKeepLogsDays() const
{
    return keepLogsDays;
}

void Settings::setKeepLogsDays(int days)
{
    if (keepLogsDays != days) {
        keepLogsDays = days;
        saveSettings();
        emit keepLogsDaysChanged(days);
    }
}

QString Settings::getLanguage() const
{
    return language;
}

void Settings::setLanguage(const QString &lang)
{
    if (language != lang) {
        language = lang;
        saveSettings();
        emit languageChanged(lang);
    }
}

QString Settings::getTheme() const
{
    return theme;
}

void Settings::setTheme(const QString &newTheme)
{
    if (theme != newTheme) {
        theme = newTheme;
        saveSettings();
        emit themeChanged(newTheme);
    }
}

QStringList Settings::getExcludedPaths() const
{
    return excludedPaths;
}

void Settings::setExcludedPaths(const QStringList &paths)
{
    excludedPaths = paths;
    saveSettings();
    emit excludedPathsChanged(paths);
}

void Settings::addExcludedPath(const QString &path)
{
    if (!excludedPaths.contains(path)) {
        excludedPaths.append(path);
        saveSettings();
        emit excludedPathsChanged(excludedPaths);
    }
}

void Settings::removeExcludedPath(const QString &path)
{
    if (excludedPaths.removeAll(path) > 0) {
        saveSettings();
        emit excludedPathsChanged(excludedPaths);
    }
}
