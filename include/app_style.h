#ifndef APP_STYLE_H
#define APP_STYLE_H

#include <QString>

class QTableWidget;

namespace UiStyle {

QString dialogStyleSheet();
void prepareTable(QTableWidget *table);

}

#endif