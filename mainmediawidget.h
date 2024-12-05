#ifndef MAINMEDIAWIDGET_H
#define MAINMEDIAWIDGET_H

#include <QWidget>

namespace Ui {
class mainMediaWidget;
}

class mainMediaWidget : public QWidget
{
    Q_OBJECT

public:
    explicit mainMediaWidget(QWidget *parent = nullptr);
    ~mainMediaWidget();

private:
    Ui::mainMediaWidget *ui;
};

#endif // MAINMEDIAWIDGET_H
