#include "sn986_play.h"

#if 1
#include "data_buf.h"
av_buffer *video_pre_buffer = NULL;
av_buffer *audio_pre_buffer = NULL;
#endif

void * snx98_avplay_read(void *arg)
{
	struct snx_avplay_info *avinfo = (struct snx_avplay_info *)arg;
	int ret;
	int keyFrame = 0;
	int main_size = 0;
	int frame_count=0;
	int play_interval_time = 0;
	
	unsigned long long pre_time=0, cur_time=0;
	int video_duration = 0;
	int audio_duration = 0;
	int arrival_time = 0;
	int video_fps_interval;
	int audio_interval;
	int video_first_flag = 1;
	int audio_first_flag = 1;

	avinfo->started = 1;

	// Calculate Video/audio time interval
	video_fps_interval = (1000000 / avinfo->fps);
	audio_interval = (100000); //use 100ms as default
	

	play_interval_time = video_fps_interval >> 3;

	printf("AVplay thread %dx%d (%d)\n", avinfo->width, avinfo->height, play_interval_time);
	
	while(1) {
		void *data = NULL;
		int size;
		int ret = read_playing_data(avinfo->play_source, &data, &size);
		gettimeofday(&avinfo->timestamp,NULL);
		//printf("[%d.%d] type:%d, size: %d\n",avinfo->timestamp.tv_sec, avinfo->timestamp.tv_usec/1000, ret, size);
		if(ret == TYPE_NONE){
			//fprintf(stderr,"ret is %d\n",ret);
			//break;
		}
		else if(ret == TYPE_VIDEO){
#if DEBUG
			fwrite(data,size,1,avinfo->videofile);
#endif

			//printf("Video %d\n", size);
			write_buffer_data(video_pre_buffer,(char *)data, size );
		}else if(ret == TYPE_AUDIO){
#if DEBUG
			fwrite(data,size,1,avinfo->audiofile);
#endif

			//printf("Audio %d\n", size);
			audio_interval = (1000000/SAMPLE_RATE) * size; //calculate audio time interval
			write_buffer_data(audio_pre_buffer,(char *)data, size );
		}
		reset_read_playing_data(data);
		

#if 1 
		{
			if(pre_time == 0) {
				pre_time = avinfo->timestamp.tv_sec * 1000000 + avinfo->timestamp.tv_usec;
			}
			else { //calculate how much frames we lost during SD encode to reach FPS
				cur_time = avinfo->timestamp.tv_sec * 1000000 + avinfo->timestamp.tv_usec;
				arrival_time = cur_time - pre_time;

				video_duration += arrival_time;
				if ((video_duration > video_fps_interval) || (video_first_flag)) {
					char *tmp_data;
					int tmp_size = 0;
					if (video_duration > video_fps_interval)
						video_duration -= video_fps_interval;
					
					read_buffer_data(video_pre_buffer, &tmp_data,&tmp_size);
					
					if ((tmp_size) && (avinfo->video_devicesource)) {
						//printf("video dispatching :%d bytes\n", tmp_size);
						avinfo->video_devicesource->videocallback(&(avinfo->timestamp), tmp_data, tmp_size, keyFrame);
						video_first_flag = 0;
						main_size += size;
						frame_count++;
					}
				}
				
				audio_duration += arrival_time;
				
				if ((audio_duration > audio_interval) || (audio_first_flag)) {
					char *tmp_data;
					int tmp_size = 0;
					
					if(audio_duration > audio_interval)
						audio_duration -= audio_interval;
					
					read_buffer_data(audio_pre_buffer, &tmp_data,&tmp_size);

					if ((tmp_size) && (avinfo->audio_devicesource)) {
						//printf("audio dispatching :%d bytes\n", tmp_size);
						avinfo->audio_devicesource->audiocallback(&(avinfo->timestamp), tmp_data, tmp_size, NULL);
						audio_first_flag = 0;
					}
				}

				pre_time = cur_time;
			}
		}
#endif

		usleep(play_interval_time);

		if ((ret == TYPE_NONE) && (frame_count >= avinfo->frames_num)) {
			fprintf(stderr,"file finished, back to the head (%d/%d)\n",frame_count, avinfo->frames_num);
			set_playing_time(avinfo->play_source,0);
			frame_count = 0;
		}

		if(avinfo->started == 0) {
			break;
		}

	} // while(1)

	ret = 0;
finally:
	destory_playing(avinfo->play_source);
#if DEBUG
	fclose(avinfo->videofile);
	fclose(avinfo->audiofile);
#endif
	pthread_exit((void *)ret);
	
}


int snx986_avplay_start(struct snx_avplay_info *avinfo)
{
	int ret =0;
#if 1
	if (video_pre_buffer)
		read_write_pos_sync(video_pre_buffer);
	else {
		ret = -1;
		goto finally;
	}
	if (audio_pre_buffer)
		read_write_pos_sync(audio_pre_buffer);
	else {
		ret = -1;
		goto finally;
	}
#endif
	ret = pthread_create(&(avinfo->thread_id), NULL, snx98_avplay_read, (void *)avinfo);
	if(ret != 0) {
		printf("Can't create thread: %s\n", strerror(ret));
		goto finally;
	}

finally:
	return ret;
}

int snx986_avplay_stop(struct snx_avplay_info *avinfo)
{
	int ret=0;
	void *thread_result;


	if (!avinfo) {
		ret = EINVAL;
		printf("null argument");
		goto finally;
	}


	if (!avinfo->started) {
		ret = -1;
		printf("not started");
		goto finally;
	}

	printf("stopping: %s \n", avinfo->filename);
	avinfo->started = 0;
	ret = pthread_join(avinfo->thread_id, &thread_result);
	if(ret != 0)
		printf("thread join failed ret==%d\n", ret);

	ret = (int)thread_result;

	ret = 0;

#if 1
	if (video_pre_buffer)
		read_write_pos_sync(video_pre_buffer);
	else {
		ret = -1;
		goto finally;
	}
	if (audio_pre_buffer)
		read_write_pos_sync(audio_pre_buffer);
	else {
		ret = -1;
		goto finally;
	}
#endif

finally:
	return ret;

}

int snx986_avplay_open (struct snx_avplay_info *avinfo)
{
	char filename[128];
	int ret;
	if(avinfo == NULL)
		return -1;
	sprintf(filename,"%s",avinfo->filename);

	avinfo->play_source = create_playing(filename);
	if (avinfo->play_source == NULL)
		return -1;

#if DEBUG
	avinfo->videofile = fopen("/tmp/video.h264", "wb");
	avinfo->audiofile = fopen("/tmp/audio.alaw", "wb");
#endif

	enum AV_CODEC video_type = read_playing_av_type(avinfo->play_source, TYPE_VIDEO);
	enum AV_CODEC audio_type = read_playing_av_type(avinfo->play_source, TYPE_AUDIO);
	
	fprintf(stderr,"video type is %d, audio type is %d\n",video_type,audio_type);

	ret = read_playing_video_resolution(avinfo->play_source, &avinfo->width, &avinfo->height);
	if(ret < 0){
		fprintf(stderr,"Read video resolution error\n");
	}
	else
		fprintf(stderr,"video res is %dx%d\n",avinfo->width,avinfo->height);

	ret = read_playing_video_frames_num(avinfo->play_source, &avinfo->frames_num);
	if(ret < 0){
		fprintf(stderr,"Read video frames number error\n");
	}
	else
		fprintf(stderr,"video frames is %d\n",avinfo->frames_num);
	
	ret = read_playing_video_fps(avinfo->play_source, &avinfo->fps);
	if(ret < 0){
		fprintf(stderr,"Read video frames fps error\n");
	}
	else
		fprintf(stderr,"video fps is %d\n",avinfo->fps);
#if 1	
	video_pre_buffer = init_av_buffer(VIDEO_PRE_BUFFER_SIZE, USED_VIDEO_PRE_BUF_NUM, MAX_VIDEO_PRE_BUF_NUM);
	if(video_pre_buffer == NULL){
		fprintf(stderr, "Video pre buffer init error\n");
		return NULL;
	}

	audio_pre_buffer = init_av_buffer(AUDIO_PRE_BUFFER_SIZE, USED_AUDIO_PRE_BUF_NUM, MAX_AUDIO_PRE_BUF_NUM);
	if(audio_pre_buffer == NULL){
		fprintf(stderr," Audio pre buffer init error\n");
		return NULL;
	}
#endif
	return 0;
}


static int snx986_avplay_close(struct snx_avplay_info *avinfo)
{
	int ret = 0;

	if (avinfo->started) {
		if ((ret = snx986_avplay_stop(avinfo))) {
			printf("failed to stop '%s': %s", avinfo->filename, strerror(ret));
			goto finally;
		}
	}
	
	//printf("closing");

	if (strlen(avinfo->filename)) {
		strcpy(avinfo->filename, "\0");
	}

#if 1
	if(audio_pre_buffer)
		deinit_av_buffer(audio_pre_buffer);

	if(video_pre_buffer)
		deinit_av_buffer(video_pre_buffer);
	
#endif
finally:
	return ret;
}

struct snx_avplay_info * snx986_avplay_new(void)
{
	int ret = 0;
	struct snx_avplay_info *avinfo = NULL;

	if (!(avinfo = (struct snx_avplay_info *)calloc(1, sizeof(struct snx_avplay_info)))) {
		ret = errno ? errno : -1;
		printf("calloc: %s", strerror(ret));
		goto finally;
	}

finally:
	
	return avinfo;
}

int snx986_avplay_free(struct snx_avplay_info *avinfo)
{
	int ret=0;


	if(!avinfo) {
		ret = EINVAL;
		goto finally;
	}
	
	ret = snx986_avplay_close(avinfo);
	if(ret != 0) {
		goto finally;
	}


	if(avinfo) {
		free(avinfo);
		avinfo = NULL;
	}
		
	ret = 0;
finally:
	return ret;
}

#if 0
int main(int   argc,   char*   argv[])
{
	FILE *file1 = NULL;
	FILE *file2 = NULL;
	int main_size = 0;
	int ret;
	int i = 0,j = 0,k = 0;
	int width,height;
	int frames_num;
	char filename[128];
	sprintf(filename,"%s",argv[1]);
	file1 = fopen("/mnt/bbb.h264", "wb");
	file2 = fopen("/mnt/ccc.alaw", "wb");
	//p_play_source play = create_playing("/mnt/2015-02-26-15-42-37.avi");

	p_play_source play = create_playing(filename);

	enum AV_CODEC video_type = read_playing_av_type(play, TYPE_VIDEO);
	enum AV_CODEC audio_type = read_playing_av_type(play, TYPE_AUDIO);
	fprintf(stderr,"video type is %d, audio type is %d\n",video_type,audio_type);
	ret = read_playing_video_resolution(play, &width, &height);
	if(ret < 0){
		fprintf(stderr,"Read video resolution error\n");
	}
	else
		fprintf(stderr,"video res is %dx%d\n",width,height);
	ret = read_playing_video_frames_num(play,&frames_num);
	if(ret < 0){
		fprintf(stderr,"Read video frames number error\n");
	}
	else
		fprintf(stderr,"video frames is %d\n",frames_num);
	//set_playing_time(play,5);
	while(1){
		void *data = NULL;
		int size;
		int ret = read_playing_data(play, &data, &size);
		if(ret == TYPE_NONE){
			fprintf(stderr,"ret is %d\n",ret);
			break;
		}
		else if(ret == TYPE_VIDEO){
			fwrite(data,size,1,file1);
			main_size += size;
			i++;
			//fprintf(stderr,"write %d,size %d\n",i,main_size);
			//fprintf(stderr,"write size is %d\n",main_size);
		}else if(ret == TYPE_AUDIO){
			fwrite(data,size,1,file2);
		}
		reset_read_playing_data(data);
		
		if(i >= 45){
			fprintf(stderr,"write over 45\n");
			break;
		}
	}
	set_playing_time(play,10);

	while(1){
		void *data = NULL;
		int size;
		int ret = read_playing_data(play, &data, &size);
		if(ret == TYPE_NONE){
			fprintf(stderr,"ret is %d\n",ret);
			break;
		}
		else if(ret == TYPE_VIDEO){
			fwrite(data,size,1,file1);
			main_size += size;
			j++;
			//fprintf(stderr,"write %d,size %d\n",j,main_size);
			//fprintf(stderr,"write size is %d\n",main_size);
		}else if(ret == TYPE_AUDIO){
			fwrite(data,size,1,file2);
		}
		reset_read_playing_data(data);
		if(j >= 45){
			fprintf(stderr,"write over 45\n");
			break;
		}
	}

	set_playing_time(play,20);

	while(1){
		void *data = NULL;
		int size;
		int ret = read_playing_data(play, &data, &size);
		if(ret == TYPE_NONE){
			fprintf(stderr,"ret is %d\n",ret);
			break;
		}
		else if(ret == TYPE_VIDEO){
			fwrite(data,size,1,file1);
			main_size += size;
			k++;
			//fprintf(stderr,"write %d,size %d\n",j,main_size);
			//fprintf(stderr,"write size is %d\n",main_size);
		}else if(ret == TYPE_AUDIO){
			fwrite(data,size,1,file2);
		}
		reset_read_playing_data(data);
		if(k >= 45){
			fprintf(stderr,"write over 45\n");
			break;
		}
	}

	destory_playing(play);
	fclose(file1);
	fclose(file2);
	return 1;
}

#endif

