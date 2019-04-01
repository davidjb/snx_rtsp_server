#ifndef _DATA_BUF_H_
#define _DATA_BUF_H_

#ifdef __cplusplus
extern "C" {
#endif


#include<stdio.h>
#include<stdlib.h>
#include<string.h>


#define MAX_NODE_NUM 480


typedef struct av_node{
	char *data;
	int size;
	int i_frame_flag;
	int read_mark;
//	int index;
//	struct av_node *next;
}av_node;


typedef struct av_buffer{
	char *buffer_start;
	int size;
	int buffer_ptr;
	int write_pos;//0~480
	int read_pos;
	int need_buf_num;//450
	int max_node_num;//480
	int polling_once;
	struct av_node node[MAX_NODE_NUM];	
}av_buffer;

av_buffer *init_av_buffer(int buf_size,int need_num, int max_node_num);
void deinit_av_buffer(av_buffer *buf);
int write_buffer_data(av_buffer *buffer, char *data, int size);
int read_buffer_data(av_buffer *buffer, char **data, int *size);
void read_write_pos_sync(av_buffer *buffer);

#ifdef __cplusplus
}
#endif

#endif



