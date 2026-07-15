#include "train_management_dialog_support.h"

#include <QDialog>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>

void StationComboBox::paintEvent(QPaintEvent *event)
{
    QComboBox::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const qreal arrowAreaWidth = 36.0;

    QPainterPath clipPath;
    clipPath.addRoundedRect(QRectF(0.5, 0.5, width() - 1.0, height() - 1.0),
                            8.0, 8.0);
    painter.setClipPath(clipPath);
    painter.fillRect(QRectF(width() - arrowAreaWidth, 0.0,
                            arrowAreaWidth, height()),
                     QColor(QStringLiteral("#eef5f1")));
    painter.setClipping(false);

    painter.setPen(QPen(QColor(QStringLiteral("#d8e0dc")), 1.0));
    painter.drawLine(QPointF(width() - arrowAreaWidth, 1.0),
                     QPointF(width() - arrowAreaWidth, height() - 1.0));

    painter.setPen(Qt::NoPen);
    painter.setBrush(isEnabled() ? QColor(QStringLiteral("#8fa5a0"))
                                 : QColor(QStringLiteral("#bdc8c4")));

    const qreal centerX = width() - arrowAreaWidth / 2.0;
    const qreal centerY = height() / 2.0;
    QPolygonF arrow;
    arrow << QPointF(centerX - 4.5, centerY - 2.5)
          << QPointF(centerX + 4.5, centerY - 2.5)
          << QPointF(centerX, centerY + 4.0);
    painter.drawPolygon(arrow);

    const qreal borderWidth = hasFocus() ? 2.0 : 1.0;
    const qreal borderOffset = borderWidth / 2.0;
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(hasFocus() ? QColor(QStringLiteral("#0f766e"))
                                   : QColor(QStringLiteral("#cbd8d2")),
                        borderWidth));
    painter.drawRoundedRect(QRectF(borderOffset, borderOffset,
                                   width() - borderWidth,
                                   height() - borderWidth),
                            8.0, 8.0);
}

void styleTrainEditDialog(QDialog &dialog)
{
    dialog.setStyleSheet(
        "QDialog {"
        "    background: #eef2f3;"
        "    color: #1f2933;"
        "    font-family: \"Microsoft YaHei UI\", \"Microsoft YaHei\", \"Segoe UI\";"
        "    font-size: 14px;"
        "}"
        "QLabel {"
        "    color: #33433d;"
        "}"
        "QLineEdit, QComboBox, QSpinBox {"
        "    color: #1f2933;"
        "    background-color: #ffffff;"
        "    border: 1px solid #cbd8d2;"
        "    border-radius: 6px;"
        "    padding: 4px 8px;"
        "    min-height: 28px;"
        "}"
        "QLineEdit::placeholder {"
        "    color: #8b9490;"
        "}"
        "QComboBox::drop-down, QSpinBox::up-button, QSpinBox::down-button {"
        "    border: none;"
        "    background: #eef5f1;"
        "}"
        "QComboBox QAbstractItemView {"
        "    color: #1f2933;"
        "    background-color: #ffffff;"
        "    border: 1px solid #cbd8d2;"
        "    selection-background-color: #d9f99d;"
        "    selection-color: #153832;"
        "}"
        "QPushButton {"
        "    background-color: #176b5b;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 8px;"
        "    padding: 8px 14px;"
        "    font-weight: 700;"
        "}"
        "QPushButton:hover {"
        "    background-color: #0f5749;"
        "}"
    );
}
