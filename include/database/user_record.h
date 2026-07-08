#pragma once

// Properties of UserRecord

#include <QString>

struct UserRecord {
    int userId = 0;
    QString username;
    QString password;
    int role = 0;
    bool enabled = true;
};
