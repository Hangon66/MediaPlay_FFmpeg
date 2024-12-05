#include "decoder.h"
#include <QDebug>
#include <QImage>

double Decoder::ispeed = 1.0;

Decoder::Decoder(QObject *parent)
: QThread(parent)
, totalTime(0)
, isStop(false)
, isPause(false)
, isReadFinish(false)
, isDecoderFinish(false)
, isSeek(false)
, playState(STOP)
, videoQueue(new AVPacketQueue) // 初始化内存空间
, filterGraph(nullptr)
, audioPlay(new AudioPlay)
,speed(1.0)
{
    av_init_packet(&seekPacket);
    seekPacket.data = (uint8_t *)"FLUSH";   //用于清空队列的标志
    connect(audioPlay, &AudioPlay::signal_AudioPlayFinished, this, &Decoder::slot_AudioFinished);

    //setIspeed(2.0);
}

void Decoder::decoderFile(QString file, QString type)
{
    //qDebug() << file << "正在解码";
    // 检查当前的播放状态是否不是停止状态。如果不是停止状态，意味着当前正在播放音视频文件，需要执行停止操作
    if(playState != STOP){ // 播放状态不是停止
        isStop = true; // 停止音视频的播放
        // 等待当前的音视频播放停止
        while(playState != STOP)
            SDL_Delay(10); // 延迟10ms
        // 在等待状态变为停止后，延迟 100 毫秒。确保上一个音视频的停止操作已经完成。
        SDL_Delay(100);
    }
    clearData(); // 清除上一个音视频的数据和状态
    SDL_Delay(100);
    // 设置当前的音视频文件和类型
    currentFile = file;
    currentType = type;
    // 启动线程，开始解码操作。这个线程会在后台执行音视频文件的解码和播放操作
    this->start(); // 启动线程
}

void Decoder::slot_Seek(qint64 pos)
{
    if(!isSeek){
        seekPos = pos * 1000000;
        isSeek = true;
    }
}

void Decoder::slot_AudioFinished()
{
    if(currentType == "music"){
        SDL_Delay(100);
        emit signal_SendState(FINISH);
    }
    isStop = true;
}

void Decoder::slot_StopVideo()
{
    if(STOP == playState){
        emit signal_SendState(STOP);
        //qDebug() << "slot_StopVideo";
        return;
    }

    gotoStop = true;
    isStop = true;
    audioPlay->stopAudio();
    if(currentType == "video"){
        while(!isReadFinish || !isDecoderFinish) {
            SDL_Delay(10);
        }
    } else {
        while(!isReadFinish){
            qDebug() << "卡读取";
            SDL_Delay(10);
        }
        if(gotoStop){ // 停止音频
            setPlayState(STOP);
        }
    }
}

void Decoder::slot_PauseVideo()
{
    if(STOP != playState && FINISH != playState){
        isPause = !isPause;
        audioPlay->pauseAudio(isPause);
        if(isPause){
            av_read_pause(pFormatCtx);
            setPlayState(PAUSE);
        } else {
            av_read_play(pFormatCtx);
            setPlayState(PLAYING);
        }
    }
}

void Decoder::run()
{
    // 声明了一个指向 AVCodec 结构的指针 pCodec，以便在后续的解码过程中使用
    AVCodec *pCodec;
    // AVPacket 结构用于存储音视频帧的数据，packet 指针将用于引用这些帧数据
    AVPacket pkt, *packet = &pkt; // pkt对象, *packet 指针，指向pkt对象

    int seekIndex; // 跳转index
    bool realTime; // 实时时间，判断是否是在线视频

    // AVFormatContext 结构的上下文对象用于存储音视频文件的相关信息和参数
    //pFormatCtx = avformat_alloc_context();    //avformat_open_input会自动调用该函数所有不用手动调用
    pFormatCtx = NULL;

    // 打开输入流：先打开格式上下文，再打开音频码上下文
    //qDebug() << "输入文件：" << currentFile;
    int res = avformat_open_input(&pFormatCtx,currentFile.toUtf8().data(), nullptr, nullptr);
    if(res != 0){
        qDebug() << "文件输入流打开失败";
        return;
    }

    // 2、查找流信息（音频流、视频流、字幕流等），
    res = avformat_find_stream_info(pFormatCtx, nullptr);   //读取流信息
    if(res < 0) {
        qDebug() << "流信息查找失败";
        // 释放 pFormatCtx 对象以及其内部的资源，以防止内存泄漏和资源泄漏
        avformat_free_context(pFormatCtx);
        return;
    }

//判断一下是否是在线视频
    realTime = false;
    // 此函数用于显示有关打开文件的信息，包括文件格式、流的数量、时长等等
    av_dump_format(pFormatCtx, 0,currentFile.toUtf8().data(), 0); // 输出打开文件的信息

    // 3、分辨媒体文件的音频流、视频流信息
    for(unsigned int i = 0;i < pFormatCtx->nb_streams; i++){// 流的数量
        if(pFormatCtx->streams[i]->codecpar->codec_type== AVMEDIA_TYPE_VIDEO){ // 找到这个流的编解码的类型
            // 视频流
            videoIndex = i;
            qDebug() << "找到了视频流" << videoIndex;
        }
        if(pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO){// 找到这个流的编解码的类型
            // 音频流
            audioIndex = i;
            qDebug() << "找到了音频流" << audioIndex;
        }
        if(pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_SUBTITLE){// 找到这个流的编解码的类型
            // 字幕流
            subtitleIndex = i;
            qDebug() << "找到了字幕流" << subtitleIndex;
        }
    }

    // 若未找到视频和音频流则reture
    if(currentType == "video") { // 视频
        if(videoIndex < 0){
            qDebug() << "视频文件中找不到视频流";
            avformat_free_context(pFormatCtx); // 释放信息
            return;
        }else if(audioIndex < 0){
            qDebug() << "视频文件中找不到音频流";
            avformat_free_context(pFormatCtx); // 释放信息
            return;
        }
    }

    // 传送总时长：判断是否为实时视频
    if(realTime) {
        //实时视频，直播？
        emit signal_SendDuration(0);
    } else { // 不是实时视频
        // 有总时长
        totalTime = pFormatCtx->duration; // 总时长
        //把总时长传递给界面
        emit signal_SendDuration(totalTime);
    }

// 分别进行音频和视频解码
//音频解码
    if(audioIndex >= 0){ //存在音频流
        res = audioPlay->openAudio(pFormatCtx, audioIndex);
        if(res < 0){
            // 打开失败
            avformat_free_context(pFormatCtx); // 释放
            return;
        }
    }

// 视频解码
    if(currentType == "video") {
        pCodecCtx = avcodec_alloc_context3(nullptr); // 初始化pCodecCtx
        // 复制参数，将音频的信息赋值给编解码器的上下文
        avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoIndex]->codecpar);
        // 为视频找解码器
        pCodec = const_cast<AVCodec*>(avcodec_find_decoder(pCodecCtx->codec_id)); // 寻找解码器
        if(pCodec == NULL){
            qDebug() << "找不到对应的解码器";
            goto fail; // 是C语言的写法，容易出现内存上的错误
        }
        // 找到了，打开编码器
        res = avcodec_open2(pCodecCtx, pCodec, nullptr);
        if(res < 0){
            qDebug() << "解码器打开失败";
            goto fail;
        }
        // 获取视频流
        videostream = pFormatCtx->streams[videoIndex];

        //TODO: 开始播放视频

        // 初始化filter过滤器
        if(initFilter() < 0){
            goto fail;// 若中间有一部执行错了
        }

        // 使用多线程SDL解析&播放视频
        SDL_CreateThread(&Decoder::videoThread, "video_thread", this);
    }
    setPlayState(PLAYING); // 设置播放状态为正在播放
    while(true) {// 死循环，将音/视频数据包加入播放队列
        if(isStop) { //当停止播放时，结束该线程死循环，第一次进来时为false
            break;// 停止
        }

 // 若跳转变量为true：解包、压入队列上面
seek:   //修改音视频播放位置
        if(isSeek){
            // 跳转
            if(currentType == "video"){
                seekIndex = videoIndex;
            } else {
                seekIndex = audioIndex;
            }
            AVRational AVRational = av_get_time_base_q();
            seekPos = av_rescale_q(seekPos, AVRational, pFormatCtx->streams[seekIndex]->time_base);
            res = av_seek_frame(pFormatCtx, seekIndex, seekPos, AVSEEK_FLAG_BACKWARD);  //修改媒体文件播放时间点
            if(res < 0){
                qDebug() << "跳转失败";
            } else {
                audioPlay->emptyAudioData(); // 清空数据
                audioPlay->enQueue(&seekPacket); // 清空队列
                if(currentType == "video"){
                    videoQueue->empty();
                    videoQueue->enQueue(&seekPacket); // 清空队列
                    videoclk = 0;
                }
            }
            isSeek = false;
        }

        if(isPause) {
            // 暂停
            SDL_Delay(10); // 等待10ms
            continue; // 保证他在死循环里面
        }

        // 解包、压入队列
        if(currentType == "video"){ // 当前类型为视频类型
            if(videoQueue->queueSize() > 512){ // 队列大于某个值不再进行解压
                SDL_Delay(100); // 延时100ms
                continue;
            }
        }
        // 从音视频文件中读取一个包出来，放在第二个参数中
        res = av_read_frame(pFormatCtx, packet); // （音视频格式的上下文，要放的包）
        if(res < 0){
            qDebug() << "文件读取完成"; // 默认读取完成了
            isReadFinish = true;
            audioPlay->readFinished();
            //TODO:通知其他类文件读取完成
            break;
        }
        // =0读取出一个包来，读取出来放到队列里就完成了
        if(packet->stream_index == videoIndex && currentType == "video"){ // 是视频
            videoQueue->enQueue(packet); // 入队
        } else if(packet->stream_index == audioIndex){
            audioPlay->enQueue(packet);
        }else{ // 不是视频
            av_packet_unref(packet); // 销毁
        }
    }
    while(!isStop) { // 不是停止状态
        // 能跳转到这个while循环中，说明视频还没暂停，只是把它解析完成了
        if(isSeek){
            goto seek;
        }
        SDL_Delay(200); // isSeek检测周期
    }

fail:   //关闭媒体文件
    if(audioIndex >= 0){
        //处理音频
        audioPlay->closeAudio();
    }
    if(videoIndex >= 0) {
        // 处理视频
        avcodec_close(pCodecCtx); // 关掉编解码器上下文
        avcodec_free_context(&pCodecCtx);
    }
    avformat_close_input(&pFormatCtx); // 关闭视频上下文
    avformat_free_context(pFormatCtx); // 释放内存资源
    isReadFinish = true; // 读取完毕

}

bool Decoder::getIsPause() const
{
    return isPause;
}

bool Decoder::getIsReadFinish() const
{
    return isReadFinish;
}

void Decoder::setIspeed(double value)
{
    Decoder::ispeed = value;
    audioPlay->setSpeed(value);
}


Decoder::PlayState Decoder::getPlayState() const
{
    return playState;
}

void Decoder::setPlayState(const PlayState &value)
{
    playState = value;
    emit signal_SendState(playState);
}


void Decoder::clearData()
{
    // 将音频索引、视频索引和字幕索引都设置为 -1，表示它们都未初始化或不存在。
    // 这是为了确保在处理新的音视频文件时，这些索引变量处于初始状态
    audioIndex = -1; // 音频索引
    videoIndex = -1; // 视频索引
    subtitleIndex = -1; // 字幕索引
    // 总时长设为0，以便在开始新的音视频文件播放时重新计算总时长
    totalTime = 0; // 总时长
    // 设置为非停止和非暂停状态。
    audioPlay->emptyAudioData(); // 清空音频数据
    // 这是为了确保在切换到新的音视频文件时，播放状态处于正常状态。
    isStop = false; // 是否停止
    isPause = false; // 是否暂停
    // 将用于控制跳转、读取完成和解析完成的标志都设置为 false
    // 表示这些操作都处于初始状态
    isSeek = false; // 是否跳转音视频进度
    isReadFinish = false; // 是否读取完成
    isDecoderFinish = false; // 是否解析完成
    videoQueue->empty();
    videoclk = 0;
}

int Decoder::videoThread(void *arg)
{
    int res; // 存放结果
    double pts;
    AVPacket packet;
    Decoder *decoder = (Decoder *)arg; // 将arg强制转换为Decoder对象
    // videoThread最终是要单独去跑的，跑起来之后就跟类对象没有关系了，所以我们需要用*decoder指向之前的一个父对象
    AVFrame *pFrame = av_frame_alloc(); // 分配一个新的帧，付给他一定空间
    while(true) {// 死循环：解析视频
        if(decoder->isStop){
            // 停止状态时
            break;
        }
        if(decoder->isPause){
            // 暂停状态
            SDL_Delay(10); // 等待
            continue;
        }

        if(decoder->videoQueue->queueSize() == 0){
            // 队列里没有东西
            if(decoder->isReadFinish){
                break;// 视频解析完毕，跳出视频解析死循环
            }
            // 解析没有来得及解析出来
            SDL_Delay(10); // 10ms，等待数据包入队
            continue; // 继续循环
        }

        // 队列有东西，拿出来
        decoder->videoQueue->deQueue(&packet, true); // 出队

        if(strcmp((char *)packet.data, "FLUSH") == 0){  //清空解析队列
            qDebug() << "正在刷新视频";
            avcodec_flush_buffers(decoder->pCodecCtx);  //刷新解码器的内部缓冲区，用于处理新的数据流
            av_packet_unref(&packet);
            continue;
        }

        // 把视频（从队列里拿出来的那一帧视频）发送给解码器
        res = avcodec_send_packet(decoder->pCodecCtx, &packet);
        if((res < 0) && (res != AVERROR(EAGAIN) && res != AVERROR_EOF)){
            qDebug() << "视频发送给编码器失败";
            av_packet_unref(&packet);
            continue; // 一直循环，直到视频读完为止
        }

        // 解析成功，会把里面的内容发到pFrame里面
        res = avcodec_receive_frame(decoder->pCodecCtx,pFrame);
        if((res < 0) && (res != AVERROR_EOF)) {
            // res<0或没有读取完成
            qDebug() << "视频解码失败";
            av_packet_unref(&packet);
            continue; // 一直循环，直到视频读完为止
        }

        //音视频同步
        //pFrame->pts /= Decoder::ispeed;
        pts = pFrame->pts; // 这一帧的pts （视频时钟）
        if(pts == AV_NOPTS_VALUE) {
            pts = 0;
        }

        pts *= av_q2d(decoder->videostream->time_base);
        pts = decoder->synchronize(pFrame, pts);    //更新视频时钟&处理帧重复
        if(decoder->audioIndex >= 0){
            while (true) {
                if(decoder->isStop){
                    break;
                }
                double audioClk = decoder->getClock();

                pts = decoder->videoclk;
                //qDebug() << "音频时钟：" << audioClk << "视频时钟：" << pts;
                if(pts <= audioClk){
                    // 视频播放慢了，视频追音频
                    break;
                }
                // 视频解析速度大于音频，使用延时等待音频解析
                int delayTime = (pts - audioClk) * 1000;
                delayTime = (delayTime > 5) ? 5 : delayTime; // 延长时间不能超过五秒
                //qDebug() << " 等待";
                SDL_Delay(delayTime); // 等待

            }
        }
        //SDL_Delay(33);//测试用：写死每秒刷新30帧
        //qDebug() << " 等待结束";
        // 解析视频
        res = av_buffersrc_add_frame(decoder->filterSrcCtx, pFrame); // 把pFrame加到过滤器中
        if(res < 0){
            // 失败
            qDebug() << "buffersrc添加失败";
            av_packet_unref(&packet);
            continue;
        }
        res = av_buffersink_get_frame(decoder->filterSinkCtx, pFrame); // 把pFrame加到过滤器中
        if(res < 0){
            // 失败
            qDebug() << "buffersink添加失败";
            av_packet_unref(&packet);
            continue;
        }

        // 执行成功
        // 初始化一张图像
        QImage tempImage(pFrame->data[0], // 图像的数据是pFrame这一帧里面的数据
                                                decoder->pCodecCtx->width, // 图像的宽
                                                decoder->pCodecCtx->height,// 图像的高
                                                QImage::Format_RGB32); // 图像的格式
        // 复制这个数据
        QImage image = tempImage.copy();

        //TODO:显示图像
        decoder->showVideo(image);


        av_frame_unref(pFrame); // 清空frame
        av_packet_unref(&packet); // 清packet
    }
    // 解析完成了
    av_frame_free(&pFrame); // 释放
    qDebug() << "视频已经解析完成了";
    if(!decoder->isStop) {
        // 停掉解码器
        decoder->isStop = true;
    }
    decoder->isDecoderFinish = true; // 解析完成
    if(decoder->gotoStop){ // 他手动停止了
        decoder->setPlayState(STOP);
    } else {
        decoder->setPlayState(FINISH);
    }

    return 0;
}

void Decoder::showVideo(QImage img)
{
    emit signal_ShowVideo(img);
}

double Decoder::getClock()
{
    if(audioIndex >= 0){
        // 存在音频
        return audioPlay->getAudioClock();
    }
    return 0;
}

void Decoder::setVolume(int value)
{
    audioPlay->setVolume(value);
}

double Decoder::synchronize(AVFrame *frame, double pts)
{
    double delay; // 延时
    if(pts != 0) {
        videoclk = pts; //更新视频时钟为当前时间戳
    } else {
        pts = videoclk;
    }
    //考虑帧重复延迟
    delay = av_q2d(pCodecCtx->time_base);
    delay += frame->repeat_pict * (delay * 0.5);
    videoclk += delay;
    return pts;
}

void Decoder::setTemp(double new_temp)
{
    audioPlay->setTemp(new_temp);
}


int Decoder::initFilter()
{
    int res; // 结果
    AVFilterInOut *out = avfilter_inout_alloc(); // 输出
    AVFilterInOut *in = avfilter_inout_alloc(); // 输入
    enum AVPixelFormat pixFmts[] = { // 图像的格式
        AV_PIX_FMT_RGB32, // 适用于绝大多数视频
        AV_PIX_FMT_NONE
    };

    if(filterGraph){ // 如果存在
        avfilter_graph_free(&filterGraph); // 清空地址
    }

    filterGraph = avfilter_graph_alloc(); // 分配空间
    QString filter = "pp=hb/vb/dr/al"; // 过滤pp=hb/vb/dr/al
    QString args = "video_size=%1x%2:" // 视频尺寸
                                    "pix_fmt=%3:" // 照片格式
                                    "time_base=%4/%5:" // 时间基
                                    "pixel_aspect=%6/%7"; // 时间戳
    args = args.arg(pCodecCtx->width)
    .arg(pCodecCtx->height)
    .arg(pCodecCtx->pix_fmt)
    .arg(videostream->time_base.num) // 来自视频流，来自格式上下文
    .arg(videostream->time_base.den)
    .arg(pCodecCtx->sample_aspect_ratio.num)
    .arg(pCodecCtx->sample_aspect_ratio.den);
    res = avfilter_graph_create_filter( // 源图像的过滤器
                                        &filterSrcCtx, // 源
                                        avfilter_get_by_name("buffer"),
                                        "in",
                                        args.toLocal8Bit().data(),
                                        nullptr, filterGraph);
    if(res < 0){
        qDebug() << "过滤器src图像创建失败：" << res;
        avfilter_graph_free(&filterGraph); // 释放
        goto out; // 最后写
    }
    res = avfilter_graph_create_filter( // sink图像的过滤器
                                                                        &filterSinkCtx, // sink
                                                                        avfilter_get_by_name("buffersink"),
                                                                        "out",
                                                                        nullptr,
                                                                        nullptr,filterGraph);
    if(res < 0){
        qDebug() << "过滤器sink图像创建失败：" << res;
        avfilter_graph_free(&filterGraph); // 释放
        goto out;
    }
    // 为编解码器或其他组件设置选项提供方便的方法
    //设置输出滤镜配置
    res = av_opt_set_int_list(filterSinkCtx,
                                                        "pix_fmts",
                                                        pixFmts,
                                                        AV_PIX_FMT_NONE, // 枚举,不带格式
                                                        AV_OPT_SEARCH_CHILDREN);
    if(res < 0){
        qDebug() << "av_opt_set_int_list失败" << res;
        avfilter_graph_free(&filterGraph); // 释放
        goto out;
    }
    // 对上面的out和in做初始化：连接
    out->name = av_strdup("in");
    out->filter_ctx = filterSrcCtx; // 源文件上下文
    out->pad_idx = 0;
    out->next = nullptr;
    in->name = av_strdup("out");
    in->filter_ctx = filterSinkCtx; // 源文件上下文
    in->pad_idx = 0;
    in->next = nullptr;
    if(filter.isNull() || filter.isEmpty()) {
        // 如果没有过滤器，就将src和sink直接连接：将buffer滤镜的输出链接到buffersink滤镜的输入
        res = avfilter_link(filterSrcCtx, 0,
        filterSinkCtx, 0);
        if(res < 0){
            qDebug() << "src和sink连接失败";
            avfilter_graph_free(&filterGraph); // 释放
            goto out;
        }
    } else {// 有过滤器， 把输入输出连接到一起，把过滤器放到图像上，类似于滤镜
        res = avfilter_graph_parse_ptr(filterGraph, filter.toLatin1().data(), &in, &out, nullptr);
        if(res < 0){
            qDebug() << "avfilter_graph_parse_ptr失败";
            avfilter_graph_free(&filterGraph); // 释放
            goto out;
        }
    }
    // 检查所有的配置和连接是否正确
    res = avfilter_graph_config(filterGraph, nullptr);
    if(res < 0) {
        qDebug() << "过滤器检查配置失败" ;
        avfilter_graph_free(&filterGraph);
        goto out;
    }
    out:
        avfilter_inout_free(&in);
        avfilter_inout_free(&out);
        return res;

}
