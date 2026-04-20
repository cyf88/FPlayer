#include "RTSPClientWrapper.h"



#include <thread>
#include <string>

namespace rtspwrap {

// C-style wrappers required by live555 responseHandler signature
static void sContinueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString) {
	auto* client = static_cast<rtspwrap::RTSPClientWrapper::MyRTSPClient*>(rtspClient);
	client->owner->continueAfterDESCRIBE(rtspClient, resultCode, resultString);
}

static void sContinueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString) {
	auto* client = static_cast<rtspwrap::RTSPClientWrapper::MyRTSPClient*>(rtspClient);
	client->owner->continueAfterSETUP(rtspClient, resultCode, resultString);
}

static void sContinueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString) {
	auto* client = static_cast<rtspwrap::RTSPClientWrapper::MyRTSPClient*>(rtspClient);
	client->owner->continueAfterPLAY(rtspClient, resultCode, resultString);
}

RTSPClientWrapper::RTSPClientWrapper()
    : scheduler_(nullptr), env_(nullptr), running_(false), watchVar_(0) {}

RTSPClientWrapper::~RTSPClientWrapper() {
    stop();
}

bool RTSPClientWrapper::start(const std::string& rtspUrl, FrameCallback cb) {
    rtspUrl_ = rtspUrl;
    callback_ = std::move(cb);
    running_ = true;
    thread_ = std::thread(&RTSPClientWrapper::run, this);
    return true;
}

void RTSPClientWrapper::stop() {
    if (!running_) return;
    running_ = false;
    watchVar_ = 1;
    if (thread_.joinable()) thread_.join();
}

void RTSPClientWrapper::run() {
    scheduler_ = BasicTaskScheduler::createNew();
    env_ = BasicUsageEnvironment::createNew(*scheduler_);
    openURL();
    watchVar_ = 0;
    env_->taskScheduler().doEventLoop(&watchVar_);
    env_->reclaim();
    delete scheduler_;
}

void RTSPClientWrapper::openURL() {
    MyRTSPClient* client = MyRTSPClient::createNew(*env_, this, rtspUrl_.c_str());
    client->sendDescribeCommand(sContinueAfterDESCRIBE);
}

void RTSPClientWrapper::continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString) {
    MyRTSPClient* client = static_cast<MyRTSPClient*>(rtspClient);
    StreamClientState& scs = client->scs;
    UsageEnvironment& env = client->envir();

    if (resultCode != 0) {
        env << "DESCRIBE failed: " << resultString << "\n";
        delete[] resultString;
        shutdownStream(rtspClient);
        return;
    }

    char* sdp = resultString;
    scs.session = MediaSession::createNew(env, sdp);
    delete[] sdp;

    scs.iter = new MediaSubsessionIterator(*scs.session);
    setupNextSubsession(rtspClient);
}

void RTSPClientWrapper::setupNextSubsession(RTSPClient* rtspClient) {
    MyRTSPClient* client = static_cast<MyRTSPClient*>(rtspClient);
    StreamClientState& scs = client->scs;
    UsageEnvironment& env = client->envir();

    scs.subsession = scs.iter->next();
    if (scs.subsession) {
        if (!scs.subsession->initiate()) {
            setupNextSubsession(rtspClient);
        } else {
            rtspClient->sendSetupCommand(*scs.subsession, sContinueAfterSETUP, False, False, False, NULL);
        }
        return;
    }
   
    rtspClient->sendPlayCommand(*scs.session, sContinueAfterPLAY);
}

void RTSPClientWrapper::continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString) {
    MyRTSPClient* client = static_cast<MyRTSPClient*>(rtspClient);
    StreamClientState& scs = client->scs;
    UsageEnvironment& env = client->envir();

    if (resultCode != 0) {
        setupNextSubsession(rtspClient);
        delete[] resultString;
        return;
    }

    scs.subsession->sink = FrameSink::createNew(env, *scs.subsession, callback_);
    scs.subsession->sink->startPlaying(*(scs.subsession->readSource()), nullptr, nullptr);

    setupNextSubsession(rtspClient);
    delete[] resultString;
}

void RTSPClientWrapper::continueAfterPLAY(RTSPClient* rtspClient, int, char* resultString) {
    delete[] resultString;
}

void RTSPClientWrapper::shutdownStream(RTSPClient* rtspClient) {
    MyRTSPClient* client = static_cast<MyRTSPClient*>(rtspClient);
    StreamClientState& scs = client->scs;
    if (scs.session) Medium::close(scs.session);
    Medium::close(rtspClient);
}

} // namespace rtspwrap
