/*
 * H264_Live_Video_Stream.hh
 *
 *  Created on: 2013-10-23
 *      Author: root
 */

#ifndef _FRAMED_FILTER_HH
#include "FramedSource.hh"
#include "H264VideoRTPSink.hh"
#include "H264VideoStreamFramer.hh"
#include "ByteStreamMemoryBufferSource.hh"
#include "ByteStreamFileSource.hh"
#include <pthread.h>
#endif


class H264LiveVideoSource: public FramedSource {
public:
  static H264LiveVideoSource* createNew(UsageEnvironment& env,
                         frame_I* frame,
                         Boolean deleteBufferOnClose = True,
                         unsigned preferredFrameSize = 0,
                         unsigned playTimePerFrame = 0 ,int fd_pipe[2]=NULL);
      // "preferredFrameSize" == 0 means 'no preference'
      // "playTimePerFrame" is in microseconds

  u_int64_t bufferSize() const { return fBuffer->size; }

  void seekToByteAbsolute(u_int64_t byteNumber, u_int64_t numBytesToStream = 0);
    // if "numBytesToStream" is >0, then we limit the stream to that number of bytes, before treating it as EOF
  void seekToByteRelative(int64_t offset);

protected:
  H264LiveVideoSource(UsageEnvironment& env,
                   frame_I* frame,
                   Boolean deleteBufferOnClose,
                   unsigned preferredFrameSize,
                   unsigned playTimePerFrame ,int pipe[2]);
    // called only by createNew()

  virtual ~H264LiveVideoSource();

private:
  // redefined virtual functions:
  virtual void doGetNextFrame();

private:
  stream_conf_s *s_stream;
  struct snx_rc *rc;
  Boolean fDeleteBufferOnClose;
  unsigned fPreferredFrameSize;
  unsigned fPlayTimePerFrame;
  unsigned fLastPlayTime;
  Boolean fLimitNumBytesToStream;
  u_int64_t fNumBytesToStream; // used iff "fLimitNumBytesToStream" is True
};

