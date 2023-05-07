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


//����OS_TCB����˫������Ľ��
typedef struct app_tcb {
    unsigned long      TCBDly;     //�������ݺ�TASK_USER_DATAһ��
	unsigned char      TCBStat;
	unsigned char      TCBPrio;
	struct app_tcb    *Next;     //ָ���һ��
	struct app_tcb    *Prev;     //ָ��ǰһ��
    unsigned long      Ts;       //��ǰʣ��ʱ��Ƭ
    unsigned long      Id;       //TCBΨһ��ʶ
} APP_TCB;


int  APPInsertListTail (APP_TCB **head, APP_TCB *ptcb);         //��˫������β��������
int  APPInsertListHead (APP_TCB **head, APP_TCB *ptcb);         //��˫������ͷ��������

