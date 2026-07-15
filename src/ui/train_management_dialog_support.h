#pragma once

#include <QComboBox>

class QDialog;
class QPaintEvent;

class StationComboBox : public QComboBox
{
public:
    using QComboBox::QComboBox;

protected:
    void paintEvent(QPaintEvent *event) override;
};

void styleTrainEditDialog(QDialog &dialog);
