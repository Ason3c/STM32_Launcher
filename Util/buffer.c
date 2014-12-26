#include <stdlib.h>
#include "buffer.h"

void Add_To_Buffer(uint32_t data,volatile buff_type* buffer) {
	buffer->data[buffer->head++]=data;//Put data in and increment
	buffer->head%=buffer->size;
	if(buffer->head==buffer->tail)	//Buffer wraparound due to filling
		buffer->tail=(buffer->tail+1)%buffer->size;
}

uint8_t Get_From_Buffer(uint32_t* data,volatile buff_type* buffer) {
	if(buffer->tail==buffer->head) {
		*data=0;		//Data reset if there is nothing to return
		return 1;		//Error - no data in buffer
	}
	else {
		*data=buffer->data[buffer->tail];//grab a data sample from the buffer
		buffer->tail++;
		buffer->tail%=buffer->size;
		return 0;		//No error
	}
}

/**
* @brief Returns a byte from the buffer.
* @param Buffer pointer
* @retval byte
*/
uint32_t Pop_From_Buffer(buff_type* buffer) {
	uint32_t d=(buffer->data)[buffer->tail];//read data at tail
	buffer->tail=(buffer->tail+1)%buffer->size;
	return d; //returns the byte
}

void Empty_Buffer(volatile buff_type* buffer) {
	buffer->tail=buffer->head;	//Set the tail to the head, to show there is no usable data present
}

void init_buffer(volatile buff_type* buff, uint16_t size) {
	buff->data=(uint32_t*)malloc(size*4);
	buff->size=size;
}
