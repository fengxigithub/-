#include "knowledgedatabasemanager.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QStandardPaths>

// 构造函数 - 注意类名大小写
KnowledgeDatabaseManager::KnowledgeDatabaseManager(QObject *parent)
    : QObject(parent)
    , database(QSqlDatabase::addDatabase("QSQLITE"))
{
    // 初始化数据库路径
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(appDataPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    databasePath = appDataPath + "/knowledge_points.db";
}

KnowledgeDatabaseManager::~KnowledgeDatabaseManager()
{
    if (database.isOpen()) {
        database.close();
    }
}

bool KnowledgeDatabaseManager::initializeDatabase(const QString &dbPath)
{
    if (!dbPath.isEmpty()) {
        databasePath = dbPath;
    }

    database.setDatabaseName(databasePath);

    if (!database.open()) {
        qDebug() << "无法打开数据库:" << database.lastError().text();
        return false;
    }

    return createTables();
}

bool KnowledgeDatabaseManager::isConnected() const
{
    return database.isOpen();
}

bool KnowledgeDatabaseManager::createTables()
{
    QSqlQuery query;

    // 创建知识点表
    QString createPointsTable =
        "CREATE TABLE IF NOT EXISTS knowledge_points ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "title TEXT NOT NULL,"
        "content TEXT,"
        "image_path TEXT,"
        "category TEXT,"
        "difficulty INTEGER DEFAULT 1,"
        "status INTEGER DEFAULT 0,"
        "mastery_level INTEGER DEFAULT 0,"
        "created_date TEXT,"
        "last_reviewed TEXT,"
        "next_review TEXT,"
        "review_count INTEGER DEFAULT 0,"
        "tags TEXT"
        ")";

    if (!query.exec(createPointsTable)) {
        qDebug() << "创建知识点表失败:" << query.lastError().text();
        return false;
    }

    // 创建复习历史表
    QString createHistoryTable =
        "CREATE TABLE IF NOT EXISTS review_history ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "point_id INTEGER,"
        "review_date TEXT,"
        "effectiveness INTEGER,"
        "notes TEXT,"
        "FOREIGN KEY (point_id) REFERENCES knowledge_points (id)"
        ")";

    if (!query.exec(createHistoryTable)) {
        qDebug() << "创建复习历史表失败:" << query.lastError().text();
        return false;
    }

    qDebug() << "数据库表创建成功";
    return true;
}

QVector<KnowledgePoint> KnowledgeDatabaseManager::getAllPoints() const
{
    QVector<KnowledgePoint> points;

    if (!database.isOpen()) {
        qDebug() << "数据库未连接";
        return points;
    }

    QSqlQuery query("SELECT * FROM knowledge_points ORDER BY next_review ASC");
    while (query.next()) {
        KnowledgePoint point;
        point.id = query.value("id").toInt();
        point.title = query.value("title").toString();
        point.content = query.value("content").toString();
        point.imagePath = query.value("image_path").toString();
        point.category = query.value("category").toString();
        point.difficulty = query.value("difficulty").toInt();
        point.status = query.value("status").toInt();
        point.masteryLevel = query.value("mastery_level").toInt();
        point.createdDate = query.value("created_date").toString();
        point.lastReviewed = query.value("last_reviewed").toString();
        point.nextReview = query.value("next_review").toString();
        point.reviewCount = query.value("review_count").toInt();
        point.tags = query.value("tags").toString();

        points.append(point);
    }

    return points;
}

bool KnowledgeDatabaseManager::addPoint(const KnowledgePoint &point)
{
    if (!database.isOpen()) {
        qDebug() << "数据库未连接";
        return false;
    }

    QSqlQuery query;
    query.prepare(
        "INSERT INTO knowledge_points "
        "(title, content, image_path, category, difficulty, status, mastery_level, "
        "created_date, last_reviewed, next_review, review_count, tags) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
        );

    query.addBindValue(point.title);
    query.addBindValue(point.content);
    query.addBindValue(point.imagePath);
    query.addBindValue(point.category);
    query.addBindValue(point.difficulty);
    query.addBindValue(point.status);
    query.addBindValue(point.masteryLevel);
    query.addBindValue(point.createdDate);
    query.addBindValue(point.lastReviewed);
    query.addBindValue(point.nextReview);
    query.addBindValue(point.reviewCount);
    query.addBindValue(point.tags);

    if (!query.exec()) {
        qDebug() << "添加知识点失败:" << query.lastError().text();
        return false;
    }

    return true;
}

bool KnowledgeDatabaseManager::updatePoint(const KnowledgePoint &point)
{
    if (!database.isOpen()) {
        qDebug() << "数据库未连接";
        return false;
    }

    QSqlQuery query;
    query.prepare(
        "UPDATE knowledge_points SET "
        "title = ?, content = ?, image_path = ?, category = ?, difficulty = ?, "
        "status = ?, mastery_level = ?, last_reviewed = ?, next_review = ?, "
        "review_count = ?, tags = ? WHERE id = ?"
        );

    query.addBindValue(point.title);
    query.addBindValue(point.content);
    query.addBindValue(point.imagePath);
    query.addBindValue(point.category);
    query.addBindValue(point.difficulty);
    query.addBindValue(point.status);
    query.addBindValue(point.masteryLevel);
    query.addBindValue(point.lastReviewed);
    query.addBindValue(point.nextReview);
    query.addBindValue(point.reviewCount);
    query.addBindValue(point.tags);
    query.addBindValue(point.id);

    if (!query.exec()) {
        qDebug() << "更新知识点失败:" << query.lastError().text();
        return false;
    }

    return true;
}

bool KnowledgeDatabaseManager::deletePoint(int pointId)
{
    if (!database.isOpen()) {
        qDebug() << "数据库未连接";
        return false;
    }

    QSqlQuery query;
    query.prepare("DELETE FROM knowledge_points WHERE id = ?");
    query.addBindValue(pointId);

    if (!query.exec()) {
        qDebug() << "删除知识点失败:" << query.lastError().text();
        return false;
    }

    return true;
}

bool KnowledgeDatabaseManager::markAsReviewed(int pointId, int effectiveness)
{
    if (!database.isOpen()) {
        qDebug() << "数据库未连接";
        return false;
    }

    // 更新知识点表的复习信息
    QSqlQuery query;
    query.prepare(
        "UPDATE knowledge_points SET "
        "last_reviewed = datetime('now'), "
        "review_count = review_count + 1, "
        "mastery_level = MIN(100, mastery_level + ?) "
        "WHERE id = ?"
        );
    query.addBindValue(effectiveness * 5); // 每次复习增加掌握度
    query.addBindValue(pointId);

    if (!query.exec()) {
        qDebug() << "标记复习失败:" << query.lastError().text();
        return false;
    }

    // 添加到复习历史
    QSqlQuery historyQuery;
    historyQuery.prepare(
        "INSERT INTO review_history (point_id, review_date, effectiveness) "
        "VALUES (?, datetime('now'), ?)"
        );
    historyQuery.addBindValue(pointId);
    historyQuery.addBindValue(effectiveness);

    if (!historyQuery.exec()) {
        qDebug() << "添加复习历史失败:" << historyQuery.lastError().text();
    }

    return true;
}

int KnowledgeDatabaseManager::getTotalCount() const
{
    if (!database.isOpen()) {
        return 0;
    }

    QSqlQuery query("SELECT COUNT(*) FROM knowledge_points");
    if (query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

int KnowledgeDatabaseManager::getDueForReviewCount() const
{
    if (!database.isOpen()) {
        return 0;
    }

    QSqlQuery query("SELECT COUNT(*) FROM knowledge_points WHERE next_review <= date('now')");
    if (query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

int KnowledgeDatabaseManager::getMasteredCount() const
{
    if (!database.isOpen()) {
        return 0;
    }

    QSqlQuery query("SELECT COUNT(*) FROM knowledge_points WHERE status = 3"); // 3 表示已掌握
    if (query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

QVector<KnowledgePoint> KnowledgeDatabaseManager::getPointsByStatus(int status) const
{
    QVector<KnowledgePoint> points;

    if (!database.isOpen()) {
        return points;
    }

    QSqlQuery query;
    query.prepare("SELECT * FROM knowledge_points WHERE status = ? ORDER BY next_review ASC");
    query.addBindValue(status);

    if (query.exec()) {
        while (query.next()) {
            KnowledgePoint point;
            point.id = query.value("id").toInt();
            point.title = query.value("title").toString();
            // ... 填充其他字段
            points.append(point);
        }
    }

    return points;
}

QVector<KnowledgePoint> KnowledgeDatabaseManager::searchPoints(const QString &keyword) const
{
    QVector<KnowledgePoint> points;

    if (!database.isOpen()) {
        return points;
    }

    QSqlQuery query;
    query.prepare(
        "SELECT * FROM knowledge_points "
        "WHERE title LIKE ? OR content LIKE ? OR tags LIKE ? "
        "ORDER BY next_review ASC"
        );
    QString searchPattern = "%" + keyword + "%";
    query.addBindValue(searchPattern);
    query.addBindValue(searchPattern);
    query.addBindValue(searchPattern);

    if (query.exec()) {
        while (query.next()) {
            KnowledgePoint point;
            point.id = query.value("id").toInt();
            point.title = query.value("title").toString();
            // ... 填充其他字段
            points.append(point);
        }
    }

    return points;
}
