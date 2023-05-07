/*
*********************************************************************************************************
*                                                uC/OS-II
*                                          The Real-Time Kernel
*
*
* File : app_queue.h
* By   : Ahnu OS Team
*********************************************************************************************************
*/

#define  APP_QUEUE_SIZE                 128u              /* Size of Circular Queue       */

//循环队列结构体
typedef struct app_queue{
    int *base;         //整型数组空间在初始化时动态分配
    int front;         //头指针
    int rear;          //尾指针
} APP_QUEUE;


int APPInitQueue(APP_QUEUE *q);               //队列初始化
int APPEnQueue(APP_QUEUE *q, int data);       //入队
int APPQueueIsEmpty(APP_QUEUE *q);            //判断队列是否为空
int APPDeQueue(APP_QUEUE *q);                 //出队
