//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////H264LiveVideoServerMediaSubsession//////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef H264_LIVE_VIDEO_STREAM_HH_
#include "H264VideoFileServerMediaSubsession.hh"
#define H264_LIVE_VIDEO_STREAM_HH_

class H264LiveVideoServerMediaSubsession: public OnDemandServerMediaSubsession{
public:
  static H264LiveVideoServerMediaSubsession*
                    createNew( UsageEnvironment& env, Boolean reuseFirstSource ,stream_conf_s *stream);
  void checkForAuxSDPLine1();
  void afterPlayingDummy1();
private:
              H264LiveVideoServerMediaSubsession(
                    UsageEnvironment& env, Boolean reuseFirstSource ,
                    stream_conf_s *stream);
  virtual ~H264LiveVideoServerMediaSubsession();

  void setDoneFlag() { fDoneFlag = ~0; }

private: // redefined virtual functions
  virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
                          unsigned& estBitrate);
  virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
                                    unsigned char rtpPayloadTypeIfDynamic,
                                    FramedSource* inputSource);
  virtual char const* getAuxSDPLine(RTPSink* rtpSink,
                    FramedSource* inputSource);

private:
  stream_conf_s *m_stream;
  char* fAuxSDPLine;
  char fDoneFlag; // used when setting up "fAuxSDPLine"
  RTPSink* fDummyRTPSink; // ditto

};


#endif /* H264_LIVE_VIDEO_STREAM_HH_ */