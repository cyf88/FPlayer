
#ifndef RTSPDECODER_H
#define RTSPDECODER_H

#include <memory>

#include "StreamDecoder.h"
#include "RTSPClientWrapper.h"

using namespace rtspwrap;

class RTSPDecoder : public StreamDecoder {

public:
    
    using StreamDecoder::StreamDecoder;

    void parse() override;


private:
    RTSPDecoder();



    int _frameNum = 0;


    // 缓存区大小
    size_t _bufferSize = 1024 * 1024;

    RTSPClientWrapper::Ptr _rtspClient;

    
    FILE* stream;


};


#endif