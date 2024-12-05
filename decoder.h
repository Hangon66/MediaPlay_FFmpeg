#ifndef DECODER_H
#define DECODER_H
#define __STDC_CONSTANT_MACROS
extern "C" {
    // 包含了与 FFmpeg 的过滤器系统相关的定义。过滤器系统用于对音视频数据进行处理和过滤，例如添加滤镜、调整颜色、裁剪等。
    #include "libavfilter/buffersink.h"
    #include "libavfilter/buffersrc.h"
    // 用于图像处理的库，允许进行不同图像格式之间的转换和调整
    #include "libswscale/swscale.h"
    // 与设备捕捉和音视频输入输出设备相关的定义。它允许连接和配置音视频设备，如摄像头、麦克风等。
    #include "libavdevice/avdevice.h" // 添加播放器设备
    // 包含了与像素格式相关的定义。它用于描述音视频数据的像素格式，例如 YUV420、RGB 等
    #include "libavutil/pixfmt.h"
    // 包含了 FFmpeg 中选项系统的定义。选项系统用于配置和设置各种 FFmpeg 组件的参数
    #include "libavutil/opt.h"
    // 包含了与图像操作相关的定义。它用于处理图像数据，如分配内存、复制图像等
    #include "libavutil/imgutils.h"
    // 包含了一些通用的定义和函数，用于处理各种多媒体数据

    #include "libavutil/common.h"
    // 包含了与编解码器相关的定义。它们用于音视频数据的编解码操作
    #include "libavcodec/avfft.h"
    #include "libavcodec/avcodec.h"

    #define SDL_MAIN_HANDLED
    #include "SDL.h"
}

#include <QThread>
#include "avpacketqueue.h"
#include <QImage>
#include "audioplay.h"

class Decoder  : public QThread
{
    Q_OBJECT
public:
    enum PlayState{    // 枚举类型，用于表示播放状态
        STOP, // 停止播放
        PAUSE, // 暂停播放
        PLAYING, // 正在播放
        FINISH // 完成播放
    };
    Decoder(QObject *parent = nullptr);
    void setPlayState(const PlayState &value);
    PlayState getPlayState() const;
    double getClock(); // 获取时钟
    void setVolume(int value);

    void setIspeed(double value);
    void setTemp(double new_temp);

    bool getIsReadFinish() const;

    bool getIsPause() const;

signals:
    void signal_ShowVideo(QImage img); // 展示视频
    void signal_SendDuration(qint64 time); // 发送时长
    void signal_SendState(Decoder::PlayState state); // 发送状态

public slots:
    /**
     * @brief  槽函数：用于解码音视频文件
     * @param file 文件路径
     * @param type 文件类型
     */
    void decoderFile(QString file, QString type);
    void slot_Seek(qint64 pos); // 进度跳转
    void slot_AudioFinished(); // 音频播放完成
    void slot_StopVideo(); // 停止播放
    void slot_PauseVideo(); // 暂停播放
protected:
    /**
     * @brief run 多线程的run函数 执行函数，通常在继承了 QThread 类的自定义线程类中重写
     * 主要任务：将音视频数据包压入解析队列
     */
    void run();
private:
    int fileType; // 文件类型
    int audioIndex, videoIndex, subtitleIndex;  // 音频索引 视频索引 字母索引
    QString currentFile, currentType; // 当前文件，文件类型
    qint64 totalTime; // 音视频文件的总时长
    double videoclk; // 保存视频的时钟
    bool isStop, isPause, isReadFinish, isDecoderFinish;    // 是否停止 暂停 读取完成 解析/解码完成
    bool isSeek, gotoStop;  // 跳转音视频进度 结束播放
    PlayState playState; // 当前播放状态
    AVPacket seekPacket; //跳转
    // 会用到的ffmpeg的变量
    AVFormatContext *pFormatCtx; // 音视频格式的上下文，相关信息
    AVCodecContext *pCodecCtx; // 编解码器的上下文

    //这些队列用于在解码器线程和播放线程之间传递音视频帧数据
    AVPacketQueue *videoQueue; // 表示视频数据包队列 库里没有，自己写
    AVPacketQueue *subtitleQueue; // 压入字幕帧队列
    AVStream *videostream; // 表示音视频文件中的视频流

    AVFilterGraph *filterGraph; // 表示多媒体过滤器的图形
    AVFilterContext *filterSinkCtx; // 输出过滤器上下文
    AVFilterContext *filterSrcCtx; // 输入过滤器上下文

    AudioPlay *audioPlay;
    qint64 seekPos; // 跳转位置
    static double ispeed; //音视频倍率
    double speed; //音视频倍率
    int initFilter(); // 初始化过滤器/滤镜图

    /**
     * @brief 清除上一个音视频的数据
     */
    void clearData();

    /**
     * @brief 解析播放视频的线程
     * @param arg
     * @return
     */
    static int videoThread(void *arg);
    void showVideo(QImage img); // 展示视频
    /**
     * @brief 更新视频时钟为时间戳&处理帧重复问题
     * @param frame
     * @param pts
     * @return
     */
    double synchronize(AVFrame *frame, double pts); // 使同步

};

#endif // DECODER_H
