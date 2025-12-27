#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "analyzer.h"
#include "cleaner.h"
#include "statsmodel.h"
#include <QBarSeries>
#include <QBarSet>
#include <QChartView>
#include <QMainWindow>
#include <QProcess>
#include <QTranslator>
#include <QValueAxis>
#include <QtCharts>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private slots:
  void on_analyzeButton_clicked();
  void on_cleanButton_clicked();
  void on_selectAllCheckbox_toggled(bool checked);
  void on_categoryCheckbox_toggled();

  void updateProgress(int value, const QString &message);
  void analysisComplete(const QList<CleanCategory> &categories);
  void cleaningComplete(const QMap<QString, qint64> &freedSpace);

  void toggleDarkMode();
  void changeLanguage(int index);
  void showAbout();
  void showSettings();

protected:
  void changeEvent(QEvent *event) override;

private:
  Ui::MainWindow *ui;
  QTranslator *translator;
  Cleaner *cleaner;
  Analyzer *analyzer;
  QThread *workerThread;

  QList<CleanCategory> currentCategories;
  QChart *chart;
  QBarSeries *series;
  QProgressBar *diskUsageBar;

  void setupUI();
  void setupChart();
  void updateChart(const QMap<QString, qint64> &freedSpace);
  void updateSelectedSize();
  void updateDiskUsage();
  void loadSettings();
  void saveSettings();
  void retranslateUiCustom();
  QString formatSize(qint64 bytes);
};

#endif // MAINWINDOW_H
