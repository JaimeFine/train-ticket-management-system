#include <QApplication>
#include <QFont>

#include "database_manager.h"
#include "login_dialog.h"
#include "login_manager.h"
#include "main_window.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    // 设置中文字体，让界面文字清楚一点。
    app.setFont(QFont(QStringLiteral("Microsoft YaHei UI"), 10));

    // 先准备数据库对象，再交给登录管理类使用。
    DatabaseManager databaseManager;
    databaseManager.initialize();

    LoginManager loginManager(&databaseManager);
    LoginDialog loginDialog(loginManager);

    if (loginDialog.exec() != QDialog::Accepted) {
        return 0;
    }

    MainWindow window(loginDialog.loginResult());
    window.show();

    return app.exec();
}
