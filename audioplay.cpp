#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include "audioplay.h"
#include "QDebug"

#define SDL_AUDIO_MIN_BUFFER_SIZE 512 // 音频解码时最小的的缓存 512
#define SDL_AUDIO_MAX_CALLBACKS_PER_SEC 30 // 回调函数，每秒最多调用30次

AudioPlay::AudioPlay(QObject *parent)
    : QObject(parent)
    , isStop(false) // 是否停止
    , isPause(false) // 是否暂停
    , isReadFinished(false) // 是否完成
    ,isDecoderFinish(false)
    , totalTime(0) // 总时长
    , clock(0) // 时钟
    , volume(128) // 声音，最高128
    , audioDeviceFormat(AUDIO_F32SYS) // 设备音频格式，系统的音频播放器
    , aCovertCtx(NULL) // 重采样的上下文
    , sendReturn(0) // 标识
    ,speed(1.0)
{

}

int AudioPlay::openAudio(AVFormatContext *pFormatCtx, int index)
{
// 打开音频的解码器
    // 定义四个变量

    AVCodec *codec; // 编码器
    SDL_AudioSpec wantedSpec; // 目标规格
    int wantedNbChannels; // 目标的通道数
    const char *env; // 环境
    int nextNbChannels[] = {0, 0, 1, 6, 2, 6, 4, 6}; //下一个的通道数，对比他的通道数
    int nextSampleRates[] = {0, 44100, 48000, 96000, 192000}; // 采样率
    int nextSampleRateIndex = FF_ARRAY_ELEMS(nextSampleRates) - 1; // 对音频采样率的通道数初始化，FF_ARRAY_ELEMS 宏用于计算数组的元素数量
    isStop = false;
    isPause = false;
    isReadFinished = false;
    isDecoderFinish = false;

    // src配置
    audioSrcFmt = AV_SAMPLE_FMT_NONE; // 源文件的格式
    audioSrcChannelLayout = 0; // src通道个数（音频src频道布局）
    audioSrcFreq = 0; // 频率

    // 获取音频流
    pFormatCtx->streams[index]->discard = AVDISCARD_DEFAULT; // 得到的流是音频流
    stream = pFormatCtx->streams[index];
    codecCtx = avcodec_alloc_context3(NULL); // 参数传编码器
    avcodec_parameters_to_context(codecCtx, stream->codecpar); // 把参数赋值到上下文里面

    //TODO: 寻找解码器
    codec = const_cast<AVCodec*>(avcodec_find_decoder(codecCtx->codec_id)); // 获取解码器id
    if(codec == NULL){
        qDebug() << "音频解码器找不到";
        avcodec_free_context(&codecCtx); // 释放
        return -1; // 执行失败
    }

    // 打开音频解码器，如果解析视频，要对音频和视频都解析
    int res = avcodec_open2(codecCtx, codec, NULL);
    if(res < 0){ // 0成功，<0失败
        qDebug() << "音频解码器找不到";
        avcodec_free_context(&codecCtx); // 释放
        return -1; // 执行失败
    }

    initFilter2();


    totalTime = pFormatCtx->duration; // 总时长就是上下文的总时长
    // 给audioDstChannelLayout赋值
    env = SDL_getenv("SDL_AUDIO_CHANNELS"); // 对sdl的初始化，获取系统音频设备的声道数
    if(env){    //根据声道数获取声道布局
        // env存在
        qDebug() << "env获取成功";
        wantedNbChannels = atoi(env); // 临时缓存，把字符串转换为数字"1 2 3 4 5 6"->1 2 3 4 5 6
        audioDstChannelLayout = av_get_default_channel_layout(wantedNbChannels);
        // 给通道个数赋值
    }
    wantedNbChannels = codecCtx->channels; // 编码器
    if(!audioDstChannelLayout || (wantedNbChannels != av_get_channel_layout_nb_channels(audioDstChannelLayout))) {
        // 如果audioDstChannelLayout不存在
        audioDstChannelLayout = av_get_default_channel_layout(wantedNbChannels);
        audioDstChannelLayout &= ~AV_CH_LAYOUT_STEREO_DOWNMIX; // 取地址
    }
    wantedSpec.channels = av_get_channel_layout_nb_channels(audioDstChannelLayout); // 目标规格的通道数
    wantedSpec.freq = codecCtx->sample_rate; // 目标规格的采样率

    // 确保channels和freq正常
    if(wantedSpec.channels <= 0 || wantedSpec.freq <= 0){
        qDebug() << "通道数和采样率数值不正确，通道数："
                            << wantedSpec.channels << "采样率："
                            << wantedSpec.freq;
        avcodec_free_context(&codecCtx);
        return -1; // 出现错误，返回-1
    }

    // 采样率相同且nextSampleRateIndex！=0
    while(nextSampleRateIndex && nextSampleRates[nextSampleRateIndex] >= wantedSpec.freq){
        nextSampleRateIndex--;
    }

    // 初始化wantedSpec
    wantedSpec.format = audioDeviceFormat; // 音频播放器格式
    wantedSpec.silence = 0;
    wantedSpec.samples = FFMAX(SDL_AUDIO_MIN_BUFFER_SIZE, 2 << av_log2(wantedSpec.freq / SDL_AUDIO_MAX_CALLBACKS_PER_SEC)); //初始化音频缓冲区大小
    // 让2左移两个值
    wantedSpec.callback = &AudioPlay::audioCallback; // 回调函数：先写下面
    wantedSpec.userdata = this;

    // 调用播放音频的函数去处理他
    while(true){
        while(SDL_OpenAudio(&wantedSpec, &spec) < 0){
            // 打开失败，尝试其他配置
            qDebug() << QString("SDL_OpenAudio(%1 channels, %2 HZ):%3")
                                .arg(wantedSpec.channels) // 通道数
                                .arg(wantedSpec.freq) // 频率
                                .arg(SDL_GetError()); // 错误信息

            wantedSpec.channels = nextNbChannels[FFMAX(7, wantedSpec.channels)]; // 去找通道数，在两个里面选最大的
            if(!wantedSpec.channels){ // 如果声道数为0
                wantedSpec.freq = nextSampleRates[nextSampleRateIndex--];   //尝试更小的采样率
                wantedSpec.channels = wantedNbChannels; //获取解码器指定的声道数
                if(!wantedSpec.freq){   //如果采样率为0
                    avcodec_free_context(&codecCtx); // 释放内存空间
                    qDebug() << "没有更多的组合去尝试，音频打开失败";
                    return -1; //OpenAudio打开失败
                }
            }
            audioDstChannelLayout = av_get_default_channel_layout(wantedSpec.channels);
        }
        if(spec.format != audioDeviceFormat){   //若音频播放器格式!=音频格式则重启音频播放器
            qDebug() << "SDL不支持音频格式";
            wantedSpec.format = spec.format;
            audioDeviceFormat = spec.format;
            SDL_CloseAudio(); // 关闭音频
        }else{
            break; // 打开成功
        }
    }


    if(spec.channels != wantedSpec.channels){
        audioDstChannelLayout = av_get_default_channel_layout(spec.channels); // channels通道数
        if(!audioDstChannelLayout){
            avcodec_free_context(&codecCtx);
            qDebug() << "SDL不支持对应的通道数";
            return -1;
        }
    }
    // 设置音频目标格式&音频深度
    switch (audioDeviceFormat) {
        case AUDIO_U8:
            audioDstFmt = AV_SAMPLE_FMT_U8;
            audioDepth = 1;
            break;
        case AUDIO_S16SYS:
            audioDstFmt = AV_SAMPLE_FMT_S16;
            audioDepth = 2;
            break;
        case AUDIO_S32SYS:
            audioDstFmt = AV_SAMPLE_FMT_S32;
            audioDepth = 4; // 32/8=4
            break;
        case AUDIO_F32SYS:
            audioDstFmt = AV_SAMPLE_FMT_FLT;
            audioDepth = 4; // 32/8=4
            break;
        default:
            audioDstFmt = AV_SAMPLE_FMT_S16;
            audioDepth = 2;
            break; // 默认按S16的配置来
    }
    SDL_PauseAudio(0); // 到这里就把音频文件打开了
    return 0;
}

void AudioPlay::enQueue(AVPacket *packet)
{
    packetQueue.enQueue(packet);
}

void AudioPlay::audioCallback(void *userdata, quint8 *stream, int SDL_AudioBufSize)
{
    //qDebug() << "audioCallback";
    AudioPlay *decoder = (AudioPlay *)userdata; // 解码器
    int decoderSize; // 解析缓冲区大小

    while(SDL_AudioBufSize > 0){
        if(decoder->isStop){
            SDL_PauseAudio(1);
            return;
        }

        if(decoder->isPause){
            SDL_Delay(50);
            continue;
        }
        if(decoder->audioBufIndex >= decoder->audioBufSize){// 音频解析缓存已无数据，解析新的音频数据
            decoderSize = decoder->decodeAudio();
            if(decoderSize <= 0){
                // 解析失败
                qDebug() << "音频解析失败";
                decoder->audioBufSize = 1024;
                decoder->audioBuf = nullptr; // 付为空指针
            } else {
                // 解析成功
                decoder->audioBufSize = decoderSize;
            }
            decoder->audioBufIndex = 0;
        }
        int left = decoder->audioBufSize - decoder->audioBufIndex; // 下标左侧有多少个，走了多少个值/还有多少数据
        if(left > SDL_AudioBufSize){    //
            //qDebug() << "left过大: "   << decoder->audioBufSize << left << SDL_AudioBufSize;
            left = SDL_AudioBufSize;
        }
        if(decoder->audioBuf) { //混合音频缓冲区
            memset(stream, 0, left);    //初始化stream缓冲区
            SDL_MixAudio(stream, decoder->audioBuf + decoder->audioBufIndex, left, decoder->volume);
        }
        SDL_AudioBufSize -= left;
        stream += left;
        decoder->audioBufIndex += left;
    }
}

int AudioPlay::initFilter()
{
// 初始化音频滤波器
    QByteArray filtersByteArray = QString("atempo=%1").arg(speed, 0, 'f', 2).toUtf8();  //设置播放速率
    const char *filtersDescr = filtersByteArray.data();
    qDebug() << filtersDescr;
    filter_graph = avfilter_graph_alloc();
    if (!filter_graph)
        return false;
    const AVFilter *abuffer = avfilter_get_by_name("abuffer");
    const AVFilter *abuffersink = avfilter_get_by_name("abuffersink");
    AVFilterInOut* out = avfilter_inout_alloc();
    AVFilterInOut* in = avfilter_inout_alloc();
    if (!abuffer || !abuffersink)
        return false;

//    filter_context = avfilter_graph_alloc_filter(filter_graph, abuffer, "src");
//    sink_context = avfilter_graph_alloc_filter(filter_graph, abuffersink, "sink");
//    AVFilterContext *atempoFilterContext = avfilter_graph_alloc_filter(filter_graph, atempo, "atempo");
    char args[512];
    snprintf(args, sizeof(args),
                             "time_base=1/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIx64, codecCtx->sample_rate, codecCtx->sample_rate, av_get_sample_fmt_name(codecCtx->sample_fmt), codecCtx->channel_layout);
    avfilter_graph_create_filter(&filter_context, abuffer, "in", args, nullptr, filter_graph);
    avfilter_graph_create_filter(&sink_context, abuffersink, "out", nullptr, nullptr, filter_graph);

    out->name = av_strdup("in");
    out->filter_ctx = filter_context;
    out->pad_idx = 0;
    out->next = nullptr;

    in->name = av_strdup("out");
    in->filter_ctx = sink_context;
    in->pad_idx = 0;
    in->next = nullptr;

    int result = avfilter_graph_parse_ptr(filter_graph, filtersDescr, &in, &out, nullptr);
    if (result < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
        av_strerror(result, errbuf, sizeof(errbuf));
        qDebug() << "ERROR: Couldn't initialize audio filter graph: " << QString::fromUtf8(errbuf);
        return -1;
    }

    avfilter_graph_config(filter_graph, nullptr);
    avfilter_inout_free(&in);
    avfilter_inout_free(&out);
    return result;
}

int AudioPlay::initFilter2()
{
    int res = -1;
    //const char *filtersDescr = "rubberband=tempo=1.0"; // 初始播放速度
    // 初始化滤镜图
    filter_graph = avfilter_graph_alloc();
    if (!filter_graph) {
        qDebug() << "Failed to allocate filter graph.";
        return -1;
    }

    const AVFilter *abuffer = avfilter_get_by_name("abuffer");
    const AVFilter *rubberband = avfilter_get_by_name("rubberband");
    const AVFilter *abuffersink = avfilter_get_by_name("abuffersink");
    if (!abuffer || !rubberband || !abuffersink) return false;

    filter_context = avfilter_graph_alloc_filter(filter_graph, abuffer, "src");
    sink_context = avfilter_graph_alloc_filter(filter_graph, abuffersink, "sink");
    atempo_ctx  = avfilter_graph_alloc_filter(filter_graph, rubberband, "rubberband");

    // 配置 abuffer 滤镜的参数
    char args[512];
    snprintf(args, sizeof(args), "time_base=%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIx64
             , stream->time_base, codecCtx->sample_rate, av_get_sample_fmt_name(codecCtx->sample_fmt), codecCtx->channel_layout);
    if (avfilter_init_str(filter_context, args) < 0) return -1;
    if (avfilter_init_str(sink_context, nullptr) < 0) return -1;
    if (avfilter_init_str(atempo_ctx, QString("tempo=%1").arg(speed, 0, 'f', 2).toUtf8()) < 0) return -1;
    if (avfilter_link(filter_context, 0, atempo_ctx, 0) < 0) return -1;
    if (avfilter_link(atempo_ctx, 0, sink_context, 0) < 0) return -1;
    if (avfilter_graph_config(filter_graph, nullptr) < 0)return -1;
    return 1;
}

void AudioPlay::setTemp(double new_tempo)
{
    speed = new_tempo;
}

bool AudioPlay::getIsStop() const
{
    return isStop;
}

void AudioPlay::setIsStop(bool value)
{
    isStop = value;
}


int AudioPlay::decodeAudio()
{
    int res;
    AVFrame *frame = av_frame_alloc();
    int resampleDataSize; // 重采样数据大小
    if(!frame){
        qDebug() << "frame初始化失败";
        return -1;
    }
    if(isStop){
        qDebug() << "修改isDecoderFinish: true";
        return -1; // 音频可能播放完成了
    }
sendPacket:
    if(packetQueue.queueSize() == 0){
        // 读取完成
        if(isReadFinished){
            isStop = true;
            qDebug() << "修改isDecoderFinish: true";
            isDecoderFinish = true;
            SDL_Delay(100);
            //TODO:通知其他类播放完成
            emit signal_AudioPlayFinished();
        }
        return -1;
    }
    if(sendReturn != AVERROR(EAGAIN)){  //若解码器不忙
        packetQueue.deQueue(&packet, true); // 把那一帧弹出来
    }

    if(strcmp((char *)packet.data, "FLUSH") == 0){  //执行播放位置更改
        qDebug() << "正在刷新音频";
        avcodec_flush_buffers(codecCtx);
        av_packet_unref(&packet);
        av_frame_free(&frame);
        sendReturn = 0;
        return -1;
    }

    int t_i = 0;

    // 音频发送解码器
    sendReturn = avcodec_send_packet(codecCtx, &packet);
    if((sendReturn < 0) && (sendReturn != AVERROR(EAGAIN)) && (sendReturn != AVERROR_EOF)) {
        av_packet_unref(&packet); // 释放
        av_frame_free(&frame); //
        qDebug() << "音频发送解码器失败";
        return sendReturn;
    }

    // 音频解码
    res = avcodec_receive_frame(codecCtx, frame); // 解码器去解析，frame接收
    if((res < 0) && (res != AVERROR(EAGAIN))){
        av_packet_unref(&packet); // 释放
        av_frame_free(&frame); //
        qDebug() << "音频解码失败";
        return res;
    }

    if(frame->pts != AV_NOPTS_VALUE) {  //时间戳有效
        //qDebug() << "时间戳有效";
        clock = av_q2d(stream->time_base) * frame->pts; // 同步，可以算出播放着一段音频的时间是多少，保存到clock里面
    }

    //使用过滤器
    //qDebug() << "输入帧样本数: " << frame->nb_samples;
    res = av_buffersrc_add_frame(filter_context, frame); // 把pFrame加到过滤器中
    if(res < 0){
        // 失败
        qDebug() << "buffersrc添加失败";
        av_packet_unref(&packet);
        return res;
    }
    t_i++;

    res = av_buffersink_get_frame(sink_context, frame); // 把pFrame加到过滤器中
//    if(res < 0){
//        // 失败
//        char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
//        av_strerror(res, errbuf, sizeof(errbuf));
//        qDebug() << "buffersink添加失败:" << QString::fromUtf8(errbuf);
//        av_packet_unref(&packet);
//        return res;
//    }
    if(t_i < 5 && res < 0){
        goto sendPacket;
    }
    //qDebug() << "输出帧样本数: " << frame->nb_samples;

    //qDebug() << "avcodec_receive_frame: 结束" << t_i;
    qint64 inChannelLayout = (frame->channel_layout && frame->channels == av_get_channel_layout_nb_channels(frame->channel_layout)) ?
                                                        frame->channel_layout : av_get_channel_layout_nb_channels(frame->channels);



    // 初始化音频重采样上下文
    if(frame->format != audioSrcFmt || inChannelLayout != audioSrcChannelLayout || frame->sample_rate != audioSrcFreq || !aCovertCtx){
        if(aCovertCtx){
            swr_free(&aCovertCtx);
        }

        aCovertCtx = swr_alloc_set_opts(nullptr, audioDstChannelLayout, audioDstFmt, spec.freq,
                                        inChannelLayout,  (AVSampleFormat)frame->format, frame->sample_rate, 0, nullptr);


        if(!aCovertCtx || (swr_init(aCovertCtx) < 0)){// 音频重采样上下文不存在
            av_packet_unref(&packet); // 释放
            av_frame_free(&frame);
            return -1;
        }
        // 对参数进行初始化
        audioSrcFmt = (AVSampleFormat)frame->format;
        audioSrcChannelLayout = inChannelLayout;
        audioSrcFreq = frame->sample_rate;
        audioSrcChannels = frame->channels;
        qDebug() << audioSrcFreq << frame->sample_rate << spec.freq << frame->nb_samples;
    }

//    if(frame->format != audioSrcFmt ||  inChannelLayout != audioSrcChannelLayout ||
//                frame->sample_rate != audioSrcFreq || !aCovertCtx){
//        // ...
//        // 初始化音频重采样上下文
//        // ...
//        if(!aCovertCtx || (swr_init(aCovertCtx) < 0)){// 音频重采样上下文不存在
//            av_packet_unref(&packet); // 释放
//            av_frame_free(&frame);
//            return -1;
//        }
//        // 对参数进行初始化
//        audioSrcFmt = (AVSampleFormat)frame->format;
//        audioSrcChannelLayout = inChannelLayout;
//        audioSrcFreq = frame->sample_rate;
//        audioSrcChannels = frame->channels;
//    }
    if(aCovertCtx){
        const quint8 **in = (const quint8 **)frame->extended_data; // 将扩展数据转换为一个quint8类型的二重指针赋值给in
        uint8_t *out[] = {audioBuf1};
        int outCount = sizeof(audioBuf1) / spec.channels / av_get_bytes_per_sample(audioDstFmt); // 每个通道可写入的最大样本数
        // 对重采样转换
        //int sampleSize = swr_convert(aCovertCtx, out, outCount, in, frame->nb_samples);
        int sampleSize = swr_convert(aCovertCtx, out, outCount, in, frame->nb_samples);
        if(sampleSize < 0){
            qDebug() << "重采样转换失败";
            av_packet_unref(&packet);
            av_frame_free(&frame);
            return -1;
        }
        if(sampleSize == outCount){
            qDebug() << "输入缓存可能太大了：";
            if(swr_init(aCovertCtx) < 0){
                swr_free(&aCovertCtx); // 释放
            }
        }
        audioBuf = audioBuf1;
        resampleDataSize = spec.channels * sampleSize * av_get_bytes_per_sample(audioDstFmt); //实际转换的样本大小
    } else {
        audioBuf = frame->data[0];
        resampleDataSize = av_samples_get_buffer_size( NULL, frame->channels, frame->nb_samples, static_cast<AVSampleFormat>(frame->format),1);
    }

    // 时钟 += 数据大小/深度*声道数*采样率
    clock += static_cast<double>(resampleDataSize) / (audioDepth * codecCtx->channels * codecCtx->sample_rate); // 用来做同步的
    if(sendReturn != AVERROR(EAGAIN)) {
        av_packet_unref(&packet); // 释放
    }
    av_frame_free(&frame); // 释放
    //qDebug() << "resampleDataSize：" << resampleDataSize;
    return resampleDataSize;
}

void AudioPlay::closeAudio()
{
    emptyAudioData(); // 清空音频数据
    SDL_PauseAudio(1);
    SDL_Delay(100);
    SDL_LockAudio(); // 锁当前正在播放的音频
    SDL_ClearQueuedAudio(0);  // 清空音频缓冲区
    SDL_CloseAudio(); // 关闭当前正在播放的音频
    SDL_UnlockAudio(); // 解锁
    avcodec_close(codecCtx); // 释放编码器上下文
    avcodec_free_context(&codecCtx); // 释放
    avfilter_graph_free(&filter_graph);
}

void AudioPlay::emptyAudioData()
{
    // 清空音频数据
    audioBuf = nullptr;
    audioBufIndex = 0; // 缓存的偏移
    audioBufSize = 0;
    audioBufSize1 = 0;
    clock = 0;
    sendReturn = 0;
    packetQueue.empty();
}

void AudioPlay::readFinished()
{
    isReadFinished = true;
}

void AudioPlay::pauseAudio(bool pause)
{
    isPause = pause;
}

void AudioPlay::stopAudio()
{
    isStop = true;
}

int AudioPlay::getVolume() const
{
    return volume;
}

void AudioPlay::setVolume(int value)
{
    volume = value;
}

double AudioPlay::getAudioClock()
{
    if(codecCtx) {
        int hBufsize = audioBufSize - audioBufIndex; // 已经播放过的字节数
        int bytesPerSec = codecCtx->sample_rate * codecCtx->channels * audioDepth; // 每秒有多少个字节
        clock -= static_cast<double>(hBufsize) / bytesPerSec; // 现在播放到的秒数
    }
    //qDebug() << "音频时钟为：" << clock;
    return clock;
}

void AudioPlay::setSpeed(int value)
{
    speed = value;
}
