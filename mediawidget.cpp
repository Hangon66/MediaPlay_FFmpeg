#include "mediawidget.h"
#include "ui_mediawidget.h"

mediaWidget::mediaWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::mediaWidget)
{
    ui->setupUi(this);
}

mediaWidget::~mediaWidget()
{
    delete ui;
}
