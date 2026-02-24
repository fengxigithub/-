#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QDate>
#include <QMap>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDateTime>
#include <QMouseEvent>
#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

// 知识点状态枚举
enum KnowledgeStatus {
    STATUS_NEW,         // 新知识点
    STATUS_LEARNING,    // 学习中
    STATUS_REVIEWING,   // 复习中
    STATUS_MASTERED     // 已掌握
};

// 知识点数据结构
struct KnowledgePoint {
    int id;
    QString title;
    QString content;
    QString imagePath;
    QString category;
    KnowledgeStatus status;
    int masteryLevel; // 掌握程度 0-100
    QDate createDate;
    QDate lastReviewDate;
    QDate nextReviewDate;
    int reviewCount;
    int reviewtureCount;
};

// 前向声明
class ImageViewerDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT



public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // 按钮点击槽函数
    void handleAddNew();
    void handleEditPoint();
    void handleMarkReviewed();
    void handleDeletePoint();
    void handleExportData();
    void handleClearSearch();

    // 其他交互槽函数
    void handleListSelectionChanged();
    void handleListItemDoubleClicked(QListWidgetItem *item);
    void handleSearchTextChanged(const QString &text);
    void handleFilterCategoryChanged(int index);
    void handleFilterStatusChanged(int index);
    void handleStatusChanged(int index);
    void handleCalendarClicked(const QDate &date);

    // 图片操作槽函数
    void handleZoomIn();
    void handleZoomOut();
    void handleResetZoom();
    //处理图片点击
    void handleImageClicked();

    void on_familiarButton_clicked();

    void on_indistinctButton_clicked();

    void on_forgetButton_clicked();

private:
    Ui::MainWindow *ui;
    QMap<int, KnowledgePoint> knowledgePoints;
    int nextId = 1;
    double imageZoomFactor = 1.0; // 图片缩放因子

    QString m_imageStoragePath; // 图片存储路径

    bool m_isRefreshing = false;// 防止刷新递归

    // 记忆曲线间隔（天数）
    const QVector<int> reviewIntervals = {1, 2, 4, 7, 15, 30, 60, 90};

    void loadKnowledgePoints();
    void saveKnowledgePoints();
    void refreshKnowledgeList();
    void updateStatistics();
    void showKnowledgePointDetails(int id);
    void addKnowledgePoint(const QString &title, const QString &content,
                           const QString &imagePath, const QString &category);
    void editKnowledgePoint(int id, const QString &title, const QString &content,
                            const QString &imagePath, const QString &category);
    void markAsReviewed(int id,int reviewvalue);
    QDate calculateNextReviewDate(int currentLevel, int reviewCount);
    void updateMasteryLevel(int id, int newLevel);
    void filterKnowledgePoints();
    void displayImage(const QString &imagePath); // 显示图片函数

    // 当前过滤条件
    QString currentSearchText;
    QString currentCategoryFilter;
    QString currentStatusFilter;

    //图片储存函数
    QString copyImageToStorage(const QString &sourceImagePath);
    QString getImageStoragePath();
    bool ensureImageStorageDirectory();

    ImageViewerDialog *m_imageViewer; // 图片查看对话框

    // void debugDataSources();//看资源在哪的，可删

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
};

#endif // MAINWINDOW_H
