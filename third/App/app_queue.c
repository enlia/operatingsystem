/*
*********************************************************************************************************
*                                                uC/OS-II
*                                          The Real-Time Kernel
*
* 实现一个循环队列
*
* File : app_queue.c
* By   : Ahnu OS Team
*********************************************************************************************************
*/
#include <stdlib.h>
#include "app_queue.h"


int APPInitQueue(APP_QUEUE *q) {
    q->base = (int *)malloc(APP_QUEUE_SIZE * sizeof(int));
    if(q->base == NULL){
        return 0;   //申请内存失败
    }
    q->front=0;
    q->rear=0;
    return 1;
}


int APPEnQueue(APP_QUEUE *q, int data) {
    if((q->rear + 1 )% APP_QUEUE_SIZE == q->front) {
        return 0;   //队列满  入队失败
    } else {
        q->base[q->rear] = (q->rear+1) % APP_QUEUE_SIZE;
        q->base[q->rear] = data;
    }
	return 1;
}


int APPQueueIsEmpty(APP_QUEUE *q) {
    return (q->rear == q->front);
}


int APPDeQueue(APP_QUEUE *q) {
    int data;
    data = q->base[q->front];
    q->front = (q->front + 1) % APP_QUEUE_SIZE;
	return data;
}
