#include "mainwindow.h"
#include "ui_mainwindow.h"
// ffmpeg 是纯 C 语言的代码，在 C++ 当中不能直接进行 include

#include "QDebug"
#include <QTimer>

#include <QFileDialog>
#include <QMouseEvent>
#include <QPainter>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    timerHide(new QTimer(this)),
    timerPb(new QTimer(this)),
    progressTimer(new QTimer(this)),
    image(":/new/prefix1/resource/icon.jpg") ,
    decoder(new Decoder),
    volume(100)
{
    ui->setupUi(this);
    progressTimer->setInterval(500); // 0.5s
    timerHide->setInterval(5000); // 每隔5s运行一次
    timerPb->setInterval(1000); // 每隔5s运行一次
    timerHide->start();
    qRegisterMetaType<Decoder::PlayState>("Decoder::PlayState");
    initUI();
    bindConnect();
    initFFmpeg();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    pos = event->pos();
    int x = pos.x();
    int y = pos.y();
    if(x > 0 && x < ui->screenWidget->width() - 1 && y > 0 && y < ui->screenWidget->height() - 1){
        //qDebug() << "在荧幕内" << x << y;
        isInscreen = true;
    }else{
        //qDebug() << "在荧幕外" << x << y;
        isInscreen = false;
    }
    // 重启定时器
    timerHide->stop();
    // 如果当前是不可见的状态
    if(!isVisible){
        showWidget(true);
        isVisible = true;
        QApplication::restoreOverrideCursor(); // 显示鼠标
        //QApplication::setOverrideCursor(Qt::ArrowCursor); // 显示鼠标
    }
    timerHide->start();


}


void MainWindow::paintEvent(QPaintEvent *)
{
//绘制纯黑背景
    QPainter painter(this); // 创建一个 QPainter 对象，它用于在当前窗口或控件上进行绘图操作。this 表示当前窗口或控件为绘制目标
    // 只能在父对象上绘画
    painter.setRenderHint(QPainter::Antialiasing, true); //设置了绘图的渲染提示。在这里，它启用了抗锯齿功能，以使绘制的图形边缘更加平滑，可写可不写
    int width = ui->screenWidget->width(); // 宽度
    int height = ui->screenWidget->height(); // 高度
    // 画图，一个全黑色的图片
    painter.setBrush(Qt::black); // 设置填充色，为黑色
    painter.drawRect(0, 0, width, height); // 画一个矩形 x轴 y轴 宽 高

// 如果保持宽高比
    if(isKeepAspectRatio) {
        // 通过 scaled 函数进行拉伸或缩小，以适应窗口或控件的宽度和高度，同时保持宽高比不变。
        //计算了图像的显示位置 (x, y)，使图像在窗口或控件中居中显示，然后使用 painter.drawImage(x, y,
        //img) 在窗口上绘制图像。
        QImage img = image.scaled(width, height, Qt::KeepAspectRatio); // 对图像做一个拉伸或缩小（宽，高，是否保持宽高比）
        // 对应的XY坐标
        int x = (ui->screenWidget->width() - img.width()) / 2;
        int y = (ui->screenWidget->height() - img.height()) / 2;
        painter.drawImage(x, y, img); // 画那张图片
    } else { // 强制拉伸或缩小以填充整个窗口或控件，不保持宽高比
        QImage img = image.scaled(width, height); // 对图像做一个拉伸或缩小（宽，高，是否保持宽高比）
        painter.drawImage(0, 0, img);
    }
}

void MainWindow::slot_TimerHide()
{
    if(isVisible){ // 当前是可见状态，全部隐藏掉
        showWidget(false); // 隐藏所有窗口部件
        isVisible = false; // 界面不可见
        if(isInscreen || isFullScreen()){//当鼠标在荧幕内时&&所有窗口状态下都隐藏鼠标
            // 若为全屏状态，则额外隐藏光标
            QApplication::setOverrideCursor(Qt::BlankCursor); // 隐藏光标
        }
    }
}

void MainWindow::slot_TimerPb()
{
    isPb = false;
    timerPb->stop();
}

void MainWindow::slot_ShowVideo(QImage img)
{
    // 把用ffmpeg解析出来的图像赋值给image
    image = img;
    update(); // 手动调用paintEvent
}

void MainWindow::bindConnect()
{
    // 绑定所有的信号和槽
    connect(timerHide, &QTimer::timeout,this, &MainWindow::slot_TimerHide);
    connect(timerPb, &QTimer::timeout,this, &MainWindow::slot_TimerPb);
    // 实现当用户在 MainWindow 中要求播放特定的媒体文件时，可以通过发出 signal_PlayMedia 信号来触发 decoder 对象的相应操作，例如开始解码和播放媒体文件
    connect(this, &MainWindow::signal_PlayMedia,decoder, &Decoder::decoderFile);
    connect(decoder, &Decoder::signal_ShowVideo, this, &MainWindow::slot_ShowVideo);
    connect(decoder, &Decoder::signal_SendDuration, this, &MainWindow::slot_ReceiveDeration);
    connect(progressTimer, &QTimer::timeout, this, &MainWindow::slot_ProgressTimer);
    connect(ui->hsProgress, &QSlider::sliderMoved, decoder, &Decoder::slot_Seek);
    connect(decoder, &Decoder::signal_SendState, this, &MainWindow::slot_StateChanged);
}

void MainWindow::initUI()
{
    this->setWindowTitle("视频播放器");
    this->setMouseTracking(true); // 捕捉鼠标轨迹
    ui->centralWidget->setMouseTracking(true); // 启用中央窗口部件的鼠标跟踪功能
    ui->screenWidget->setMouseTracking(true);
    ui->lbTitle->setMouseTracking(true);
    showMediaList(true);
    ui->cbSpeed->setCurrentIndex(1);

//代码形式实现ui
    //上一首
//    ui->pbPrev->setIcon(QIcon(":/new/prefix1/resource/prevBlue.png"));
//    ui->pbPrev->setIconSize(QSize(50, 50)); // 大小
//    ui->pbPrev->setStyleSheet("background:transparent; "
//                                                                "border:none;"); // 背景颜色透明，无边框
//    ui->pbPrev->setText(""); // 将文字删掉

    //下一首
//    ui->pbNext->setIcon(QIcon(":/new/prefix1/resource/nextBlue.png"));
//    ui->pbNext->setIconSize(QSize(50, 50)); // 大小
//    ui->pbNext->setStyleSheet("background:transparent; "
//                                                                "border:none;"); // 背景颜色透明，无边框
//    ui->pbNext->setText(""); // 将文字删掉

    //播放/暂停
//    ui->pbPlay->setIcon(QIcon(":/new/prefix1/resource/playBlue.png"));
//    ui->pbPlay->setIconSize(QSize(50, 50)); // 大小
//    ui->pbPlay->setStyleSheet("background:transparent; "
//                                                                "border:none;"); // 背景颜色透明，无边框
//    ui->pbPlay->setText(""); // 将文字删掉

    //停止
//    ui->pbStop->setIcon(QIcon(":/new/prefix1/resource/stopBlue.png"));
//    ui->pbStop->setIconSize(QSize(50, 50)); // 大小
//    ui->pbStop->setStyleSheet("background:transparent; "
//                                                                "border:none;"); // 背景颜色透明，无边框
//    ui->pbStop->setText(""); // 将文字删掉

    //添加本地文件
//    ui->pbOpen->setIcon(QIcon(":/new/prefix1/resource/addBlue.png"));
//    ui->pbOpen->setIconSize(QSize(50, 50)); // 大小
//    ui->pbOpen->setStyleSheet("background:transparent; "
//                                                                "border:none;"); // 背景颜色透明，无边框
//    ui->pbOpen->setText(""); // 将文字删掉

//默认隐藏窗口部件
    setHide(ui->pbNext);
    setHide(ui->pbPrev);
    setHide(ui->pbPlay);
    setHide(ui->pbStop);
    setHide(ui->pbOpen);
    setHide(ui->hsVolume);
    setHide(ui->hsProgress);
    setHide(ui->pbVolume);
    setHide(ui->pbList);
    setHide(ui->lbTime);
    setHide(ui->lbTitle);
    setHide(ui->cbSpeed);
    ui->hsVolume->setValue(100);
}

void MainWindow::initFFmpeg()
{
    // 初始化 FFmpeg 网络功能，通常用于处理网络流媒体
    if(avformat_network_init()){
        qDebug() << "网络初始化失败";
    }
    // 初始化 SDL，包括音频和定时器功能。SDL 是用于多媒体和游戏开发的库
    if(SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        qDebug() << "SDL初始化失败";
    }
}

void MainWindow::setHide(QWidget *w)
{
    hideVector.append(w); // 将窗口部件添加到 hideVector 容器中，以便稍后进行处理或隐藏这些窗口部件
}

void MainWindow::showMediaList(bool flag)
{
    isShowList = flag;
    if(flag){
        ui->lwMediaList->show();
    }else{
        ui->lwMediaList->hide();
    }
}

void MainWindow::showWidget(bool show)
{
    // 根据传递的布尔值参数 show 来显示或隐藏 hideVector 容器中包含的窗口部件（QWidget 对象）
    // 如果show，展示
    if(show) {
        for(QWidget *w : hideVector){
            w->show();
        }
    } else {
        for(QWidget *w : hideVector){
            w->hide();
        }
    }
}

void MainWindow::addPathToList(QString path)
{
    QDir dir(path);
    int t_index = 0;
    // 匹配文件名以确定是否为支持的音频或视频文件格式
    QRegExp re(".*\\.(mp4|avi|mov|flv|mp3|wav|flac)$"); // 匹配后缀 .*：长度
    // 获取指定路径下的文件列表
    QFileInfoList list = dir.entryInfoList(QDir::Files);
    for (int i = 0; i < list.count(); i++){    // 遍历文件列表，对每个文件进行正则匹配，如果匹配成功，将其绝对路径添加到播放列表中
        QFileInfo fileinfo = list.at(i);    // 判断文件名是否和上面的正则表达式匹配
        if(re.exactMatch(fileinfo.fileName())){// 匹配上，视音频或者视频
            QString filename = fileinfo.absoluteFilePath(); // 获取文件绝对路径
            if(!playList.contains(filename)) {
                    //qDebug() << filename;
                    playList.push_back(filename); // 把这个文件的绝对路径压到playList
                    ui->lwMediaList->addItem(fileinfo.fileName());
                    if(filename == currentPlay){
                        t_index = ui->lwMediaList->count() - 1;
                    }
            }
        }
    }

    //qDebug() << "当前行: " << t_index;
    ui->lwMediaList->setCurrentRow(t_index);
    //ui->lwMediaList->setFocus();
    ui->lwMediaList->item(t_index)->setData(Qt::ForegroundRole, QBrush(QColor("#00aaff")));
    focusIndex = t_index;
}

void MainWindow::setMute(bool flag)
{
    isMute = flag;
    if(flag){
        volume = ui->hsVolume->value();
        ui->hsVolume->setValue(0);
        ui->pbVolume->setIcon(QIcon(":/new/prefix1/resource/mute.png"));
    }else{
        ui->hsVolume->setValue(volume);
        ui->pbVolume->setIcon(QIcon(":/new/prefix1/resource/volume.png"));
    }
}

QString MainWindow::fileType(QString file)
{
    // 创建一个 QString 类型的变量 type，用于存储文件的类型（"music" 或 "video"）
    QString type;
    // file.lastIndexOf('.') 来获取文件路径中最后一个点号的位置，从而定位文件的扩展名
    // file.right() 函数截取文件路径中扩展名部分
    // 将截取得到的扩展名存储在 suffix 变量中
    // mng.mp3 7-3-1=4 mp3
    QString suffix = file.right(file.size() - file.lastIndexOf('.') - 1);
    // 如果扩展名是 "mp3"、"wav" 或 "flac"，则将 type 设置为 "music"，否则将其设置为 "video"
    if(suffix == "mp3" || suffix == "wav" || suffix == "flac")
        type = "music";
    else
        type = "video";

    return type;
}

void MainWindow::playMedia(QString file)
{
    // 停止当前正在播放的多媒体文件
    decoder->slot_StopVideo();
    SDL_Delay(500);
    // 设置 currentPlay 变量为要播放的文件路径，以便跟踪当前播放的文件
    currentPlay = file;
    // 使用fileType函数获取文件的类型，并将其存储在currentPlayType变量中
    currentPlayType = fileType(file);
    // 设置标题标签 (ui->lbTitle) 的样式和文本，以显示当前播放的文件名称
    ui->lbTitle->setStyleSheet( // 标题样式
        "color:rgb(25, 125, 203);"
        "font-size: 24px;"
        "background: transparent;");
    ui->lbTitle->setText("当前播放：" + file);

    // 播放媒体文件
    //progressTimer->start(); // 运行查看效果后删掉
    emit signal_PlayMedia(currentPlay, currentPlayType);

}

void MainWindow::playMediaIndex(const int &index)
{
    QString t_path = playList[index];
    playMedia(t_path);
}

void MainWindow::on_pbOpen_clicked()
{
    filePath = QFileDialog::getOpenFileName(
                                                        this, "打开文件", "D:/Progarm/Qt/QtDemo/FFmpegTest3/resource",
                                                        "*.mp4 *.avi *.mov *.flv 视频;;"
                                                        "*.mp3 *.wav *.flac 音频");
    // 检查用户是否已选择了一个文件路径
    if(!filePath.isNull() && !filePath.isEmpty()) {
        // 如果 filePath 不为 null 并且不为空，表示用户已经选择了一个文件
        // 播放
        playMedia(filePath);
        //获取音视频目录
        QString path = filePath.left(filePath.lastIndexOf('/') + 1); // 音视频路径,截掉最左边的一部分
        //将目录中的音视频添加到播放列表QList
        addPathToList(path);
    }
}

void MainWindow::slot_ReceiveDeration(qint64 time)
{
    totalTime = time / 1000000; // 转换成秒
    ui->hsProgress->setRange(0, totalTime);
    int hour = totalTime / 60 / 60; // 小时
    int min = (totalTime / 60) % 60; // 分钟
    int sec = totalTime % 60; // 秒
    ui->lbTime->setText(QString("00:00:00/%1:%2:%3")
                .arg(hour, 2, 10, QLatin1Char('0')) // (参数，所占位数， 进制， 默认 值[没有数字的用0补充])
                .arg(min, 2, 10, QLatin1Char('0'))
                        .arg(sec, 2, 10, QLatin1Char('0')));
}

void MainWindow::slot_ProgressTimer()
{
    qint64 currentTime = static_cast<qint64>(decoder->getClock()); // 去把getClock()改成公共的
    // currentTime /= 1000000;
    if(!ishsPressed)
        ui->hsProgress->setValue(currentTime);
    int hourCurrent = currentTime / 60 / 60;
    int minCurrent = (currentTime / 60) % 60;
    int secCurrent = currentTime % 60;
    int hour = totalTime / 60 / 60;
    int min = (totalTime / 60) % 60;
    int sec = totalTime % 60;
    //qDebug() << hourCurrent << minCurrent << minCurrent;
    ui->lbTime->setText(QString("%1:%2:%3/%4:%5:%6")
                                .arg(hourCurrent, 2, 10, QLatin1Char('0')) // (参数，所占位数， 进制， 默认值[没有数字的用0补充])
                                .arg(minCurrent, 2, 10, QLatin1Char('0'))
                                .arg(secCurrent, 2, 10, QLatin1Char('0'))
                                .arg(hour, 2, 10, QLatin1Char('0')) // (参数，所占位数， 进制， 默认值[没有数字的用0补充])
                                .arg(min, 2, 10, QLatin1Char('0'))
                                .arg(sec, 2, 10, QLatin1Char('0')));

}

void MainWindow::slot_StateChanged(Decoder::PlayState state)
{
    switch (state) {
        case Decoder::PLAYING: // 播放
            ui->pbPlay->setIcon(QIcon(":/new/prefix1/resource/pauseBlue.png"));
            playState = Decoder::PLAYING;
            progressTimer->start();
            break;
        case Decoder::STOP: // 停止
            ui->pbPlay->setIcon(QIcon(":/new/prefix1/resource/playBlue.png"));
            playState = Decoder::STOP;
            progressTimer->stop();
            image = QImage(":/new/prefix1/resource/icon.jpg");   //停止播放时的画面
            ui->lbTime->setText("00:00:00/00:00:00");
            ui->hsProgress->setValue(0);
            totalTime = 0;
            update();
            //qDebug() << "停止";
            break;
        case Decoder::PAUSE: // 暂停
            ui->pbPlay->setIcon(QIcon(":/new/prefix1/resource/playBlue.png"));
            playState = Decoder::PAUSE;
            progressTimer->stop();
            break;
        case Decoder::FINISH: // 完成
            //TODO: 播放模式【单曲循环，随机播放，列表播放，只播放一次】 自己去写

            break;
        default:
            break;
    }
}

void MainWindow::on_pbPlay_clicked()
{
    decoder->slot_PauseVideo();
}

void MainWindow::on_pbStop_clicked()
{
    decoder->slot_StopVideo();
}

void MainWindow::on_hsVolume_valueChanged(int value)
{
    timerHide->stop();
    decoder->setVolume(value);
    timerHide->start();
}

void MainWindow::on_hsProgress_sliderPressed()
{
    timerHide->stop();
    ishsPressed = true;
    if(Decoder::PAUSE == playState){
        decoder->slot_PauseVideo();
    }
}

void MainWindow::on_hsProgress_sliderReleased()
{
    timerHide->start();
    ishsPressed = true;
}


void MainWindow::on_lwMediaList_itemDoubleClicked(QListWidgetItem *item)
{
    if(isPb)return;
    isPb = !isPb;
    QListWidgetItem *temp = ui->lwMediaList->item(focusIndex);
    temp->setData(Qt::ForegroundRole, QBrush(Qt::white));
    focusIndex = ui->lwMediaList->row(item);
    item->setData(Qt::ForegroundRole, QBrush(QColor("#00aaff")));
    playMediaIndex(focusIndex);
    timerPb->start();
}

void MainWindow::on_pbList_clicked()
{
    showMediaList(!isShowList);
}

void MainWindow::on_pbVolume_clicked()
{
    setMute(!isMute);
}

void MainWindow::on_pushButton_clicked()
{
    decoder->setTemp(1.0);
}

void MainWindow::on_cbSpeed_currentIndexChanged(int index)
{
    switch (index) {
    case 0:
        decoder->setTemp(0.5);
        break;
    case 1:
        decoder->setTemp(1.0);
        break;
    case 2:
        decoder->setTemp(2.0);
        break;
    default:
        break;
    }
}

void MainWindow::on_pbNext_clicked()
{
    int index = 0;
    int maxIndex = ui->lwMediaList->count() - 1;
    if(focusIndex < maxIndex){
        index = focusIndex + 1;
    }else{
        index = focusIndex;
    }
    on_lwMediaList_itemDoubleClicked(ui->lwMediaList->item(index));
}

void MainWindow::on_pbPrev_clicked()
{
    int index = 0;
    int maxIndex = ui->lwMediaList->count() - 1;
    if(focusIndex > 0){
        index = focusIndex - 1;
    }else{
        index = focusIndex;
    }
    on_lwMediaList_itemDoubleClicked(ui->lwMediaList->item(index));
}
