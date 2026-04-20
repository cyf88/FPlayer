#ifndef __tricoreapi__
#define __tricoreapi__
#define TEST
#define TRI_WINDLL
#ifdef TRI_WINDLL
// Prevent winsock.h from being pulled in via windows.h
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <thread>
#include <list>
#define TRI_RET unsigned long __stdcall
#define TRI_DLLCALL __stdcall
#else
#define TRI_RET ULONG
#define FM_DLLCALL
#endif


#define TRI_I /* input  parameter */
#define TRI_O /* output parameter */
#define TRI_B /* both input and output parameter */

#define TRI_ERROR_NOT_SET_GB_CODE 20000
#define TRI_ERROR_NOT_SET_SERVER_IP 20001
#define TRI_ERROR_NOT_SET_SERVER_PORT 20002
#define TRI_ERROR_NOT_SET_CLIENT_ID 20003
#define TRI_ERROR_NOT_SET_CLIENT_PORT 20004
#define TRI_SDK_HAS_INITED 20005
#define TRI_SDK_SIP_DESTORY_FAIL 20006
#define TRI_SDK_LOGIN_HAS_NOT_INITED 20007
#define TRI_SDK_HAS_NOT_INITED 20008

#define TRI_NO_RESPONSE_FROM_SERVER 30004
#define TRI_NO_R1_FROM_SERVER 30005
#define TRI_UNKNOWN_ERROR_FROM_SERVER 30006
#define TRI_LOGIN_ERROR_FROM_SERVER 30007
#define TRI_NO_DECODEHANDLE 30008
#define TRI_SIGN_R_ERROR 30009
#define TRI_SIGN_S_ERROR 30010
#define TRI_VERIFY_SIGN_FAIL 30011
#define TRI_SES_ERROR 30012
#define TRI_MEMERY_MOLLOC_ERROR 30013
#define TRI_INPUT_DATA_ERROR 30014
#define TRI_DECODE_SAME_TIME 30015
#define TRI_GENERATE_EVEK_ERROR 30016

#define TRI_PARAM_GB_Code "SIPServerGBCode"
#define TRI_PARAM_SERVER_IP "SIPServerIP"
#define TRI_PARAM_SERVER_PORT "SIPServerPort"
#define TRI_PARAM_CLIENT_ID "SIPClientID"
#define TRI_PARAM_CLIENT_PORT "SIPClientPort"
#define TRI_PARAM_KAT "KeepAliveTime"


#ifdef __cplusplus
extern "C" {
#endif

    //**********媒体流整帧数据结构****************
    typedef struct stru_TRI_ImgFrame_Unit  // 编码后媒体数据信息结构体
    {
        unsigned int type;       // 'I' = I帧编码数据; 'P' = P帧编码数据;
        unsigned int fsn;        // 帧序号
        unsigned int width;      // 解码帧宽度
        unsigned int height;     // 解码帧高度
        unsigned int imgsz;      // 以字节为单位的编码后媒体数据长度
        unsigned char* img_buf;  // 编码后媒体数据缓冲区  
        unsigned int sign_result;  // 验签结果，0-验签成功1-验签失败 	2 - 没有签名
        unsigned char stream_id[20];  //码流中的id
        unsigned int security_level; // 安全等级  0-未知  1-A   2-B   3-C 
    } IMG_FRAME_UNIT;


    typedef struct decoder_info_resp {

        char deviceId[20];
        unsigned int online;
        char name[100];

    } DECODER_INFO_RESP;
    /*
            客户端插件初始化

            返回值：0 调用成功 其它 调用失败，返回错误码。
    */
    TRI_RET TRI_SUT_Init(TRI_I char* PIN);

    /*
            客户端插件参数设置，具体参数有：
            SIPServerGBCode= 30420000001090000001
            SIPServerIP=192.168.1.2
            SIPServerPort=5060
            KeepAliveTime=60
            ExpireTime=3600
            返回值：0 调用成功 其它 调用失败，返回错误码。
    */
    TRI_RET TRI_SUT_SetConfigParameter(TRI_I const char* key, TRI_I char* value);

    /*
            客户端登录
            pUserId：返回用户国标ID
            idLength： 返回用户国标id长度
            token：返回用户token
            tokenlength：返回用户token长度
            返回值：0 调用成功 其它 调用失败，返回错误码。
    */
    TRI_RET TRI_SUT_Login(TRI_O char** pUserId, TRI_O ULONG* idLength,
        TRI_O char** token, TRI_O ULONG* tokenLength);

    /*
            获取UKey在线状态
            puiUKeyStatus 0：正常可用； 非0：不可用状态
            返回值：0 调用成功 其它 调用失败，返回错误码。
    */
    TRI_RET TRI_SUT_GetUKeyStatus(TRI_O UINT* puiUKeyStatus);

    /*
            申请视频流解密通道，调用此接口后，此时插件返回句柄。
            decodeHandle返回解密句柄。后续传加密码流时需传入此参数表明是同一路流的数据。
            返回值：0 调用成功 其它 调用失败，返回错误码。
    */
    TRI_RET TRI_SUT_GetStreamSecDecode(TRI_O ULONG* decodeHandle);

    /*
            申请实时流解密通道，调用此接口后，此时插件需要事先查询VKEK,并接收VKEK的变更通知。
            decodeHandle，返回解密句柄。后续传加密码流时需传入此参数表明是同一路流的数据。
            deviceId,请求解密的通道ID。
            返回值：0 调用成功 其它 调用失败，返回错误码。
    */
    TRI_RET TRI_SUT_GetRealStreamSecDecode(TRI_I ULONG* decodeHandle,
        TRI_I char* deviceId);

    /*
            申请录像流解密。调用此接口后，插件需事先查询历史VKEK。
            decodeHandle，返回解密句柄。后续传加密码流时需传入此参数表明是同一路流的数据。
            deviceId,请求解密的通道ID。
            beginTime，开始时间，格式” 2019-07-30T10:28:47.000”
            endTime，结束时间, 格式” 2019-07-31T10:28:47.000”
            返回值：0 调用成功 其它 调用失败，返回错误码。
    */
    TRI_RET TRI_SUT_GetPlayBackSecDecode(TRI_I ULONG* decodeHandle,
        TRI_I char* deviceId,
        TRI_I char* beginTime,
        TRI_I char* endTime);

    /*
            对视频流数据进行解密处理，视频数据解密扩展接口
            decodeHandle：解密通道句柄
            pstImgData：指向视频解密前的视频数据指针
            ppstOutData：指向视频解密后的视频数据指针的指针（其内存空间由内部申请，需要由调用者使用后释放）
            isSignVerify：是否进行验签，0不进行验签，其他进行验签，默认不进行验签
            返回值：0 调用成功 其它 调用失败，返回错误码。
    */
    TRI_RET TRI_SUT_VideoDataSecDecodeExt(TRI_I ULONG* decodeHandle,
        TRI_I IMG_FRAME_UNIT* pstImgData,
        TRI_O IMG_FRAME_UNIT** ppstOutData,
        TRI_I BOOL isSignVerify);

    /*
            申请视频流加密通道，调用此接口后，此时插件返回句柄。
            encodeHandle，返回加密句柄。后续传待加密码流时需传入此参数表明是同一路流的数据。
            返回值：0 调用成功 其它 调用失败，返回错误码。
    */
    TRI_RET TRI_SUT_GetStreamSecEncode(TRI_O ULONG* encodeHandle);

    /*
            对视频流数据进行加密处理，视频数据加密扩展接口
            encodeHandle：加密通道句柄
            pstImgData：指向视频加密前的视频数据指针
            ppstOutData：指向视频加密后的视频数据指针的指针（其内存空间由内部申请，需要由调用者使用后释放）
            返回值：0 调用成功 其它 调用失败，返回错误码。
    */
    TRI_RET TRI_SUT_VideoDataSecEncodeExt(TRI_I ULONG* encodeHandle,
        TRI_I IMG_FRAME_UNIT* pstImgData,
        TRI_O IMG_FRAME_UNIT** ppstOutData);


    /*
            下发实时视频点播信令
    // 参数: deviceId：点播相机的国标编号
    //      address：sdp参数点播收流IP
    //      port：sdp参数点播收流端口
    //      ssrc：sdp参数点播ssrc
    //      isVideo：sdp参数是否视频
    //      isTcp：sdp参数是否使用tcp
    //      setup：sdp参数流获取类型默认passive
    返回值：0 调用成功 其它 调用失败，返回错误码。

    */
    TRI_RET TRI_SUT_PLAY_INVITE(TRI_O ULONG* inviteHandle, TRI_I const char* deviceId, TRI_I const char* address, TRI_I int port, TRI_I const char* ssrc, TRI_I bool isVideo, TRI_I bool isTcp, TRI_I const char* setup);

    /*
           下发历史视频点播信令
    // 参数: deviceId：点播相机的国标编号
    //		startTime：sdp参数开始时间时间戳
    //		endTime：sdp参数结束时间时间戳
    //      address：sdp参数点播收流IP
    //      port：sdp参数点播收流端口
    //      ssrc：sdp参数点播ssrc
    //      isVideo：sdp参数是否视频
    //      isTcp：sdp参数是否使用tcp
    //      setup：sdp参数流获取类型默认passive
   */
    TRI_RET TRI_SUT_PLAYBACK_INVITE(TRI_O ULONG* inviteHandle, TRI_I const char* deviceId, TRI_I int startTime, TRI_I int endTime, TRI_I const char* address, TRI_I int port, TRI_I const char* ssrc, TRI_I bool isVideo, TRI_I bool isTcp, TRI_I const char* setup);



    /*
        获取sdk关联的解码器信息
        返回解码器结构数据
    */
    TRI_RET TRI_SUT_GET_DECODERS(TRI_O std::list<DECODER_INFO_RESP>* decoderInfoResp);
    

    /*
    对墙进行分屏
     参数:  decoderDeviceId：解码器编号
            decoderChannelId：解码器通道号
            row：宽需要分几个
            col：高需要分几个
    */
    TRI_RET TRI_SUT_SET_DECODERS_CONTROL(TRI_I char* decoderDeviceId, TRI_I char* decoderChannelId, TRI_I char* row, TRI_I char* col);
    
    
    /*
    * 上墙
    参数:   decoderDeviceId：解码器编号
            decoderChannelId：解码器通道号
            numScreen：屏幕编号 屏幕编号以从上到下从左都有顺序排序1234
            deviceId：需要播放的视频设备编号
            channelId 需要播放的视频通道号
    */
    TRI_RET TRI_SUT_SET_DECODERS_INVITE(TRI_I char* decoderDeviceId, TRI_I char* decoderChannelId, TRI_I char* numScreen, TRI_I char* deviceId, TRI_I char* channelId);
    
    /*
    * 发送墙点播bye
    参数:   decoderDeviceId：解码器编号
            decoderChannelId：解码器通道号
            numScreen：屏幕编号 屏幕编号以从上到下从左都有顺序排序1234
    */
    TRI_RET TRI_SUT_SET_DECODERS_BYE(TRI_I char* decoderDeviceId, TRI_I char* decoderChannelId, TRI_I char* numScreen);
    
    /*
    对invite信令下发BYE
    */
    TRI_RET TRI_SUT_BYE(TRI_I const char* deviceId, TRI_I ULONG* inviteHandle);


    /*
            相关内存清理
    */
    TRI_RET TRI_SUT_Free(TRI_I IMG_FRAME_UNIT** ppstOutData);

    /*
            相关资源清理
    */
    TRI_RET TRI_SUT_Cleanup();

    /*
            释放解密通道
    */
    TRI_RET TRI_SUT_Close(TRI_I ULONG handle);

#ifdef __cplusplus
}
#endif /* #ifdef  __cplusplus */

#endif  // !__tricoreapi__
