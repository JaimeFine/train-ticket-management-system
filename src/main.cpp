#include <QApplication>
#include <QFont>
#include <QMessageBox>

#include "database_manager.h"
#include "login_dialog.h"
#include "login_manager.h"
#include "main_window.h"
#include "train_management_dialog.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    // 中文字体单独设一下，界面看着清楚。
    app.setFont(QFont(QStringLiteral("Microsoft YaHei UI"), 10));

    // 先准备数据库，再交给登录模块用。
    DatabaseManager databaseManager;
    if (!databaseManager.initialize()) {
        QMessageBox::critical(nullptr,
                              QStringLiteral("数据库初始化失败"),
                              QStringLiteral("无法打开或初始化数据库：\n%1").arg(databaseManager.lastError()));
        return 1;
    }

    LoginManager loginManager(&databaseManager);

    // 每次循环先显示登录窗口，再显示对应身份的主窗口。
    // 用户点“退出登录”时主窗口会结束这一次事件循环，然后回到这里重新登录；
    // 如果用户直接关闭主窗口，就正常结束程序。
    while (true) {
        LoginDialog loginDialog(loginManager);
        if (loginDialog.exec() != QDialog::Accepted) {
            return 0;
        }

        MainWindow window(loginDialog.loginResult(), loginManager);
        window.show();

        const int result = app.exec();
        if (!window.logoutRequested()) {
            return result;
        }
    }
}
