#ifndef IMAGEVIEWERDIALOG_H
#define IMAGEVIEWERDIALOG_H

#include <QDialog>
#include <QPixmap>
#include <QScrollBar>

class QLabel;
class QScrollArea;

class ImageViewerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ImageViewerDialog(QWidget *parent = nullptr);
    void setImage(const QString &imagePath);        // 文件路径版本
    void setImage(const QPixmap *pixmap);           // 指针版本
    void setImage(const QPixmap &pixmap);           // 引用版本


protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void zoomIn();
    void zoomOut();
    void resetZoom();
    void fitToWindow();

private:
    void scaleImage(double factor);
    void adjustScrollBar(QScrollBar *scrollBar, double factor);

    QLabel *m_imageLabel;
    QScrollArea *m_scrollArea;
    QPixmap m_originalPixmap;
    double m_scaleFactor;
};

#endif // IMAGEVIEWERDIALOG_H
