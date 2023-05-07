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

//ѭ�����нṹ��
typedef struct app_queue{
    int *base;         //��������ռ��ڳ�ʼ��ʱ��̬����
    int front;         //ͷָ��
    int rear;          //βָ��
} APP_QUEUE;


int APPInitQueue(APP_QUEUE *q);               //���г�ʼ��
int APPEnQueue(APP_QUEUE *q, int data);       //���
int APPQueueIsEmpty(APP_QUEUE *q);            //�ж϶����Ƿ�Ϊ��
int APPDeQueue(APP_QUEUE *q);                 //����
