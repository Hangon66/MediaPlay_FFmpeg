#ifndef MEDIAWIDGET_H
#define MEDIAWIDGET_H

#include <QWidget>

namespace Ui {
class c;
}

class mediaWidget : public QWidget
{
    Q_OBJECT

public:
    explicit mediaWidget(QWidget *parent = nullptr);
    ~mediaWidget();

private:
    Ui::mediaWidget *ui;
};

#endif // MEDIAWIDGET_H
