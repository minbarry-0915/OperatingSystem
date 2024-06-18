#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

typedef struct {
	pthread_mutex_t lock ;
	char ** elem ;
	int capacity ;
	int num ; 
	int front ;
	int rear ;
} bounded_buffer ;

bounded_buffer * buf = 0x0 ;

void 
bounded_buffer_init (bounded_buffer * buf, int capacity) {
	pthread_mutex_init(&(buf->lock), 0x0) ;
	buf->capacity = capacity ;
	buf->elem = (char **) calloc(sizeof(char *), capacity) ;
	buf->num = 0 ;
	buf->front = 0 ;
	buf->rear = 0 ;
}

void 
bounded_buffer_queue (bounded_buffer * buf, char * msg) 
{
	pthread_mutex_lock(&(buf->lock)) ;
	while (!(buf->num < buf->capacity)) { //버퍼의 개수가 버퍼의 크기보다 작지 않다면: 버퍼가 꽉찬다면
		pthread_mutex_unlock(&(buf->lock)) ;

		pthread_mutex_lock(&(buf->lock)) ;
	}
	//버퍼의 원소 갯수가 버퍼의 용량보다 작다면: 원소를 삽입할 수 있는 조건이 되므로 탈출
	buf->elem[buf->rear] = msg ;
	buf->rear = (buf->rear + 1) % buf->capacity ; //원형 큐라서 버퍼의 용량을 넘어갔을때의 조건 추가
	buf->num += 1 ; 
	
	pthread_mutex_unlock(&(buf->lock)) ;
}

char * 
bounded_buffer_dequeue (bounded_buffer * buf) 
{
	char * r = 0x0 ;
	pthread_mutex_lock(&(buf->lock)) ;
	while (!(buf->num > 0)) {  //버퍼에 원소가 없으면
		pthread_mutex_unlock(&(buf->lock)) ;

		pthread_mutex_lock(&(buf->lock)) ;
	}
	//버퍼에 원소가 있으면
	r = buf->elem[buf->front] ;
	buf->front = (buf->front + 1) % buf->capacity ;
	buf->num -= 1 ;
	
	pthread_mutex_unlock(&(buf->lock)) ;
	return r ;
}


void * 
producer (void * ptr) 
{
	char msg[128] ;
	pthread_t tid ;
	int i ;
	
	tid = pthread_self() ; //tid에 스레드의 id를 넣음
	for (i = 0 ; i < 10 ; i++) {
		snprintf(msg, 128, "(%ld,%d)", (unsigned long) tid, i) ; //(버퍼포인터, 버퍼크기, 서식 문자열, 넣을 변수들)
		bounded_buffer_queue(buf, strdup(msg)) ;
	}
	return 0x0 ;
}

void * 
consumer (void * ptr) 
{
	pthread_t tid ;
	char * msg ; 
	int i ;

	tid = pthread_self() ;

	for (i = 0 ; i < 10 ; i++) {
		msg = bounded_buffer_dequeue(buf) ;
		if (msg != 0x0) {
			printf("[%ld] reads %s\n", (unsigned long) tid, msg) ;
			free(msg) ;
		}
	}
}

int 
main() 
{
	pthread_t prod[5] ;
	pthread_t cons[5] ;
	int i ;

	buf = malloc(sizeof(bounded_buffer)) ;
	bounded_buffer_init(buf, 3) ;

	for (i = 0 ; i < 5 ; i++) {
		pthread_create(&(prod[i]), 0x0 , producer, 0x0) ;
		pthread_create(&(cons[i]), 0x0, consumer, 0x0) ;
	}

	for (i = 0 ; i < 5 ; i++) {
		pthread_join(prod[i], 0x0) ;
		pthread_join(cons[i], 0x0) ;
	}
	exit(0) ;
}
