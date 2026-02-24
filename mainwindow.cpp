#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QTextStream>
#include <QRandomGenerator>
#include <QDebug>
#include <QFileInfo>
#include <QPixmap>
#include <QToolBar> // 添加 QToolBar 头文件
#include <QSizePolicy> // 添加 QSizePolicy 头文件
#include "imageviewerdialog.h"
#include <QIcon>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_isRefreshing(false)
    , m_imageViewer(nullptr)
{
    ui->setupUi(this);
    qDebug() << "MainWindow constructed";
    // 设置窗口图标
    setWindowIcon(QIcon("icon.png"));
    this->setWindowTitle("知识点记忆系统 - 麻辣兔头");
    // 初始化图片查看器
    m_imageViewer = new ImageViewerDialog(this);
    // 设置图片标签可点击
    ui->labelImageDisplay->setCursor(Qt::PointingHandCursor);
    ui->labelImageDisplay->installEventFilter(this);

    // 初始化图片存储路径
    m_imageStoragePath = getImageStoragePath();
    qDebug() << "Image storage path:" << m_imageStoragePath;
    // 确保存储目录存在
    if (!ensureImageStorageDirectory()) {
        QMessageBox::warning(this, "警告", "无法创建图片存储目录，图片保存功能可能受限");
    }

    // 先清空组合框
    ui->comboStatus->clear();
    ui->comboFilterStatus->clear();

    // 初始化状态组合框
    ui->comboStatus->addItem("新知识点", STATUS_NEW);
    ui->comboStatus->addItem("学习中", STATUS_LEARNING);
    ui->comboStatus->addItem("复习中", STATUS_REVIEWING);
    ui->comboStatus->addItem("已掌握", STATUS_MASTERED);

    ui->comboFilterStatus->addItem("全部状态", "");
    ui->comboFilterStatus->addItem("新知识点", "new");
    ui->comboFilterStatus->addItem("学习中", "learning");
    ui->comboFilterStatus->addItem("复习中", "reviewing");
    ui->comboFilterStatus->addItem("已掌握", "mastered");

    // 设置图片标签
    ui->labelImageDisplay->setMinimumSize(400, 300);
    ui->labelImageDisplay->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->labelImageDisplay->setAlignment(Qt::AlignCenter);
    ui->labelImageDisplay->setText("图片显示区域");

    // 加载数据
    loadKnowledgePoints();
    qDebug() << "Loaded" << knowledgePoints.size() << "knowledge points";

    // 如果没有数据，显示提示
    if (knowledgePoints.isEmpty()) {
        qDebug() << "No knowledge points found, showing welcome message";
        ui->textContent->setPlainText("欢迎使用记忆曲线复习系统！\n请点击\"添加\"按钮创建第一个知识点。");
    }

    refreshKnowledgeList();
    updateStatistics();


    // debugDataSources();//测试可删

    // 连接信号槽
    connect(ui->btnAddNew, &QPushButton::clicked, this, &MainWindow::handleAddNew);
    connect(ui->btnEditPoint, &QPushButton::clicked, this, &MainWindow::handleEditPoint);
    connect(ui->btnMarkReviewed, &QPushButton::clicked, this, &MainWindow::handleMarkReviewed);
    connect(ui->btnDeletePoint, &QPushButton::clicked, this, &MainWindow::handleDeletePoint);
    connect(ui->btnExportData, &QPushButton::clicked, this, &MainWindow::handleExportData);
    connect(ui->btnClearSearch, &QPushButton::clicked, this, &MainWindow::handleClearSearch);

    connect(ui->listKnowledgePoints, &QListWidget::itemSelectionChanged,
            this, &MainWindow::handleListSelectionChanged);
    connect(ui->listKnowledgePoints, &QListWidget::itemDoubleClicked,
            this, &MainWindow::handleListItemDoubleClicked);
    connect(ui->editSearch, &QLineEdit::textChanged,
            this, &MainWindow::handleSearchTextChanged);
    connect(ui->comboFilterCategory, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::handleFilterCategoryChanged);
    connect(ui->comboFilterStatus, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::handleFilterStatusChanged);
    connect(ui->comboStatus, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::handleStatusChanged);
    connect(ui->calendarReview, &QCalendarWidget::clicked,
            this, &MainWindow::handleCalendarClicked);

    qDebug() << "MainWindow initialization completed";
}

MainWindow::~MainWindow()
{
    saveKnowledgePoints();
    delete m_imageViewer; // 释放图片查看器
    delete ui;
}

// 以下是所有槽函数的实现，只需要重命名即可

void MainWindow::handleAddNew()
{
    qDebug() << "handleAddNew called";

    bool ok;
    QString title = QInputDialog::getText(this, "添加知识点", "请输入知识点标题:",
                                          QLineEdit::Normal, "", &ok);

    if (!ok) {
        qDebug() << "Add new cancelled by user";
        return;
    }

    if (title.isEmpty()) {
        qDebug() << "User entered empty title";
        QMessageBox::warning(this, "错误", "知识点标题不能为空!");
        return;
    }

    QString content = QInputDialog::getMultiLineText(this, "添加知识点",
                                                     "请输入知识点内容:", "", &ok);
    if (!ok) {
        qDebug() << "Add new cancelled at content stage";
        return;
    }

    QString imagePath;
    if (QMessageBox::question(this, "添加图片", "是否要添加图片?") == QMessageBox::Yes) {
        QString selectedImagePath = QFileDialog::getOpenFileName(this, "选择图片", "",
                                                                 "Images (*.png *.jpg *.jpeg *.bmp *.gif *.tiff)");
        if (!selectedImagePath.isEmpty()) {
            // 复制图片到专用存储目录
            imagePath = copyImageToStorage(selectedImagePath);
            qDebug() << "Selected image:" << selectedImagePath << "-> Stored at:" << imagePath;
        }
    }

    QString category = QInputDialog::getText(this, "添加分类", "请输入分类名称:",
                                             QLineEdit::Normal, "未分类", &ok);
    if (!ok) {
        category = "未分类";
    }

    addKnowledgePoint(title, content, imagePath, category);
    qDebug() << "Add new completed";
}

void MainWindow::handleEditPoint()
{
    QListWidgetItem *currentItem = ui->listKnowledgePoints->currentItem();
    if (!currentItem) {
        QMessageBox::information(this, "提示", "请先选择一个知识点");
        return;
    }

    int id = currentItem->data(Qt::UserRole).toInt();
    if (!knowledgePoints.contains(id)) {
        QMessageBox::warning(this, "错误", "选中的知识点不存在!");
        return;
    }

    const KnowledgePoint &point = knowledgePoints[id];

    bool ok;
    QString title = QInputDialog::getText(this, "编辑知识点", "修改标题:",
                                          QLineEdit::Normal, point.title, &ok);
    if (!ok) return;

    QString content = QInputDialog::getMultiLineText(this, "编辑知识点",
                                                     "修改内容:", point.content, &ok);
    if (!ok) return;

    QString imagePath = point.imagePath;
    if (QMessageBox::question(this, "修改图片", "是否要修改图片?") == QMessageBox::Yes) {
        QString selectedImagePath = QFileDialog::getOpenFileName(this, "选择图片", "",
                                                                 "Images (*.png *.jpg *.jpeg *.bmp *.gif *.tiff)");
        if (!selectedImagePath.isEmpty()) {
            // 复制新图片到专用存储目录
            imagePath = copyImageToStorage(selectedImagePath);
            qDebug() << "New image selected:" << selectedImagePath << "-> Stored at:" << imagePath;

            // 可选：删除旧的图片文件（如果它在专用目录中）
            if (!point.imagePath.isEmpty() && point.imagePath.startsWith(m_imageStoragePath)) {
                QFile oldFile(point.imagePath);
                if (oldFile.exists()) {
                    oldFile.remove();
                    qDebug() << "Old image removed:" << point.imagePath;
                }
            }
        }
    }

    QString category = QInputDialog::getText(this, "修改分类", "修改分类名称:",
                                             QLineEdit::Normal, point.category, &ok);
    if (!ok) {
        category = point.category;
    }

    editKnowledgePoint(id, title, content, imagePath, category);
}

void MainWindow::handleMarkReviewed()
{
    qDebug() << "handleMarkReviewed called";

    QListWidgetItem *currentItem = ui->listKnowledgePoints->currentItem();
    if (!currentItem) {
        qDebug() << "No item selected";
        QMessageBox::warning(this, "提示", "请先选择一个知识点进行复习!");
        return;
    }

    int id = currentItem->data(Qt::UserRole).toInt();
    qDebug() << "Selected item ID:" << id;

    if (!knowledgePoints.contains(id)) {
        qDebug() << "Knowledge point not found for ID:" << id;
        QMessageBox::warning(this, "错误", "选中的知识点不存在!");
        return;
    }
    int reviewvalue=0;
    qDebug() << "Marking as reviewed...";
    // markAsReviewed(id,reviewvalue);//功能由三个熟悉，模糊，忘记代码替代
    qDebug() << "Mark as reviewed completed";
}

void MainWindow::handleDeletePoint()
{
    QListWidgetItem *currentItem = ui->listKnowledgePoints->currentItem();
    if (!currentItem) return;

    int id = currentItem->data(Qt::UserRole).toInt();
    // 在删除前先获取知识点的图片文件名
    QString imageFileName;
    qDebug() << imageFileName;
    if (knowledgePoints.contains(id)) {
        imageFileName = knowledgePoints[id].imagePath;
    }
    if (QMessageBox::question(this, "确认删除", "确定要删除这个知识点吗?") == QMessageBox::Yes) {
        // 删除对应的图片文件
        if (!imageFileName.isEmpty()) {
            QFile imageFile(imageFileName);
            if (imageFile.exists()) {
                imageFile.remove();
            }
        }
        knowledgePoints.remove(id);
        saveKnowledgePoints();
        refreshKnowledgeList();
        updateStatistics();

        // 清空显示
        ui->textContent->clear();
        ui->labelImageDisplay->setText("图片显示");
        ui->progressMastery->setValue(0);
        ui->labelMasteryPercen->setText("0%");
    }
}

void MainWindow::handleExportData()
{
    QString fileName = QFileDialog::getSaveFileName(this, "导出数据", "",
                                                    "JSON Files (*.json)");
    if (fileName.isEmpty()) return;

    QJsonArray jsonArray;
    for (const auto &point : knowledgePoints) {
        QJsonObject jsonObject;
        jsonObject["id"] = point.id;
        jsonObject["title"] = point.title;
        jsonObject["content"] = point.content;
        jsonObject["imagePath"] = point.imagePath;
        jsonObject["category"] = point.category;
        jsonObject["status"] = static_cast<int>(point.status);
        jsonObject["masteryLevel"] = point.masteryLevel;
        jsonObject["createDate"] = point.createDate.toString(Qt::ISODate);
        jsonObject["lastReviewDate"] = point.lastReviewDate.toString(Qt::ISODate);
        jsonObject["nextReviewDate"] = point.nextReviewDate.toString(Qt::ISODate);
        jsonObject["reviewCount"] = point.reviewCount;

        jsonArray.append(jsonObject);
    }

    QJsonDocument doc(jsonArray);
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
        QMessageBox::information(this, "导出成功", "数据导出成功!");
    } else {
        QMessageBox::warning(this, "导出失败", "无法保存文件!");
    }
}

void MainWindow::handleClearSearch()
{
    ui->editSearch->clear();
    ui->comboFilterCategory->setCurrentIndex(0);
    ui->comboFilterStatus->setCurrentIndex(0);

    currentSearchText = "";
    currentCategoryFilter = "";
    currentStatusFilter = "";

    filterKnowledgePoints();
}

void MainWindow::handleListSelectionChanged()
{
    if (m_isRefreshing) {
        qDebug() << "Currently refreshing, skipping selection change";
        return;
    }

    qDebug() << "handleListSelectionChanged called";

    static bool inSelectionChange = false; // 防止递归的标志

    if (inSelectionChange) {
        qDebug() << "Already in selection change, skipping";
        return;
    }

    inSelectionChange = true;

    qDebug() << "handleListSelectionChanged called";

    QListWidgetItem *currentItem = ui->listKnowledgePoints->currentItem();
    if (!currentItem) {
        qDebug() << "No item selected";
        // 清空显示，避免显示无效数据
        ui->textContent->clear();
        ui->labelImageDisplay->setText("图片显示");
        ui->progressMastery->setValue(0);
        ui->labelMasteryPercen->setText("0%");
        ui->labelLastReviewValue->setText("");
        ui->labelNextReviewValue->setText("");
        inSelectionChange = false;
        return;
    }

    int id = currentItem->data(Qt::UserRole).toInt();
    qDebug() << "Selected item ID:" << id;

    showKnowledgePointDetails(id);

    inSelectionChange = false;
    qDebug() << "handleListSelectionChanged completed";

    qDebug() << "handleListSelectionChanged completed";
}

void MainWindow::handleListItemDoubleClicked(QListWidgetItem *item)
{
    Q_UNUSED(item);
    handleMarkReviewed();
}

void MainWindow::handleSearchTextChanged(const QString &text)
{
    currentSearchText = text;
    filterKnowledgePoints();
}

void MainWindow::handleFilterCategoryChanged(int index)
{
    currentCategoryFilter = ui->comboFilterCategory->itemData(index).toString();
    filterKnowledgePoints();
}

void MainWindow::handleFilterStatusChanged(int index)
{
    currentStatusFilter = ui->comboFilterStatus->itemData(index).toString();
    filterKnowledgePoints();
}

void MainWindow::handleStatusChanged(int index)
{
    qDebug() << "handleStatusChanged called with index:" << index;

    QListWidgetItem *currentItem = ui->listKnowledgePoints->currentItem();
    if (!currentItem) {
        qDebug() << "No item selected, ignoring status change";
        return;
    }

    int id = currentItem->data(Qt::UserRole).toInt();
    if (!knowledgePoints.contains(id)) {
        qDebug() << "Knowledge point not found for ID:" << id;
        return;
    }

    // 检查索引是否有效
    if (index < 0 || index >= ui->comboStatus->count()) {
        qDebug() << "Invalid combo box index:" << index;
        return;
    }

    KnowledgePoint &point = knowledgePoints[id];
    KnowledgeStatus newStatus = static_cast<KnowledgeStatus>(ui->comboStatus->itemData(index).toInt());

    qDebug() << "Changing status from" << point.status << "to" << newStatus;

    point.status = newStatus;

    saveKnowledgePoints();
    refreshKnowledgeList();
    updateStatistics();

    qDebug() << "Status change completed";
}

void MainWindow::handleCalendarClicked(const QDate &date)
{
    // 高亮显示需要复习的日期
    QTextCharFormat format;
    format.setBackground(Qt::yellow);

    for (const auto &point : knowledgePoints) {
        if (point.nextReviewDate == date && point.status != STATUS_MASTERED) {
            ui->calendarReview->setDateTextFormat(date, format);
        }
    }
}

void MainWindow::loadKnowledgePoints()
{
    qDebug() << "Loading knowledge points...";

    QSettings settings("MyCompany", "KnowledgeReview");

    qDebug() << "设置文件路径:" << settings.fileName();
    qDebug() << "组织名称:" << settings.organizationName();
    qDebug() << "应用名称:" << settings.applicationName();

    // 列出所有现有的键
    qDebug() << "所有配置键:";
    foreach (QString key, settings.allKeys()) {
        qDebug() << key << ":" << settings.value(key).toString();
    }

    int count = settings.value("knowledgeCount", 0).toInt();
    qDebug() << "Found" << count << "knowledge points in settings";

    knowledgePoints.clear();
    nextId = 1;

    for (int i = 0; i < count; ++i) {
        QString prefix = QString("point_%1_").arg(i);

        // 检查必要的键是否存在
        if (!settings.contains(prefix + "id") ||
            !settings.contains(prefix + "title")) {
            qDebug() << "Skipping invalid entry at index" << i;
            continue;
        }

        KnowledgePoint point;
        point.id = settings.value(prefix + "id").toInt();
        point.title = settings.value(prefix + "title").toString();
        point.content = settings.value(prefix + "content").toString();
        point.imagePath = settings.value(prefix + "imagePath").toString();
        point.category = settings.value(prefix + "category").toString();
        point.status = static_cast<KnowledgeStatus>(settings.value(prefix + "status").toInt());
        point.masteryLevel = settings.value(prefix + "masteryLevel").toInt();
        point.createDate = settings.value(prefix + "createDate").toDate();
        point.lastReviewDate = settings.value(prefix + "lastReviewDate").toDate();
        point.nextReviewDate = settings.value(prefix + "nextReviewDate").toDate();
        point.reviewCount = settings.value(prefix + "reviewCount").toInt();

        // 验证数据有效性
        if (point.id <= 0 || point.title.isEmpty()) {
            qDebug() << "Skipping invalid knowledge point:" << point.id << point.title;
            continue;
        }

        knowledgePoints[point.id] = point;
        if (point.id >= nextId) nextId = point.id + 1;

        qDebug() << "Loaded point:" << point.id << point.title;
    }

    qDebug() << "Total loaded:" << knowledgePoints.size() << "valid knowledge points";
}

void MainWindow::saveKnowledgePoints()
{
    qDebug() << "Saving" << knowledgePoints.size() << "knowledge points...";

    // 阻塞所有可能触发刷新的信号
    bool oldListState = ui->listKnowledgePoints->blockSignals(true);
    bool oldComboState = ui->comboStatus->blockSignals(true);

    QSettings settings("MyCompany", "KnowledgeReview");

    // 将知识点列表按下次复习时间排序（由近到远）
    QList<KnowledgePoint> sortedPoints = knowledgePoints.values();

    // 使用稳定排序，按下次复习时间升序排列（最近的在前）
    std::sort(sortedPoints.begin(), sortedPoints.end(),
              [](const KnowledgePoint &a, const KnowledgePoint &b) {
                  return a.nextReviewDate < b.nextReviewDate;
              });

    // 保存排序后的知识点数量
    settings.setValue("knowledgeCount", sortedPoints.size());

    int index = 0;
    for (const auto &point : sortedPoints) {
        QString prefix = QString("point_%1_").arg(index);

        settings.setValue(prefix + "id", point.id);
        settings.setValue(prefix + "title", point.title);
        settings.setValue(prefix + "content", point.content);
        settings.setValue(prefix + "imagePath", point.imagePath);
        settings.setValue(prefix + "category", point.category);
        settings.setValue(prefix + "status", static_cast<int>(point.status));
        settings.setValue(prefix + "masteryLevel", point.masteryLevel);
        settings.setValue(prefix + "createDate", point.createDate);
        settings.setValue(prefix + "lastReviewDate", point.lastReviewDate);
        settings.setValue(prefix + "nextReviewDate", point.nextReviewDate);
        settings.setValue(prefix + "reviewCount", point.reviewCount);

        qDebug() << "Saved point:" << point.id << point.title;
        index++;
    }

    settings.sync(); // 确保数据写入磁盘

    // 恢复信号状态
    ui->listKnowledgePoints->blockSignals(oldListState);
    ui->comboStatus->blockSignals(oldComboState);

    qDebug() << "Data saved successfully, software continues to run";
}

void MainWindow::refreshKnowledgeList()
{
    if (m_isRefreshing) {
        qDebug() << "Already refreshing, skipping recursive call";
        return;
    }

    m_isRefreshing = true;
    qDebug() << "refreshKnowledgeList called";

    // 阻塞信号，防止触发选择变化事件
    bool oldState = ui->listKnowledgePoints->blockSignals(true);

    // 保存当前选中的项目
    QListWidgetItem *currentItem = ui->listKnowledgePoints->currentItem();
    int currentId = -1;
    if (currentItem) {
        currentId = currentItem->data(Qt::UserRole).toInt();
    }

    ui->listKnowledgePoints->clear();
    qDebug() << "List cleared";

    // 获取所有分类并更新过滤器
    QSet<QString> categories;
    ui->comboFilterCategory->clear();
    ui->comboFilterCategory->addItem("全部分类", "");

    for (const auto &point : knowledgePoints) {
        if (!point.category.isEmpty() && !categories.contains(point.category)) {
            categories.insert(point.category);
            ui->comboFilterCategory->addItem(point.category, point.category);
        }
    }
    qDebug() << "Categories updated:" << categories.size();

    // 添加知识点到列表
    int addedCount = 0;
    QListWidgetItem *selectedItem = nullptr;

    for (const auto &point : knowledgePoints) {
        // 应用过滤器
        if (!currentSearchText.isEmpty() &&
            !point.title.contains(currentSearchText, Qt::CaseInsensitive) &&
            !point.content.contains(currentSearchText, Qt::CaseInsensitive)) {
            continue;
        }

        if (!currentCategoryFilter.isEmpty() && point.category != currentCategoryFilter) {
            continue;
        }

        if (!currentStatusFilter.isEmpty()) {
            QString statusStr;
            switch (point.status) {
            case STATUS_NEW: statusStr = "new"; break;
            case STATUS_LEARNING: statusStr = "learning"; break;
            case STATUS_REVIEWING: statusStr = "reviewing"; break;
            case STATUS_MASTERED: statusStr = "mastered"; break;
            }
            if (statusStr != currentStatusFilter) continue;
        }

        QListWidgetItem *item = new QListWidgetItem(point.title);
        item->setData(Qt::UserRole, point.id);

        // 根据状态设置颜色
        switch (point.status) {
        case STATUS_NEW:
            item->setBackground(Qt::lightGray);
            break;
        case STATUS_LEARNING:
            item->setBackground(QColor(255, 255, 200)); // 浅黄色
            break;
        case STATUS_REVIEWING:
            if (point.nextReviewDate <= QDate::currentDate()) {
                item->setBackground(QColor(255, 200, 200)); // 浅红色（需要复习）
            } else {
                item->setBackground(QColor(200, 255, 200)); // 浅绿色
            }
            break;
        case STATUS_MASTERED:
            item->setBackground(Qt::cyan);
            break;
        }

        ui->listKnowledgePoints->addItem(item);
        addedCount++;

        // 记录需要选中的项目
        if (point.id == currentId) {
            selectedItem = item;
        }
    }

    qDebug() << "Added" << addedCount << "items to list";

    // 恢复选中状态
    if (selectedItem) {
        selectedItem->setSelected(true);
        ui->listKnowledgePoints->setCurrentItem(selectedItem);
        qDebug() << "Restored selection to item ID:" << currentId;
    } else if (ui->listKnowledgePoints->count() > 0) {
        // 如果没有匹配的选中项目，选择第一个
        ui->listKnowledgePoints->setCurrentRow(0);
        qDebug() << "Auto-selected first item";
    }

    // 恢复信号
    ui->listKnowledgePoints->blockSignals(oldState);

    qDebug() << "refreshKnowledgeList completed";

    m_isRefreshing = false;
    qDebug() << "refreshKnowledgeList completed";
}

void MainWindow::updateStatistics()
{
    qDebug() << "updateStatistics called";

    int total = knowledgePoints.size();
    int due = 0;
    int learning = 0;
    int mastered = 0;

    QDate today = QDate::currentDate();

    for (const auto &point : knowledgePoints) {
        if (point.status == STATUS_LEARNING) learning++;
        else if (point.status == STATUS_MASTERED) mastered++;
        else if (point.status == STATUS_REVIEWING && point.nextReviewDate <= today) due++;
    }

    ui->labelStatsTotal->setText(QString("总计：%1").arg(total));
    ui->labelStatsDue->setText(QString("待复习：%1").arg(due));
    ui->label_3->setText(QString("学习中：%1").arg(learning));
    ui->label_4->setText(QString("已掌握：%1").arg(mastered));

    qDebug() << "Statistics: Total:" << total << "Due:" << due << "Learning:" << learning << "Mastered:" << mastered;
    qDebug() << "updateStatistics completed";
}

void MainWindow::showKnowledgePointDetails(int id)
{
    qDebug() << "showKnowledgePointDetails called with ID:" << id;

    if (!knowledgePoints.contains(id)) {
        qDebug() << "Error: Knowledge point not found in showDetails!";
        // 清空显示，避免显示无效数据
        ui->textContent->clear();
        ui->labelImageDisplay->setText("无数据");
        ui->progressMastery->setValue(0);
        ui->labelMasteryPercen->setText("0%");
        ui->labelLastReviewValue->setText("");
        ui->labelNextReviewValue->setText("");
        return;
    }

    const KnowledgePoint &point = knowledgePoints[id];
    qDebug() << "Showing details for:" << point.title;

    // 显示基本信息
    ui->textContent->setPlainText(point.content);

    // 显示图片
    displayImage(point.imagePath);

    // 显示掌握程度
    ui->progressMastery->setValue(point.masteryLevel);
    ui->labelMasteryPercen->setText(QString("%1%").arg(point.masteryLevel));

    // 显示状态 - 阻塞信号避免递归
    bool oldState = ui->comboStatus->blockSignals(true);
    int statusIndex = -1;
    for (int i = 0; i < ui->comboStatus->count(); ++i) {
        if (ui->comboStatus->itemData(i).toInt() == static_cast<int>(point.status)) {
            statusIndex = i;
            break;
        }
    }
    if (statusIndex >= 0) {
        ui->comboStatus->setCurrentIndex(statusIndex);
    } else {
        ui->comboStatus->setCurrentIndex(0);
    }
    ui->comboStatus->blockSignals(oldState);

    // 显示复习时间
    ui->labelLastReviewValue->setText(point.lastReviewDate.isValid() ?
                                          point.lastReviewDate.toString("yyyy-MM-dd") : "从未复习");
    ui->labelNextReviewValue->setText(point.nextReviewDate.isValid() ?
                                          point.nextReviewDate.toString("yyyy-MM-dd") : "未设置");

    qDebug() << "Details shown successfully";
}

void MainWindow::addKnowledgePoint(const QString &title, const QString &content,
                                   const QString &imagePath, const QString &category)
{
    qDebug() << "addKnowledgePoint called with title:" << title;

    if (title.isEmpty()) {
        qDebug() << "Cannot add knowledge point with empty title";
        QMessageBox::warning(this, "错误", "知识点标题不能为空!");
        return;
    }

    KnowledgePoint point;
    point.id = nextId++;
    point.title = title;
    point.content = content;
    point.imagePath = imagePath;
    point.category = category;
    point.status = STATUS_NEW;
    point.masteryLevel = 0;
    point.createDate = QDate::currentDate();
    point.lastReviewDate = QDate();
    point.nextReviewDate = QDate::currentDate().addDays(1);
    point.reviewCount = 0;

    knowledgePoints[point.id] = point;
    qDebug() << "Point added to map, ID:" << point.id << "Total points now:" << knowledgePoints.size();

    // 立即保存数据
    saveKnowledgePoints();
    qDebug() << "saveKnowledgePoints completed";

    // 刷新界面
    refreshKnowledgeList();
    qDebug() << "refreshKnowledgeList completed";

    updateStatistics();
    qDebug() << "updateStatistics completed";

    qDebug() << "addKnowledgePoint completed for:" << point.id << point.title;
}

void MainWindow::editKnowledgePoint(int id, const QString &title, const QString &content,
                                    const QString &imagePath, const QString &category)
{
    if (!knowledgePoints.contains(id)) return;

    KnowledgePoint &point = knowledgePoints[id];
    point.title = title;
    point.content = content;
    point.imagePath = imagePath;
    point.category = category;

    // 不再立即保存
    refreshKnowledgeList();
    showKnowledgePointDetails(id);
}

void MainWindow::markAsReviewed(int id,int reviewvalue)
{
    qDebug() << "markAsReviewed called with ID:" << id;

    if (!knowledgePoints.contains(id)) {
        qDebug() << "Error: Knowledge point not found!";
        return;
    }

    KnowledgePoint &point = knowledgePoints[id];
    qDebug() << "Before review - Mastery:" << point.masteryLevel << "Review count:" << point.reviewCount;

    point.lastReviewDate = QDate::currentDate();
    if(reviewvalue==-5||reviewvalue==-10){
        point.reviewCount=0;
    }
    point.reviewCount++;
    point.reviewtureCount++;

    // 根据记忆曲线计算下次复习时间
    point.nextReviewDate = calculateNextReviewDate(point.masteryLevel, point.reviewCount);

    // 更新掌握程度（每次复习根据记忆情况变化）
    int improvement = reviewvalue; // 加上熟悉，模糊，忘记的赋值
    point.masteryLevel = qMin(100, point.masteryLevel + improvement);

    qDebug() << "Improvement:" << improvement << "New mastery:" << point.masteryLevel;

    // 如果掌握程度达到100%，标记为已掌握
    if (point.masteryLevel >= 100) {
        point.status = STATUS_MASTERED;
        point.masteryLevel = 100;
        qDebug() << "Status changed to MASTERED";
    } else if (point.masteryLevel >= 50) {
        point.status = STATUS_REVIEWING;
        qDebug() << "Status changed to REVIEWING";
    } else {
        point.status = STATUS_LEARNING;
        qDebug() << "Status changed to LEARNING";
    }

    // 立即保存数据
    saveKnowledgePoints();
    qDebug() << "Data saved";

    // 只刷新一次，避免递归
    refreshKnowledgeList();
    qDebug() << "List refreshed";

    updateStatistics();
    qDebug() << "Statistics updated";

    // 显示详细信息
    showKnowledgePointDetails(id);
    qDebug() << "Details shown";

    qDebug() << "markAsReviewed completed";
}

QDate MainWindow::calculateNextReviewDate(int currentLevel, int reviewCount)
{
    QDate nextDate = QDate::currentDate();

    if (currentLevel < 99) {
        // 使用预设的记忆曲线间隔
        nextDate = nextDate.addDays(reviewIntervals[reviewCount]);
    } else {
        // 超过预设间隔后，根据掌握程度动态计算
        int baseInterval = 30; // 基础间隔30天
        int levelFactor = (100 - currentLevel) / 10; // 掌握程度越低，复习越频繁
        nextDate = nextDate.addDays(baseInterval + levelFactor * 5);
    }
    // if (reviewCount < reviewIntervals.size()) {
    //     // 使用预设的记忆曲线间隔
    //     nextDate = nextDate.addDays(reviewIntervals[reviewCount]);
    // } else {
    //     // 超过预设间隔后，根据掌握程度动态计算
    //     int baseInterval = 30; // 基础间隔30天
    //     int levelFactor = (100 - currentLevel) / 10; // 掌握程度越低，复习越频繁
    //     nextDate = nextDate.addDays(baseInterval + levelFactor * 5);
    // }

    return nextDate;
}

void MainWindow::updateMasteryLevel(int id, int newLevel)
{
    if (!knowledgePoints.contains(id)) return;

    KnowledgePoint &point = knowledgePoints[id];
    point.masteryLevel = newLevel;

    // 更新状态
    if (newLevel >= 100) {
        point.status = STATUS_MASTERED;
    } else if (newLevel >= 50) {
        point.status = STATUS_REVIEWING;
    } else {
        point.status = STATUS_LEARNING;
    }

    saveKnowledgePoints();
    refreshKnowledgeList();
    updateStatistics();
}

void MainWindow::filterKnowledgePoints()
{
    refreshKnowledgeList();
    updateStatistics();
}

void MainWindow::displayImage(const QString &imagePath)
{
    ui->labelImageDisplay->clear();

    if (!imagePath.isEmpty()) {
        QFileInfo fileInfo(imagePath);
        if (fileInfo.exists() && fileInfo.isFile()) {
            QPixmap pixmap(imagePath);
            if (!pixmap.isNull()) {
                // 应用缩放因子
                QSize scaledSize = pixmap.size() * imageZoomFactor;
                QPixmap scaledPixmap = pixmap.scaled(scaledSize,
                                                     Qt::KeepAspectRatio,
                                                     Qt::SmoothTransformation);
                ui->labelImageDisplay->setPixmap(scaledPixmap);
                return;
            } else {
                ui->labelImageDisplay->setText("图片加载失败");
            }
        } else {
            ui->labelImageDisplay->setText("图片文件不存在");
        }
    } else {
        ui->labelImageDisplay->setText("无图片");
    }
}

void MainWindow::handleZoomIn()
{
    imageZoomFactor *= 1.2;
    QListWidgetItem *currentItem = ui->listKnowledgePoints->currentItem();
    if (currentItem) {
        int id = currentItem->data(Qt::UserRole).toInt();
        if (knowledgePoints.contains(id)) {
            displayImage(knowledgePoints[id].imagePath);
        }
    }
}

void MainWindow::handleZoomOut()
{
    imageZoomFactor /= 1.2;
    if (imageZoomFactor < 0.1) imageZoomFactor = 0.1;

    QListWidgetItem *currentItem = ui->listKnowledgePoints->currentItem();
    if (currentItem) {
        int id = currentItem->data(Qt::UserRole).toInt();
        if (knowledgePoints.contains(id)) {
            displayImage(knowledgePoints[id].imagePath);
        }
    }
}

void MainWindow::handleResetZoom()
{
    imageZoomFactor = 1.0;
    QListWidgetItem *currentItem = ui->listKnowledgePoints->currentItem();
    if (currentItem) {
        int id = currentItem->data(Qt::UserRole).toInt();
        if (knowledgePoints.contains(id)) {
            displayImage(knowledgePoints[id].imagePath);
        }
    }
}
//图片储存
QString MainWindow::getImageStoragePath()
{
    // 使用应用程序所在目录下的 images 文件夹
    QString appDir = QCoreApplication::applicationDirPath();
    QString imageDirPath = QDir(appDir).filePath("images");

    qDebug() << "应用程序目录:" << appDir;
    qDebug() << "图片存储目录:" << imageDirPath;

    // 确保目录存在
    QDir imageDir(imageDirPath);
    if (!imageDir.exists()) {
        imageDir.mkpath(".");
    }

    return imageDirPath;
}

bool MainWindow::ensureImageStorageDirectory()
{
    QDir imageDir(m_imageStoragePath);
    if (!imageDir.exists()) {
        return imageDir.mkpath(".");
    }
    return true;
}

QString MainWindow::copyImageToStorage(const QString &sourceImagePath)
{
    if (sourceImagePath.isEmpty()) {
        return "";
    }

    QFileInfo sourceFileInfo(sourceImagePath);
    if (!sourceFileInfo.exists() || !sourceFileInfo.isFile()) {
        qDebug() << "Source image file does not exist:" << sourceImagePath;
        return "";
    }

    // 确保存储目录存在
    if (!ensureImageStorageDirectory()) {
        qDebug() << "Failed to create image storage directory";
        return sourceImagePath; // 返回原路径作为备用
    }

    // 生成唯一的文件名（使用时间戳+随机数避免重名）
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz");
    int randomNum = QRandomGenerator::global()->bounded(1000, 9999);
    QString baseName = QString("%1_%2").arg(timestamp).arg(randomNum);

    // 保持原文件扩展名
    QString extension = sourceFileInfo.suffix();
    if (extension.isEmpty()) {
        extension = "png"; // 默认扩展名
    }

    QString targetFileName = baseName + "." + extension;
    QString targetFilePath = QDir(m_imageStoragePath).filePath(targetFileName);

    // 复制文件
    if (QFile::copy(sourceImagePath, targetFilePath)) {
        qDebug() << "Image copied to:" << targetFilePath;
        return targetFilePath;
    } else {
        qDebug() << "Failed to copy image from" << sourceImagePath << "to" << targetFilePath;
        return sourceImagePath; // 复制失败，返回原路径
    }
}
//以上

//图片放大
bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->labelImageDisplay && event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            handleImageClicked();
            return true;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}
void MainWindow::handleImageClicked()
{
    QListWidgetItem *currentItem = ui->listKnowledgePoints->currentItem();
    if (!currentItem) return;

    int id = currentItem->data(Qt::UserRole).toInt();
    if (!knowledgePoints.contains(id)) return;

    const KnowledgePoint &point = knowledgePoints[id];

    // 优先使用文件路径
    if (!point.imagePath.isEmpty() && QFile::exists(point.imagePath)) {
        m_imageViewer->setImage(point.imagePath);
        m_imageViewer->exec();
        return;
    }

    // 安全的方式：检查是否有图片显示
    // if (ui->labelImageDisplay->pixmap() != nullptr) {
    //     const QPixmap *pixmap = ui->labelImageDisplay->pixmap();
    //     if (!pixmap->isNull()) {
    //         m_imageViewer->setImage(*pixmap);
    //         m_imageViewer->exec();
    //     }
    // }
}

void MainWindow::on_familiarButton_clicked()
{
    QListWidgetItem *currentItem = ui->listKnowledgePoints->currentItem();
    int id = currentItem->data(Qt::UserRole).toInt();
    int reviewvalue=10;
    markAsReviewed(id,reviewvalue);
}


void MainWindow::on_indistinctButton_clicked()
{
    QListWidgetItem *currentItem = ui->listKnowledgePoints->currentItem();
    int id = currentItem->data(Qt::UserRole).toInt();
    int reviewvalue=-5;
    markAsReviewed(id,reviewvalue);
}


void MainWindow::on_forgetButton_clicked()
{
    QListWidgetItem *currentItem = ui->listKnowledgePoints->currentItem();
    int id = currentItem->data(Qt::UserRole).toInt();
    int reviewvalue=-10;
    markAsReviewed(id,reviewvalue);
}

// void MainWindow::debugDataSources()
// {
//     qDebug() << "=== 数据源调试信息 ===";

//     // 1. 检查注册表
//     QSettings settings("MyCompany", "KnowledgeReview");
//     qDebug() << "注册表知识点数量:" << settings.value("knowledgeCount", 0).toInt();

//     // 2. 检查内存中的数据
//     qDebug() << "内存中知识点数量:" << knowledgePoints.size();
//     qDebug() << "内存中知识点ID列表:" << knowledgePoints.keys();

//     // 3. 检查文件数据
//     QFileInfoList dataFiles = QDir(".").entryInfoList(
//         QStringList() << "*.dat" << "*.json" << "*.db" << "*.ini");
//     foreach (QFileInfo file, dataFiles) {
//         qDebug() << "发现数据文件:" << file.fileName() << "大小:" << file.size();
//     }
// }

