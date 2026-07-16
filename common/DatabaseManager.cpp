#include "DatabaseManager.h"
#include "JsonHelper.h"
#include <QCryptographicHash>
#include <QDir>
#include <QDebug>
#include <QSet>
#include <algorithm>
#include <random>

DatabaseManager *DatabaseManager::m_instance = nullptr;

DatabaseManager::DatabaseManager(QObject *parent)
    : QObject(parent)
{
}

DatabaseManager::~DatabaseManager()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

DatabaseManager* DatabaseManager::instance()
{
    if (!m_instance) {
        m_instance = new DatabaseManager();
    }
    return m_instance;
}

bool DatabaseManager::initialize(const QString &dbPath)
{
    QMutexLocker locker(&m_mutex);

    if (m_db.isOpen()) {
        m_db.close();
    }

    // 释放旧连接引用，避免 removeDatabase 时仍有活跃引用导致警告
    m_db = QSqlDatabase();
    if (QSqlDatabase::contains("exam_connection")) {
        QSqlDatabase::removeDatabase("exam_connection");
    }

    m_db = QSqlDatabase::addDatabase(Constants::DB_TYPE, "exam_connection");
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qDebug() << "Failed to open database:" << m_db.lastError().text();
        return false;
    }

    // 启用外键约束
    QSqlQuery query(m_db);
    query.exec("PRAGMA foreign_keys = ON");

    return createTables() && insertDefaultData();
}

bool DatabaseManager::createTables()
{
    QMutexLocker locker(&m_mutex);
    QSqlQuery query(m_db);

    // 用户表
    bool ok = query.exec(
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "username TEXT UNIQUE NOT NULL,"
        "password TEXT NOT NULL,"
        "role INTEGER NOT NULL DEFAULT 0,"
        "real_name TEXT,"
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
        ")"
    );
    if (!ok) {
        qDebug() << "Failed to create users table:" << query.lastError().text();
        return false;
    }

    // 题目表
    ok = query.exec(
        "CREATE TABLE IF NOT EXISTS questions ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "type INTEGER NOT NULL DEFAULT 0,"
        "content TEXT NOT NULL,"
        "options TEXT NOT NULL,"
        "answer TEXT NOT NULL,"
        "category TEXT NOT NULL,"
        "difficulty INTEGER NOT NULL DEFAULT 0,"
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
        ")"
    );
    if (!ok) {
        qDebug() << "Failed to create questions table:" << query.lastError().text();
        return false;
    }

    // 考试配置表
    ok = query.exec(
        "CREATE TABLE IF NOT EXISTS exams ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT NOT NULL,"
        "question_count INTEGER NOT NULL DEFAULT 25,"
        "duration INTEGER NOT NULL DEFAULT 30,"
        "categories TEXT,"
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
        ")"
    );
    if (!ok) {
        qDebug() << "Failed to create exams table:" << query.lastError().text();
        return false;
    }

    // 考试记录表
    ok = query.exec(
        "CREATE TABLE IF NOT EXISTS exam_records ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "user_id INTEGER NOT NULL,"
        "exam_id INTEGER NOT NULL,"
        "questions TEXT NOT NULL,"
        "answers TEXT NOT NULL,"
        "score REAL NOT NULL DEFAULT 0,"
        "start_time TIMESTAMP NOT NULL,"
        "end_time TIMESTAMP,"
        "status INTEGER NOT NULL DEFAULT 0,"
        "FOREIGN KEY (user_id) REFERENCES users(id),"
        "FOREIGN KEY (exam_id) REFERENCES exams(id)"
        ")"
    );
    if (!ok) {
        qDebug() << "Failed to create exam_records table:" << query.lastError().text();
        return false;
    }

    return true;
}

bool DatabaseManager::insertDefaultData()
{
    QMutexLocker locker(&m_mutex);
    QSqlQuery query(m_db);

    // 插入默认用户（密码使用 MD5 加密）
    auto hashPassword = [](const QString &pwd) -> QString {
        return QString::fromUtf8(QCryptographicHash::hash(pwd.toUtf8(), QCryptographicHash::Md5).toHex());
    };

    // 默认账号列表：用户名、密码、角色、真实姓名
    struct DefaultUser {
        QString username;
        QString password;
        Constants::UserRole role;
        QString realName;
    };
    QList<DefaultUser> defaultUsers = {
        {"admin", hashPassword("admin123"), Constants::RoleAdmin, QString::fromUtf8("系统管理员")},
        {"teacher", hashPassword("teacher123"), Constants::RoleTeacher, QString::fromUtf8("张老师")},
        {"student", hashPassword("student123"), Constants::RoleStudent, QString::fromUtf8("小明")},
        {"student2", hashPassword("student123"), Constants::RoleStudent, QString::fromUtf8("小红")}
    };

    // 先修正已存在默认账号的角色（解决旧数据库角色被错误写入 0 的问题）
    for (const auto &u : defaultUsers) {
        query.prepare("UPDATE users SET role = ?, real_name = ? WHERE username = ?");
        query.addBindValue(static_cast<int>(u.role));
        query.addBindValue(u.realName);
        query.addBindValue(u.username);
        if (!query.exec()) {
            qDebug() << "Failed to update default user" << u.username
                     << ":" << query.lastError().text();
        }
    }

    // 再插入不存在的默认账号
    for (const auto &u : defaultUsers) {
        query.prepare("INSERT OR IGNORE INTO users (username, password, role, real_name) VALUES (?, ?, ?, ?)");
        query.addBindValue(u.username);
        query.addBindValue(u.password);
        query.addBindValue(static_cast<int>(u.role));
        query.addBindValue(u.realName);
        if (!query.exec()) {
            qDebug() << "Failed to insert default user" << u.username
                     << ":" << query.lastError().text();
        }
    }

    // 插入示例题目
    auto insertQuestion = [&](Constants::QuestionType type, const QString &content,
                               const QStringList &options, const QString &answer,
                               const QString &category, Constants::Difficulty difficulty) {
        QJsonArray optionsArray;
        for (const QString &opt : options) {
            optionsArray.append(opt);
        }
        query.prepare("INSERT OR IGNORE INTO questions (type, content, options, answer, category, difficulty) VALUES (?, ?, ?, ?, ?, ?)");
        query.addBindValue(static_cast<int>(type));
        query.addBindValue(content);
        query.addBindValue(QString::fromUtf8(QJsonDocument(optionsArray).toJson(QJsonDocument::Compact)));
        query.addBindValue(answer);
        query.addBindValue(category);
        query.addBindValue(static_cast<int>(difficulty));
        query.exec();
    };

    // 计算机基础 - 单选题
    insertQuestion(Constants::TypeSingleChoice, QString::fromUtf8("C++中，以下哪个关键字用于定义类？"),
                   QStringList{QString::fromUtf8("struct"), QString::fromUtf8("class"), QString::fromUtf8("union"), QString::fromUtf8("enum")},
                   "B", QString::fromUtf8("计算机基础"), Constants::DiffEasy);

    insertQuestion(Constants::TypeSingleChoice, QString::fromUtf8("以下哪种数据结构是先进先出（FIFO）？"),
                   QStringList{QString::fromUtf8("栈"), QString::fromUtf8("队列"), QString::fromUtf8("树"), QString::fromUtf8("图")},
                   "B", QString::fromUtf8("计算机基础"), Constants::DiffEasy);

    insertQuestion(Constants::TypeSingleChoice, QString::fromUtf8("TCP/IP协议中，IP协议工作在哪一层？"),
                   QStringList{QString::fromUtf8("应用层"), QString::fromUtf8("传输层"), QString::fromUtf8("网络层"), QString::fromUtf8("数据链路层")},
                   "C", QString::fromUtf8("计算机网络"), Constants::DiffMedium);

    insertQuestion(Constants::TypeSingleChoice, QString::fromUtf8("数据库的ACID特性中，'A'代表什么？"),
                   QStringList{QString::fromUtf8("原子性"), QString::fromUtf8("一致性"), QString::fromUtf8("隔离性"), QString::fromUtf8("持久性")},
                   "A", QString::fromUtf8("数据库"), Constants::DiffMedium);

    insertQuestion(Constants::TypeSingleChoice, QString::fromUtf8("以下哪种排序算法的时间复杂度最坏情况下为O(n²)？"),
                   QStringList{QString::fromUtf8("快速排序"), QString::fromUtf8("归并排序"), QString::fromUtf8("堆排序"), QString::fromUtf8("冒泡排序")},
                   "D", QString::fromUtf8("算法"), Constants::DiffMedium);

    // 多选题
    insertQuestion(Constants::TypeMultipleChoice, QString::fromUtf8("以下哪些是面向对象的三大特性？"),
                   QStringList{QString::fromUtf8("封装"), QString::fromUtf8("继承"), QString::fromUtf8("多态"), QString::fromUtf8("重载")},
                   "ABC", QString::fromUtf8("计算机基础"), Constants::DiffEasy);

    insertQuestion(Constants::TypeMultipleChoice, QString::fromUtf8("以下哪些是HTTP请求方法？"),
                   QStringList{QString::fromUtf8("GET"), QString::fromUtf8("POST"), QString::fromUtf8("DELETE"), QString::fromUtf8("CONNECT")},
                   "ABCD", QString::fromUtf8("计算机网络"), Constants::DiffEasy);

    insertQuestion(Constants::TypeMultipleChoice, QString::fromUtf8("以下哪些是Qt的信号与槽连接方式？"),
                   QStringList{QString::fromUtf8("Qt::AutoConnection"), QString::fromUtf8("Qt::DirectConnection"), QString::fromUtf8("Qt::QueuedConnection"), QString::fromUtf8("Qt::BlockingQueuedConnection")},
                   "ABCD", QString::fromUtf8("Qt开发"), Constants::DiffMedium);

    // 判断题
    insertQuestion(Constants::TypeTrueFalse, QString::fromUtf8("C++中，虚函数可以在构造函数中调用。"),
                   QStringList{QString::fromUtf8("正确"), QString::fromUtf8("错误")},
                   "B", QString::fromUtf8("计算机基础"), Constants::DiffEasy);

    insertQuestion(Constants::TypeTrueFalse, QString::fromUtf8("SQLite是一种关系型数据库。"),
                   QStringList{QString::fromUtf8("正确"), QString::fromUtf8("错误")},
                   "A", QString::fromUtf8("数据库"), Constants::DiffEasy);

    insertQuestion(Constants::TypeTrueFalse, QString::fromUtf8("Qt的QThreadPool默认最大线程数等于CPU核心数。"),
                   QStringList{QString::fromUtf8("正确"), QString::fromUtf8("错误")},
                   "A", QString::fromUtf8("Qt开发"), Constants::DiffMedium);

    // 插入更多题目以确保随机抽题有足够数据
    insertQuestion(Constants::TypeSingleChoice, QString::fromUtf8("以下哪个不是C++的标准容器？"),
                   QStringList{QString::fromUtf8("vector"), QString::fromUtf8("list"), QString::fromUtf8("array"), QString::fromUtf8("map")},
                   "C", QString::fromUtf8("计算机基础"), Constants::DiffEasy);

    insertQuestion(Constants::TypeSingleChoice, QString::fromUtf8("以下哪个协议用于电子邮件传输？"),
                   QStringList{QString::fromUtf8("HTTP"), QString::fromUtf8("FTP"), QString::fromUtf8("SMTP"), QString::fromUtf8("DNS")},
                   "C", QString::fromUtf8("计算机网络"), Constants::DiffEasy);

    insertQuestion(Constants::TypeSingleChoice, QString::fromUtf8("SQL中，用于删除表中所有数据的语句是？"),
                   QStringList{QString::fromUtf8("DELETE"), QString::fromUtf8("DROP"), QString::fromUtf8("TRUNCATE"), QString::fromUtf8("REMOVE")},
                   "C", QString::fromUtf8("数据库"), Constants::DiffMedium);

    insertQuestion(Constants::TypeMultipleChoice, QString::fromUtf8("以下哪些是Qt的模块？"),
                   QStringList{QString::fromUtf8("QtCore"), QString::fromUtf8("QtGui"), QString::fromUtf8("QtNetwork"), QString::fromUtf8("QtCharts")},
                   "ABCD", QString::fromUtf8("Qt开发"), Constants::DiffEasy);

    insertQuestion(Constants::TypeTrueFalse, QString::fromUtf8("TCP是面向连接的协议，UDP是无连接的协议。"),
                   QStringList{QString::fromUtf8("正确"), QString::fromUtf8("错误")},
                   "A", QString::fromUtf8("计算机网络"), Constants::DiffEasy);

    // 补充题目：确保随机抽题可满足 15 单选 + 5 多选 + 5 判断 = 25 题 / 50 分
    // 单选题（新增 8 道）
    insertQuestion(Constants::TypeSingleChoice, QString::fromUtf8("C++中，用于动态内存分配的关键字是？"),
                   QStringList{QString::fromUtf8("new"), QString::fromUtf8("malloc"), QString::fromUtf8("alloc"), QString::fromUtf8("free")},
                   "A", QString::fromUtf8("计算机基础"), Constants::DiffEasy);

    insertQuestion(Constants::TypeSingleChoice, QString::fromUtf8("OSI参考模型共有多少层？"),
                   QStringList{QString::fromUtf8("4"), QString::fromUtf8("5"), QString::fromUtf8("6"), QString::fromUtf8("7")},
                   "D", QString::fromUtf8("计算机网络"), Constants::DiffEasy);

    insertQuestion(Constants::TypeSingleChoice, QString::fromUtf8("HTTP状态码 404 表示？"),
                   QStringList{QString::fromUtf8("服务器内部错误"), QString::fromUtf8("未授权"), QString::fromUtf8("请求资源未找到"), QString::fromUtf8("重定向")},
                   "C", QString::fromUtf8("计算机网络"), Constants::DiffEasy);

    insertQuestion(Constants::TypeSingleChoice, QString::fromUtf8("SQL中，用于查询数据的关键字是？"),
                   QStringList{QString::fromUtf8("INSERT"), QString::fromUtf8("UPDATE"), QString::fromUtf8("SELECT"), QString::fromUtf8("DELETE")},
                   "C", QString::fromUtf8("数据库"), Constants::DiffEasy);

    insertQuestion(Constants::TypeSingleChoice, QString::fromUtf8("数据库中，主键的主要作用是？"),
                   QStringList{QString::fromUtf8("排序记录"), QString::fromUtf8("唯一标识每条记录"), QString::fromUtf8("加密数据"), QString::fromUtf8("建立索引")},
                   "B", QString::fromUtf8("数据库"), Constants::DiffEasy);

    insertQuestion(Constants::TypeSingleChoice, QString::fromUtf8("二叉树中，一个度为2的节点最多有多少个子节点？"),
                   QStringList{QString::fromUtf8("1"), QString::fromUtf8("2"), QString::fromUtf8("3"), QString::fromUtf8("4")},
                   "B", QString::fromUtf8("算法"), Constants::DiffEasy);

    insertQuestion(Constants::TypeSingleChoice, QString::fromUtf8("以下哪个不是常见的排序算法？"),
                   QStringList{QString::fromUtf8("冒泡排序"), QString::fromUtf8("快速排序"), QString::fromUtf8("哈希排序"), QString::fromUtf8("归并排序")},
                   "C", QString::fromUtf8("算法"), Constants::DiffMedium);

    insertQuestion(Constants::TypeSingleChoice, QString::fromUtf8("Qt中，提供信号与槽、元对象系统等核心机制的模块是？"),
                   QStringList{QString::fromUtf8("QtCore"), QString::fromUtf8("QtGui"), QString::fromUtf8("QtNetwork"), QString::fromUtf8("QtWidgets")},
                   "A", QString::fromUtf8("Qt开发"), Constants::DiffEasy);

    // 多选题（新增 3 道）
    insertQuestion(Constants::TypeMultipleChoice, QString::fromUtf8("以下哪些属于数据库对象？"),
                   QStringList{QString::fromUtf8("表"), QString::fromUtf8("视图"), QString::fromUtf8("索引"), QString::fromUtf8("触发器")},
                   "ABCD", QString::fromUtf8("数据库"), Constants::DiffMedium);

    insertQuestion(Constants::TypeMultipleChoice, QString::fromUtf8("算法的基本特性包括？"),
                   QStringList{QString::fromUtf8("有穷性"), QString::fromUtf8("可读性"), QString::fromUtf8("确定性"), QString::fromUtf8("可行性")},
                   "ACD", QString::fromUtf8("算法"), Constants::DiffMedium);

    insertQuestion(Constants::TypeMultipleChoice, QString::fromUtf8("以下哪些是C++的访问控制修饰符？"),
                   QStringList{QString::fromUtf8("public"), QString::fromUtf8("private"), QString::fromUtf8("protected"), QString::fromUtf8("friendly")},
                   "ABC", QString::fromUtf8("计算机基础"), Constants::DiffEasy);

    // 判断题（新增 3 道）
    insertQuestion(Constants::TypeTrueFalse, QString::fromUtf8("二叉搜索树中，左子树上所有节点的值均小于根节点的值。"),
                   QStringList{QString::fromUtf8("正确"), QString::fromUtf8("错误")},
                   "A", QString::fromUtf8("算法"), Constants::DiffMedium);

    insertQuestion(Constants::TypeTrueFalse, QString::fromUtf8("C++中，构造函数可以被继承。"),
                   QStringList{QString::fromUtf8("正确"), QString::fromUtf8("错误")},
                   "B", QString::fromUtf8("计算机基础"), Constants::DiffEasy);

    insertQuestion(Constants::TypeTrueFalse, QString::fromUtf8("Qt的信号与槽机制只能在同一线程中使用。"),
                   QStringList{QString::fromUtf8("正确"), QString::fromUtf8("错误")},
                   "B", QString::fromUtf8("Qt开发"), Constants::DiffMedium);

    // 插入默认考试配置
    query.prepare("INSERT INTO exams (name, question_count, duration, categories) VALUES (?, ?, ?, ?)");
    query.addBindValue(QString::fromUtf8("综合测试"));
    query.addBindValue(25);
    query.addBindValue(30);
    QJsonArray cats;
    cats.append(QString::fromUtf8("计算机基础"));
    cats.append(QString::fromUtf8("计算机网络"));
    cats.append(QString::fromUtf8("数据库"));
    cats.append(QString::fromUtf8("算法"));
    cats.append(QString::fromUtf8("Qt开发"));
    query.addBindValue(QString::fromUtf8(QJsonDocument(cats).toJson(QJsonDocument::Compact)));
    query.exec();

    return true;
}

User DatabaseManager::getUserByUsername(const QString &username)
{
    QMutexLocker locker(&m_mutex);
    QSqlQuery query(m_db);
    query.prepare("SELECT id, username, password, role, real_name FROM users WHERE username = ?");
    query.addBindValue(username);

    User user;
    if (query.exec() && query.next()) {
        user.setId(query.value(0).toInt());
        user.setUsername(query.value(1).toString());
        user.setPassword(query.value(2).toString());
        user.setRole(static_cast<Constants::UserRole>(query.value(3).toInt()));
        user.setRealName(query.value(4).toString());
    }
    return user;
}

User DatabaseManager::getUserById(int id)
{
    QMutexLocker locker(&m_mutex);
    QSqlQuery query(m_db);
    query.prepare("SELECT id, username, password, role, real_name FROM users WHERE id = ?");
    query.addBindValue(id);

    User user;
    if (query.exec() && query.next()) {
        user.setId(query.value(0).toInt());
        user.setUsername(query.value(1).toString());
        user.setPassword(query.value(2).toString());
        user.setRole(static_cast<Constants::UserRole>(query.value(3).toInt()));
        user.setRealName(query.value(4).toString());
    }
    return user;
}

QList<User> DatabaseManager::getAllUsers()
{
    QMutexLocker locker(&m_mutex);
    QList<User> users;
    QSqlQuery query(m_db);
    query.prepare("SELECT id, username, password, role, real_name FROM users ORDER BY id");

    if (query.exec()) {
        while (query.next()) {
            User user;
            user.setId(query.value(0).toInt());
            user.setUsername(query.value(1).toString());
            user.setPassword(query.value(2).toString());
            user.setRole(static_cast<Constants::UserRole>(query.value(3).toInt()));
            user.setRealName(query.value(4).toString());
            users.append(user);
        }
    }
    return users;
}

bool DatabaseManager::addUser(const User &user)
{
    QMutexLocker locker(&m_mutex);
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO users (username, password, role, real_name) VALUES (?, ?, ?, ?)");
    query.addBindValue(user.username());
    query.addBindValue(user.password());
    query.addBindValue(static_cast<int>(user.role()));
    query.addBindValue(user.realName());
    return query.exec();
}

bool DatabaseManager::updateUser(const User &user)
{
    QMutexLocker locker(&m_mutex);
    QSqlQuery query(m_db);
    query.prepare("UPDATE users SET username = ?, password = ?, role = ?, real_name = ? WHERE id = ?");
    query.addBindValue(user.username());
    query.addBindValue(user.password());
    query.addBindValue(static_cast<int>(user.role()));
    query.addBindValue(user.realName());
    query.addBindValue(user.id());
    return query.exec();
}

bool DatabaseManager::deleteUser(int id)
{
    QMutexLocker locker(&m_mutex);
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM users WHERE id = ?");
    query.addBindValue(id);
    return query.exec();
}

Question DatabaseManager::questionFromQuery(const QSqlQuery &query) const
{
    Question q;
    q.setId(query.value(0).toInt());
    q.setType(static_cast<Constants::QuestionType>(query.value(1).toInt()));
    q.setContent(query.value(2).toString());

    QJsonArray optionsArray = QJsonDocument::fromJson(query.value(3).toString().toUtf8()).array();
    QStringList options;
    options.reserve(optionsArray.size());
    for (const QJsonValue &val : optionsArray) {
        options.append(val.toString());
    }
    q.setOptions(options);

    q.setAnswer(query.value(4).toString());
    q.setCategory(query.value(5).toString());
    q.setDifficulty(static_cast<Constants::Difficulty>(query.value(6).toInt()));
    return q;
}

Question DatabaseManager::getQuestionById(int id)
{
    QMutexLocker locker(&m_mutex);
    QSqlQuery query(m_db);
    query.prepare("SELECT id, type, content, options, answer, category, difficulty FROM questions WHERE id = ?");
    query.addBindValue(id);

    Question q;
    if (query.exec() && query.next()) {
        q = questionFromQuery(query);
    }
    return q;
}

QList<Question> DatabaseManager::getAllQuestions()
{
    QMutexLocker locker(&m_mutex);
    QList<Question> questions;
    QSqlQuery query(m_db);
    query.prepare("SELECT id, type, content, options, answer, category, difficulty FROM questions ORDER BY id");

    if (query.exec()) {
        while (query.next()) {
            questions.append(questionFromQuery(query));
        }
    }
    return questions;
}

QList<Question> DatabaseManager::getQuestionsByCategory(const QString &category)
{
    QMutexLocker locker(&m_mutex);
    QList<Question> questions;
    QSqlQuery query(m_db);
    query.prepare("SELECT id, type, content, options, answer, category, difficulty FROM questions WHERE category = ? ORDER BY id");
    query.addBindValue(category);

    if (query.exec()) {
        while (query.next()) {
            questions.append(questionFromQuery(query));
        }
    }
    return questions;
}

QList<Question> DatabaseManager::getRandomQuestions(int count, const QString &category)
{
    QMutexLocker locker(&m_mutex);
    QList<Question> questions;
    QSqlQuery query(m_db);

    if (category.isEmpty()) {
        query.prepare("SELECT id, type, content, options, answer, category, difficulty FROM questions ORDER BY RANDOM() LIMIT ?");
        query.addBindValue(count);
    } else {
        query.prepare("SELECT id, type, content, options, answer, category, difficulty FROM questions WHERE category = ? ORDER BY RANDOM() LIMIT ?");
        query.addBindValue(category);
        query.addBindValue(count);
    }

    if (query.exec()) {
        while (query.next()) {
            questions.append(questionFromQuery(query));
        }
    }
    return questions;
}

QList<Question> DatabaseManager::getExamQuestions(const QString &category)
{
    QMutexLocker locker(&m_mutex);

    // 固定题型组成：15 单选(2分) + 5 多选(3分) + 5 判断(1分) = 25 题 50 分
    // 该组成保证每次抽题总分相同，且各题型从独立池子中随机抽取，避免重复。
    const int singleChoiceCount = 15;
    const int multipleChoiceCount = 5;
    const int trueFalseCount = 5;

    QList<Question> result;
    result.reserve(singleChoiceCount + multipleChoiceCount + trueFalseCount);

    QSet<int> selectedIds;
    QSet<QString> selectedContents;

    auto pickByType = [&](Constants::QuestionType type, int needCount) -> int {
        QSqlQuery query(m_db);
        if (category.isEmpty()) {
            query.prepare("SELECT id, type, content, options, answer, category, difficulty FROM questions WHERE type = ? ORDER BY RANDOM()");
            query.addBindValue(static_cast<int>(type));
        } else {
            query.prepare("SELECT id, type, content, options, answer, category, difficulty FROM questions WHERE type = ? AND category = ? ORDER BY RANDOM()");
            query.addBindValue(static_cast<int>(type));
            query.addBindValue(category);
        }

        int picked = 0;
        if (!query.exec()) {
            return picked;
        }

        while (query.next() && picked < needCount) {
            Question q = questionFromQuery(query);
            // 同时按 ID 和内容去重，防止题库中有重复记录进入同一场考试
            if (selectedIds.contains(q.id()) || selectedContents.contains(q.content())) {
                continue;
            }
            result.append(q);
            selectedIds.insert(q.id());
            selectedContents.insert(q.content());
            ++picked;
        }
        return picked;
    };

    int gotSingle = pickByType(Constants::TypeSingleChoice, singleChoiceCount);
    int gotMultiple = pickByType(Constants::TypeMultipleChoice, multipleChoiceCount);
    int gotTrueFalse = pickByType(Constants::TypeTrueFalse, trueFalseCount);

    // 若任何一类题目数量不足，无法保证固定总分，返回空列表，由调用方提示题库不足
    if (gotSingle != singleChoiceCount || gotMultiple != multipleChoiceCount || gotTrueFalse != trueFalseCount) {
        return QList<Question>();
    }

    // 打乱最终顺序，避免题型按固定顺序出现
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(result.begin(), result.end(), g);

    return result;
}

int DatabaseManager::getExamDuration(int examId)
{
    QMutexLocker locker(&m_mutex);
    QSqlQuery query(m_db);
    query.prepare("SELECT duration FROM exams WHERE id = ?");
    query.addBindValue(examId);

    if (query.exec() && query.next()) {
        int duration = query.value(0).toInt();
        if (duration > 0) {
            return duration;
        }
    }
    return Constants::DEFAULT_EXAM_DURATION;
}

bool DatabaseManager::addQuestion(const Question &question)
{
    QMutexLocker locker(&m_mutex);
    QSqlQuery query(m_db);

    QJsonArray optionsArray;
    for (const QString &opt : question.options()) {
        optionsArray.append(opt);
    }

    query.prepare("INSERT INTO questions (type, content, options, answer, category, difficulty) VALUES (?, ?, ?, ?, ?, ?)");
    query.addBindValue(static_cast<int>(question.type()));
    query.addBindValue(question.content());
    query.addBindValue(QString::fromUtf8(QJsonDocument(optionsArray).toJson(QJsonDocument::Compact)));
    query.addBindValue(question.answer());
    query.addBindValue(question.category());
    query.addBindValue(static_cast<int>(question.difficulty()));
    return query.exec();
}

bool DatabaseManager::updateQuestion(const Question &question)
{
    QMutexLocker locker(&m_mutex);
    QSqlQuery query(m_db);

    QJsonArray optionsArray;
    for (const QString &opt : question.options()) {
        optionsArray.append(opt);
    }

    query.prepare("UPDATE questions SET type = ?, content = ?, options = ?, answer = ?, category = ?, difficulty = ? WHERE id = ?");
    query.addBindValue(static_cast<int>(question.type()));
    query.addBindValue(question.content());
    query.addBindValue(QString::fromUtf8(QJsonDocument(optionsArray).toJson(QJsonDocument::Compact)));
    query.addBindValue(question.answer());
    query.addBindValue(question.category());
    query.addBindValue(static_cast<int>(question.difficulty()));
    query.addBindValue(question.id());
    return query.exec();
}

bool DatabaseManager::deleteQuestion(int id)
{
    QMutexLocker locker(&m_mutex);
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM questions WHERE id = ?");
    query.addBindValue(id);
    return query.exec();
}

QList<QString> DatabaseManager::getAllCategories()
{
    QMutexLocker locker(&m_mutex);
    QList<QString> categories;
    QSqlQuery query(m_db);
    query.prepare("SELECT DISTINCT category FROM questions ORDER BY category");

    if (query.exec()) {
        while (query.next()) {
            categories.append(query.value(0).toString());
        }
    }
    return categories;
}

ExamRecord DatabaseManager::examRecordFromQuery(const QSqlQuery &query) const
{
    ExamRecord record;
    record.setId(query.value(0).toInt());
    record.setUserId(query.value(1).toInt());
    record.setExamId(query.value(2).toInt());

    QJsonArray questionArray = QJsonDocument::fromJson(query.value(3).toString().toUtf8()).array();
    QList<int> questionIds;
    questionIds.reserve(questionArray.size());
    for (const QJsonValue &val : questionArray) {
        questionIds.append(val.toInt());
    }
    record.setQuestionIds(questionIds);

    QJsonObject answerObj = QJsonDocument::fromJson(query.value(4).toString().toUtf8()).object();
    QMap<int, QString> answers;
    for (const QString &key : answerObj.keys()) {
        answers[key.toInt()] = answerObj[key].toString();
    }
    record.setAnswers(answers);

    record.setScore(query.value(5).toDouble());
    record.setStartTime(QDateTime::fromString(query.value(6).toString(), Qt::ISODate));
    record.setEndTime(QDateTime::fromString(query.value(7).toString(), Qt::ISODate));
    record.setStatus(static_cast<Constants::ExamStatus>(query.value(8).toInt()));
    return record;
}

ExamRecord DatabaseManager::getExamRecordById(int id)
{
    QMutexLocker locker(&m_mutex);
    QSqlQuery query(m_db);
    query.prepare("SELECT id, user_id, exam_id, questions, answers, score, start_time, end_time, status FROM exam_records WHERE id = ?");
    query.addBindValue(id);

    ExamRecord record;
    if (query.exec() && query.next()) {
        record = examRecordFromQuery(query);
    }
    return record;
}

QList<ExamRecord> DatabaseManager::getExamRecordsByUserId(int userId)
{
    QMutexLocker locker(&m_mutex);
    QList<ExamRecord> records;
    QSqlQuery query(m_db);
    query.prepare("SELECT id, user_id, exam_id, questions, answers, score, start_time, end_time, status FROM exam_records WHERE user_id = ? ORDER BY start_time DESC");
    query.addBindValue(userId);

    if (query.exec()) {
        while (query.next()) {
            records.append(examRecordFromQuery(query));
        }
    }
    return records;
}

QList<ExamRecord> DatabaseManager::getAllExamRecords()
{
    QMutexLocker locker(&m_mutex);
    QList<ExamRecord> records;
    QSqlQuery query(m_db);
    query.prepare("SELECT id, user_id, exam_id, questions, answers, score, start_time, end_time, status FROM exam_records ORDER BY start_time DESC");

    if (query.exec()) {
        while (query.next()) {
            records.append(examRecordFromQuery(query));
        }
    }
    return records;
}

bool DatabaseManager::addExamRecord(ExamRecord &record)
{
    QMutexLocker locker(&m_mutex);
    QSqlQuery query(m_db);

    QJsonArray questionArray;
    for (int qid : record.questionIds()) {
        questionArray.append(qid);
    }

    QJsonObject answerObj;
    const QMap<int, QString> &answers = record.answers();
    for (auto it = answers.begin(); it != answers.end(); ++it) {
        answerObj[QString::number(it.key())] = it.value();
    }

    query.prepare("INSERT INTO exam_records (user_id, exam_id, questions, answers, score, start_time, end_time, status) VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(record.userId());
    query.addBindValue(record.examId());
    query.addBindValue(QString::fromUtf8(QJsonDocument(questionArray).toJson(QJsonDocument::Compact)));
    query.addBindValue(QString::fromUtf8(QJsonDocument(answerObj).toJson(QJsonDocument::Compact)));
    query.addBindValue(record.score());
    query.addBindValue(record.startTime().toString(Qt::ISODate));
    query.addBindValue(record.endTime().toString(Qt::ISODate));
    query.addBindValue(static_cast<int>(record.status()));

    if (query.exec()) {
        record.setId(query.lastInsertId().toInt());
        return true;
    }
    return false;
}

bool DatabaseManager::updateExamRecord(const ExamRecord &record)
{
    QMutexLocker locker(&m_mutex);
    QSqlQuery query(m_db);

    QJsonArray questionArray;
    for (int qid : record.questionIds()) {
        questionArray.append(qid);
    }

    QJsonObject answerObj;
    const QMap<int, QString> &answers = record.answers();
    for (auto it = answers.begin(); it != answers.end(); ++it) {
        answerObj[QString::number(it.key())] = it.value();
    }

    query.prepare("UPDATE exam_records SET user_id = ?, exam_id = ?, questions = ?, answers = ?, score = ?, start_time = ?, end_time = ?, status = ? WHERE id = ?");
    query.addBindValue(record.userId());
    query.addBindValue(record.examId());
    query.addBindValue(QString::fromUtf8(QJsonDocument(questionArray).toJson(QJsonDocument::Compact)));
    query.addBindValue(QString::fromUtf8(QJsonDocument(answerObj).toJson(QJsonDocument::Compact)));
    query.addBindValue(record.score());
    query.addBindValue(record.startTime().toString(Qt::ISODate));
    query.addBindValue(record.endTime().toString(Qt::ISODate));
    query.addBindValue(static_cast<int>(record.status()));
    query.addBindValue(record.id());
    return query.exec();
}

double DatabaseManager::getAverageScore(int examId)
{
    QMutexLocker locker(&m_mutex);
    QSqlQuery query(m_db);

    if (examId > 0) {
        query.prepare("SELECT AVG(score) FROM exam_records WHERE exam_id = ? AND status > 0");
        query.addBindValue(examId);
    } else {
        query.prepare("SELECT AVG(score) FROM exam_records WHERE status > 0");
    }

    if (query.exec() && query.next()) {
        return query.value(0).toDouble();
    }
    return 0.0;
}

int DatabaseManager::getExamCount(int examId)
{
    QMutexLocker locker(&m_mutex);
    QSqlQuery query(m_db);

    if (examId > 0) {
        query.prepare("SELECT COUNT(*) FROM exam_records WHERE exam_id = ? AND status > 0");
        query.addBindValue(examId);
    } else {
        query.prepare("SELECT COUNT(*) FROM exam_records WHERE status > 0");
    }

    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

QMap<QString, int> DatabaseManager::getScoreDistribution(int examId)
{
    QMutexLocker locker(&m_mutex);
    QMap<QString, int> distribution;
    distribution[QString::fromUtf8("优秀")] = 0;  // 90-100
    distribution[QString::fromUtf8("良好")] = 0;  // 80-89
    distribution[QString::fromUtf8("及格")] = 0;  // 60-79
    distribution[QString::fromUtf8("不及格")] = 0; // 0-59

    QSqlQuery query(m_db);
    if (examId > 0) {
        query.prepare("SELECT score FROM exam_records WHERE exam_id = ? AND status > 0");
        query.addBindValue(examId);
    } else {
        query.prepare("SELECT score FROM exam_records WHERE status > 0");
    }

    if (query.exec()) {
        while (query.next()) {
            double score = query.value(0).toDouble();
            if (score >= 90) {
                distribution[QString::fromUtf8("优秀")]++;
            } else if (score >= 80) {
                distribution[QString::fromUtf8("良好")]++;
            } else if (score >= 60) {
                distribution[QString::fromUtf8("及格")]++;
            } else {
                distribution[QString::fromUtf8("不及格")]++;
            }
        }
    }
    return distribution;
}

QMap<QString, double> DatabaseManager::getAverageScoreByCategory()
{
    QMutexLocker locker(&m_mutex);
    QMap<QString, double> result;

    // 一次性读取题目 id -> 分类映射
    QMap<int, QString> questionCategoryMap;
    QSqlQuery catQuery(m_db);
    catQuery.prepare("SELECT id, category FROM questions");
    if (catQuery.exec()) {
        while (catQuery.next()) {
            questionCategoryMap[catQuery.value(0).toInt()] = catQuery.value(1).toString();
        }
    }

    if (questionCategoryMap.isEmpty()) {
        return result;
    }

    // 一次性读取所有已提交考试记录
    QSqlQuery recordQuery(m_db);
    recordQuery.prepare("SELECT questions, score FROM exam_records WHERE status > 0");
    if (!recordQuery.exec()) {
        return result;
    }

    QMap<QString, double> scoreSums;
    QMap<QString, int> counts;

    while (recordQuery.next()) {
        double score = recordQuery.value(1).toDouble();
        QJsonArray questionArray = QJsonDocument::fromJson(recordQuery.value(0).toString().toUtf8()).array();

        // 同一记录包含多道同分类题目时只统计一次该分类
        QSet<QString> countedCategories;
        for (const QJsonValue &val : questionArray) {
            int qid = val.toInt();
            auto it = questionCategoryMap.find(qid);
            if (it != questionCategoryMap.end()) {
                const QString &category = it.value();
                if (!countedCategories.contains(category)) {
                    scoreSums[category] += score;
                    counts[category] += 1;
                    countedCategories.insert(category);
                }
            }
        }
    }

    for (auto it = scoreSums.begin(); it != scoreSums.end(); ++it) {
        result[it.key()] = it.value() / counts[it.key()];
    }

    return result;
}
