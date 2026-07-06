#pragma once

class QString;

class LoginManager
{
public:
    bool authenticate(const QString &username, const QString &password) const;
};
