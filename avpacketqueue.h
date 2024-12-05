#ifndef AVPACKETQUEUE_H
#define AVPACKETQUEUE_H
extern "C" {
    #include "libavcodec/avcodec.h" // FFmpeg 库的头文件，用于音视频编解码
    #include "libavformat/avformat.h" // 音视频文件格式的处理

    #define SDL_MAIN_HANDLED
    #include "SDL.h" // SDL库的头文件，用于多媒体处理和音视频渲染
}
#include <QQueue> // 用于管理音视频帧的队列

class AVPacketQueue
{
public:
    AVPacketQueue();

    /**
     * @brief 将 AVPacket 结构压入队列
     * @param packet
     */
    void enQueue(AVPacket *packet);

    /**
     * @brief 从队列中取出 AVPacket 结构
     * @param packet
     * @param isBlock 是否阻塞等待直到队列非空
     */
    void deQueue(AVPacket *packet, bool isBlock);
    bool isEmpty(); // 检查队列是否为空
    void empty(); // 清空队列
    int queueSize(); // 获取队列的大小

private:
    //用于保护音视频帧队列，以确保在多线程中的访问是安全的。
    SDL_mutex *mutex; // 用于线程同步的互斥体（锁）
    // 条件变量用于线程之间的通信，以便一个线程可以等待另一个线程满足某个条件后再继续执行。
    //在这里，cond 用于通知等待中的线程队列中的某个线程，当队列非空时，该线程可以继续执行
    SDL_cond *cond; // 用于线程同步的条件变量
    // 用于存储 AVPacket 结构。queue 用于管理音视频帧的顺序，确保它们按正确的顺序出队，以便在解码和播放过程中按正确的顺序处理音视频帧。
    QQueue<AVPacket> queue; // 临界资源：音视频帧的队列
};

#endif // AVPACKETQUEUE_H
