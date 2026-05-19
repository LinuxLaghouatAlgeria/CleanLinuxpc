#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QBarSeries>
#include <QBarSet>
#include <QChart>
#include <QDebug>
#include <QEvent>
#include <QFileDialog>
#include <QLabel>
#include <QMessageBox>
#include <QSettings>
#include <QStandardItemModel>
#include <QStorageInfo>
#include <QStyleFactory>
#include <QTranslator>
#include <QValueAxis>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow),
      translator(new QTranslator(this)), cleaner(new Cleaner(this)),
      analyzer(new Analyzer(this)), workerThread(new QThread(this)),
      currentCategories(), chart(nullptr), series(nullptr),
      diskUsageBar(nullptr) {
  ui->setupUi(this);

  // إصلاح مشكلة القوائم في بعض بيئات سطح المكتب
  menuBar()->setNativeMenuBar(false);

  setupUI();
  setupChart();
  retranslateUiCustom(); // Translation manual after all components are ready
  loadSettings();

  // نقل الكائنات إلى thread منفصل
  analyzer->moveToThread(workerThread);
  cleaner->moveToThread(workerThread);
  workerThread->start();

  // توصيل الإشارات
  connect(analyzer, &Analyzer::progressUpdated, this,
          &MainWindow::updateProgress);
  connect(analyzer, &Analyzer::analysisFinished, this,
          &MainWindow::analysisComplete);
  connect(analyzer, &Analyzer::errorOccurred, this,
          [this](const QString &error) {
            QMessageBox::warning(this, tr("خطأ"), error);
          });

  connect(cleaner, &Cleaner::progressUpdated, this,
          &MainWindow::updateProgress);
  connect(cleaner, &Cleaner::cleaningFinished, this,
          &MainWindow::cleaningComplete);
  connect(cleaner, &Cleaner::errorOccurred, this, [this](const QString &error) {
    QMessageBox::warning(this, tr("خطأ"), error);
  });
}

MainWindow::~MainWindow() {
  workerThread->quit();
  workerThread->wait();
  saveSettings();
  delete ui;
}

void MainWindow::setupUI() {
  // إعداد الجدول
  QStandardItemModel *model = new QStandardItemModel(this);
  model->setHorizontalHeaderLabels(
      {tr("✔"), tr("النوع"), tr("الوصف"), tr("الحجم"), tr("الملفات")});
  ui->categoriesTable->setModel(model);
  ui->categoriesTable->setColumnWidth(0, 40);
  ui->categoriesTable->setColumnWidth(1, 150);
  ui->categoriesTable->setColumnWidth(2, 250);
  ui->categoriesTable->setColumnWidth(3, 100);
  ui->categoriesTable->setColumnWidth(4, 100);

  // توصيل الأزرار
  connect(ui->analyzeButton, &QToolButton::clicked, this,
          &MainWindow::on_analyzeButton_clicked);
  connect(ui->cleanButton, &QToolButton::clicked, this,
          &MainWindow::on_cleanButton_clicked);
  // Manual clean button removed
  connect(ui->darkModeButton, &QPushButton::clicked, this,
          &MainWindow::toggleDarkMode);
  connect(ui->languageComboBox,
          QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &MainWindow::changeLanguage);
  connect(ui->selectAllCheckbox, &QCheckBox::toggled, this,
          &MainWindow::on_selectAllCheckbox_toggled);

  // توصيل القوائم
  connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::showAbout);
  connect(ui->actionPreferences, &QAction::triggered, this,
          &MainWindow::showSettings);
  connect(ui->actionExit, &QAction::triggered, this, &MainWindow::close);

  // إعداد شريط حالة القرص
  diskUsageBar = new QProgressBar(this);
  diskUsageBar->setRange(0, 100);
  diskUsageBar->setTextVisible(true);
  diskUsageBar->setFormat("%p% مستخدم");
  diskUsageBar->setFixedWidth(200);

  // إضافة تسمية وشريط إلى شريط الحالة
  ui->statusBar->addPermanentWidget(new QLabel(tr("القرص: "), this));
  ui->statusBar->addPermanentWidget(diskUsageBar);

  updateDiskUsage();
}

void MainWindow::setupChart() {
  chart = new QChart();
  chart->setTitle(tr("المساحة المسترجعة"));
  chart->setAnimationOptions(QChart::SeriesAnimations);

  series = new QBarSeries();
  chart->addSeries(series);

  QValueAxis *axisY = new QValueAxis();
  axisY->setTitleText(tr("المساحة (ميغابايت)"));
  chart->addAxis(axisY, Qt::AlignLeft);
  series->attachAxis(axisY);

  ui->chartView->setChart(chart);
  ui->chartView->setRenderHint(QPainter::Antialiasing);
}

void MainWindow::on_analyzeButton_clicked() {
  ui->analyzeButton->setEnabled(false);
  ui->progressBar->setValue(0);
  ui->statusBar->showMessage(tr("جاري تحليل النظام..."));
  QMetaObject::invokeMethod(analyzer, "startAnalysis", Qt::QueuedConnection);
}

void MainWindow::on_cleanButton_clicked() {
  if (currentCategories.isEmpty()) {
    QMessageBox::information(this, tr("معلومة"),
                             tr("الرجاء تحليل النظام أولاً"));
    return;
  }

  QList<CleanCategory> selected;
  for (const auto &category : currentCategories) {
    if (category.checked) {
      selected.append(category);
    }
  }

  if (selected.isEmpty()) {
    QMessageBox::information(this, tr("معلومة"),
                             tr("الرجاء تحديد ملفات للتنظيف"));
    return;
  }

  QMessageBox::StandardButton reply =
      QMessageBox::question(this, tr("تأكيد"),
                            tr("هل تريد تنظيف الملفات المحددة؟\nهذا "
                               "الإجراء لا يمكن التراجع عنه."),
                            QMessageBox::Yes | QMessageBox::No);

  if (reply == QMessageBox::Yes) {
    ui->cleanButton->setEnabled(false);
    QMetaObject::invokeMethod(cleaner, "cleanSelected", Qt::QueuedConnection,
                              Q_ARG(QList<CleanCategory>, selected));
  }
}

void MainWindow::updateProgress(int value, const QString &message) {
  ui->progressBar->setValue(value);
  ui->statusBar->showMessage(message);
}

void MainWindow::analysisComplete(const QList<CleanCategory> &categories) {
  currentCategories = categories;

  QStandardItemModel *model =
      qobject_cast<QStandardItemModel *>(ui->categoriesTable->model());
  model->removeRows(0, model->rowCount());

  qint64 totalSize = 0;
  int fileCount = 0;

  for (int i = 0; i < categories.size(); ++i) {
    const CleanCategory &cat = categories[i];

    QStandardItem *checkItem = new QStandardItem();
    checkItem->setCheckable(true);
    checkItem->setCheckState(cat.checked ? Qt::Checked : Qt::Unchecked);

    QStandardItem *typeItem = new QStandardItem(cat.name);
    QStandardItem *descItem = new QStandardItem(cat.description);
    QStandardItem *sizeItem = new QStandardItem(formatSize(cat.size));
    QStandardItem *countItem =
        new QStandardItem(QString::number(cat.files.size()));

    model->appendRow({checkItem, typeItem, descItem, sizeItem, countItem});

    totalSize += cat.size;
    fileCount += cat.files.size();
  }

  ui->totalSizeLabel->setText(
      tr("الحجم الإجمالي: %1").arg(formatSize(totalSize)));
  ui->selectedSizeLabel->setText(
      tr("الحجم المحدد: %1").arg(formatSize(totalSize)));
  ui->filesCountLabel->setText(tr("عدد الملفات: %1").arg(fileCount));

  ui->analyzeButton->setEnabled(true);
  ui->statusBar->showMessage(tr("اكتمل التحليل"), 3000);
}

void MainWindow::cleaningComplete(const QMap<QString, qint64> &freedSpace) {
  updateChart(freedSpace);
  updateDiskUsage();

  qint64 totalFreed = 0;
  for (qint64 size : freedSpace) {
    totalFreed += size;
  }

  QMessageBox::information(this, tr("نجاح"),
                           tr("تم تنظيف النظام بنجاح!\nالمساحة المسترجعة: %1")
                               .arg(formatSize(totalFreed)));

  ui->cleanButton->setEnabled(true);
  // Manual clean button removed

  // تحديث القائمة بعد التنظيف
  on_analyzeButton_clicked();
}

void MainWindow::updateChart(const QMap<QString, qint64> &freedSpace) {
  series->clear();

  for (auto it = freedSpace.begin(); it != freedSpace.end(); ++it) {
    QBarSet *set = new QBarSet(it.key());
    *set << (it.value() / (1024.0 * 1024.0)); // تحويل إلى ميغابايت
    series->append(set);
  }

  chart->setTitle(tr("المساحة المسترجعة - الإجمالي: %1")
                      .arg(formatSize(std::accumulate(freedSpace.begin(),
                                                      freedSpace.end(), 0LL))));
}

void MainWindow::on_selectAllCheckbox_toggled(bool checked) {
  for (auto &category : currentCategories) {
    category.checked = checked;
  }

  QStandardItemModel *model =
      qobject_cast<QStandardItemModel *>(ui->categoriesTable->model());
  for (int i = 0; i < model->rowCount(); ++i) {
    model->item(i, 0)->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
  }

  updateSelectedSize();
}

void MainWindow::updateSelectedSize() {
  qint64 selectedSize = 0;
  for (const auto &category : currentCategories) {
    if (category.checked) {
      selectedSize += category.size;
    }
  }
  ui->selectedSizeLabel->setText(
      tr("الحجم المحدد: %1").arg(formatSize(selectedSize)));
}

void MainWindow::toggleDarkMode() {
  QSettings settings;
  bool darkMode = settings.value("DarkMode", false).toBool();
  settings.setValue("DarkMode", !darkMode);

  if (!darkMode) {
    qApp->setStyle(QStyleFactory::create("Fusion"));
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    qApp->setPalette(darkPalette);
  } else {
    qApp->setPalette(qApp->style()->standardPalette());
  }
}

void MainWindow::changeLanguage(int index) {
  QSettings settings;
  QString langCode = (index == 1) ? "ar" : "en";
  settings.setValue("Language", langCode);

  qApp->removeTranslator(translator);

  QString qmFile = QString(":/translations/cleanlinux_%1.qm").arg(langCode);
  if (translator->load(qmFile)) {
    qApp->installTranslator(translator);
  }

  // تحديث الواجهة
  ui->retranslateUi(this);
  retranslateUiCustom();
  updateDiskUsage();
}

void MainWindow::changeEvent(QEvent *event) {
  if (event->type() == QEvent::LanguageChange) {
    ui->retranslateUi(this);
    retranslateUiCustom();
  }
  QMainWindow::changeEvent(event);
}

void MainWindow::retranslateUiCustom() {
  // تحديث عناوين الجدول
  if (auto model =
          qobject_cast<QStandardItemModel *>(ui->categoriesTable->model())) {
    model->setHorizontalHeaderLabels(
        {tr("✔"), tr("النوع"), tr("الوصف"), tr("الحجم"), tr("الملفات")});
  }

  // تحديث شريط الحالة
  if (diskUsageBar) {
    diskUsageBar->setFormat(tr("%p% مستخدم"));
  }

  // تحديث عنوان الرسم البياني
  if (chart) {
    chart->setTitle(tr("المساحة المسترجعة"));
  }

  // تحديث نصوص المجموعات (Group Boxes)
  ui->categoriesGroup->setTitle(tr("الملفات التي يمكن تنظيفها"));
  ui->statsGroup->setTitle(tr("الإحصائيات"));
  ui->chartGroup->setTitle(tr("المساحة المسترجعة"));

  // تحديث نصوص أخرى
  ui->selectAllCheckbox->setText(tr("تحديد الكل"));
}

void MainWindow::loadSettings() {
  QSettings settings;

  // تحميل اللغة
  QString lang = settings.value("Language", "en").toString();
  int langIndex = (lang == "ar") ? 1 : 0;
  ui->languageComboBox->setCurrentIndex(langIndex);
  // changeLanguage will be called automatically due to index change signal if
  // needed, or we can call it explicitly if signal is blocked. But signals are
  // connected in setupUI which is called BEFORE loadSettings. However,
  // setCurrentIndex only emits if changed.

  // Force load initial language even if index doesn't change (default is 0)
  changeLanguage(langIndex);

  // تحميل الوضع الداكن
  if (settings.value("DarkMode", false).toBool()) {
    toggleDarkMode();
    ui->darkModeButton->setText("☀️");
  }

  restoreGeometry(settings.value("WindowGeometry").toByteArray());
  restoreState(settings.value("WindowState").toByteArray());
}

void MainWindow::saveSettings() {
  QSettings settings;
  settings.setValue("WindowGeometry", saveGeometry());
  settings.setValue("WindowState", saveState());
}

QString MainWindow::formatSize(qint64 bytes) {
  const QStringList units = {"بايت", "كيلوبايت", "ميغابايت", "جيجابايت",
                             "تيرابايت"};
  int unitIndex = 0;
  double size = bytes;

  while (size >= 1024 && unitIndex < units.size() - 1) {
    size /= 1024;
    unitIndex++;
  }

  return QString("%1 %2").arg(QString::number(size, 'f', 2), units[unitIndex]);
}

void MainWindow::on_categoryCheckbox_toggled() {
  // تحديث الحجم المحدد عند تغيير حالة التحديد
  updateSelectedSize();
}

void MainWindow::showAbout() {
  QString aboutText =
      tr("<h3>Clean Linux v1.0.0</h3>"
         "<p><b>أداة تنظيف وتحسين نظام لينكس</b></p>"
         "<p>برنامج مفتوح المصدر يساعدك على تحرير المساحة وتحسين أداء النظام "
         "من خلال تنظيف الملفات المؤقتة وغير الضرورية.</p>"
         "<p><b>الميزات:</b></p>"
         "<ul>"
         "<li>تنظيف ذاكرة التخزين المؤقت (APT, Flatpak, Snap)</li>"
         "<li>حذف مخلفات المتصفحات (Firefox, Chrome)</li>"
         "<li>إزالة الحزم اليتيمة وغير المستخدمة</li>"
         "<li>مراقبة حالة القرص</li>"
         "</ul>"
         "<p>تم البرمجة بواسطة: <b>linuxlaghouatalgeria</b></p>"
         "<p>حقوق النشر © 2025</p>");

  QMessageBox::about(this, tr("حول البرنامج"), aboutText);
}

void MainWindow::showSettings() {
  // يمكن إضافة نافذة إعدادات لاحقاً
  QMessageBox::information(this, tr("إعدادات"),
                           tr("نافذة الإعدادات قيد التطوير"));
}

void MainWindow::updateDiskUsage() {
  QStorageInfo storage = QStorageInfo::root();
  storage.refresh();

  if (storage.isValid()) {
    qint64 total = storage.bytesTotal();
    qint64 available = storage.bytesAvailable();
    qint64 used = total - available;

    int usagePercent = (used * 100) / total;

    diskUsageBar->setValue(usagePercent);
    diskUsageBar->setToolTip(tr("مستخدم: %1 / %2\nمتوفر: %3")
                                 .arg(formatSize(used))
                                 .arg(formatSize(total))
                                 .arg(formatSize(available)));

    // تغيير اللون حسب الاستخدام
    QString style;
    if (usagePercent > 90) {
      style = "QProgressBar::chunk { background-color: #ff3333; }";
    } else if (usagePercent > 70) {
      style = "QProgressBar::chunk { background-color: #ffaa33; }";
    } else {
      style = "QProgressBar::chunk { background-color: #33aa33; }";
    }
    diskUsageBar->setStyleSheet(style);
  }
}
