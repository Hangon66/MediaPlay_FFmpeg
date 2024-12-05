#include "mainmediawidget.h"
#include "ui_mainmediawidget.h"

mainMediaWidget::mainMediaWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::mainMediaWidget)
{
    ui->setupUi(this);
}

mainMediaWidget::~mainMediaWidget()
{
    delete ui;
}
