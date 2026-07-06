#include "main_window.h"

#include <QLabel>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("Train Ticket System"));
    resize(960, 640);

    auto *centralWidget = new QWidget(this);
    auto *layout = new QVBoxLayout(centralWidget);
    auto *label = new QLabel(QStringLiteral("Train Ticket System skeleton is ready."), centralWidget);

    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);

    setCentralWidget(centralWidget);
    statusBar()->showMessage(QStringLiteral("Qt Creator + CMake bootstrap build"));
}
