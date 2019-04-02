# snx_rtsp_server

Customised version of the RTSP server for use with Xiaofang cameras. This adds
the following functionality:

* Username/password authentication on RTSP feeds
* Tested and working with 1920x1080 resolution

There are lots of this code floating around the Internet and in different
forms of executables and source code repositories but this is confirmed to
work, at least on the Xiaofang devices with `fang-hacks` installed and with an
ARM CPU.


## Building

If you just want to trust some random compiled code you've found online, then
step this way into my
[fang-hacks](https://github.com/davidjb/fang-hacks/tree/master/updates)
repository.  I don't make a habit of distributing spyware but if you're
downloading and running the code there, you're completely trusting me.

TODO


## Installation

TODO

## Original README

### About snx_rtsp_server

snx_rtsp_server is a small rtsp streaming server developed base on live555 library. snx_rtsp_server supports,

*  Support H264 / MJPEG Video stream
*  Support G.711 Alaw Audio stream
*  UDP over RTP/RTSP
*  TCP over RTP/RTSP
*  TCP over RTP/RTSP over HTTP
*  unicast and multicast
*  Single stream support (one video stream + one audio stream)
*  Frame buffer tunning for streaming server (default is 10 frames buffering (audio/video))
*  customized RTSP port and http port
*  Support AVI/MP4 file to rtsp stream playback

### Advantage

* low DDR and CPU loading

### Usage

```
   snx_rtsp_server [-a] [-j mjpeg_qp] [-m] [-P RTSP port][-T RTSP/HTTP port][-Q queueSize] [-M groupaddress]]
         -Q length: Number of frame queue  (default 10)
         RTSP options :
         -u url   : unicast url (default unicast)
         -m url   : multicast url (default multicast)
         -M addr  : multicast group   (default is a random address)
         -P port  : RTSP port (default 8554)
         -T port  : RTSP over HTTP port (default 0)
         V4L2 options :
         -F fps   : V4L2 capture framerate (default 30)
         -W width : V4L2 capture width (default 1280)
         -H height: V4L2 capture height (default 720)
         V4L2 H264 options :
         -b bitrate: V4L2 capture bitrate kbps(default 1024 kbps)
         -g gop   : V4L2 capture gop (default 30 )
         device   : V4L2 capture device (default /dev/video1)
         V4L2 MJPEG options :
         -j  mjpeg_qp     : MJPEG streaming and qp (default is 60)
         -a       : enable A-law pcm streaming
         H264 example   : /etc/snx_rtsp_server -a -Q 5 -u media/stream1 -P 554
         MJPEG example   : /etc/snx_rtsp_server -W 640 -H 480 -j 60 -Q 5 -u media/stream1 -P 554
```

### Reference

Modified from https://github.com/mpromonet/h264_v4l2_rtspserver
Version 99b4c42a07ea32d7dbbc950c473d9a48abde1541

MJPEG modified from http://stackoverflow.com/questions/12158716/jpeg-streaming-with-live555/20584296#20584296

