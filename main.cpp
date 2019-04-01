/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** main.cpp
** 
** V4L2 RTSP streamer                                                                 
**                                                                                    
** H264 capture using middleware_video                                                            
** RTSP using live555                                                                 
**                                                                                    
** -------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sstream>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>


// libv4l2
#include <linux/videodev2.h>

// live555
#include <BasicUsageEnvironment.hh>
#include <GroupsockHelper.hh>

// project
#include "sn98600_v4l2.h"
#include "sn986_play.h"
#include "H264_V4l2DeviceSource.h"
#include "ServerMediaSubsession.h"
#include "sn98600_record_audio.h"
#include "AlsaDeviceSource.h"


#define SNX_RTSP_SERVER_VERSION		("V00.01.06p")

#define AV_CAP_MODE					(0)
#define AV_PLAY_MODE				(1)
#define ALEK_ADD_MULTI    1

// create live555 environment
UsageEnvironment* env = NULL;

TaskScheduler* scheduler = NULL;

// create RTSP server
RTSPServer* rtspServer = NULL;

snx_v4l2_video* videoCapture = NULL;

struct snx_avplay_info *avinfo = NULL;


V4L2DeviceSource* videoES = NULL;

StreamReplicator*  replicator = NULL;

#if ALEK_ADD_MULTI 
int uvc_flag = 0; //default disable
UsageEnvironment* env1 = NULL;
UsageEnvironment* env_uvc = NULL;


TaskScheduler* scheduler1 = NULL;
TaskScheduler* scheduler_uvc = NULL;


RTSPServer* rtspServer1 = NULL;
RTSPServer* rtspServer_uvc = NULL;


snx_v4l2_video* videoCapture1 = NULL;
snx_v4l2_video* videoCapture_uvc = NULL;


V4L2DeviceSource* videoES1 = NULL;
V4L2DeviceSource* videoES_uvc = NULL;


StreamReplicator*  replicator1 = NULL;
StreamReplicator*  replicator_uvc = NULL;

#endif


#if AUDIO_STREAM
	sonix_audio *audioCapture = NULL;

	AlsaDeviceSource*  audioES = NULL;
#endif



// -----------------------------------------
//    add an RTSP session
// -----------------------------------------
void addSession( RTSPServer* rtspServer, const char* sessionName, ServerMediaSubsession *subSession, ServerMediaSubsession *audio_subSession)
{ 

	UsageEnvironment& env1(rtspServer->envir());

	ServerMediaSession* sms = ServerMediaSession::createNew(env1, sessionName);

	sms->addSubsession(subSession);
 
	if (audio_subSession)
		sms->addSubsession(audio_subSession);

	rtspServer->addServerMediaSession(sms);

	char* url = rtspServer->rtspURL(sms);

	fprintf(stderr, "lay this stream using the URL: \"%s\"\n", url );

	delete[] url;			
}

// -----------------------------------------
//    create video capture interface
// -----------------------------------------
struct snx_v4l2_video* createVideoCapure(const V4L2DeviceParameters & param)
{

	struct snx_v4l2_video *m_fd = snx98600_video_new();

	if (m_fd) {
		//m_fd->resolution_type = RESOLUTION_HD;
		m_fd->cb = NULL;

		m_fd->m2m->codec_fmt = param.m_format;
		m_fd->m2m->m2m = param.m_m2m_en;

		m_fd->m2m->width = param.m_width;
		m_fd->m2m->height = param.m_height;
		m_fd->m2m->scale = param.m_scale;

		m_fd->m2m->codec_fps = param.m_fps;
		m_fd->m2m->isp_fps = param.m_isp_fps;
		m_fd->m2m->gop = param.m_gop;

		if (m_fd->m2m->codec_fmt  == V4L2_PIX_FMT_MJPEG)
			m_fd->m2m->qp = param.m_mjpeg_qp;

		m_fd->m2m->bit_rate = param.m_bitrate << 10; //(Kbps)

		strcpy(m_fd->m2m->codec_dev, param.m_devName.c_str());

#if defined(CONFIG_SYSTEM_PLATFORM_ST58660FPGA) || defined(CONFIG_SYSTEM_PLATFORM_SN98660)
		if (m_fd->m2m->codec_fmt  == V4L2_PIX_FMT_H264) {

			if ( !strcmp(m_fd->m2m->codec_dev, "/dev/video1") )
				strcpy(m_fd->m2m->ds_dev_name, "1_h");
			else if ( !strcmp(m_fd->m2m->codec_dev, "/dev/video2") )
				strcpy(m_fd->m2m->ds_dev_name, "2_h");
			else {
				printf("[MROI Setting] wrong device name\n");
				m_fd = 0;
			}
		}
#endif
#if ALEK_ADD_MULTI
    if (uvc_flag == 1)
    {
      if ( !strcmp(m_fd->m2m->codec_dev, "/dev/video4") || !strcmp(m_fd->m2m->codec_dev, "/dev/video5") )
      {  
        m_fd->uvc = 1;
        printf ("now is uvc device\n");
      }  
		}
#endif    
    snx98600_video_open(m_fd, NULL );
	} 

	return m_fd;
}

struct snx_avplay_info* createVideoPlay(V4L2DeviceParameters & param)
{

	struct snx_avplay_info *m_fd = snx986_avplay_new();


	if (m_fd) {
		strcpy(m_fd->filename, param.m_devName.c_str());
		snx986_avplay_open(m_fd);
#if 1
		param.m_width = m_fd->width;
		param.m_height = m_fd->height;
		param.m_fps = m_fd->fps;
		param.m_isp_fps = m_fd->fps;
		param.m_gop = m_fd->fps;
#endif		
	} 

	return m_fd;
}

void closeVideoCapure(struct snx_v4l2_video* m_fd)
{
	int rc;
	if (m_fd) {
		if ((rc = snx98600_video_free(m_fd))) {
			fprintf(stderr, "failed to close video source: %s\n", strerror(rc));
		}
	}

}

void closeVideoPlay(struct snx_avplay_info* m_fd)
{
	int rc;
	if (m_fd) {
		if ((rc = snx986_avplay_free(m_fd))) {
			fprintf(stderr, "failed to close video source: %s\n", strerror(rc));
		}
	}

}

#if 1
struct sonix_audio* createAudioCapure(void)
{
	int rc;
	struct sonix_audio *m_fd = snx98600_record_audio_new(AUDIO_RECORD_DEV, NULL, NULL);

	if (!m_fd) {
		rc = errno ? errno : -1;
		fprintf(stderr, "failed to create audio source: %s\n", strerror(rc));
	}

	return m_fd;
}

void closeAudioCapure(struct sonix_audio* m_fd)
{
	int rc;
	if (m_fd) {
		if ((rc = snx98600_record_audio_stop(m_fd))) {
			fprintf(stderr, "failed to start audio source: %s\n", strerror(rc));
		}

		if (m_fd) {
			snx98600_record_audio_free(m_fd);
			m_fd = NULL;
		}
	}

}
#endif

// -----------------------------------------
//    signal handler
// -----------------------------------------
/*
 * NOTE: Please finish this program by kill -2
 */

char quit = 0;
void sighandler(int n)
{ 
	printf("Signal received (%d)\n", n);
	quit =1;
#if 0
#if AUDIO_STREAM
	if(audioES)
		Medium::close(audioES);
#endif

	if(videoES)
		Medium::close(videoES);

#if AUDIO_STREAM
	if (audioCapture) 
		closeAudioCapure(audioCapture);	
#endif

	if (videoCapture)
		closeVideoCapure(videoCapture);

	if (rtspServer)
	Medium::close(rtspServer);

	if (env)
		env->reclaim();
#if ALEK_ADD_MULTI
  	if(videoES1)
		Medium::close(videoES1);


	if (videoCapture1)
		closeVideoCapure(videoCapture1);

	if (rtspServer1)
	Medium::close(rtspServer1);

	if (env1)
		env1->reclaim();
#endif    
	delete scheduler;
#endif
}

// -----------------------------------------
//    create output
// -----------------------------------------
int createOutput(const std::string & outputFile, int inputFd)
{
	int outputFd = -1;
	if (!outputFile.empty())
	{
		struct stat sb;		
		if ( (stat(outputFile.c_str(), &sb)==0) && ((sb.st_mode & S_IFMT) == S_IFCHR) ) 
		{
			// open & initialize a V4L2 output
			outputFd = open(outputFile.c_str(), O_WRONLY);
			if (outputFd != -1)
			{
				struct v4l2_capability cap;
				memset(&(cap), 0, sizeof(cap));
				if (0 == ioctl(outputFd, VIDIOC_QUERYCAP, &cap)) 
				{			

					fprintf(stderr, "Output device name: %s, cap: %x0x\n", cap.driver,cap.capabilities );
					if (cap.capabilities & V4L2_CAP_VIDEO_OUTPUT) 
					{				
						struct v4l2_format   fmt;			
						memset(&(fmt), 0, sizeof(fmt));
						fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
						if (ioctl(inputFd, VIDIOC_G_FMT, &fmt) == -1)
						{

							fprintf(stderr, "Cannot get input format  (%s)\n", strerror(errno));
						}		
						else 
						{
							fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
							if (ioctl(outputFd, VIDIOC_S_FMT, &fmt) == -1)
							{

								fprintf(stderr, "Cannot set output format  (%s)\n", strerror(errno));
							}		
						}
					}			
				}
			}
			else
			{
				//LOG(ERROR) << "Cannot open " << outputFile << " " << strerror(errno);
				fprintf(stderr, "Cannot open  %s (%s)\n", outputFile.c_str(), strerror(errno));
			}
		}
		else
		{		
			outputFd = open(outputFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
		}
		
		if (outputFd == -1)		
		{
			//LOG(NOTICE) << "Error openning " << outputFile << " " << strerror(errno);
			fprintf(stderr, "Error openning  %s (%s)\n", outputFile.c_str(), strerror(errno));
		}
		
	}
	return outputFd;
}	


#if ALEK_ADD_MULTI
pthread_t stream_thread;
pthread_t stream1_thread;
pthread_t stream_uvc_thread;
void* snx_stream_flow(void *arg)
{  
  env->taskScheduler().doEventLoop(&quit); 
}
void* snx_stream1_flow(void *arg)
{  
  env1->taskScheduler().doEventLoop(&quit); 
}
void* snx_stream_uvc_flow(void *arg)
{  
  env_uvc->taskScheduler().doEventLoop(&quit); 
}
#endif
	
// -----------------------------------------
//    entry point
// -----------------------------------------

int main(int argc, char** argv) 
{

#if AUDIO_STREAM

	StreamReplicator* audio_replicator = NULL;
#endif
	// default parameters
	const char *dev_name = "/dev/video1";	
	int format = V4L2_PIX_FMT_H264;
	int width = 1280;
	int height = 720;
	int queueSize = 5;
	int fps = 30;
	int isp_fps = 30;
	int bitrate = 1024 ; //(Kbps)
	int mjpeg_qp = 120;
	int m2m_en = 1;
	int scale = 1;
	int gop = fps;
	unsigned short rtpPortNum = 20000;
	unsigned short rtcpPortNum = rtpPortNum+1;
	unsigned char ttl = 5;
	struct in_addr destinationAddress;
	unsigned short rtspPort = 554;
	unsigned short rtspOverHTTPPort = 0;
	bool multicast = false;
	int verbose = 0;
	std::string outputFile;
	//bool useMmap = true;
	std::string url = "media/stream1";
#if ALEK_ADD_MULTI
  const char *dev_name1 = "/dev/video2";
  std::string url1 = "media/stream2";
  
  int format1 = V4L2_PIX_FMT_H264, width1 = 640, height1= 480, fps1 = 30,bitrate1 = 1024, scale1=1, gop1 = fps1, m2m_en1 = 1;
  
  const char *dev_name_uvc = "/dev/video5"; //default uvc is h264
  std::string url_uvc = "media/stream_uvc";
  
  int format_uvc = V4L2_PIX_FMT_H264, width_uvc = 1280, height_uvc= 720, fps_uvc = 30,bitrate_uvc = 1024, scale_uvc=1, gop_uvc = fps1;  
#endif  
	std::string murl = "multicast";
	bool useThread = true;
	in_addr_t maddr = INADDR_NONE;
	bool audio_en = false;
	int mode = AV_CAP_MODE;

	// decode parameters
	int c = 0;     
	//while ((c = getopt (argc, argv, "hW:H:Q:P:F:v::O:T:m:u:rsM:")) != -1)
#if AUDIO_STREAM
	while ((c = getopt (argc, argv, "hb:B:wW:rH:gG:p:P:f:i:O:T:m:u:U:M:aj:J:cs:S:Q:F:l:k:e:z:y:p:v:")) != -1)
#else
	while ((c = getopt (argc, argv, "hb:B:wW:rH:gG:p:P:f:i:O:T:m:u:U:M:j:J:cs:S:Q:F:l:k:e:z:y:p:v:")) != -1)
#endif
	{
		switch (c)
		{
			case 'O':	outputFile = optarg; break;
			//case 'v':	verbose = 1; if (optarg && *optarg=='v') verbose++;  break;
			case 'm':	multicast = true; if (optarg) murl = optarg; break;
			case 'M':	multicast = true; if (optarg) maddr = inet_addr(optarg); break;
			case 'g':	gop = atoi(optarg); break; 
     
			case 'b':	bitrate = atoi(optarg); break;
      
			case 'w':	width = atoi(optarg); break;
      
			case 'r':	height = atoi(optarg); break;
			
			case 'Q':	queueSize = atoi(optarg); break;
			case 'P':	rtspPort = atoi(optarg); break;
			case 'T':	rtspOverHTTPPort = atoi(optarg); break;
			case 'f':	fps = atoi(optarg); break;
			case 'i':	isp_fps = atoi(optarg); break;
			//case 'r':	useMmap =  false; break;
			//case 's':	useThread =  false; break;
			case 'u':	url = optarg; break;
      
#if AUDIO_STREAM
			case 'a':	audio_en = true; break;
#endif
			case 'c':	m2m_en = 0; break;
			case 's':	

						scale = atoi(optarg); 
						if ((scale < 1) || (scale > 4) || (scale == 3) ) {
							printf("Wrong scale param(%d)\n", scale);
							scale = 1;
						}

						break;
           
			case 'j':	format = V4L2_PIX_FMT_MJPEG; mjpeg_qp = atoi(optarg);break;
#if ALEK_ADD_MULTI      
      case 'U':	url1 = optarg; break;
      case 'H':	height1 = atoi(optarg); break;
      case 'W':	width1 = atoi(optarg); break;
      case 'B':	bitrate1 = atoi(optarg); break; 
      case 'F':	fps1 = atoi(optarg); break;
      case 'G':	gop1 = atoi(optarg); break;
      case 'S':	

						scale1 = atoi(optarg); 
						if ((scale1 < 1) || (scale1 > 4) || (scale1 == 3) ) {
							printf("Wrong scale1 param(%d)\n", scale1);
							scale1 = 1;
						}

						break; 
      case 'J':	format1 = V4L2_PIX_FMT_MJPEG; mjpeg_qp = atoi(optarg);break;
      
      case 'l':	uvc_flag = atoi(optarg); break;
      case 'k':	url_uvc = optarg; break;
      case 'e':	height_uvc = atoi(optarg); break;
      case 'z':	width_uvc = atoi(optarg); break;
      case 'y':	bitrate_uvc = atoi(optarg); break; 
      case 'v':	fps_uvc = atoi(optarg); break;
      case 'p':	
               {
                format_uvc = V4L2_PIX_FMT_MJPEG; 
                mjpeg_qp = atoi(optarg); 
                dev_name_uvc = "/dev/video4"; 
                printf ("mjpeg_qp = %d\n",mjpeg_qp);
                break;
               } 	
      
#endif      
			case 'h':
			default:
			{
				std::cout << argv[0] << "Version:" << SNX_RTSP_SERVER_VERSION										<< std::endl;
				std::cout << "Usage :"                                                              				<< std::endl;
				std::cout << "\t " << argv[0] << " [-a] [-j mjpeg_qp] [-m] [-P RTSP port][-T RTSP/HTTP port][-Q queueSize] [-M groupaddress] [-b bitrate] [-W width] [-H height] [-F fps] [-i isp_fps] [device]" << std::endl;

				std::cout << "\t -Q length: Number of frame queue  (default "<< queueSize << ")"                   << std::endl;
				std::cout << "\t RTSP options :"                                                                   << std::endl;
				std::cout << "\t -u url     : unicast url (default " << url << ")"                                   << std::endl;
        
				std::cout << "\t -m url     : multicast url (default " << murl << ")"                                << std::endl;
				std::cout << "\t -M addr    : multicast group   (default is a random address)"                                << std::endl;
				std::cout << "\t -P port    : RTSP port (default "<< rtspPort << ")"                                 << std::endl;
				std::cout << "\t -T port    : RTSP over HTTP port (default "<< rtspOverHTTPPort << ")"               << std::endl;
				std::cout << "\t V4L2 options :"                                                                   << std::endl;
				//std::cout << "\t -r       : V4L2 capture using read interface (default use memory mapped buffers)" << std::endl;
				//std::cout << "\t -s       : V4L2 capture using live555 mainloop (default use a separated reading thread)" << std::endl;
				std::cout << "\t -f fps     : V4L2 capture framerate (default "<< fps << ")"                         << std::endl;
        
				std::cout << "\t -i isp_fps : ISP capture framerate (default "<< isp_fps << ")"                         << std::endl;
				std::cout << "\t -w width   : V4L2 capture width (default "<< width << ")"                           << std::endl;
        
        
        std::cout << "\t -r height  : V4L2 capture height (default "<< height << ")"                         << std::endl;
				
				
				std::cout << "\t V4L2 H264 options :"                                                              << std::endl;

				std::cout << "\t -b bitrate : V4L2 capture bitrate kbps(default "<< bitrate << " kbps)"				<< std::endl;
        
				std::cout << "\t -g gop     : V4L2 capture gop (default "<< gop << " )"									<< std::endl;
        

				std::cout << "\t V4L2 MJPEG options :"                                                              << std::endl;
				std::cout << "\t -j mjpeg_qp :  set MJPEG streaming and qp (default is 60)"							<< std::endl;
				std::cout << "\t -c capture path enable (default is disable)"									<< std::endl;
				std::cout << "\t -s encoder scale (1/2/4) (default is 1)"												<< std::endl;
        
#if AUDIO_STREAM
        
				std::cout << "\t -a         : enable A-law pcm streaming "											 << std::endl;
				std::cout << "\t H264 example : "<< argv[0] << " -a -Q 5 -u media/stream1 -P 554"                       << std::endl;
#else
				std::cout << "\t H264 example : "<< argv[0] << " -Q 5 -u media/stream1 -P 554"                       << std::endl;
#endif
				std::cout << "\t MJPEG example : "<< argv[0] << " -W 640 -H 480 -j 120 -Q 5 -u media/stream1 -P 554"		<< std::endl;
#if ALEK_ADD_MULTI  
        std::cout << "\t -u url1     : stream2 url (default " << url1 << ")"                                   << std::endl;
        std::cout << "\t -F fps1     : stream2 V4L2 capture framerate (default "<< fps1 << ")"                         << std::endl;
        std::cout << "\t -W width1   : stream2 V4L2 capture width1 (default "<< width1 << ")"                           << std::endl;
        std::cout << "\t -H height1  : stream2 V4L2 capture height1 (default "<< height1 << ")"                         << std::endl;
        std::cout << "\t -B bitrate1 : stream2 V4L2 capture bitrate kbps(default "<< bitrate1 << " kbps)"				<< std::endl;
        std::cout << "\t -g gop1     : stream2 V4L2 capture gop1 (default "<< gop1 << " )"									<< std::endl;  
        std::cout << "\t -S stream2 encoder scale (1/2/4) (default is 1)"												<< std::endl;
        std::cout << "\t -J mjpeg_qp : set MJPEG streaming2 and qp (default is 60)"							<< std::endl;

        std::cout << "\t -l uvc_enable     : uvc enable (default " << uvc_flag << ")"                            << std::endl;
        std::cout << "\t -k url_uvc     :uvc stream url (default " << url_uvc << ")"                                   << std::endl;
        
        std::cout << "\t -z width_uvc   :uvc stream V4L2 capture width_uvc (default "<< width_uvc << ")"                           << std::endl;
        std::cout << "\t -e height_uvc  :uvc stream V4L2 capture height_uvc (default "<< height_uvc << ")"                         << std::endl;
        std::cout << "\t -v framerate_uvc  :uvc stream V4L2 capture framerate_uvc (default "<< fps_uvc << ")"                         << std::endl;
        std::cout << "\t -y bitrate_uvc :uvc stream V4L2 capture bitrate kbps(default "<< bitrate_uvc << " kbps)"				<< std::endl;
        std::cout << "\t -p mjpeg_qp : set uvc MJPEG streaming and qp (default is 60)"							<< std::endl;
#endif				
        exit(0);
			}
		}
	}
 
	if (optind<argc)
	{
		dev_name = argv[optind];
	}
  
	// create live555 environment
	scheduler = BasicTaskScheduler::createNew();
	env = BasicUsageEnvironment::createNew(*scheduler);	
	
	// create RTSP server
	rtspServer = RTSPServer::createNew(*env, rtspPort);
#if ALEK_ADD_MULTI

  scheduler1 = BasicTaskScheduler::createNew();
	env1 = BasicUsageEnvironment::createNew(*scheduler1);	
	
	// create RTSP server
	rtspServer1 = RTSPServer::createNew(*env1, rtspPort+1);
  if (uvc_flag == 1)
  {
    scheduler_uvc = BasicTaskScheduler::createNew();
    env_uvc = BasicUsageEnvironment::createNew(*scheduler_uvc);	
	
	// create RTSP server
    rtspServer_uvc = RTSPServer::createNew(*env_uvc, rtspPort+2);

  }
#endif  
	if (rtspServer == NULL) 
	{
		//LOG(ERROR) << "Failed to create RTSP server: " << env->getResultMsg();
		fprintf(stderr, "Failed to create RTSP server: %s \n", env->getResultMsg());
	}
	else
	{
		// set http tunneling
		if (rtspOverHTTPPort)
		{
			rtspServer->setUpTunnelingOverHTTP(rtspOverHTTPPort);
		}
		
		// Init capture
		//LOG(NOTICE) << "Create V4L2 Source..." << dev_name;
		fprintf(stderr, "Video source = %s \n", dev_name);
		V4L2DeviceParameters param(dev_name,format,width,height,fps, isp_fps, verbose, bitrate, m2m_en, scale, gop, mjpeg_qp, queueSize );
#if ALEK_ADD_MULTI
		V4L2DeviceParameters param1(dev_name1,format1,width1,height1,fps1, isp_fps, verbose, bitrate1, m2m_en1, scale1, gop1, mjpeg_qp, queueSize );
    //if(uvc_flag == 1)
      V4L2DeviceParameters param_uvc(dev_name_uvc,format_uvc,width_uvc,height_uvc,fps_uvc, fps_uvc/*uvc fps*/, verbose, bitrate_uvc, 1, 1, gop_uvc, mjpeg_qp, queueSize );
#endif		
    if (strcmp(dev_name, "/dev/video1") && strcmp(dev_name, "/dev/video2") ) {
			mode = AV_PLAY_MODE;
			fprintf(stderr, "Video Playback mode = %s \n", dev_name);
			avinfo = createVideoPlay(param);
		} else {
			mode = AV_CAP_MODE;
			videoCapture = createVideoCapure(param);
#if ALEK_ADD_MULTI
      videoCapture1 = createVideoCapure(param1);
      if(uvc_flag == 1)
        videoCapture_uvc = createVideoCapure(param_uvc);
#endif      
		}

		

#if AUDIO_STREAM
		if (audio_en) {
			if(mode == AV_CAP_MODE) {
				audioCapture = createAudioCapure();
			}
		}
#endif
		if (videoCapture || avinfo)
		{
			int outputFd = -1;
#if ALEK_ADD_MULTI
      int outputFd1 = -1;
      int outputFd_uvc = -1;
#endif      
			if(mode == AV_CAP_MODE) {
			
				//int outputFd = createOutput(outputFile, videoCapture->getFd());			
				//LOG(NOTICE) << "Start V4L2 Capture..." << dev_name;
				fprintf(stderr, "Start V4L2 Capture... %s \n",  dev_name);
				//videoCapture->captureStart();

				snx98600_video_start(videoCapture);
#if ALEK_ADD_MULTI
        snx98600_video_start(videoCapture1);
        if(uvc_flag == 1)
          snx98600_video_start(videoCapture_uvc);
#endif              
				printf("\n\n------- V4L2 Infomation -------- \n");
				printf("m2m_en: %d\n", videoCapture->m2m->m2m);
				printf("codec_dev: %s\n", videoCapture->m2m->codec_dev);
				printf("codec_fps: %d\n", videoCapture->m2m->codec_fps);
				if(videoCapture->m2m->m2m)
					printf("isp_fps: %d\n", videoCapture->m2m->isp_fps);
				printf("width: %d\n", videoCapture->m2m->width);
				printf("height: %d\n", videoCapture->m2m->height);
				printf("scale: %d\n", videoCapture->m2m->scale);
				printf("bit_rate: %d\n", videoCapture->m2m->bit_rate);
//				printf("dyn_fps_en: %d\n", videoCapture->m2m->dyn_fps_en);
//				if(videoCapture->m2m->dyn_fps_en) {
//					printf("framerate: %d\n", videoCapture->rate_ctl->framerate);
//				}
				printf("GOP: %d\n", videoCapture->rate_ctl->gop);
				printf("ds_font_num: %d\n", videoCapture->m2m->ds_font_num);
				printf("\n----------------------------- \n\n");

			
#if AUDIO_STREAM
				/* 
					Start Audio Device 

				*/
				if (audio_en) {
					int rc;
					if (audioCapture) {
						if ((rc = snx98600_record_audio_start(audioCapture))) {
							fprintf(stderr, "failed to start audio source: %s\n", strerror(rc));
						}
					}
				}
#endif
			}  //if AV_CAP_MODE

			if(mode == AV_PLAY_MODE) {
				snx986_avplay_start(avinfo);
				printf("\n\n------- AVPLAY Infomation -------- \n");
				printf("width: %d\n", avinfo->width);
				printf("height: %d\n", avinfo->height);
				printf("fps: %d\n", avinfo->fps);
				printf("frame number: %d\n", avinfo->frames_num);
				printf("\n----------------------------- \n\n");
			} //AV_PLAY_MODE

			/* Determind which Class to use */
			if (format == V4L2_PIX_FMT_H264)  
				videoES =  H264_V4L2DeviceSource::createNew(*env, param, outputFd, useThread);
      else        
				videoES = V4L2DeviceSource::createNew(*env, param, outputFd, useThread);  
#if ALEK_ADD_MULTI
      if (format1 == V4L2_PIX_FMT_H264)
        videoES1 =  H264_V4L2DeviceSource::createNew(*env1, param1, outputFd1, useThread);
      else
        videoES1 =  V4L2DeviceSource::createNew(*env1, param1, outputFd1, useThread); 
         
      if (format_uvc == V4L2_PIX_FMT_H264)
      { 
        if(uvc_flag == 1)
          videoES_uvc =  H264_V4L2DeviceSource::createNew(*env_uvc, param_uvc, outputFd_uvc, useThread);
      }
      else
      {
        if(uvc_flag == 1)
          videoES_uvc = V4L2DeviceSource::createNew(*env_uvc, param_uvc, outputFd_uvc, useThread);
      }    
#endif                
			/*  check if create a Device source success */
			if (videoES == NULL)
			{
				//LOG(FATAL) << "Unable to create source for device " << dev_name;
				fprintf(stderr, "Unable to create source for device  %s \n",  dev_name);
			}
			else
			{

				if(mode == AV_CAP_MODE) {
					videoCapture->devicesource = videoES;
#if ALEK_ADD_MULTI
          videoCapture1->devicesource = videoES1;
          if(uvc_flag == 1)
            videoCapture_uvc->devicesource = videoES_uvc;
#endif				
					// Setup the outpacket size;
					if (m2m_en) {
						//OutPacketBuffer::maxSize = (unsigned int)videoCapture->m2m->isp_buffers->length;
						OutPacketBuffer::maxSize = bitrate << 8;    //2X Bitrate as the max packet size
						fprintf(stderr, "isp buffers: %u , outpack maxsize : %u\n", (unsigned int)videoCapture->m2m->isp_buffers->length, OutPacketBuffer::maxSize  );
            
					}else {

						OutPacketBuffer::maxSize = width * height * 3 / 2;
					}
				}
				if(mode == AV_PLAY_MODE) {
					printf("avplay devicesource\n");
					avinfo->video_devicesource = videoES;
					OutPacketBuffer::maxSize = width * height * 3 / 2;
				}

#if AUDIO_STREAM
				/* 
					create Alsa Device source Class 
				*/
				if (audio_en) {
					audioES =  AlsaDeviceSource::createNew(*env, -1, queueSize, useThread);

					if (audioES == NULL) 
					{
						fprintf(stderr, "Unable to create audio devicesource \n");
					}
					else
					{
						if((mode == AV_CAP_MODE )&&(audioCapture)) {
							audioCapture->devicesource = audioES;
						}
						if(mode == AV_PLAY_MODE) {
						
							avinfo->audio_devicesource = audioES;
						}
					}
				}
#endif

				replicator = StreamReplicator::createNew(*env, videoES, false);
#if ALEK_ADD_MULTI
        replicator1 = StreamReplicator::createNew(*env1, videoES1, false);
        if(uvc_flag == 1)
          replicator_uvc = StreamReplicator::createNew(*env_uvc, videoES_uvc, false);
#endif

#if AUDIO_STREAM
				if (audio_en && audioES)
					audio_replicator = StreamReplicator::createNew(*env, audioES, false);
#endif
				// Create Server Multicast Session
				if (multicast)
				{
					ServerMediaSubsession * multicast_video_subSession = NULL;
					ServerMediaSubsession * multicast_audio_subSession = NULL;
					if (maddr == INADDR_NONE) maddr = chooseRandomIPv4SSMAddress(*env);	
					destinationAddress.s_addr = maddr;
					//LOG(NOTICE) << "Mutlicast address " << inet_ntoa(destinationAddress);
					fprintf(stderr, "Mutlicast address  %s \n",  inet_ntoa(destinationAddress));


					multicast_video_subSession = MulticastServerMediaSubsession::createNew(*env,destinationAddress, Port(rtpPortNum), Port(rtcpPortNum), ttl, replicator,format,param);
#if AUDIO_STREAM
					if (audio_en && audioES) 
						multicast_audio_subSession =  MulticastServerMediaSubsession::createNew(*env,destinationAddress, Port(rtpPortNum), Port(rtcpPortNum), ttl, audio_replicator,WA_PCMA,param);
#endif
					addSession(rtspServer, murl.c_str(), multicast_video_subSession, multicast_audio_subSession);
				
				}

				ServerMediaSubsession * video_subSession = NULL;
				ServerMediaSubsession * audio_subSession = NULL;

				video_subSession = UnicastServerMediaSubsession::createNew(*env,replicator,format, param);
#if ALEK_ADD_MULTI
        ServerMediaSubsession * video_subSession1 = NULL;
        video_subSession1 = UnicastServerMediaSubsession::createNew(*env1,replicator1,format1, param1);
        ServerMediaSubsession * video_subSession_uvc = NULL;
        if(uvc_flag == 1)
          video_subSession_uvc = UnicastServerMediaSubsession::createNew(*env_uvc,replicator_uvc,format_uvc, param_uvc);

#endif

#if AUDIO_STREAM
				if (audio_en && audioES) 
					audio_subSession = UnicastServerMediaSubsession::createNew(*env,audio_replicator,WA_PCMA, param);
#endif
				// Create Server Unicast Session
				addSession( rtspServer, url.c_str(), video_subSession, audio_subSession);
#if ALEK_ADD_MULTI
        addSession( rtspServer1, url1.c_str(), video_subSession1, NULL);
        if(uvc_flag == 1)
          addSession( rtspServer_uvc, url_uvc.c_str(), video_subSession_uvc, NULL);
#endif
				// main loop
				signal(SIGINT,sighandler);
#if !ALEK_ADD_MULTI        
				env->taskScheduler().doEventLoop(&quit); 
#endif        
#if ALEK_ADD_MULTI  
       
        int ret = -1;  
        ret = pthread_create(&stream_thread, NULL, snx_stream_flow, NULL);
        ret = pthread_create(&stream1_thread, NULL, snx_stream1_flow, NULL);
        if(uvc_flag == 1)
          ret = pthread_create(&stream_uvc_thread, NULL, snx_stream_uvc_flow, NULL);  
	      if(ret != 0) {
		      printf("exit thread creation failed\n");   
	      }
	      pthread_detach(stream_thread);
        pthread_detach(stream1_thread);
        if(uvc_flag == 1)
          pthread_detach(stream_uvc_thread);         
        while(1)
        {
          char c = getchar();
          if (c=='q')
            break;
          sleep(1);    
        }
#endif	
				fprintf(stderr, "Exiting....  \n");		

#if AUDIO_STREAM
				if (audioES && audioES) 
				{
					Medium::close(audioES);
				}
#endif
					Medium::close(videoES);
         
			}
#if AUDIO_STREAM
			if (audio_en && audioCapture) 
				closeAudioCapure(audioCapture);
#endif
			if (videoCapture)
			closeVideoCapure(videoCapture);
			
			if (avinfo)
			closeVideoPlay(avinfo);
			
			//delete videoCapture;
			if (outputFd != -1)
			{
				close(outputFd);
			}    
		
		  Medium::close(rtspServer);
      usleep(50000);
#if ALEK_ADD_MULTI

      Medium::close(videoES1);
      
      if (videoCapture1)
			closeVideoCapure(videoCapture1);
      if (outputFd1 != -1)
			{
				close(outputFd1);
			}
      Medium::close(rtspServer1);
      usleep(50000);  
    if(uvc_flag == 1)
    {
      Medium::close(videoES_uvc);
      if (videoCapture_uvc)
			closeVideoCapure(videoCapture_uvc);
      if (outputFd_uvc != -1)
			{
				close(outputFd_uvc);
			}
      Medium::close(rtspServer_uvc);  
    }
    
#endif
    }    
	}
	
	env->reclaim();
  
#if ALEK_ADD_MULTI
  env1->reclaim();
  
  if(uvc_flag == 1)
    env_uvc->reclaim();
  
#endif  
	delete scheduler;	
  
#if ALEK_ADD_MULTI  
	delete scheduler1;
  
  if(uvc_flag == 1)
    delete scheduler_uvc;  
    
#endif  
	return 0;
}



