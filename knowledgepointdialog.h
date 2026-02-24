#ifndef KNOWLEDGEPOINTDIALOG_H
#define KNOWLEDGEPOINTDIALOG_H
//知识点
#include <QDialog>
#include <QMessageBox>

class QLineEdit;
class QTextEdit;
class QComboBox;
class QSpinBox;
class QPushButton;
class QLabel;

class KnowledgePointDialog : public QDialog
{
    Q_OBJECT

public:
    explicit KnowledgePointDialog(QWidget *parent = nullptr, int pointId = -1);
    ~KnowledgePointDialog();

    QString getTitle() const;
    QString getContent() const;
    QString getImagePath() const;
    QString getCategory() const;
    int getDifficulty() const;
    int getStatus() const;

private slots:
    void onBrowseImage();
    void onSavePoint();

private:
    QLineEdit *editTitle;
    QTextEdit *textContent;
    QLineEdit *editImagePath;
    QPushButton *btnBrowseImage;
    QLabel *labelImagePreview;
    QComboBox *comboCategory;
    QSpinBox *spinDifficulty;
    QComboBox *comboStatus;
    QPushButton *btnSave;
    QPushButton *btnCancel;

    int currentPointId;

    void setupUI();
    void loadPointData(int pointId);
};

#endif // KNOWLEDGEPOINTDIALOG_H
