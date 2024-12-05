#ifndef AUDIOPLAY_H
#define AUDIOPLAY_H
extern "C"{
    #include "libswresample/swresample.h" // 对音频重采样
    #include "libavcodec/avcodec.h"
    #include <libavfilter/avfilter.h>
    #include <libavfilter/buffersink.h>
    #include <libavfilter/buffersrc.h>
    #include <libavformat/avformat.h>
    #include <libavutil/error.h>
}
#include "avpacketqueue.h" // 队列

#include <QObject>

class AudioPlay : public QObject
{
    Q_OBJECT
public:
    explicit AudioPlay(QObject *parent = nullptr);

    // 这两个参数，是
    //这个时候就要调用audioplay去解析音频，
    //
    /**
     * @brief 打开音频（播放音频），在decoder的run函数里解析有音频流的时候，我们要对音频流进行一个解码，
     *  这时候就需要你把这个文件（第一个参数）和音频流的下标传给她，然后根据音频流的下标去判断是音频还是视频，去做相应的操作
     * @param pFormatCtx 音频上下文
     * @param index 下标
     * @return
     */
    int openAudio(AVFormatContext *pFormatCtx, int index);
    void closeAudio(); // 关闭音频
    void enQueue(AVPacket *packet); // 入队
    /**
     * @brief 解码音频
     * @return
     */
    int decodeAudio();
    void emptyAudioData(); // 清空音频数据
    void readFinished(); // 读取文件完成
    void pauseAudio(bool pause); // 暂停音频
    void stopAudio(); // 停止音频
    int getVolume() const; // 获取音量
    void setVolume(int value); // 设置音量
    double getAudioClock(); // 音频时钟
    void setSpeed(int value);
    bool getIsStop() const;
    void setIsStop(bool value);
    void setTemp(double new_tempo);
    bool isReadFinished, isDecoderFinish; // 是否完成



signals:
    void signal_AudioPlayFinished();


private:
    /**
     * @brief 提供音频数据
     * @param userdata 用户参数
     * @param stream    音频设备流接收缓冲区
     * @param SDL_AudioBufSize  音频设备流接收缓冲区可用大小
     */
    static void audioCallback(void *userdata, quint8 *stream, int SDL_AudioBufSize); //回调函数
    /**
     * @brief 初始化滤镜图，使用atempo，固定播放速率
     * @return
     */
    int initFilter();
    /**
     * @brief 初始化滤镜图，使用rubberband，动态可变播放速率
     * @return
     */
    int initFilter2(); // 初始化过滤器/滤镜图



    bool isStop; // 是否停止
    bool isPause; // 是否暂停

    qint64 totalTime; // 总时长
    double clock; // 时钟 做音视频同步时使用的，用来存储时间
    int volume; // 音量
    AVStream *stream; // 音频流
    // 解析音频
    quint8 *audioBuf; // 解析缓存空间
    quint32 audioBufSize; // 解析缓存大小
    uint8_t audioBuf1[192000] __attribute__((aligned(16)));// 采样率
    //DECLARE_ALIGNED(16, quint8, audioBuf1)[192000]; // 采样率 //该宏在ffmpeg6.1.1中被弃用
    quint64 audioBufSize1;
    quint64 audioBufIndex;
    // 以上都是解析音频要用到的，音频和视频不一样，视频是一秒钟想放几张就放几张，但是音频不跟视
    //频一样，音频是你这一段就是5秒，他就会占用5s的时间去播放，所以说我们需要一个缓存来保存我们缓存好的
    //音频数据，这五个是做这个操作的
    SDL_AudioSpec spec;
    // SDL不仅可以过滤图像，它最主要的功能就是由他去播放原始的音频数据，他有一套很完整的流程，
    quint32 audioDeviceFormat; // 音频播放器的格式
    quint8 audioDepth; // 音频的深度
    SwrContext *aCovertCtx; // 重采样的上下文
    qint64 audioDstChannelLayout; // 声道，音频的声道
    qint64 audioSrcChannelLayout;
    AVSampleFormat audioDstFmt; // 音频的目标格式，这个东西是一个枚举
    AVSampleFormat audioSrcFmt; // 音频的源格式
    int audioSrcChannels; // 源格式的声道数
    int audioSrcFreq; // 源格式的频率

    AVFilterGraph *filter_graph = nullptr;  //滤镜图
    AVFilterContext *filter_context = nullptr;  //输入过滤器
    AVFilterContext *sink_context = nullptr;    //输出过滤器
    AVFilterContext *atempo_ctx; // atempo 滤镜的上下文

    // 解码需要用的东西
    AVCodecContext *codecCtx; // 编码器的上下文
    AVPacketQueue packetQueue; // 包的队列，自己写的库
    AVPacket packet; // 包


    int sendReturn; // 一个标识
    double speed;   //倍速

};

#endif // AUDIOPLAY_H
