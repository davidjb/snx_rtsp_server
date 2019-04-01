#include "data_buf.h"


av_buffer *init_av_buffer(int buf_size,int need_num, int max_node_num){
	int i;
	av_buffer *buffer = (av_buffer *)malloc(sizeof(av_buffer));
	if(buffer == NULL){
		printf("cannot get memory for buffer in init_av_buffer\n");
		return NULL;	
	}
	memset(buffer, 0, sizeof(av_buffer));
		//av_node *node = init_av_node(node_num);
	buffer->need_buf_num = need_num;
	if(max_node_num == 0)
		buffer->max_node_num = MAX_NODE_NUM;
	else
		buffer->max_node_num = max_node_num;
	buffer->buffer_start = (char *)malloc(buf_size);
	if(buffer->buffer_start == NULL){
		printf("cannot get memory for buffer->buffer_start in init_av_buffer\n");
		free(buffer);
		return NULL;	
	}
	memset(buffer->buffer_start, 0, buf_size);
	buffer->size = buf_size;
	buffer->read_pos = -1;
	buffer->need_buf_num = need_num;
	return buffer;
}

void deinit_av_buffer(av_buffer *buf){
	if(buf){
		if(buf->buffer_start)
			free(buf->buffer_start);
		free(buf);
	}
	return;
}
/*
int write_buffer_data(av_buffer *buffer, char *data, int size){
	av_buffer *buf = buffer;
		
	if(buffer){
	if(buf->polling_once == 0){
		if(buf->write_pos == buf->need_buf_num)
			buf->polling_once = 1;
	}
	if(buf->write_pos == buf->max_node_num)	{
		buf->write_pos = 0;
		
	}fprintf(stderr,"%d  %d   %d      %d----\n",buf->size, buf->buffer_ptr,size, buf->write_pos);
	if((buf->size - buf->buffer_ptr) >= size){
		memcpy(buf->buffer_start + buf->buffer_ptr, data, size);
	}else{
		int subsize = size - (buf->size - buf->buffer_ptr);
		memcpy(buf->buffer_start + buf->buffer_ptr, data, buf->size - buf->buffer_ptr);
		memcpy(buf->buffer_start,data + (buf->size - buf->buffer_ptr), subsize);
	}
	buf->node[buf->write_pos].data = buf->buffer_start + buf->buffer_ptr;
	buf->node[buf->write_pos].size = size;
	buf->node[buf->write_pos].read_mark = 0;
	if((buf->buffer_ptr + size) <= buf->size)
		buf->buffer_ptr += size;
	else
		buf->buffer_ptr = (buf->buffer_ptr + size) - buf->size;

	buf->write_pos ++; //maybe write pos need to ++ at the end of the fun
	if(buf->write_pos == buf->max_node_num)	{
		buf->write_pos = 0;
		
	}
	return 0;
	}else
		return -1;
}
*/

int write_buffer_data(av_buffer *buffer, char *data, int size){
	av_buffer *buf = buffer;
		
	if(buffer){
		if(buf->polling_once == 0){//fprintf(stderr,"%d   %d \n",buf->write_pos ,buf->need_buf_num);
			if(buf->write_pos == buf->need_buf_num -1)
				buf->polling_once = 1;
		}
		if(buf->write_pos == buf->max_node_num)	{
			buf->write_pos = 0;
			
		}//fprintf(stderr,"%d  %d   %d      %d----\n",buf->size, buf->buffer_ptr,size, buf->write_pos);
		if((buf->size - buf->buffer_ptr) >= size){
			memcpy(buf->buffer_start + buf->buffer_ptr, data, size);
			buf->node[buf->write_pos].data = buf->buffer_start + buf->buffer_ptr;
			buf->buffer_ptr += size;
			//fprintf(stderr,"%p  %d      %d ----\n",buf->node[buf->write_pos].data, buf->buffer_ptr, buf->write_pos);
		}else{
			memcpy(buf->buffer_start, data, size);
			buf->node[buf->write_pos].data = buf->buffer_start;
			buf->buffer_ptr = size;
		}
		buf->node[buf->write_pos].size = size;
		buf->node[buf->write_pos].read_mark = 0;

		buf->write_pos ++; //maybe write pos need to ++ at the end of the fun
		if(buf->write_pos == buf->max_node_num)	{
			buf->write_pos = 0;
			
		}
		return 0;
	}else
		return -1;
}

int read_buffer_data(av_buffer *buffer, char **data, int *size){
	av_buffer *buf = buffer;
	int i;
	//int read_pos = buf->read_pos;
	if(buffer){
		if(buf->polling_once == 0)
			buf ->read_pos = 0;
		else{
			if(buf->read_pos == -1){
				if((buf->write_pos - buf->need_buf_num) >= 0){
					buf ->read_pos = buf->write_pos - buf->need_buf_num;
					
				}else{
					buf->read_pos = buf->max_node_num - (buf->need_buf_num - buf->write_pos);
				}
			}
		}
	//	fprintf(stderr,"111111read pos is %d,write pos is %d \n",buf->read_pos, buf->write_pos);
		while(1){
			if(buf->node[buf->read_pos].read_mark == 0){
				buf->node[buf->read_pos].read_mark = 1;
				*data = buf->node[buf->read_pos].data;
				*size = buf->node[buf->read_pos].size;
				//fprintf(stderr,"22222read pos is %d,write pos is %d  read addr is %p   size %d\n",buf->read_pos, buf->write_pos, buf->node[buf->read_pos].data,buf->node[buf->read_pos].size);
				buf->read_pos++;
				if(buf->read_pos == buf->max_node_num){
					buf->read_pos = 0;
				}
				
				return 0;
			}else{
				if(buf->read_pos == buf->write_pos){
					*size = 0;
					*data = NULL;//fprintf(stderr,"33333read pos is %d,write pos is %d \n",buf->read_pos, buf->write_pos);
					return -1;
				}
				buf->read_pos++;
				if(buf->read_pos == buf->max_node_num){
					buf->read_pos = 0;
				}
			}
			
		}
		return 0;
	}else
		return -1;
	
}

void read_write_pos_sync(av_buffer *buffer){
	int i;
	buffer->read_pos = -1;
	for(i = 0; i < MAX_NODE_NUM; i++ )
		buffer->node[i].read_mark = 0;
	return;
}


