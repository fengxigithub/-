#include "imageviewerdialog.h"
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QAction>
#include <QScrollBar>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QFileInfo>
#include <QDebug>

ImageViewerDialog::ImageViewerDialog(QWidget *parent)
    : QDialog(parent)
    , m_scaleFactor(1.0)
{
    setWindowTitle("图片查看器");
    resize(800, 600);

    // 创建滚动区域
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setBackgroundRole(QPalette::Dark);
    m_scrollArea->setAlignment(Qt::AlignCenter);

    // 创建图片标签
    m_imageLabel = new QLabel();
    m_imageLabel->setBackgroundRole(QPalette::Base);
    m_imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    m_imageLabel->setScaledContents(false);
    m_imageLabel->setAlignment(Qt::AlignCenter);

    m_scrollArea->setWidget(m_imageLabel);

    // 创建工具栏
    QToolBar *toolBar = new QToolBar(this);

    QAction *zoomInAction = new QAction("放大", this);
    QAction *zoomOutAction = new QAction("缩小", this);
    QAction *resetAction = new QAction("重置", this);
    QAction *fitAction = new QAction("适应窗口", this);

    connect(zoomInAction, &QAction::triggered, this, &ImageViewerDialog::zoomIn);
    connect(zoomOutAction, &QAction::triggered, this, &ImageViewerDialog::zoomOut);
    connect(resetAction, &QAction::triggered, this, &ImageViewerDialog::resetZoom);
    connect(fitAction, &QAction::triggered, this, &ImageViewerDialog::fitToWindow);

    toolBar->addAction(zoomInAction);
    toolBar->addAction(zoomOutAction);
    toolBar->addAction(resetAction);
    toolBar->addAction(fitAction);

    // 布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(toolBar);
    mainLayout->addWidget(m_scrollArea);

    setLayout(mainLayout);
}

void ImageViewerDialog::setImage(const QPixmap *pixmap)
{
    if (pixmap && !pixmap->isNull()) {
        m_originalPixmap = *pixmap;  // 解引用指针获取对象
        m_imageLabel->setPixmap(m_originalPixmap);
        resetZoom();
    }
}

void ImageViewerDialog::setImage(const QPixmap &pixmap)
{
    if (!pixmap.isNull()) {
        m_originalPixmap = pixmap;
        m_imageLabel->setPixmap(m_originalPixmap);
        resetZoom();
    }
}
void ImageViewerDialog::setImage(const QString &imagePath)
{
    if (!imagePath.isEmpty()) {
        QPixmap pixmap(imagePath);
        if (!pixmap.isNull()) {
            m_originalPixmap = pixmap;
            m_imageLabel->setPixmap(m_originalPixmap);
            resetZoom();
        } else {
            // 可选：处理加载失败的情况
            qWarning() << "Failed to load image:" << imagePath;
        }
    }
}

void ImageViewerDialog::zoomIn()
{
    scaleImage(1.25);
}

void ImageViewerDialog::zoomOut()
{
    scaleImage(0.8);
}

void ImageViewerDialog::resetZoom()
{
    m_scaleFactor = 1.0;
    m_imageLabel->setPixmap(m_originalPixmap);
    m_imageLabel->adjustSize();
}

void ImageViewerDialog::fitToWindow()
{
    if (m_originalPixmap.isNull()) return;

    QSize labelSize = m_scrollArea->size();
    QSize pixmapSize = m_originalPixmap.size();

    double scale = qMin(
        double(labelSize.width()) / pixmapSize.width(),
        double(labelSize.height()) / pixmapSize.height()
        );

    scaleImage(scale / m_scaleFactor);
}

void ImageViewerDialog::scaleImage(double factor)
{
    m_scaleFactor *= factor;

    QSize newSize = m_originalPixmap.size() * m_scaleFactor;
    QPixmap scaledPixmap = m_originalPixmap.scaled(
        newSize,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation
        );

    m_imageLabel->setPixmap(scaledPixmap);
    m_imageLabel->adjustSize();
}

void ImageViewerDialog::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->angleDelta().y() > 0) {
            zoomIn();
        } else {
            zoomOut();
        }
        event->accept();
    } else {
        QDialog::wheelEvent(event);
    }
}

void ImageViewerDialog::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton) {
        resetZoom();
        event->accept();
    } else {
        QDialog::mousePressEvent(event);
    }
}

void ImageViewerDialog::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Plus:
    case Qt::Key_Equal:
        zoomIn();
        break;
    case Qt::Key_Minus:
        zoomOut();
        break;
    case Qt::Key_0:
        resetZoom();
        break;
    case Qt::Key_Escape:
        close();
        break;
    case Qt::Key_F:
        fitToWindow();
        break;
    default:
        QDialog::keyPressEvent(event);
    }
}
