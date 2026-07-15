#ifndef APP_STYLE_H
#define APP_STYLE_H

#include <QString>

class QTableWidget;

namespace UiStyle {

// 弹窗共用的控件样式，颜色和交互状态只在一处维护。
QString dialogStyleSheet();

// 统一表格的选择方式、行高、文字位置和焦点显示。
void prepareTable(QTableWidget *table);

}

#endif
