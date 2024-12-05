#include "avpacketqueue.h"

AVPacketQueue::AVPacketQueue()
{
    // 创建了一个互斥体，用于确保在多线程环境中对队列的访问是线程安全的。
    mutex = SDL_CreateMutex(); // 锁
    // 创建了一个条件变量，用于线程之间的通信和等待。
    cond = SDL_CreateCond(); // 辅助锁是否去等待

}

void AVPacketQueue::enQueue(AVPacket *packet)
{
    // 上锁
    SDL_LockMutex(mutex);
    // 将传入的 AVPacket 结构入队到队列中。
    // 这表示该音视频帧已经被成功添加到队列中，等待后续处理
    queue.enqueue(*packet);
    // 当队列中有新的音视频帧入队时，条件变量 cond 被触发。
    // 这将唤醒一个等待中的线程，以便它可以继续执行
    SDL_CondSignal(cond); // 用于通知等待的线程
    // 解锁互斥体 mutex，允许其他线程访问队列。
    // 现在，队列中已经包含了音视频帧，等待其他线程来处理
    SDL_UnlockMutex(mutex); // 解锁
}

void AVPacketQueue::deQueue(AVPacket *packet, bool isBlock)
{
    SDL_LockMutex(mutex); // 上锁
    // 循环直到成功出队一个 AVPacket 结构
    while(true){
        // 如果队列不为空，表示有音视频帧可以出队
        if(!queue.isEmpty()){
            // 队列不为空 将队列中的 AVPacket 结构出队并存储到传入的 packet 中。
            // 这表示音视频帧已经成功出队
            *packet = queue.dequeue(); // 出队
            break; // 出队成功后，循环退出。
        } else if(!isBlock){ // 不阻塞
            // 如果队列为空，且设置了不阻塞(isBlock 为 false)，则直接退出循环，表示不等待。
            break;
        } else { // 阻塞
            // 如果队列为空，且设置了阻塞 (isBlock 为 true)，则进入等待状态，
            // 等待条件变量 cond 被触发，即等待有新的音视频帧入队，线程才会继续执行。
            SDL_CondWait(cond, mutex); // 等待解码出新的packet
        }
    }
    SDL_UnlockMutex(mutex); // 解锁 允许其他线程访问队列
    // 线程现在已经成功出队了一个音视频帧
}

bool AVPacketQueue::isEmpty()
{
    // 检查队列是否为空 如果队列为空，返回 true，否则返回 false
    return queue.isEmpty();
}

void AVPacketQueue::empty()
{
    SDL_LockMutex(mutex); // 上锁
    // 检查队列是否包含音视频帧。只有当队列中仍然有数据时才执行清空操作
    while(queue.size() > 0) {
        // 不能使用queue.clear()，可能造成内存泄漏
        // 在每次迭代中，从队列中出队一个音视频帧（AVPacket 结构）。
        // 这样可以逐个处理队列中的音视频帧
        AVPacket packet = queue.dequeue();
        // 调用 FFmpeg 的 av_packet_unref 函数，用于销毁音视频帧。
        // 这是释放音视频帧相关资源的必要步骤
        av_packet_unref(&packet); // 销毁
    }
    SDL_UnlockMutex(mutex); // 解锁
}

int AVPacketQueue::queueSize()
{
    // 返回队列中的音视频帧数量，即队列的大小
    return queue.size();
}

