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

/* 任务优先级  */
#define          TASK_START_PRIO     0      //起始任务具有最高优先级

#define          TASK_1_PRIO        10      //5个用户任务
#define          TASK_2_PRIO        11
#define          TASK_3_PRIO        12
#define          TASK_4_PRIO        13
#define          TASK_5_PRIO        14

#define          TASK_DISP_PRIO     25      //负责显示用户任务信息
#define          TASK_CLK_PRIO      30      //负责显示系统时间 任务数 CPU利用率


//保存被查询任务的相关信息
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


FP32    AppCPUUsage;     //第2种CPU利用率
INT32U  time_idle;

OS_TCB  tcb_copy;                   //用来复制被查询任务的TCB

TASK_USER_DATA  TaskUserData[5];    //存放5个用户任务的查询信息

APP_TCB         AppTCBTbl[128];     //全局APP_TCB数组  含128个块  仿真OSTCBTbl
APP_TCB        *AppTCBFreeList;     //空闲APP_TCB链头
APP_TCB        *AppTCBPrioTbl[64];  //对应64个优先级的指针数组  用来连接APP_TCB双向链表  区别于OSTCBPrioTbl

APP_QUEUE       Buffer;             //定义缓冲池

OS_EVENT       *SempFull;           //定义Full信号量--------资源1
OS_EVENT       *SempEmpty;          //定义Empty信号量-------资源2
OS_EVENT       *Mutex;              //定义互斥信号量--------资源3

//对本例，5个用户任务，3种资源      P0~P4对应MyTask1~MyTask5
int available[3];          //可利用资源向量
int allocation[5][3];      //已分配矩阵
int need[5][3];            //需求矩阵
INT8U finish[5];           //标记是否有足够资源  1 TRUE；0 FALSE

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

    InitAppTCBList();          //初始化所有APP_TCB块  构建空闲链
	
	time_idle  = 0;

	PC_ElapsedInit();

	/* ToDo: Create semaphores, mailboxes etc. here						       */
	SempFull = OSSemCreate(0);
	SempEmpty = OSSemCreate(APP_QUEUE_SIZE);
	Mutex = OSMutexCreate(1, &err);    //优先级天花板为1

	/* 以下初始化是为了死锁检测     */
	available[0] = 0;                //没有产品
	available[1] = APP_QUEUE_SIZE;   //池中有128个空位
	available[2] = 1;                //互斥锁目前可用
	OS_MemClr((INT8U *)&finish[0], sizeof(finish));    //清零finish向量

	APPInitQueue(&Buffer);     //初始化缓冲池

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
	PC_DispStr( 0,  0, "                           实时多任务操作系统uC/OS-II                           ", DISP_FGND_WHITE + DISP_BGND_RED);
    PC_DispStr( 0,  1, " 版权声明:                                                                      ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  2, "  1.内核源码由Jean J. Labrosse提供                                              ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  3, "  2.x86-Win32平台移植由Werner Zimmermann提供                                    ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  4, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  5, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  6, "  CH5 死锁                                                                      ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  7, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  8, "    UserTask   上台次数  OSTCBPrio    Status   上台间隔   APP_TCB块   当前轮到ID", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
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
    PC_DispStr( 0, 22, "    总任务数  :         每秒任务切换:           CPU利用率1:      %   2:      %  ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 23, "    内核版本  :                                                                 ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 24, "    当前时间  :                                       <--按‘ESC’键退出-->     ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);

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

	for (i = 0; i < 2; i++) {         //初始化2个APP_TCB块 依次插入链表AppTCBPrioTbl[10]
        AppTCBInit(TASK_1_PRIO, 1);   //1个时间片
    }

    while (1) {
		time1  = PC_ElapsedStop(1);
        PC_ElapsedStart(1);
		sprintf(s, "%5.2f", time1/ (FP32)1000000);
        PC_DispStr(47, 10, s, DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);  //显示上台时间间隔
		
		AppSchedRR(TASK_1_PRIO);    //按时间片轮转   时间片单位 = MyTask1上台间隔
		//AppSchedRR2(TASK_1_PRIO);    //替换为三级反馈队列

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
	
	for (i = 0; i < 3; i++) {         //初始化3个APP_TCB块 依次插入链表AppTCBPrioTbl[11]
        AppTCBInit(TASK_2_PRIO, 2);   //2个时间片
    }

	pdata = pdata;                            /* Prevent compiler warning      */
    while (1) {
		time2  = PC_ElapsedStop(2);
        PC_ElapsedStart(2);
		sprintf(s, "%5.2f", time2/ (FP32)1000000);
        PC_DispStr(47, 11, s, DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);  //显示上台时间间隔

		AppSchedRR(TASK_2_PRIO);    //按时间片轮转   时间片单位 = MyTask2上台间隔
		//AppSchedRR2(TASK_2_PRIO);      //替换为三级反馈队列

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

	for (i = 0; i < 5; i++) {         //初始化5个APP_TCB块 依次插入链表AppTCBPrioTbl[12]
        AppTCBInit(TASK_3_PRIO, 4);   //4个时间片
    }

	while (1) {
		time3  = PC_ElapsedStop(3);
        PC_ElapsedStart(3);
		sprintf(s, "%5.2f", time3/ (FP32)1000000);
        PC_DispStr(47, 12, s, DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);  //显示上台时间间隔

		AppSchedRR(TASK_3_PRIO);    //按时间片轮转   时间片单位 = MyTask3上台间隔
		
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
    PC_DispStr(10, 16, "生产者MyTask4  产品已入池", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);


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
	PC_DispStr(10, 17, "消费者MyTask5  已消费", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);


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

		PC_DispStr(70, 14, "          ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);    //每秒清除消息显示
		OSCtxSwCtr = 0;   //每秒清零总切换次数
		
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
		for (i = 0; i < 5; i++) {             //刷新5个用户任务
			DispTaskData(i);                                
        }

        OSTimeDly(35);                        //值过小刷新频率太快  过大不易捕捉状态变化
    }
}


/*$PAGE*/
/*
*********************************************************************************************************
*                                         与TASK_USER_DATA有关函数
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
	} else if (1 == TaskUserData[id].MyTCBStat) {    //等待信号量事件
		PC_DispStr(35, (INT8U)(id + 10), " Wait_Sem", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	} else if (4 == TaskUserData[id].MyTCBStat) {    //等待消息事件
		PC_DispStr(35, (INT8U)(id + 10), " Wait_Msg", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    } else if (8 == TaskUserData[id].MyTCBStat) {    //被挂起 等待恢复事件
		PC_DispStr(35, (INT8U)(id + 10), "Suspended", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	} else if (16 == TaskUserData[id].MyTCBStat) {   //等待互斥信号量事件
		PC_DispStr(35, (INT8U)(id + 10), " Wait_Mtx", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	} else {
		PC_DispStr(35, (INT8U)(id + 10), "    Other", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	}

	//sprintf(s, "%6d", TaskUserData[id].MyTCBCycles);
    //PC_DispStr(47, (INT8U)(id + 10), s, DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
}

/*
*********************************************************************************************************
*                                           与APP_TCB有关函数         
*
*  1. InitAppTCBList() -- Initialize the free list of APP_TCBs
*  2. AppTCBInit()     -- 初始化一个APP_TCB块 并将其插入对应的AppTCBPrioTbl[prio]
*  3. AppSchedRR()     -- 在双向链表上实现轮转调度
*  4. AppSchedRR2()    -- 在轮转调度基础上实现多级反馈队列
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
    for (ix = 0u; ix < 128; ix++) {         //初始化128个APP_TCB块
        ix_next       = ix + 1u;
        ptcb1         = &AppTCBTbl[ix];
        ptcb2         = &AppTCBTbl[ix_next];
        ptcb1->Next   = ptcb2;
        ptcb1->Id     = ix;              //数组下标是组成ID的一部分
    }
    ptcb1       = &AppTCBTbl[ix];
    ptcb1->Next = (APP_TCB *)0;      //最后一个块

    AppTCBFreeList = &AppTCBTbl[0];  //空闲APP_TCB链头
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
		ptcb->Next        = (APP_TCB *)0;        //后向指针清空
        
		ptcb->TQ          = time_quanta;   //设置时间片
		ptcb->TQctr       = time_quanta;   //设置当前剩余时间片
        
		ptcb->Id         += 1000u;         //每个新划分TCB的标识由四位数字组成
     
		APPInsertListTail(&AppTCBPrioTbl[prio], ptcb);  //将新划分的TCB按优先级插入对应链表
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

//待实现
void  AppSchedRR2 (INT8U  prio)
{
	


}

/*
*********************************************************************************************************
*                                                死锁检测
*********************************************************************************************************
*/

//判断一个进程需求资源是否全为零   返回值：1全为零；0不全为零
int isAllZero(int idx)
{
	int j;
	for(j = 0; j < 3; j++) {
		if(need[idx][j] != 0)
			return 0;
	}
	return 1;
}


//待实现
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
   
    for (i = 0; i < 5; i++) {             //仅更新用户任务数据  不打印
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

