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
    INT32U       MyTCBCyclesTot;
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

OS_TCB  tcb_copy;                   //�������Ʊ���ѯ�����TCB

TASK_USER_DATA  TaskUserData[5];    //���5���û�����Ĳ�ѯ��Ϣ

APP_TCB         AppTCBTbl[128];     //ȫ��APP_TCB����  ��128����  ����OSTCBTbl
APP_TCB        *AppTCBFreeList;     //����APP_TCB��ͷ
APP_TCB        *AppTCBPrioTbl[64];  //��Ӧ64�����ȼ���ָ������  ��������APP_TCB˫������  ������OSTCBPrioTbl


OS_EVENT       *RW;                 //��д�� ���ļ��Ļ������
OS_EVENT       *Mutex;              //��readcount�����Ļ������
int readcount;                      //��ǰ���߼���

OS_EVENT       *MsgQ;               //������Ϣ����
void           *MsgQStr[20];        //������Ϣָ������


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
void  AppTCBInit (INT8U  prio, INT32U  slice);
void  AppSchedRR(void);

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

	/* ToDo: Create semaphores, mailboxes etc. here						       */
	RW = OSMutexCreate(1, &err);             //������д��
	Mutex = OSMutexCreate(2, &err);          //���������ź���
	readcount = 0;
	MsgQ = OSQCreate(&MsgQStr[0], 20);       //������Ϣ����

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
    //INT8U  err;
	INT8U    i;
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
    PC_DispStr( 0,  6, "  CH4 ���Ȼ���                                                                  ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  7, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  8, "    UserTask    ��̨����    OSTCBDly   OSTCBPrio          Status                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  9, "   ----------  ----------  ---------  ----------    ------------                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
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
    PC_DispStr( 0, 22, "    ��������  :          CPU������:      %    ÿ�������л�:                     ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
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
    INT8U  err;
	char  s[20];
	
	pdata = pdata;                            /* Prevent compiler warning      */
	PC_DispStr(10, 16, "����MyTask1", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    while (1) {
		OSMutexPend(Mutex, 0, &err);
		if (readcount == 0)
			OSMutexPend(RW, 0, &err);
		readcount++;
		err = OSMutexPost(Mutex);
		PC_DispStr(22, 18, "          ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);   //���������д��
   
		//���ļ�
		PC_DispStr(22, 16, "���ڶ�", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
		OSTimeDly(150);                       /* Wait 150 ticks                  */

		OSMutexPend(Mutex, 0, &err);
		readcount--;
		if (readcount == 0)
			err = OSMutexPost(RW);
		err = OSMutexPost(Mutex);
    }
}

/*
*********************************************************************************************************
*                                               TASK #2
*********************************************************************************************************
*/
void MyTask2(void *pdata)
{ 
    //INT8U  err;
	//char  s[20];
	
	pdata = pdata;                            /* Prevent compiler warning      */
    while (1) {
		
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
	//char  s[20];
	
	pdata = pdata;                            /* Prevent compiler warning      */

	for (i = 0; i < 10; i++) {         //��ʼ��10��APP_TCB�� ���β�������AppTCBPrioTbl[12]
        AppTCBInit(TASK_3_PRIO,  2);   //����ʱ��ƬΪ2
    }

	while (1) {
		AppSchedRR(TASK_3_PRIO);    //��ʱ��Ƭ��ת   ʱ��Ƭ��λ = MyTask3��̨���

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
    INT8U  err;
	char  msg[20];

	pdata = pdata;                            /* Prevent compiler warning      */
    strcpy(&msg[0], "Task4��Ϣ!");
	while (1) { 
		err = OSQPost(MsgQ, (void *)&msg[0]);    //������Ϣ

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
    INT8U  err;
	char  *msg;

	pdata = pdata;                            /* Prevent compiler warning      */
	PC_DispStr(10, 18, "д��MyTask5", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	while (1) { 
		msg = (char *)OSQPend(MsgQ, 0, &err);                         //������Ϣ
		PC_DispStr(70, 14, msg, DISP_FGND_YELLOW + DISP_BGND_BLUE);   //��ʾ���󵽵���Ϣ
		err = OSQFlush(MsgQ);                                         //��ϴ��Ϣ����
		
		OSMutexPend(RW, 0, &err);
		PC_DispStr(22, 16, "          ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);  //��������ڶ���
		
		//д�ļ�
		PC_DispStr(22, 18, "����д", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
		OSTimeDly(30);                       /* Wait 30 ticks                  */ 
		
		err = OSMutexPost(RW);
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
		sprintf(s, "%3d", OSCPUUsage);
		PC_DispStr(38, 22, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);      /* CPU usage                */
		sprintf(s, "%3d", OSCtxSwCtr);
		PC_DispStr(62, 22, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);      /* Context switch counter   */
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
	TaskUserData[id].MyTCBCyclesTot = tcb_copy.OSTCBCyclesTot;
}

void  DispTaskData (INT8U id)
{
	char  s[80];

	sprintf(s, "     %3d         %3d       %5d", TaskUserData[id].MyTCBCtxSwCtr, TaskUserData[id].MyTCBDly, TaskUserData[id].MyTCBPrio);
    PC_DispStr(13, (INT8U)(id + 10), s, DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);

	if (0 == TaskUserData[id].MyTCBStat) {
		if (TaskUserData[id].MyTCBDly <= 10) {
			PC_DispStr(55, (INT8U)(id + 10), "    Ready", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
		} else {
			PC_DispStr(55, (INT8U)(id + 10), "  Delayed", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
		}
	} else if (1 == TaskUserData[id].MyTCBStat) {    //�ȴ��ź����¼�
		PC_DispStr(55, (INT8U)(id + 10), " Wait_Sem", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	} else if (4 == TaskUserData[id].MyTCBStat) {    //�ȴ���Ϣ�¼�
		PC_DispStr(55, (INT8U)(id + 10), " Wait_Msg", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    } else if (8 == TaskUserData[id].MyTCBStat) {    //������ �ȴ��ָ��¼�
		PC_DispStr(55, (INT8U)(id + 10), "Suspended", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	} else if (16 == TaskUserData[id].MyTCBStat) {   //�ȴ������ź����¼�
		PC_DispStr(55, (INT8U)(id + 10), " Wait_Mtx", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	} else {
		PC_DispStr(55, (INT8U)(id + 10), "    Other", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	}
}

/*
*********************************************************************************************************
*                                           ��APP_TCB�йغ���         
*
*  1. InitAppTCBList() -- Initialize the free list of APP_TCBs
*  2. AppTCBInit()     -- ��ʼ��һ��APP_TCB�� ����������Ӧ��AppTCBPrioTbl[prio]
*  3. AppSchedRR()     -- ��˫��������ʵ����ת����
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

void  AppTCBInit (INT8U  prio, INT32U  slice)
{
    APP_TCB    *ptcb;

    ptcb = AppTCBFreeList;                                  /* Get a free TCB from the free TCB list    */
    if (ptcb != (APP_TCB *)0) {
        AppTCBFreeList    = ptcb->Next;                     /* Update pointer to free TCB list          */

        ptcb->TCBPrio     = prio;                           /* Load task priority into TCB              */
        ptcb->TCBStat     = OS_STAT_RDY;                    /* Task is ready to run                     */
        ptcb->TCBDly      = 0u;                             /* Task is not delayed                      */
		ptcb->Next        = (APP_TCB *)0;  //����ָ�����
        ptcb->Ts          = slice;         //����ʱ��Ƭ
        ptcb->Id         += 1000u;         //ÿ���»���TCB�ı�ʶ����λ�������
    }

	APPInsertListTail(&AppTCBPrioTbl[prio], ptcb);  //���»��ֵ�TCB�����ȼ������Ӧ����
}

//��ʵ��
//��ʱ��Ƭ��˫������AppTCBPrioTbl[prio]����ת����
void  AppSchedRR(INT8U  prio)
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
*                                           DUMMY HOOKS
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

void  OSTaskSwHook (void)
{
}