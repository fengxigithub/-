#include "knowledgepointdialog.h"  // 确保包含正确的头文件
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>

// 构造函数 - 注意类名大小写
KnowledgePointDialog::KnowledgePointDialog(QWidget *parent, int pointId)
    : QDialog(parent)
    , currentPointId(pointId)
{
    setupUI();
    if (pointId != -1) {
        loadPointData(pointId);
    }
}

KnowledgePointDialog::~KnowledgePointDialog()
{
    // 清理资源
}

void KnowledgePointDialog::setupUI()
{
    // 创建组件
    editTitle = new QLineEdit(this);
    textContent = new QTextEdit(this);
    editImagePath = new QLineEdit(this);
    btnBrowseImage = new QPushButton("浏览...", this);
    labelImagePreview = new QLabel(this);
    comboCategory = new QComboBox(this);
    spinDifficulty = new QSpinBox(this);
    comboStatus = new QComboBox(this);
    btnSave = new QPushButton("保存", this);
    btnCancel = new QPushButton("取消", this);

    // 设置组件属性
    spinDifficulty->setRange(1, 5);
    labelImagePreview->setFixedSize(200, 150);
    labelImagePreview->setStyleSheet("border: 1px solid gray;");
    labelImagePreview->setAlignment(Qt::AlignCenter);

    // 添加分类选项
    comboCategory->addItems({"编程", "数学", "语言", "科学", "历史", "其他"});

    // 添加状态选项
    comboStatus->addItems({"新知识点", "学习中", "复习中", "已掌握"});

    // 布局设置
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->addRow("标题:", editTitle);
    formLayout->addRow("分类:", comboCategory);
    formLayout->addRow("难度:", spinDifficulty);
    formLayout->addRow("状态:", comboStatus);

    QHBoxLayout *imageLayout = new QHBoxLayout();
    imageLayout->addWidget(new QLabel("图片路径:"));
    imageLayout->addWidget(editImagePath);
    imageLayout->addWidget(btnBrowseImage);

    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(imageLayout);
    mainLayout->addWidget(new QLabel("图片预览:"));
    mainLayout->addWidget(labelImagePreview);
    mainLayout->addWidget(new QLabel("内容:"));
    mainLayout->addWidget(textContent);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(btnSave);
    buttonLayout->addWidget(btnCancel);

    mainLayout->addLayout(buttonLayout);

    // 连接信号槽
    connect(btnBrowseImage, &QPushButton::clicked, this, &KnowledgePointDialog::onBrowseImage);
    connect(btnSave, &QPushButton::clicked, this, &KnowledgePointDialog::onSavePoint);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
}

void KnowledgePointDialog::onBrowseImage()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    "选择知识点图片",
                                                    "",
                                                    "图片文件 (*.png *.jpg *.jpeg *.bmp)");

    if (!fileName.isEmpty()) {
        editImagePath->setText(fileName);

        // 显示图片预览
        QPixmap pixmap(fileName);
        if (!pixmap.isNull()) {
            labelImagePreview->setPixmap(pixmap.scaled(labelImagePreview->size(),
                                                       Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    }
}

void KnowledgePointDialog::onSavePoint()
{
    // 验证输入
    if (editTitle->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "警告", "请输入知识点标题");
        return;
    }

    accept(); // 关闭对话框并返回 Accepted
}

// 获取方法实现
QString KnowledgePointDialog::getTitle() const
{
    return editTitle->text().trimmed();
}

QString KnowledgePointDialog::getContent() const
{
    return textContent->toPlainText().trimmed();
}

QString KnowledgePointDialog::getImagePath() const
{
    return editImagePath->text().trimmed();
}

QString KnowledgePointDialog::getCategory() const
{
    return comboCategory->currentText();
}

int KnowledgePointDialog::getDifficulty() const
{
    return spinDifficulty->value();
}

int KnowledgePointDialog::getStatus() const
{
    return comboStatus->currentIndex();
}

void KnowledgePointDialog::loadPointData(int pointId)
{
    // 这里实现从数据库加载数据
    // 暂时用假数据演示
    editTitle->setText("示例知识点");
    textContent->setPlainText("这是知识点的详细内容...");
    comboCategory->setCurrentText("编程");
    spinDifficulty->setValue(3);
    comboStatus->setCurrentIndex(1);
}
