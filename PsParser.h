#pragma once
#include <cstdint>
#include <functional>
#include <vector>

#define PS_PES_AUDIO					(0x000001C0)
#define PS_PES_VIDEO					(0x000001E0)

// // 帧数据回调函数类型
// // 参数: frameData - 帧数据指针, frameLength - 帧数据长度, frameType - 帧类型(I_frame/P_frame/Audio_frame)
// typedef std::function<void(uint8_t *frameData, size_t frameLength, int frameType)> FrameCallback;

/**
 * PS流解析器
 * 用于从RTP脱壳后的PS流数据中解析出完整的PES帧数据
 * 使用vector管理缓冲区，自动处理内存管理
 */
class PsParser {
public:
    enum class FrameType { I_frame, P_frame, Audio_frame};

    using FrameCallback = std::function<void(uint8_t* frameData, size_t frameLength, FrameType frameType, int codecId)>;

    /**
     * 构造函数
     * @param maxBufferSize 最大缓冲区大小，默认3MB
     */
    PsParser(size_t maxBufferSize = 3 * 1024 * 1024);

    /**
     * 析构函数
     */
    ~PsParser() = default;

    // 禁止拷贝和赋值
    PsParser(const PsParser &) = delete;
    PsParser &operator=(const PsParser &) = delete;

    /**
     * 设置帧数据回调函数
     * @param callback 当解析出完整帧时调用的回调函数
     */
    void setFrameCallback(FrameCallback callback);

    /**
     * 输入PS流数据片段（从RTP脱壳后的数据）
     * @param data 数据指针
     * @param length 数据长度
     * @return 成功返回true，失败返回false
     */
    bool input(const uint8_t *data, size_t length);

    /**
     * 清空内部缓冲区
     */
    void reset();

    /**
     * 获取当前缓冲区使用大小
     * @return 缓冲区已使用大小（字节）
     */
    size_t getBufferSize() const { return _psBuffer.size(); }

    /**
     * 预留缓冲区空间（可选，用于性能优化）
     * @param capacity 预留容量
     */
    void reserve(size_t capacity);

private:
    /**
     * 查找PS包起始位置（查找0x01BA）
     * @param buffer 缓冲区数据指针
     * @param length 缓冲区长度
     * @return 找到返回起始位置（>=0），未找到返回-1
     */
    int findPSTag(const uint8_t *buffer, size_t length);

    /**
     * 移除PS头，提取PES帧数据
     * @param buffer PS包数据指针
     * @param size PS包大小
     * @param frametype 输出参数：帧类型
     * @return 提取的帧数据长度，失败返回-1
     */
    int removePSHead(const uint8_t *buffer, size_t size, int &frametype);

private:
    std::vector<uint8_t> _psBuffer; // PS流缓冲区（使用vector自动管理内存）
    std::vector<uint8_t> _currentPESFrame; // 当前PES帧缓冲区
    size_t _maxBufferSize; // 最大缓冲区大小
    FrameCallback _frameCallback; // 帧数据回调函数
    int _codecId; // 编码ID 参考mpeg-proto.h

};
