
#ifndef STREAMDECODER_H
#define STREAMDECODER_H

#include "SVACDecoder.h"
#include "SDKManager.h"


class StreamDecoder {

public:

	using OnFrame = std::function<void(const Frame& frame)>;

    StreamDecoder(const std::string filePath, const std::string streamId) {
        _filePath = filePath;
        _streamId = streamId;
    }
    StreamDecoder(const std::string filePath) {
        _filePath = filePath;
    }
    ~StreamDecoder() {};


    virtual void parse() = 0;

    //解码帧回调,不要子类实现
    virtual void setOnFrame(OnFrame onFrame) final {
        _onFrame = onFrame;
    }

    //原始帧回调
    virtual void setOnOrignFrame(OnFrame onFrame) final {
        _onOriginFrame = onFrame;
    }


protected:

    int _frameNum = 0;

    // 文件路径
    std::string _filePath;
    std::string _streamId;


    // 回调函数
    OnFrame _onFrame;

    OnFrame _onOriginFrame;

    volatile bool _bStop = false;

    SVACDecoder::Ptr _svacDecoder = NULL;


};

#endif
