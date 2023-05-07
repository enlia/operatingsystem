/*
*********************************************************************************************************
*                                                uC/OS-II
*                                          The Real-Time Kernel
*
*                                               ROUTINE #4
*********************************************************************************************************
*/
#define _CRT_SECURE_NO_WARNINGS
#include "includes.h"
#include "app_linked_list.h"
#include "app_queue.h"

/*
*********************************************************************************************************
*                                              CONSTANTS
*********************************************************************************************************
*/

#define          TASK_STK_SIZE     512                /* Size of each task's stacks (# of WORDs) */

/* �������ȼ�  */
#define          TASK_START_PRIO     0      //��ʼ�������������ȼ�

#define          TASK_1_PRIO        10      //5���û�����
#define          TASK_2_PRIO        11
#define          TASK_3_PRIO        12
#define          TASK_4_PRIO        13
#define          TASK_5_PRIO        14

#define          TASK_DISP_PRIO     25      //������ʾ�û�������Ϣ
#define          TASK_CLK_PRIO      30      //������ʾϵͳʱ�� ������ CPU������


//���汻��ѯ����������Ϣ
typedef struct {
	INT32U       MyTCBDly;
	INT8U        MyTCBStat;
	INT8U        MyTCBPrio;
    INT32U       MyTCBCtxSwCtr;    
    INT32U       MyTCBCycles;
} TASK_USER_DATA;


/*
*********************************************************************************************************
*                                              VARIABLES
*********************************************************************************************************
*/

OS_STK          TaskStartStk[TASK_STK_SIZE];          /* Startup     task stack                     */
OS_STK          Task1Stk[TASK_STK_SIZE];              /* MyTask1     task stack                     */
OS_STK          Task2Stk[TASK_STK_SIZE];              /* MyTask2     task stack                     */
OS_STK          Task3Stk[TASK_STK_SIZE];              /* MyTask3     task stack                     */
OS_STK          Task4Stk[TASK_STK_SIZE];              /* MyTask4     task stack                     */
OS_STK          Task5Stk[TASK_STK_SIZE];              /* MyTask5     task stack                     */
OS_STK          TaskClkStk[TASK_STK_SIZE];            /* Clock       task stack                     */
OS_STK          TaskDispStk[TASK_STK_SIZE];           /* Display     task stack                     */


FP32    AppCPUUsage;     //��2��CPU������
INT32U  time_idle;

OS_TCB  tcb_copy;                   //�������Ʊ���ѯ�����TCB

TASK_USER_DATA  TaskUserData[5];    //���5���û�����Ĳ�ѯ��Ϣ

APP_TCB         AppTCBTbl[128];     //ȫ��APP_TCB����  ��128����  ����OSTCBTbl
APP_TCB        *AppTCBFreeList;     //����APP_TCB��ͷ
APP_TCB        *AppTCBPrioTbl[64];  //��Ӧ64�����ȼ���ָ������  ��������APP_TCB˫������  ������OSTCBPrioTbl

APP_QUEUE       Buffer;             //���建���

OS_EVENT       *SempFull;           //����Full�ź���--------��Դ1
OS_EVENT       *SempEmpty;          //����Empty�ź���-------��Դ2
OS_EVENT       *Mutex;              //���廥���ź���--------��Դ3

//�Ա�����5���û�����3����Դ      P0~P4��ӦMyTask1~MyTask5
int available[3];          //��������Դ����
int allocation[5][3];      //�ѷ������
int need[5][3];            //�������
INT8U finish[5];           //����Ƿ����㹻��Դ  1 TRUE��0 FALSE

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

void  TaskStart(void *data);
void  MyTask1(void *data);
void  MyTask2(void *data);
void  MyTask3(void *data);
void  MyTask4(void *data);
void  MyTask5(void *data);
void  TaskClock(void *data);
void  TaskDisp(void *data);

void  UpdateTaskData(INT8U id);
void  DispTaskData(INT8U id);

void  InitAppTCBList (void);
int   AppTCBInit (INT8U  prio, INT32U  slice);
void  AppSchedRR  (INT8U  prio);
void  AppSchedRR2 (INT8U  prio);

void  AppDeadLock();

/*$PAGE*/
/*
*********************************************************************************************************
*                                                  MAIN
*********************************************************************************************************
*/

int main (void)
{
    INT8U  err;

	PC_DispClrScr(DISP_BGND_BLACK);                        /* Clear the screen                         */

    OSInit();                                              /* Initialize uC/OS-II                      */

    InitAppTCBList();          //��ʼ������APP_TCB��  ����������
	
	time_idle  = 0;

	PC_ElapsedInit();

	/* ToDo: Create semaphores, mailboxes etc. here						       */
	SempFull = OSSemCreate(0);
	SempEmpty = OSSemCreate(APP_QUEUE_SIZE);
	Mutex = OSMutexCreate(1, &err);    //���ȼ��컨��Ϊ1

	/* ���³�ʼ����Ϊ���������     */
	available[0] = 0;                //û�в�Ʒ
	available[1] = APP_QUEUE_SIZE;   //������128����λ
	available[2] = 1;                //������Ŀǰ����
	OS_MemClr((INT8U *)&finish[0], sizeof(finish));    //����finish����

	APPInitQueue(&Buffer);     //��ʼ�������

    OSTaskCreate(TaskStart, (void *)0, &TaskStartStk[TASK_STK_SIZE - 1], TASK_START_PRIO);	/* Create the startup task */
    
	OSStart();                                             /* Start multitasking                       */

    return 0;
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                               STARTUP TASK
*********************************************************************************************************
*/
void  TaskStart (void *pdata)
{
    char     s[40];
	INT16S   key;

    pdata = pdata;                                         /* Prevent compiler warning                 */

    /* Setup fixed part of screen  */
	PC_DispStr( 0,  0, "                           ʵʱ���������ϵͳuC/OS-II                           ", DISP_FGND_WHITE + DISP_BGND_RED);
    PC_DispStr( 0,  1, " ��Ȩ����:                                                                      ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  2, "  1.�ں�Դ����Jean J. Labrosse�ṩ                                              ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  3, "  2.x86-Win32ƽ̨��ֲ��Werner Zimmermann�ṩ                                    ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  4, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  5, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  6, "  CH5 ����                                                                      ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  7, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  8, "    UserTask   ��̨����  OSTCBPrio    Status   ��̨���   APP_TCB��   ��ǰ�ֵ�ID", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  9, "   ----------  --------  ---------  --------  ---------   ---------   ----------", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 10, "     MyTask1                                                                    ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 11, "     MyTask2                                                                    ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 12, "     MyTask3                                                                    ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 13, "     MyTask4                                                                    ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 14, "     MyTask5                                                                    ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 15, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 16, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 17, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 18, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 19, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	PC_DispStr( 0, 20, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 21, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 22, "    ��������  :         ÿ�������л�:           CPU������1:      %   2:      %  ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 23, "    �ں˰汾  :                                                                 ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 24, "    ��ǰʱ��  :                                       <--����ESC�����˳�-->     ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);

    /* Initialize uC/OS-II's statistics  */
	OSStatInit();
	
	/* Create clock & display task */
	OSTaskCreate(TaskClock, (void *)0, &TaskClkStk[TASK_STK_SIZE - 1], TASK_CLK_PRIO);
	OSTaskCreate(TaskDisp, (void *)0, &TaskDispStk[TASK_STK_SIZE - 1], TASK_DISP_PRIO);

	/* Create user tasks */
	OSTaskCreate(MyTask1, (void *)0, &Task1Stk[TASK_STK_SIZE - 1], TASK_1_PRIO);
	OSTaskCreate(MyTask2, (void *)0, &Task2Stk[TASK_STK_SIZE - 1], TASK_2_PRIO);
	OSTaskCreate(MyTask3, (void *)0, &Task3Stk[TASK_STK_SIZE - 1], TASK_3_PRIO);
	OSTaskCreate(MyTask4, (void *)0, &Task4Stk[TASK_STK_SIZE - 1], TASK_4_PRIO);
	OSTaskCreate(MyTask5, (void *)0, &Task5Stk[TASK_STK_SIZE - 1], TASK_5_PRIO);

	/* Display the current version of OS   */
	sprintf(s, "uC/OS-II V%1d.%02d", OSVersion() / 100, OSVersion() % 100);
    PC_DispStr(18, 23, s, DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);

    for (;;) {
        if (PC_GetKey(&key)) {                             /* See if key has been pressed              */
            if (key == 0x1B) {                             /* Yes, see if it's the ESCAPE key          */
                exit(0);                                   /* Yes, return to DOS                       */
            }
        }

        /* ToDo: Don't forget to call the uCOS scheduler with OSTimeDly etc., to give other tasks a chance to run */
		OSTimeDly(400); 
    }
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                               TASK #1
*********************************************************************************************************
*/
void MyTask1(void *pdata)
{ 
    INT8U   i;
	char    s[20];
	INT32U  time1;
	
	pdata = pdata;                            /* Prevent compiler warning      */

	for (i = 0; i < 2; i++) {         //��ʼ��2��APP_TCB�� ���β�������AppTCBPrioTbl[10]
        AppTCBInit(TASK_1_PRIO, 1);   //1��ʱ��Ƭ
    }

    while (1) {
		time1  = PC_ElapsedStop(1);
        PC_ElapsedStart(1);
		sprintf(s, "%5.2f", time1/ (FP32)1000000);
        PC_DispStr(47, 10, s, DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);  //��ʾ��̨ʱ����
		
		AppSchedRR(TASK_1_PRIO);    //��ʱ��Ƭ��ת   ʱ��Ƭ��λ = MyTask1��̨���
		//AppSchedRR2(TASK_1_PRIO);    //�滻Ϊ������������

		i = APPGetListLen(&AppTCBPrioTbl[TASK_1_PRIO]);
		sprintf(s, "%2d", i);
		PC_DispStr(62, 10, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);
		
		if (i>0) {
			sprintf(s, "%4u", AppTCBPrioTbl[TASK_1_PRIO]->Id);
			PC_DispStr(74, 10, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);
		} else {
			PC_DispStr(74, 10, "    ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
		}
		
		OSTimeDly(150);                       /* Wait 150 ticks                  */
    }
}

/*
*********************************************************************************************************
*                                               TASK #2
*********************************************************************************************************
*/
void MyTask2(void *pdata)
{ 
    INT8U  i;
	char  s[20];
	INT32U  time2;
	
	for (i = 0; i < 3; i++) {         //��ʼ��3��APP_TCB�� ���β�������AppTCBPrioTbl[11]
        AppTCBInit(TASK_2_PRIO, 2);   //2��ʱ��Ƭ
    }

	pdata = pdata;                            /* Prevent compiler warning      */
    while (1) {
		time2  = PC_ElapsedStop(2);
        PC_ElapsedStart(2);
		sprintf(s, "%5.2f", time2/ (FP32)1000000);
        PC_DispStr(47, 11, s, DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);  //��ʾ��̨ʱ����

		AppSchedRR(TASK_2_PRIO);    //��ʱ��Ƭ��ת   ʱ��Ƭ��λ = MyTask2��̨���
		//AppSchedRR2(TASK_2_PRIO);      //�滻Ϊ������������

		i = APPGetListLen(&AppTCBPrioTbl[TASK_2_PRIO]);
		sprintf(s, "%2d", i);
		PC_DispStr(62, 11, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);

		if (i>0) {
			sprintf(s, "%4u", AppTCBPrioTbl[TASK_2_PRIO]->Id);
			PC_DispStr(74, 11, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);
		} else {
			PC_DispStr(74, 11, "    ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
		}
		
        OSTimeDly(120);                       /* Wait 120 ticks                  */
    }
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                               TASK #3
*********************************************************************************************************
*/
void MyTask3(void *pdata)
{ 
    INT8U  i;
	char  s[20];
	INT32U  time3;
	
	pdata = pdata;                            /* Prevent compiler warning      */

	for (i = 0; i < 5; i++) {         //��ʼ��5��APP_TCB�� ���β�������AppTCBPrioTbl[12]
        AppTCBInit(TASK_3_PRIO, 4);   //4��ʱ��Ƭ
    }

	while (1) {
		time3  = PC_ElapsedStop(3);
        PC_ElapsedStart(3);
		sprintf(s, "%5.2f", time3/ (FP32)1000000);
        PC_DispStr(47, 12, s, DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);  //��ʾ��̨ʱ����

		AppSchedRR(TASK_3_PRIO);    //��ʱ��Ƭ��ת   ʱ��Ƭ��λ = MyTask3��̨���
		
		i = APPGetListLen(&AppTCBPrioTbl[TASK_3_PRIO]);
		sprintf(s, "%2d", i);
		PC_DispStr(62, 12, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);
		
		sprintf(s, "%4u", AppTCBPrioTbl[TASK_3_PRIO]->Id);
		PC_DispStr(74, 12, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);

        OSTimeDly(90);                       /* Wait 90 ticks                  */
    }
}

/*
*********************************************************************************************************
*                                               TASK #4
*********************************************************************************************************
*/
void MyTask4(void *pdata)
{ 
    INT8U  err, count1 = 0;
	char   s[20];

	pdata = pdata;                            /* Prevent compiler warning      */
    PC_DispStr(10, 16, "������MyTask4  ��Ʒ�����", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);


	while (1) {
		
		
		OSSemPend(SempEmpty, 0, &err);
		

		count1++;
		sprintf(s, "%2d", count1);
		PC_DispStr(36, 16, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);

		OSMutexPend(Mutex, 0, &err);

		APPEnQueue(&Buffer, 1);

		err = OSMutexPost(Mutex);
        
		err = OSSemPost(SempFull);
		

		OSTimeDly(70);                       /* Wait 70 ticks                  */
    }
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                               TASK #5
*********************************************************************************************************
*/
void MyTask5(void *pdata)
{ 
    INT8U  err, count2 = 0;
	char   s[20];

	pdata = pdata;                            /* Prevent compiler warning      */
	PC_DispStr(10, 17, "������MyTask5  ������", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);


	while (1) {
		
		OSMutexPend(Mutex, 0, &err);
		
        
		OSSemPend(SempFull, 0, &err);
		
		count2++;
		sprintf(s, "%2d", count2);
		PC_DispStr(36, 17, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);
		

		APPDeQueue(&Buffer);


		err = OSMutexPost(Mutex);
		

		err = OSSemPost(SempEmpty);
		
		
		OSTimeDly(30);                       /* Wait 30 ticks                  */ 
    }
}

/*
*********************************************************************************************************
*                                               CLOCK TASK
*********************************************************************************************************
*/
void  TaskClock (void *pdata)
{
	char  s[40];

    pdata = pdata;                            /* Prevent compiler warning      */
    for (;;) {
		sprintf(s, "%3d", OSTaskCtr);
		PC_DispStr(18, 22, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);      /* Total tasks              */
		
		sprintf(s, "%3d", OSCtxSwCtr);
		PC_DispStr(39, 22, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);      /* Context switch counter   */
		
		sprintf(s, "%5.2f", OSCPUUsage);
		PC_DispStr(60, 22, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);      /* CPU usage                */
		sprintf(s, "%5.2f", AppCPUUsage * 100);
		PC_DispStr(72, 22, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);      /* CPU usage 2              */
		
		PC_GetDateTime(s);
        PC_DispStr(18, 24, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);      /* System time              */

		PC_DispStr(70, 14, "          ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);    //ÿ�������Ϣ��ʾ
		OSCtxSwCtr = 0;   //ÿ���������л�����
		
		OSTimeDly(OS_TICKS_PER_SEC);
    }
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                               DISPLAY TASK
*********************************************************************************************************
*/
void TaskDisp(void *pdata)
{ 
    INT8U   i;
	
	pdata = pdata;                            /* Prevent compiler warning      */
    while (1) { 
		for (i = 0; i < 5; i++) {             //ˢ��5���û�����
			DispTaskData(i);                                
        }

        OSTimeDly(35);                        //ֵ��Сˢ��Ƶ��̫��  �����ײ�׽״̬�仯
    }
}


/*$PAGE*/
/*
*********************************************************************************************************
*                                         ��TASK_USER_DATA�йغ���
*********************************************************************************************************
*/

void  UpdateTaskData (INT8U id)
{
    INT8U err;

	err = OSTaskQuery (id + 10, &tcb_copy);
	TaskUserData[id].MyTCBDly = tcb_copy.OSTCBDly;
	TaskUserData[id].MyTCBStat = tcb_copy.OSTCBStat;
	TaskUserData[id].MyTCBPrio = tcb_copy.OSTCBPrio;
	TaskUserData[id].MyTCBCtxSwCtr = tcb_copy.OSTCBCtxSwCtr;
	//TaskUserData[id].MyTCBCyclesTot = tcb_copy.OSTCBCyclesTot;
}

void  DispTaskData (INT8U id)
{
	char  s[40];

	sprintf(s, "     %3d       %3d", TaskUserData[id].MyTCBCtxSwCtr, TaskUserData[id].MyTCBPrio);
    PC_DispStr(13, (INT8U)(id + 10), s, DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);

	if (0 == TaskUserData[id].MyTCBStat) {
		if (TaskUserData[id].MyTCBDly <= 5) {
			PC_DispStr(35, (INT8U)(id + 10), "    Ready", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
		} else {
			PC_DispStr(35, (INT8U)(id + 10), "  Delayed", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
		}
	} else if (1 == TaskUserData[id].MyTCBStat) {    //�ȴ��ź����¼�
		PC_DispStr(35, (INT8U)(id + 10), " Wait_Sem", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	} else if (4 == TaskUserData[id].MyTCBStat) {    //�ȴ���Ϣ�¼�
		PC_DispStr(35, (INT8U)(id + 10), " Wait_Msg", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    } else if (8 == TaskUserData[id].MyTCBStat) {    //������ �ȴ��ָ��¼�
		PC_DispStr(35, (INT8U)(id + 10), "Suspended", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	} else if (16 == TaskUserData[id].MyTCBStat) {   //�ȴ������ź����¼�
		PC_DispStr(35, (INT8U)(id + 10), " Wait_Mtx", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	} else {
		PC_DispStr(35, (INT8U)(id + 10), "    Other", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	}

	//sprintf(s, "%6d", TaskUserData[id].MyTCBCycles);
    //PC_DispStr(47, (INT8U)(id + 10), s, DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
}

/*
*********************************************************************************************************
*                                           ��APP_TCB�йغ���         
*
*  1. InitAppTCBList() -- Initialize the free list of APP_TCBs
*  2. AppTCBInit()     -- ��ʼ��һ��APP_TCB�� ����������Ӧ��AppTCBPrioTbl[prio]
*  3. AppSchedRR()     -- ��˫��������ʵ����ת����
*  4. AppSchedRR2()    -- ����ת���Ȼ�����ʵ�ֶ༶��������
*********************************************************************************************************
*/

void  InitAppTCBList (void)
{
    INT8U    ix;
    INT8U    ix_next;
    APP_TCB  *ptcb1;
    APP_TCB  *ptcb2;

    OS_MemClr((INT8U *)&AppTCBTbl[0], sizeof(AppTCBTbl));              /* Clear all the APP_TCBs         */
    OS_MemClr((INT8U *)&AppTCBPrioTbl[0], sizeof(AppTCBPrioTbl));      /* Clear the priority table       */
    for (ix = 0u; ix < 128; ix++) {         //��ʼ��128��APP_TCB��
        ix_next       = ix + 1u;
        ptcb1         = &AppTCBTbl[ix];
        ptcb2         = &AppTCBTbl[ix_next];
        ptcb1->Next   = ptcb2;
        ptcb1->Id     = ix;              //�����±������ID��һ����
    }
    ptcb1       = &AppTCBTbl[ix];
    ptcb1->Next = (APP_TCB *)0;      //���һ����

    AppTCBFreeList = &AppTCBTbl[0];  //����APP_TCB��ͷ
}

int  AppTCBInit (INT8U  prio, INT32U  time_quanta)
{
    APP_TCB    *ptcb;

    ptcb = AppTCBFreeList;                                  /* Get a free TCB from the free TCB list    */
	if (ptcb != (APP_TCB *)0 && ptcb->Id < 1000) {
        AppTCBFreeList    = ptcb->Next;                     /* Update pointer to free TCB list          */

        ptcb->TCBPrio     = prio;               
        ptcb->TCBStat     = OS_STAT_RDY;            
        ptcb->TCBDly      = 0u;                           
		ptcb->Next        = (APP_TCB *)0;        //����ָ�����
        
		ptcb->TQ          = time_quanta;   //����ʱ��Ƭ
		ptcb->TQctr       = time_quanta;   //���õ�ǰʣ��ʱ��Ƭ
        
		ptcb->Id         += 1000u;         //ÿ���»���TCB�ı�ʶ����λ�������
     
		APPInsertListTail(&AppTCBPrioTbl[prio], ptcb);  //���»��ֵ�TCB�����ȼ������Ӧ����
		return 1;
	}
	return 0;
}


void  AppSchedRR (INT8U  prio)
{
	APP_TCB    *ptcb;

	ptcb = AppTCBPrioTbl[prio];	
	if (ptcb->TQctr > 0u) {
		ptcb->TQctr--;
		return;
	}

	if (ptcb->Next != NULL) {
		AppTCBPrioTbl[prio] = ptcb->Next;
		ptcb->Next = NULL;
		ptcb->TQctr = ptcb->TQ;
		APPInsertListTail(&AppTCBPrioTbl[prio], ptcb);
	} else {
		ptcb->TQctr = ptcb->TQ;
	}
}

//��ʵ��
void  AppSchedRR2 (INT8U  prio)
{
	


}

/*
*********************************************************************************************************
*                                                �������
*********************************************************************************************************
*/

//�ж�һ������������Դ�Ƿ�ȫΪ��   ����ֵ��1ȫΪ�㣻0��ȫΪ��
int isAllZero(int idx)
{
	int j;
	for(j = 0; j < 3; j++) {
		if(need[idx][j] != 0)
			return 0;
	}
	return 1;
}


//��ʵ��
void AppDeadLock()
{




}

/*
*********************************************************************************************************
*                                           STATISTIC TASK HOOK
*********************************************************************************************************
*/

void  OSTaskStatHook (void)
{
    INT8U   i;
   
    for (i = 0; i < 5; i++) {             //�������û���������  ����ӡ
        UpdateTaskData(i);
    }
}

/*
*********************************************************************************************************
*                                           TASK SWITCH HOOK
*********************************************************************************************************
*/

void  OSTaskSwHook (void)
{
    INT32U    time;

    time  = PC_ElapsedStop(0);                   /* This task is done                                  */
    PC_ElapsedStart(0);                          /* Start for next task                                */

	/*if ((OSTCBCur->OSTCBPrio >= 10) && (OSTCBCur->OSTCBPrio <= 14)) {
		TaskUserData[(OSTCBCur->OSTCBPrio) - 10].MyTCBCycles = time;
	}*/

	if (OSTCBCur->OSTCBPrio == 30) {
		AppCPUUsage = 1.0 - time_idle / (FP32)1000000;
		time_idle = 0;
	}

	if (OSTCBCur->OSTCBPrio == 63) {
		time_idle +=  time;
	}
}

/*
*********************************************************************************************************
*                                              DUMMY HOOKS
*********************************************************************************************************
*/
void OSInitHookEnd()
{
}

void OSTimeTickHook()
{
}

void OSTaskCreateHook(OS_TCB *ptcb)
{
}

void OSTaskIdleHook()
{   
	//OS_SLEEP();
}
