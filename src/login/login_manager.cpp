#include "login_manager.h"

#include <QString>

bool LoginManager::authenticate(const QString &username, const QString &password) const
{
    return !username.trimmed().isEmpty() && !password.isEmpty();
}
