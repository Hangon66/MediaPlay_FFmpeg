#ifndef MAINWINDOW_H
#define MAINWINDOW_H
extern "C" {
    #include <libavformat/avformat.h>   // 多媒体文件格式处理功能
    #include "libavfilter/avfilter.h" // 滤镜库

    #define SDL_MAIN_HANDLED //对于在头文件中引用SDL.h需要使用该宏
    #include "SDL.h"
}
#include <QMainWindow>
#include "decoder.h"
#include <QListWidget>


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    /**
     * @brief 用于在播放媒体文件时发出通知
     * @param file 要播放的媒体文件路径
     * @param type 文件类型
     */
    void signal_PlayMedia(QString file, QString type);

protected:
    void mouseMoveEvent(QMouseEvent *event); // 鼠标移动事件,不用他里面的任何东西，只是用一下他，可以不写参数变量名
    void paintEvent(QPaintEvent *); // 绘图事件

private slots:
    /**
     * @brief 槽函数：鼠标一定时间内不动则隐藏部分组件
     */
    void slot_TimerHide();
    void slot_TimerPb();
    void slot_ShowVideo(QImage img); // 展示视频
    void on_pbOpen_clicked();
    /**
     * @brief 更新视频总时长
     * @param time
     */
    void slot_ReceiveDeration(qint64 time);
    /**
     * @brief 更新当前播放时间
     */
    void slot_ProgressTimer();

    /**
     * @brief 改变播放状态
     * @param state
     */
    void slot_StateChanged(Decoder::PlayState state);
    void on_pbPlay_clicked();
    void on_pbStop_clicked();
    void on_hsVolume_valueChanged(int value);
    void on_hsProgress_sliderPressed();
    void on_hsProgress_sliderReleased();



    void on_lwMediaList_itemDoubleClicked(QListWidgetItem *item);

    void on_pbList_clicked();

    void on_pbVolume_clicked();

    void on_pushButton_clicked();

    void on_cbSpeed_currentIndexChanged(int index);

    void on_pbNext_clicked();

    void on_pbPrev_clicked();

private:
    Ui::MainWindow *ui;
    /**
     * @brief 集中绑定信号与槽
     */
    void bindConnect();

    /**
     * @brief 初始化UI界面
     */
    void initUI();

    /**
     * @brief 初始化 FFmpeg 相关的功能
     */
    void initFFmpeg();

    /**
     * @brief 将一个部件加入容器，以便后期显示/隐藏控件组
     * @param w
     */
    void setHide(QWidget *w);

    /**
     * @brief 显示/隐藏播放列表
     * @param flag
     */
    void showMediaList(bool flag);
    /**
     * @brief 显示/隐藏的按钮组
     * @param show 0/1
     */
    void showWidget(bool show);
    /**
     * @brief 将目录中的视频/音频文件绝对路径加入播放列表QList
     * @param path
     */
    void addPathToList(QString path);

    /**
     * @brief 静音/取消静音
     * @param flag
     */
    void setMute(bool flag);
    /**
     * @brief 返回文件类型
     * @param file 文件路径
     * @return
     */
    QString fileType(QString file);

    /**
     * @brief 播放多媒体文件
     * @param file
     */
    void playMedia(QString file);
    void playMediaIndex(const int &index);
    QVector<QWidget *> hideVector; // QVector类型的容器hideVector，用来管理一组窗口部件，用于在程序中隐藏或显示它们
    bool isVisible{true}; // 跟踪窗口部件的可见性状态 true 可见
    bool isMediaListVisible{false};
    bool isKeepAspectRatio{true}; // 是否保持图像的宽高比
    QTimer *timerHide; // 用于实现定时隐藏界面按钮的功能
    QTimer *timerPb; // 用于实现定时隐藏界面按钮的功能
    QTimer *progressTimer; // 正在播放的时间
    QString filePath; // 文件路径
    QImage image; // 用于存储图像数据
    QList<QString> playList; // 播放列表
    QString currentPlay; // 当前播放的文件名
    QString currentPlayType; // 当前播放的文件类型
    Decoder *decoder; // 用于管理音视频文件的解码和播放
    Decoder::PlayState playState; // 当前的播放状态
    int totalTime; // 总时长
    bool ishsPressed = false;

    int focusIndex = 0;
    QPoint pos;
    bool isInscreen = false;
    bool isShowList = false;
    bool isMute = false;
    int volume;
    bool isPb = false;
};

#endif // MAINWINDOW_H
