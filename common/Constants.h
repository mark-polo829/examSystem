#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QString>

namespace Constants {
    // 服务器配置
    const QString SERVER_HOST = "127.0.0.1";
    const int SERVER_PORT = 8080;
    const QString API_BASE = "http://127.0.0.1:8080/api";

    // 数据库配置
    const QString DB_NAME = "exam.db";
    const QString DB_TYPE = "QSQLITE";

    // 角色定义
    enum UserRole {
        RoleStudent = 0,    // 考生
        RoleTeacher = 1,    // 教师
        RoleAdmin = 2       // 管理员
    };

    // 题型定义
    enum QuestionType {
        TypeSingleChoice = 0,   // 单选
        TypeMultipleChoice = 1, // 多选
        TypeTrueFalse = 2       // 判断
    };

    // 考试状态
    enum ExamStatus {
        StatusInProgress = 0,   // 进行中
        StatusSubmitted = 1,    // 已提交
        StatusAutoSubmit = 2    // 自动交卷
    };

    // 难度定义
    enum Difficulty {
        DiffEasy = 0,   // 易
        DiffMedium = 1, // 中
        DiffHard = 2    // 难
    };

    // 默认考试配置
    const int DEFAULT_QUESTION_COUNT = 25;  // 默认抽题数量（15单选+5多选+5判断=25题50分）
    const int DEFAULT_EXAM_DURATION = 30;   // 默认考试时长（分钟）
}

#endif // CONSTANTS_H
