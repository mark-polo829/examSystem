#include "RequestHandler.h"
#include "JsonHelper.h"
#include "DatabaseManager.h"
#include <QCryptographicHash>
#include <QDebug>
#include <QUrlQuery>

RequestHandler::RequestHandler(qintptr socketDescriptor)
    : m_socketDescriptor(socketDescriptor), m_socket(nullptr)
{
}

RequestHandler::~RequestHandler()
{
    if (m_socket) {
        m_socket->close();
        delete m_socket;
    }
}

void RequestHandler::run()
{
    // 在工作线程中创建 QTcpSocket，避免跨线程 QSocketNotifier 警告
    m_socket = new QTcpSocket();
    if (!m_socket->setSocketDescriptor(m_socketDescriptor)) {
        qDebug() << "Failed to set socket descriptor:"
                 << m_socket->errorString();
        delete m_socket;
        m_socket = nullptr;
        return;
    }

    if (!m_socket->waitForReadyRead(30000)) {
        m_socket->close();
        delete m_socket;
        m_socket = nullptr;
        return;
    }

    QByteArray requestData = m_socket->readAll();
    while (m_socket->waitForReadyRead(100)) {
        requestData += m_socket->readAll();
    }

    HttpRequest request = parseRequest(requestData);
    HttpResponse response = handleRequest(request);
    sendResponse(response);

    m_socket->close();
    delete m_socket;
    m_socket = nullptr;
}

HttpRequest RequestHandler::parseRequest(const QByteArray &data)
{
    HttpRequest request;
    QString requestStr = QString::fromUtf8(data);
    QStringList lines = requestStr.split("\r\n");

    if (lines.isEmpty()) {
        return request;
    }

    // 解析请求行
    QStringList requestLine = lines[0].split(' ');
    if (requestLine.size() >= 2) {
        request.method = requestLine[0];
        QString fullPath = requestLine[1];
        request.version = requestLine.size() >= 3 ? requestLine[2] : "HTTP/1.1";

        // 分离路径和查询参数
        QUrl url(fullPath);
        request.path = url.path();
        QUrlQuery query(url);
        for (const auto &pair : query.queryItems()) {
            request.queryParams[pair.first] = pair.second;
        }
    }

    // 解析头部
    int i = 1;
    for (; i < lines.size(); ++i) {
        if (lines[i].isEmpty()) {
            ++i;
            break;
        }
        int colonIndex = lines[i].indexOf(':');
        if (colonIndex > 0) {
            QString key = lines[i].left(colonIndex).trimmed();
            QString value = lines[i].mid(colonIndex + 1).trimmed();
            request.headers[key] = value;
        }
    }

    // 解析请求体
    if (i < lines.size()) {
        QStringList bodyLines;
        for (; i < lines.size(); ++i) {
            bodyLines.append(lines[i]);
        }
        request.bodyString = bodyLines.join("\r\n");
        request.body = request.bodyString.toUtf8();
    }

    return request;
}

void RequestHandler::sendResponse(const HttpResponse &response)
{
    if (!m_socket || !m_socket->isOpen()) {
        return;
    }

    QString httpResponse = QString("HTTP/1.1 %1 %2\r\n")
        .arg(response.statusCode)
        .arg(response.statusText);

    // 添加默认头部
    httpResponse += "Content-Type: application/json; charset=utf-8\r\n";
    httpResponse += QString("Content-Length: %1\r\n").arg(response.body.size());
    httpResponse += "Connection: close\r\n";

    // 添加自定义头部
    for (auto it = response.headers.begin(); it != response.headers.end(); ++it) {
        httpResponse += QString("%1: %2\r\n").arg(it.key(), it.value());
    }

    httpResponse += "\r\n";

    m_socket->write(httpResponse.toUtf8());
    m_socket->write(response.body);

    // 循环等待所有数据发送完成，避免大响应被截断
    while (m_socket->bytesToWrite() > 0 && m_socket->waitForBytesWritten(5000)) {
        // 继续等待直到缓冲区清空或超时
    }
}

HttpResponse RequestHandler::handleRequest(const HttpRequest &request)
{
    HttpResponse response;

    // 添加CORS头部
    response.headers["Access-Control-Allow-Origin"] = "*";
    response.headers["Access-Control-Allow-Methods"] = "GET, POST, PUT, DELETE, OPTIONS";
    response.headers["Access-Control-Allow-Headers"] = "Content-Type, Authorization";

    // OPTIONS请求直接返回
    if (request.method == "OPTIONS") {
        response.statusCode = 204;
        response.statusText = "No Content";
        return response;
    }

    // 路由分发
    QString path = request.path;

    if (path == "/api/login" && request.method == "POST") {
        return handleLogin(request);
    }
    else if (path == "/api/questions/random" && request.method == "GET") {
        return handleGetRandomQuestions(request);
    }
    else if (path == "/api/exam/submit" && request.method == "POST") {
        return handleSubmitExam(request);
    }
    else if (path == "/api/exam/result" && request.method == "GET") {
        return handleGetExamResult(request);
    }
    else if (path == "/api/exam/records" && request.method == "GET") {
        return handleGetUserExamRecords(request);
    }
    else if (path == "/api/questions" && request.method == "GET") {
        return handleGetQuestions(request);
    }
    else if (path == "/api/questions" && request.method == "POST") {
        return handleAddQuestion(request);
    }
    else if (path.startsWith("/api/questions/") && request.method == "PUT") {
        int id = path.mid(QString("/api/questions/").size()).toInt();
        return handleUpdateQuestion(request, id);
    }
    else if (path.startsWith("/api/questions/") && request.method == "DELETE") {
        int id = path.mid(QString("/api/questions/").size()).toInt();
        return handleDeleteQuestion(request, id);
    }
    else if (path == "/api/scores/statistics" && request.method == "GET") {
        return handleGetScoreStatistics(request);
    }
    else if (path == "/api/users" && request.method == "GET") {
        return handleGetUsers(request);
    }
    else if (path == "/api/users" && request.method == "POST") {
        return handleAddUser(request);
    }
    else if (path.startsWith("/api/users/") && request.method == "PUT") {
        int id = path.mid(QString("/api/users/").size()).toInt();
        return handleUpdateUser(request, id);
    }
    else if (path.startsWith("/api/users/") && request.method == "DELETE") {
        int id = path.mid(QString("/api/users/").size()).toInt();
        return handleDeleteUser(request, id);
    }
    else if (path == "/api/categories" && request.method == "GET") {
        return handleGetCategories(request);
    }

    // 404 Not Found
    response.statusCode = 404;
    response.statusText = "Not Found";
    response.body = JsonHelper::toJsonBytes(JsonHelper::createErrorResponse(QString::fromUtf8("接口不存在")));
    return response;
}

QJsonObject RequestHandler::parseBody(const HttpRequest &request)
{
    return JsonHelper::parseJsonObject(request.body);
}

QString RequestHandler::getQueryParam(const HttpRequest &request, const QString &key)
{
    return request.queryParams.value(key);
}

bool RequestHandler::checkAuth(const HttpRequest &request, Constants::UserRole minRole)
{
    // 简化版：从Authorization头部获取用户ID
    // 实际生产环境应使用JWT等Token验证
    QString auth = request.headers.value("Authorization");
    if (auth.isEmpty()) {
        return false;
    }

    // 格式: "Bearer userId"
    if (auth.startsWith("Bearer ")) {
        int userId = auth.mid(7).toInt();
        if (userId > 0) {
            User user = DatabaseManager::instance()->getUserById(userId);
            return user.id() > 0 && user.role() >= minRole;
        }
    }
    return false;
}

int RequestHandler::getUserIdFromToken(const HttpRequest &request)
{
    QString auth = request.headers.value("Authorization");
    if (auth.startsWith("Bearer ")) {
        return auth.mid(7).toInt();
    }
    return 0;
}

HttpResponse RequestHandler::handleLogin(const HttpRequest &request)
{
    QJsonObject body = parseBody(request);
    QString username = JsonHelper::getString(body, "username");
    QString password = JsonHelper::getString(body, "password");

    if (username.isEmpty() || password.isEmpty()) {
        HttpResponse response(400, "Bad Request");
        response.body = JsonHelper::toJsonBytes(JsonHelper::createErrorResponse(QString::fromUtf8("用户名或密码不能为空")));
        return response;
    }

    User user = DatabaseManager::instance()->getUserByUsername(username);
    if (user.id() == 0) {
        HttpResponse response(401, "Unauthorized");
        response.body = JsonHelper::toJsonBytes(JsonHelper::createErrorResponse(QString::fromUtf8("用户不存在")));
        return response;
    }

    // 验证密码（MD5）
    QString hashedPwd = QString::fromUtf8(QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Md5).toHex());
    if (user.password() != hashedPwd) {
        HttpResponse response(401, "Unauthorized");
        response.body = JsonHelper::toJsonBytes(JsonHelper::createErrorResponse(QString::fromUtf8("密码错误")));
        return response;
    }

    // 生成Token（简化版：使用用户ID作为Token）
    QJsonObject data = user.toJson();
    data["token"] = QString::number(user.id());

    qDebug() << "[handleLogin] username:" << username
             << "role:" << user.role()
             << "responseData:" << data;

    HttpResponse response(200, "OK");
    response.body = JsonHelper::toJsonBytes(JsonHelper::createSuccessResponse(QString::fromUtf8("登录成功"), data));
    return response;
}

HttpResponse RequestHandler::handleGetRandomQuestions(const HttpRequest &request)
{
    if (!checkAuth(request, Constants::RoleStudent)) {
        HttpResponse response(401, "Unauthorized");
        response.body = JsonHelper::toJsonBytes(JsonHelper::createErrorResponse(QString::fromUtf8("未授权")));
        return response;
    }

    QString category = getQueryParam(request, "category");

    QList<Question> questions = DatabaseManager::instance()->getExamQuestions(category);

    QJsonArray questionsArray;
    for (const Question &q : questions) {
        questionsArray.append(q.toJson());
    }

    QJsonObject data;
    data["questions"] = questionsArray;
    data["count"] = questionsArray.size();
    data["duration"] = DatabaseManager::instance()->getExamDuration(1);

    HttpResponse response(200, "OK");
    response.body = JsonHelper::toJsonBytes(JsonHelper::createSuccessResponse(QString::fromUtf8("获取成功"), data));
    return response;
}

HttpResponse RequestHandler::handleSubmitExam(const HttpRequest &request)
{
    if (!checkAuth(request, Constants::RoleStudent)) {
        HttpResponse response(401, "Unauthorized");
        response.body = JsonHelper::toJsonBytes(JsonHelper::createErrorResponse(QString::fromUtf8("未授权")));
        return response;
    }

    QJsonObject body = parseBody(request);
    int userId = getUserIdFromToken(request);
    int examId = JsonHelper::getInt(body, "exam_id", 1);

    QJsonArray questionArray = body["questions"].toArray();
    QJsonObject answerObj = body["answers"].toObject();

    if (questionArray.isEmpty()) {
        HttpResponse response(400, "Bad Request");
        response.body = JsonHelper::toJsonBytes(JsonHelper::createErrorResponse(QString::fromUtf8("题目列表为空")));
        return response;
    }

    // 计算得分
    double totalScore = 0.0;
    QMap<int, QString> answers;

    for (const QJsonValue &val : questionArray) {
        int qid = val.toInt();
        Question q = DatabaseManager::instance()->getQuestionById(qid);
        if (q.id() == 0) continue;

        QString userAnswer = answerObj[QString::number(qid)].toString();
        answers[qid] = userAnswer;

        if (q.checkAnswer(userAnswer)) {
            totalScore += q.score();
        }
    }

    // 保存考试记录
    QList<int> questionIds;
    for (const QJsonValue &val : questionArray) {
        questionIds.append(val.toInt());
    }

    ExamRecord record;
    record.setUserId(userId);
    record.setExamId(examId);
    record.setQuestionIds(questionIds);
    record.setAnswers(answers);
    record.setScore(totalScore);
    QDateTime startTime = QDateTime::fromString(JsonHelper::getString(body, "start_time"), Qt::ISODate);
    record.setStartTime(startTime);

    // 将结束时间限制在考试限定时间内，避免考试记录中的用时超过限定时间
    QDateTime endTime = QDateTime::currentDateTime();
    if (startTime.isValid()) {
        int examDuration = DatabaseManager::instance()->getExamDuration(examId);
        QDateTime latestEndTime = startTime.addSecs(examDuration * 60);
        if (endTime > latestEndTime) {
            endTime = latestEndTime;
        }
    }
    record.setEndTime(endTime);

    record.setStatus(Constants::StatusSubmitted);

    if (!DatabaseManager::instance()->addExamRecord(record)) {
        HttpResponse response(500, "Internal Server Error");
        response.body = JsonHelper::toJsonBytes(JsonHelper::createErrorResponse(QString::fromUtf8("保存考试记录失败")));
        return response;
    }

    QJsonObject data;
    data["score"] = totalScore;
    data["record_id"] = record.id();

    HttpResponse response(200, "OK");
    response.body = JsonHelper::toJsonBytes(JsonHelper::createSuccessResponse(QString::fromUtf8("提交成功"), data));
    return response;
}

HttpResponse RequestHandler::handleGetExamResult(const HttpRequest &request)
{
    if (!checkAuth(request, Constants::RoleStudent)) {
        HttpResponse response(401, "Unauthorized");
        response.body = JsonHelper::toJsonBytes(JsonHelper::createErrorResponse(QString::fromUtf8("未授权")));
        return response;
    }

    int userId = getUserIdFromToken(request);
    int recordId = getQueryParam(request, "record_id").toInt();

    ExamRecord record;
    if (recordId > 0) {
        record = DatabaseManager::instance()->getExamRecordById(recordId);
    } else {
        QList<ExamRecord> records = DatabaseManager::instance()->getExamRecordsByUserId(userId);
        if (!records.isEmpty()) {
            record = records.first();
        }
    }

    if (record.id() == 0) {
        HttpResponse response(404, "Not Found");
        response.body = JsonHelper::toJsonBytes(JsonHelper::createErrorResponse(QString::fromUtf8("考试记录不存在")));
        return response;
    }

    HttpResponse response(200, "OK");
    response.body = JsonHelper::toJsonBytes(JsonHelper::createSuccessResponse(QString::fromUtf8("获取成功"), record.toJson()));
    return response;
}

HttpResponse RequestHandler::handleGetUserExamRecords(const HttpRequest &request)
{
    if (!checkAuth(request, Constants::RoleStudent)) {
        HttpResponse response(401, "Unauthorized");
        response.body = JsonHelper::toJsonBytes(JsonHelper::createErrorResponse(QString::fromUtf8("未授权")));
        return response;
    }

    int userId = getUserIdFromToken(request);
    QList<ExamRecord> records = DatabaseManager::instance()->getExamRecordsByUserId(userId);

    QJsonArray recordsArray;
    for (const ExamRecord &record : records) {
        QJsonObject recordObj = record.toJson();

        // 对已完成/已交卷的记录，附加每道题目详情（题目内容、正确答案、考生答案、是否正确）
        if (record.status() != Constants::StatusInProgress) {
            QJsonArray detailsArray;
            for (int qid : record.questionIds()) {
                Question q = DatabaseManager::instance()->getQuestionById(qid);
                if (q.id() == 0) {
                    continue;
                }

                QString userAnswer = record.answers().value(qid, QString::fromUtf8("未作答"));
                QJsonObject qObj = q.toJson();
                qObj["user_answer"] = userAnswer;
                qObj["is_correct"] = q.checkAnswer(userAnswer);
                detailsArray.append(qObj);
            }
            recordObj["question_details"] = detailsArray;
        }

        recordsArray.append(recordObj);
    }

    QJsonObject data;
    data["records"] = recordsArray;
    data["total"] = recordsArray.size();

    HttpResponse response(200, "OK");
    response.body = JsonHelper::toJsonBytes(JsonHelper::createSuccessResponse(QString::fromUtf8("获取成功"), data));
    return response;
}

HttpResponse RequestHandler::handleGetQuestions(const HttpRequest &request)
{
    if (!checkAuth(request, Constants::RoleTeacher)) {
        HttpResponse response(401, "Unauthorized");
        response.body = JsonHelper::toJsonBytes(JsonHelper::createErrorResponse(QString::fromUtf8("需要教师或管理员权限")));
        return response;
    }

    QString category = getQueryParam(request, "category");
    QList<Question> questions;

    if (category.isEmpty()) {
        questions = DatabaseManager::instance()->getAllQuestions();
    } else {
        questions = DatabaseManager::instance()->getQuestionsByCategory(category);
    }

    QJsonArray questionsArray;
    for (const Question &q : questions) {
        questionsArray.append(q.toJson());
    }

    QJsonObject data;
    data["questions"] = questionsArray;
    data["total"] = questionsArray.size();

    HttpResponse response(200, "OK");
    response.body = JsonHelper::toJsonBytes(JsonHelper::createSuccessResponse(QString::fromUtf8("获取成功"), data));
    return response;
}

HttpResponse RequestHandler::handleAddQuestion(const HttpRequest &request)
{
    if (!checkAuth(request, Constants::RoleTeacher)) {
        HttpResponse response(401, "Unauthorized");
        response.body = JsonHelper::toJsonBytes(JsonHelper::createErrorResponse(QString::fromUtf8("需要教师或管理员权限")));
        return response;
    }

    QJsonObject body = parseBody(request);
    Question q = Question::fromJson(body);

    if (q.content().isEmpty() || q.answer().isEmpty()) {
        HttpResponse response(400, "Bad Request");
        response.body = JsonHelper::toJsonBytes(JsonHelper::createErrorResponse(QString::fromUtf8("题目内容或答案不能为空")));
        return response;
    }

    if (!DatabaseManager::instance()->addQuestion(q)) {
        HttpResponse response(500, "Internal Server Error");
        response.body = JsonHelper::toJsonBytes(JsonHelper::createErrorResponse(QString::fromUtf8("添加题目失败")));
        return response;
    }

    HttpResponse response(200, "OK");
    response.body = JsonHelper::toJsonBytes(JsonHelper::createSuccessResponse(QString::fromUtf8("添加成功")));
    return response;
}

HttpResponse RequestHandler::handleUpdateQuestion(const HttpRequest &request, int id)
{
    if (!checkAuth(request, Constants::RoleTeacher)) {
        HttpResponse response(401, "Unauthorized");
        response.body = JsonHelper::toJsonBytes(JsonHelper::createErrorResponse(QString::fromUtf8("需要教师或管理员权限")));
        return response;
    }

    QJsonObject body = parseBody(request);
    Question q = Question::fromJson(body);
    q.setId(id);

    if (!DatabaseManager::instance()->updateQuestion(q)) {
        HttpResponse response(500, "Internal Server Error");
        response.body = JsonHelper::toJsonBytes(JsonHelper::createErrorResponse(QString::fromUtf8("更新题目失败")));
        return response;
    }

    HttpResponse response(200, "OK");
    response.body = JsonHelper::toJsonBytes(JsonHelper::createSuccessResponse(QString::fromUtf8("更新成功")));
    return response;
}

HttpResponse RequestHandler::handleDeleteQuestion(const HttpRequest &request, int id)
{
    if (!checkAuth(request, Constants::RoleTeacher)) {
        HttpResponse response(401, "Unauthorized");
        response.body = JsonHelper::toJsonBytes(JsonHelper::createErrorResponse(QString::fromUtf8("需要教师或管理员权限")));
        return response;
    }

    if (!DatabaseManager::instance()->deleteQuestion(id)) {
        HttpResponse response(500, "Internal Server Error");
        response.body = JsonHelper::toJsonBytes(JsonHelper::createErrorResponse(QString::fromUtf8("删除题目失败")));
        return response;
    }

    HttpResponse response(200, "OK");
    response.body = JsonHelper::toJsonBytes(JsonHelper::createSuccessResponse(QString::fromUtf8("删除成功")));
    return response;
}

HttpResponse RequestHandler::handleGetScoreStatistics(const HttpRequest &request)
{
    if (!checkAuth(request, Constants::RoleTeacher)) {
        HttpResponse response(401, "Unauthorized");
        response.body = JsonHelper::toJsonBytes(JsonHelper::createErrorResponse(QString::fromUtf8("需要教师或管理员权限")));
        return response;
    }

    int examId = getQueryParam(request, "exam_id").toInt();

    double avgScore = DatabaseManager::instance()->getAverageScore(examId);
    int examCount = DatabaseManager::instance()->getExamCount(examId);
    QMap<QString, int> distribution = DatabaseManager::instance()->getScoreDistribution(examId);

    QJsonObject data;
    data["average_score"] = avgScore;
    data["exam_count"] = examCount;

    QJsonObject distObj;
    for (auto it = distribution.begin(); it != distribution.end(); ++it) {
        distObj[it.key()] = it.value();
    }
    data["distribution"] = distObj;

    // 获取所有考试记录
    QList<ExamRecord> records;
    if (examId > 0) {
        records = DatabaseManager::instance()->getAllExamRecords();
        // 过滤指定考试
        QList<ExamRecord> filtered;
        for (const ExamRecord &r : records) {
            if (r.examId() == examId) {
                filtered.append(r);
            }
        }
        records = filtered;
    } else {
        records = DatabaseManager::instance()->getAllExamRecords();
    }

    QJsonArray recordsArray;
    for (const ExamRecord &r : records) {
        QJsonObject recordObj = r.toJson();
        User user = DatabaseManager::instance()->getUserById(r.userId());
        recordObj["user_name"] = user.realName();
        recordsArray.append(recordObj);
    }
    data["records"] = recordsArray;

    HttpResponse response(200, "OK");
    response.body = JsonHelper::toJsonBytes(JsonHelper::createSuccessResponse(QString::fromUtf8("获取成功"), data));
    return response;
}

HttpResponse RequestHandler::handleGetUsers(const HttpRequest &request)
{
    if (!checkAuth(request, Constants::RoleAdmin)) {
        HttpResponse response(401, "Unauthorized");
        response.body = JsonHelper::toJsonBytes(JsonHelper::createErrorResponse(QString::fromUtf8("需要管理员权限")));
        return response;
    }

    QList<User> users = DatabaseManager::instance()->getAllUsers();
    QJsonArray usersArray;
    for (const User &u : users) {
        QJsonObject userObj = u.toJson();
        userObj.remove("password");  // 不返回密码
        usersArray.append(userObj);
    }

    QJsonObject data;
    data["users"] = usersArray;
    data["total"] = usersArray.size();

    HttpResponse response(200, "OK");
    response.body = JsonHelper::toJsonBytes(JsonHelper::createSuccessResponse(QString::fromUtf8("获取成功"), data));
    return response;
}

HttpResponse RequestHandler::handleAddUser(const HttpRequest &request)
{
    if (!checkAuth(request, Constants::RoleAdmin)) {
        HttpResponse response(401, "Unauthorized");
        response.body = JsonHelper::toJsonBytes(JsonHelper::createErrorResponse(QString::fromUtf8("需要管理员权限")));
        return response;
    }

    QJsonObject body = parseBody(request);
    User user;
    user.setUsername(JsonHelper::getString(body, "username"));
    user.setRealName(JsonHelper::getString(body, "real_name"));
    user.setRole(static_cast<Constants::UserRole>(JsonHelper::getInt(body, "role", 0)));

    QString password = JsonHelper::getString(body, "password");
    if (password.isEmpty()) {
        password = "123456";  // 默认密码
    }
    user.setPassword(QString::fromUtf8(QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Md5).toHex()));

    if (user.username().isEmpty()) {
        HttpResponse response(400, "Bad Request");
        response.body = JsonHelper::toJsonBytes(JsonHelper::createErrorResponse(QString::fromUtf8("用户名不能为空")));
        return response;
    }

    if (!DatabaseManager::instance()->addUser(user)) {
        HttpResponse response(500, "Internal Server Error");
        response.body = JsonHelper::toJsonBytes(JsonHelper::createErrorResponse(QString::fromUtf8("添加用户失败")));
        return response;
    }

    HttpResponse response(200, "OK");
    response.body = JsonHelper::toJsonBytes(JsonHelper::createSuccessResponse(QString::fromUtf8("添加成功")));
    return response;
}

HttpResponse RequestHandler::handleUpdateUser(const HttpRequest &request, int id)
{
    if (!checkAuth(request, Constants::RoleAdmin)) {
        HttpResponse response(401, "Unauthorized");
        response.body = JsonHelper::toJsonBytes(JsonHelper::createErrorResponse(QString::fromUtf8("需要管理员权限")));
        return response;
    }

    QJsonObject body = parseBody(request);
    User user = DatabaseManager::instance()->getUserById(id);
    if (user.id() == 0) {
        HttpResponse response(404, "Not Found");
        response.body = JsonHelper::toJsonBytes(JsonHelper::createErrorResponse(QString::fromUtf8("用户不存在")));
        return response;
    }

    if (body.contains("username")) user.setUsername(body["username"].toString());
    if (body.contains("real_name")) user.setRealName(body["real_name"].toString());
    if (body.contains("role")) user.setRole(static_cast<Constants::UserRole>(body["role"].toInt()));

    if (body.contains("password") && !body["password"].toString().isEmpty()) {
        QString password = body["password"].toString();
        user.setPassword(QString::fromUtf8(QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Md5).toHex()));
    }

    if (!DatabaseManager::instance()->updateUser(user)) {
        HttpResponse response(500, "Internal Server Error");
        response.body = JsonHelper::toJsonBytes(JsonHelper::createErrorResponse(QString::fromUtf8("更新用户失败")));
        return response;
    }

    HttpResponse response(200, "OK");
    response.body = JsonHelper::toJsonBytes(JsonHelper::createSuccessResponse(QString::fromUtf8("更新成功")));
    return response;
}

HttpResponse RequestHandler::handleDeleteUser(const HttpRequest &request, int id)
{
    if (!checkAuth(request, Constants::RoleAdmin)) {
        HttpResponse response(401, "Unauthorized");
        response.body = JsonHelper::toJsonBytes(JsonHelper::createErrorResponse(QString::fromUtf8("需要管理员权限")));
        return response;
    }

    if (!DatabaseManager::instance()->deleteUser(id)) {
        HttpResponse response(500, "Internal Server Error");
        response.body = JsonHelper::toJsonBytes(JsonHelper::createErrorResponse(QString::fromUtf8("删除用户失败")));
        return response;
    }

    HttpResponse response(200, "OK");
    response.body = JsonHelper::toJsonBytes(JsonHelper::createSuccessResponse(QString::fromUtf8("删除成功")));
    return response;
}

HttpResponse RequestHandler::handleGetCategories(const HttpRequest &request)
{
    if (!checkAuth(request, Constants::RoleTeacher)) {
        HttpResponse response(401, "Unauthorized");
        response.body = JsonHelper::toJsonBytes(JsonHelper::createErrorResponse(QString::fromUtf8("需要教师或管理员权限")));
        return response;
    }

    QList<QString> categories = DatabaseManager::instance()->getAllCategories();
    QJsonArray categoriesArray;
    for (const QString &cat : categories) {
        categoriesArray.append(cat);
    }

    QJsonObject data;
    data["categories"] = categoriesArray;

    HttpResponse response(200, "OK");
    response.body = JsonHelper::toJsonBytes(JsonHelper::createSuccessResponse(QString::fromUtf8("获取成功"), data));
    return response;
}
