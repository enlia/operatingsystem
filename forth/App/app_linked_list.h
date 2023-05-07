/*
*********************************************************************************************************
*                                                uC/OS-II
*                                          The Real-Time Kernel
*
*
* File : app_linked_list.h
* By   : Ahnu OS Team
*********************************************************************************************************
*/


//仿照OS_TCB构造双向链表的结点
typedef struct app_tcb {
    unsigned long      TCBDly;     //基本数据和TASK_USER_DATA一致
	unsigned char      TCBStat;
	unsigned char      TCBPrio;
	struct app_tcb    *Next;     //指向后一个
	struct app_tcb    *Prev;     //指向前一个
    unsigned long      Ts;       //当前剩余时间片
    unsigned long      Id;       //TCB唯一标识
} APP_TCB;


int  APPInsertListTail (APP_TCB **head, APP_TCB *ptcb);         //在双向链表尾部插入结点
int  APPInsertListHead (APP_TCB **head, APP_TCB *ptcb);         //在双向链表头部插入结点

