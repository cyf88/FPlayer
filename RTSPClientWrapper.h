
#pragma once

#include <memory>

#include <functional>
#include <string>
#include <liveMedia.hh>
#include <BasicUsageEnvironment.hh>
#include <cstdint>
#include <thread>

// 在第三方头文件之后再次包含，避免个别环境下 std::shared_ptr 不可见
#include <memory>

#pragma comment(lib, "UsageEnvironment.lib")
#pragma comment(lib, "BasicUsageEnvironment.lib")
#pragma comment(lib, "groupsock.lib")
#pragma comment(lib, "liveMedia.lib")
#pragma comment(lib, "ws2_32.lib")





// Forward declarations from LIVE555
class UsageEnvironment;

namespace rtspwrap {

class RTSPClientWrapper {
public:
	using FrameCallback = std::function<void(const uint8_t* data, unsigned size, timeval presentationTime)>;
	using Ptr = std::shared_ptr<RTSPClientWrapper>;

	RTSPClientWrapper();
	~RTSPClientWrapper();

	RTSPClientWrapper(const RTSPClientWrapper&) = delete;
	RTSPClientWrapper& operator=(const RTSPClientWrapper&) = delete;

	bool start(const std::string& rtspUrl, FrameCallback cb);
	void stop();

	// Exposed for live555 C-style callbacks
	void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString);
	void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString);
	void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString);

private:
	void run();
	void openURL();
	void setupNextSubsession(RTSPClient* rtspClient);
	void shutdownStream(RTSPClient* rtspClient);

private:
	struct StreamClientState {
		StreamClientState()
			: iter(nullptr), session(nullptr), subsession(nullptr),
			streamTimerTask(nullptr), duration(0.0) {}
		~StreamClientState() {
			delete iter;
			if (session) {
				UsageEnvironment& env = session->envir();
				env.taskScheduler().unscheduleDelayedTask(streamTimerTask);
				Medium::close(session);
			}
		}
		MediaSubsessionIterator* iter;
		MediaSession* session;
		MediaSubsession* subsession;
		TaskToken streamTimerTask;
		double duration;
	};

public:
	class MyRTSPClient : public RTSPClient {
	public:
		static MyRTSPClient* createNew(UsageEnvironment& env, RTSPClientWrapper* owner, const char* rtspURL) {
			return new MyRTSPClient(env, owner, rtspURL);
		}
		StreamClientState scs;
		RTSPClientWrapper* owner;
	private:
		MyRTSPClient(UsageEnvironment& env, RTSPClientWrapper* owner, const char* rtspURL)
			: RTSPClient(env, rtspURL, 1, "RTSPClientWrapper", 0, -1), owner(owner) {}
	};

private:
	class FrameSink : public MediaSink {
	public:
		static FrameSink* createNew(UsageEnvironment& env, MediaSubsession& subsession, FrameCallback cb) {
			return new FrameSink(env, subsession, std::move(cb));
		}

	private:
		FrameSink(UsageEnvironment& env, MediaSubsession& subsession, FrameCallback cb)
			: MediaSink(env), fSubsession(subsession), fReceiveBuffer(new u_int8_t[bufferSize]), callback_(std::move(cb)) {}
		virtual ~FrameSink() { delete[] fReceiveBuffer; }

		static void afterGettingFrame(void* clientData, unsigned frameSize, unsigned, struct timeval presentationTime, unsigned) {
			auto* sink = static_cast<FrameSink*>(clientData);
			sink->afterGettingFrame(frameSize, presentationTime);
		}

		void afterGettingFrame(unsigned frameSize, struct timeval presentationTime) {
			if (callback_) callback_(fReceiveBuffer, frameSize, presentationTime);
			continuePlaying();
		}

		Boolean continuePlaying() override {
			if (!fSource) return False;
			fSource->getNextFrame(fReceiveBuffer, bufferSize, afterGettingFrame, this, onSourceClosure, this);
			return True;
		}

	private:
		MediaSubsession& fSubsession;
		u_int8_t* fReceiveBuffer;
		FrameCallback callback_;
		static constexpr unsigned bufferSize = 3 * 1024 * 1024;
	};

private:
	std::string rtspUrl_;
	TaskScheduler* scheduler_;
	UsageEnvironment* env_;
	FrameCallback callback_;
	bool running_;
	volatile char watchVar_;
	std::thread thread_;
};

} // namespace rtspwrap


