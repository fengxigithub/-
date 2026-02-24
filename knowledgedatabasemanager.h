#ifndef KNOWLEDGEDATABASEMANAGER_H
#define KNOWLEDGEDATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

struct KnowledgePoint {
    int id;
    QString title;
    QString content;
    QString imagePath;
    QString category;
    int difficulty;
    int status;
    int masteryLevel;
    QString createdDate;
    QString lastReviewed;
    QString nextReview;
    int reviewCount;
    QString tags;
};

class KnowledgeDatabaseManager : public QObject
{
    Q_OBJECT

public:
    explicit KnowledgeDatabaseManager(QObject *parent = nullptr);
    ~KnowledgeDatabaseManager();

    bool initializeDatabase(const QString &dbPath = "");
    bool isConnected() const;

    // CRUD 操作
    QVector<KnowledgePoint> getAllPoints() const;
    QVector<KnowledgePoint> getPointsByStatus(int status) const;
    QVector<KnowledgePoint> searchPoints(const QString &keyword) const;

    bool addPoint(const KnowledgePoint &point);
    bool updatePoint(const KnowledgePoint &point);
    bool deletePoint(int pointId);
    bool markAsReviewed(int pointId, int effectiveness);

    // 统计方法
    int getTotalCount() const;
    int getDueForReviewCount() const;
    int getMasteredCount() const;

private:
    QSqlDatabase database;
    QString databasePath;
    bool createTables();
};

#endif // KNOWLEDGEDATABASEMANAGER_H
