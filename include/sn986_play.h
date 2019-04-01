#ifndef PLAY_H_
#define PLAY_H_

#include "play_media.h"
#include "V4l2DeviceSource.h"
#include "AlsaDeviceSource.h"

struct snx_avplay_info { 
	char filename[128];
#if DEBUG
	FILE *audiofile = NULL;
	FILE *videofile = NULL;
#endif
	p_play_source play_source;

	struct timeval timestamp;
	int width;
	int height;
	int frames_num;
	int fps;
	int started;
	pthread_t thread_id;

	void *cbarg;

	V4L2DeviceSource * video_devicesource;
	AlsaDeviceSource * audio_devicesource;
};

void * snx98_avplay_read(void *arg);
int snx986_avplay_start(struct snx_avplay_info *avinfo);
int snx986_avplay_stop(struct snx_avplay_info *avinfo);
int snx986_avplay_open(struct snx_avplay_info *avinfo);
struct snx_avplay_info * snx986_avplay_new(void);
int snx986_avplay_free(struct snx_avplay_info *avinfo);



#endif /* PLAY_H_ */
