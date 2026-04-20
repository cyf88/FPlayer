#include "PsParser.h"
#include "logger.h"
#include <algorithm>
#include <iostream>

using namespace std;

// 最大帧大小
#define MAX_FRAME_SIZE (3 * 1024 * 1024)

PsParser::PsParser(size_t maxBufferSize)
    : _maxBufferSize(maxBufferSize)
    , _frameCallback(nullptr)
    , _codecId(-1) {
    _psBuffer.reserve(maxBufferSize);
    _currentPESFrame.reserve(MAX_FRAME_SIZE);
}

void PsParser::reserve(size_t capacity) {
    _psBuffer.reserve(capacity);
}

void PsParser::setFrameCallback(FrameCallback callback) {
    _frameCallback = callback;
}

void PsParser::reset() {
    _psBuffer.clear();
    _currentPESFrame.clear();
}

bool PsParser::input(const uint8_t *data, size_t length) {
    if (data == nullptr || length == 0) {
        return false;
    }

    // 检查缓冲区是否足够
    if (_psBuffer.size() + length > _maxBufferSize) {
        BLOG(error) << "PS buffer overflow, reset buffer. size: " << _psBuffer.size() << ", new data: " << length << ", max: " << _maxBufferSize;
        reset();
        return false;
    }

    // 将新数据追加到缓冲区（vector自动管理内存）
    _psBuffer.insert(_psBuffer.end(), data, data + length);

    // 防止缓冲区过大，重置缓冲区
    if (_psBuffer.size() > 10 * 1024 * 1024) {
        BLOG(error) << "PS buffer too large, reset buffer. size: " << _psBuffer.size();
        reset();
        return false;
    }

    // 循环解析PS包，直到没有完整的PS包
    while (true) {
        // 查找PS包起始位置
        int psStartPos = findPSTag(_psBuffer.data(), _psBuffer.size());
        if (psStartPos < 0) {
            // 没有找到PS包起始位置，可能数据不完整，等待更多数据
            break;
        }

        // 查找下一个PS包起始位置，确定当前PS包的大小
        int nextPsStartPos = findPSTag(_psBuffer.data() + psStartPos + 4, _psBuffer.size() - psStartPos - 4);
        int psPacketSize = 0;

        if (nextPsStartPos > 0) {
            // 找到了下一个PS包
            psPacketSize = nextPsStartPos + 4; // 包含起始码4字节
        } else {
            // 没有找到下一个PS包，可能当前PS包还未完整，等待更多数据
            break;
        }

        // 解析当前PS包
        int frametypeInt = -1;
        _currentPESFrame.clear(); // 清空上一帧数据
        int pesFrameLength = removePSHead(_psBuffer.data() + psStartPos, psPacketSize, frametypeInt);

        if (pesFrameLength > 0 && _frameCallback != nullptr) {
            // 调用回调函数，传递解析出的帧数据
            FrameType frametype = static_cast<FrameType>(frametypeInt);
            _frameCallback(_currentPESFrame.data(), pesFrameLength, frametype, _codecId);
        }

        // 移除已解析的PS包，保留剩余数据
        size_t remainingSize = _psBuffer.size() - psStartPos - psPacketSize;
        if (remainingSize > 0) {
            // 使用vector的erase和begin迭代器，更高效
            _psBuffer.erase(_psBuffer.begin(), _psBuffer.begin() + psStartPos + psPacketSize);
        } else {
            _psBuffer.clear();
        }
    }

    return true;
}

int PsParser::findPSTag(const uint8_t *buffer, size_t length) {
    if (buffer == nullptr || length < 4) {
        return -1;
    }

    uint32_t code = 0xFFFFFFFF;
    const uint8_t *pos = buffer;
    size_t rest = length;

    while (rest-- > 0) {
        code = (code << 8) | *pos++;
        if (code == 0x01BA) { // PS包起始码
            return length - rest - 4;
        }
    }

    return -1;
}

int PsParser::removePSHead(const uint8_t *pBuf, size_t size, int &frametype) {
    if (pBuf == nullptr || size == 0) {
        return -1;
    }

    const uint8_t *pCurBuf = pBuf;
    frametype = -1;

    _currentPESFrame.clear();
    _currentPESFrame.reserve(MAX_FRAME_SIZE); // 预留空间以提高性能

    while (pCurBuf < (pBuf + size)) {
        if (pCurBuf + 4 > pBuf + size) {
            break;
        }

        uint32_t *pcode = (uint32_t *)pCurBuf;

        if (*pcode == 0xBA010000) {
            // PS Pack Header (0x000001BA)
            frametype = static_cast<int>(FrameType::P_frame);
            if (pCurBuf + 13 > pBuf + size) {
                break;
            }
            pCurBuf += 13;
            if (pCurBuf < pBuf + size) {
                uint32_t pack_stuffing_length = *pCurBuf & 7;
                pCurBuf++;
                if (pCurBuf + pack_stuffing_length <= pBuf + size) {
                    pCurBuf += pack_stuffing_length;
                } else {
                    break;
                }
            }
            continue;
        } else if (*pcode == 0xBB010000) {
            // System Header (0x000001BB)
            if (pCurBuf + 6 > pBuf + size) {
                break;
            }
            int systemHeaderLength = *(pCurBuf + 4) * 256 + *(pCurBuf + 5);
            if (pCurBuf + 4 + 2 + systemHeaderLength > pBuf + size) {
                break;
            }
            pCurBuf += systemHeaderLength + 4 + 2;
            frametype = static_cast<int>(FrameType::I_frame);
            continue;
        } else if (*pcode == 0xBC010000) {
            // Program Stream Map (0x000001BC)
            if (pCurBuf + 6 > pBuf + size) {
                break;
            }
            int psmLength = *(pCurBuf + 4) * 256 + *(pCurBuf + 5);
            if (pCurBuf + psmLength + 4 + 2 > pBuf + size) {
                break;
            }

            // 解析PSM中的PSI和ESM
            if (pCurBuf + 12 > pBuf + size) {
                break;
            }
            uint32_t u32PSILength = 0;
            u32PSILength = u32PSILength | pCurBuf[8];
            u32PSILength = (u32PSILength << 8) | pCurBuf[9];

            uint32_t u32ESMLength = 0;
            if (pCurBuf + 10 + u32PSILength + 2 > pBuf + size) {
                break;
            }
            u32ESMLength = u32ESMLength | pCurBuf[10 + u32PSILength];
            u32ESMLength = (u32ESMLength << 8) | pCurBuf[11 + u32PSILength];

            uint8_t *pESM = (uint8_t *)(pCurBuf + 12 + u32PSILength);
            uint32_t u32Tmp = 0;
            while (u32Tmp < u32ESMLength && pESM + u32Tmp + 4 < pBuf + size) {
                uint32_t u32StreamID = pESM[1 + u32Tmp];
                u32StreamID &= 0x000000FF;
                u32StreamID |= 0x00000100;

                _codecId = pESM[0 + u32Tmp];
                uint32_t u32ESILength = 0;
                if (pESM + u32Tmp + 4 > pBuf + size) {
                    break;
                }
                u32ESILength = u32ESILength | pESM[2 + u32Tmp];
                u32ESILength = (u32ESILength << 8) | pESM[3 + u32Tmp];
                if (u32ESILength == 0) {
                    break;
                }
                u32Tmp += (2 + 2 + u32ESILength);
            }

            pCurBuf += psmLength + 4 + 2;
            frametype = static_cast<int>(FrameType::I_frame);
            continue;
        } else if (*pcode == 0xE0010000) {
            // Video PES (0x000001E0)
            if (pCurBuf + 9 > pBuf + size) {
                break;
            }
            int pesPacketLength = *(pCurBuf + 4) * 256 + *(pCurBuf + 5);
            int pesHeaderDataLength = *(pCurBuf + 8);
            int payloadLength = pesPacketLength - 3 - pesHeaderDataLength;

            if (pCurBuf + 9 + pesHeaderDataLength + payloadLength > pBuf + size) {
                // PES包不完整
                break;
            }

            pCurBuf += 9 + pesHeaderDataLength;

            if (_currentPESFrame.size() + payloadLength > MAX_FRAME_SIZE) {
                BLOG(error) << "PES frame too large: " << (_currentPESFrame.size() + payloadLength);
                break;
            }

            // 使用vector的insert更安全
            _currentPESFrame.insert(_currentPESFrame.end(), pCurBuf, pCurBuf + payloadLength);
            pCurBuf += payloadLength;
            continue;
        } else if (*pcode == 0xC0010000) {
            // Audio PES (0x000001C0)
            if (pCurBuf + 9 > pBuf + size) {
                break;
            }
            int pesPacketLength = *(pCurBuf + 4) * 256 + *(pCurBuf + 5);
            int pesHeaderDataLength = *(pCurBuf + 8);
            int payloadLength = pesPacketLength - 3 - pesHeaderDataLength;

            if (pCurBuf + 9 + pesHeaderDataLength + payloadLength > pBuf + size) {
                // PES包不完整
                break;
            }

            pCurBuf += 9 + pesHeaderDataLength;
            pCurBuf += payloadLength;
            frametype = static_cast<int>(FrameType::Audio_frame);
            continue;
        } else {
            // 未知类型，跳过
            break;
        }
    }

    return _currentPESFrame.size();
}
