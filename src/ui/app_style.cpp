#include "app_style.h"

#include <QAbstractItemView>
#include <QHeaderView>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>
#include <QTableWidget>

namespace {

class CenteredItemDelegate : public QStyledItemDelegate
{
public:
    explicit CenteredItemDelegate(QObject *parent)
        : QStyledItemDelegate(parent)
    {
    }

    void initStyleOption(QStyleOptionViewItem *option,
                         const QModelIndex &index) const override
    {
        QStyledItemDelegate::initStyleOption(option, index);
        option->displayAlignment = Qt::AlignCenter;

        // 表格获得焦点时，Qt 默认会画一条蓝色焦点框，这里只保留浅绿色选中效果。
        option->state &= ~QStyle::State_HasFocus;
    }
};

}

namespace UiStyle {

QString dialogStyleSheet()
{
    return QStringLiteral(R"QSS(
        QDialog {
            background: #eef2f3;
            color: #1f2933;
            font-family: "Microsoft YaHei UI", "Microsoft YaHei", "Segoe UI";
            font-size: 14px;
        }
        QLabel {
            color: #1f2933;
        }
        QLabel#titleLabel {
            color: #17231f;
            font-size: 22px;
            font-weight: 700;
        }
        QLabel#hintLabel {
            color: #42514b;
        }
        QLineEdit, QComboBox, QTimeEdit, QSpinBox {
            min-height: 32px;
            border: 1px solid #cbd8d2;
            border-radius: 8px;
            padding: 4px 10px;
            background: #ffffff;
            color: #1f2933;
            selection-background-color: #0f766e;
            selection-color: #ffffff;
        }
        QLineEdit:focus, QComboBox:focus, QTimeEdit:focus, QSpinBox:focus {
            border: 2px solid #0f766e;
            padding: 3px 9px;
        }
        QComboBox#stationSearchCombo {
            padding-right: 40px;
        }
        QComboBox#stationSearchCombo:focus {
            padding-right: 39px;
        }
        QComboBox#stationSearchCombo::drop-down {
            subcontrol-origin: border;
            subcontrol-position: top right;
            width: 36px;
            border: none;
            background: transparent;
        }
        QComboBox#stationSearchCombo::down-arrow {
            image: none;
        }
        QComboBox QAbstractItemView {
            color: #1f2933;
            background: #ffffff;
            border: 1px solid #cbd8d2;
            selection-background-color: #d9f99d;
            selection-color: #153832;
            outline: 0;
        }
        QTableWidget {
            color: #1f2933;
            background: #ffffff;
            alternate-background-color: #f6faf8;
            border: 1px solid #cbd8d2;
            border-radius: 8px;
            gridline-color: #e5ece8;
            selection-background-color: #d9f99d;
            selection-color: #153832;
            outline: 0;
        }
        QTableWidget::item {
            padding: 4px;
        }
        QTableWidget::item:selected {
            color: #153832;
            background: #d9f99d;
        }
        QHeaderView::section {
            background: #eef5f1;
            color: #33433d;
            border: none;
            padding: 7px;
            font-weight: 700;
        }
        QPushButton {
            min-height: 34px;
            border-radius: 8px;
            padding: 6px 16px;
            font-weight: 700;
            color: #ffffff;
            background: #176b5b;
            border: none;
        }
        QPushButton:hover {
            background: #0f5749;
        }
        QPushButton:pressed {
            background: #0b493e;
        }
        QPushButton:disabled {
            color: #f3f6f5;
            background: #9eada7;
        }
        QPushButton#dangerButton {
            background: #b91c1c;
        }
        QPushButton#dangerButton:hover {
            background: #991b1b;
        }
        QPushButton#dangerButton:disabled {
            color: #f3f6f5;
            background: #c5a0a0;
        }
    )QSS");
}

void prepareTable(QTableWidget *table)
{
    if (table == nullptr) {
        return;
    }

    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setAlternatingRowColors(true);
    table->verticalHeader()->setVisible(false);
    table->verticalHeader()->setDefaultSectionSize(38);
    table->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    table->setItemDelegate(new CenteredItemDelegate(table));
}

}
