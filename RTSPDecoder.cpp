
#include "RTSPDecoder.h"
#include <iostream>
#include <vector>
#include <fstream>



void RTSPDecoder::parse() {

	_rtspClient = std::make_shared<RTSPClientWrapper>();
	int j = 0;
	
	if ((stream = fopen("D://rtsp3/rtsp7.es", "wb")) == NULL)  // 文件写入
	{
		std::cout << "stream file open error !" << std::endl;
		return;
	}

	auto onFrame = [&](const uint8_t* data, unsigned size, timeval presentationTime) {
		// 示例：打印帧信息（真实场景应区分 codec、解析/封包等）
		long long pts_us = static_cast<long long>(presentationTime.tv_sec) * 1000000LL
			+ static_cast<long long>(presentationTime.tv_usec);
		std::cout << "Frame: size=" << size << " bytes, pts(us)=" << pts_us << std::endl;

		//fwrite(data, sizeof(unsigned char), size, stream);

		//std::ofstream outfile;
		//outfile.open((std::string("F://rtsp3//frame.") + std::to_string(j++)).c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
		//outfile.write((char*)data, size);
		//outfile.close();
		//  在这里创建解码器
		Frame frame;
		frame.data = std::vector<uint8_t>(data, data + size);
		frame.size = size;
		frame.timestamp = 0; // 如果有时间戳信息，可以在这里填充
		_frameNum++;

		if (_onOriginFrame) {
			_onOriginFrame(frame);
		}
		// 如果设置了回调函数，调用回调
		if (_onFrame) {
			//解密34020000001319000111
			//std::string chid = "13102800001310434532";
			//std::string chid = "31011501031311000001";
			//chid = "46020300001321901221";

			/*  svac 解密解码*/
			
            // if (!_decryptChannel) {
            // 	_decryptChannel = SDKManager::Instance().createChannel(_streamId, ChannelType::DecryptChannel);
            // 	if (!_decryptChannel) return;
            // }
            // Frame decFrame;
            // _decryptChannel->doWork(frame, decFrame);

            // if (!_svacDecoder) {
            // 	_svacDecoder = std::make_shared<SVACDecoder>(0);
            // 	_svacDecoder->setOnDecode([&](const Frame& frame) {
            // 		_onFrame(frame);
            // 		});
            // }
            // _svacDecoder->inputFrame(decFrame);
			

			// h264解码

			//if (!_h264Decoder) {
			//	_h264Decoder = std::make_shared<H264Decoder>(1);
			//	_h264Decoder->setOnDecode([&](const Frame& frame) {
			//		_onFrame(frame);
			//		});
			//}
			//_h264Decoder->inputFrame(frame);

		}
	
	};

	if (!_rtspClient->start(_filePath, onFrame)) {
		std::cerr << "rtsp client start failed, url: " << _filePath << std::endl;
	}


}


