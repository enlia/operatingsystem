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

OS_TCB  tcb_copy;                   //用来复制被查询任务的TCB

TASK_USER_DATA  TaskUserData[5];    //存放5个用户任务的查询信息

APP_TCB         AppTCBTbl[128];     //全局APP_TCB数组  含128个块  仿真OSTCBTbl
APP_TCB        *AppTCBFreeList;     //空闲APP_TCB链头
APP_TCB        *AppTCBPrioTbl[64];  //对应64个优先级的指针数组  用来连接APP_TCB双向链表  区别于OSTCBPrioTbl


OS_EVENT       *RW;                 //读写锁 对文件的互斥访问
OS_EVENT       *Mutex;              //对readcount变量的互斥访问
int readcount;                      //当前读者计数

OS_EVENT       *MsgQ;               //定义消息队列
void           *MsgQStr[20];        //定义消息指针数组


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

    InitAppTCBList();          //初始化所有APP_TCB块  构建空闲链

	/* ToDo: Create semaphores, mailboxes etc. here						       */
	RW = OSMutexCreate(1, &err);             //创建读写锁
	Mutex = OSMutexCreate(2, &err);          //创建互斥信号量
	readcount = 0;
	MsgQ = OSQCreate(&MsgQStr[0], 20);       //创建消息队列

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
	PC_DispStr( 0,  0, "                           实时多任务操作系统uC/OS-II                           ", DISP_FGND_WHITE + DISP_BGND_RED);
    PC_DispStr( 0,  1, " 版权声明:                                                                      ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  2, "  1.内核源码由Jean J. Labrosse提供                                              ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  3, "  2.x86-Win32平台移植由Werner Zimmermann提供                                    ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  4, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  5, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  6, "  CH4 调度基础                                                                  ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  7, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  8, "    UserTask    上台次数    OSTCBDly   OSTCBPrio          Status                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
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
    PC_DispStr( 0, 22, "    总任务数  :          CPU利用率:      %    每秒任务切换:                     ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
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
    INT8U  err;
	char  s[20];
	
	pdata = pdata;                            /* Prevent compiler warning      */
	PC_DispStr(10, 16, "读者MyTask1", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    while (1) {
		OSMutexPend(Mutex, 0, &err);
		if (readcount == 0)
			OSMutexPend(RW, 0, &err);
		readcount++;
		err = OSMutexPost(Mutex);
		PC_DispStr(22, 18, "          ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);   //清除“正在写”
   
		//读文件
		PC_DispStr(22, 16, "正在读", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
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

	for (i = 0; i < 10; i++) {         //初始化10个APP_TCB块 依次插入链表AppTCBPrioTbl[12]
        AppTCBInit(TASK_3_PRIO,  2);   //设置时间片为2
    }

	while (1) {
		AppSchedRR(TASK_3_PRIO);    //按时间片轮转   时间片单位 = MyTask3上台间隔

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
    strcpy(&msg[0], "Task4消息!");
	while (1) { 
		err = OSQPost(MsgQ, (void *)&msg[0]);    //发送消息

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
	PC_DispStr(10, 18, "写者MyTask5", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	while (1) { 
		msg = (char *)OSQPend(MsgQ, 0, &err);                         //请求消息
		PC_DispStr(70, 14, msg, DISP_FGND_YELLOW + DISP_BGND_BLUE);   //显示请求到的消息
		err = OSQFlush(MsgQ);                                         //清洗消息队列
		
		OSMutexPend(RW, 0, &err);
		PC_DispStr(22, 16, "          ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);  //清除“正在读”
		
		//写文件
		PC_DispStr(22, 18, "正在写", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
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
	} else if (1 == TaskUserData[id].MyTCBStat) {    //等待信号量事件
		PC_DispStr(55, (INT8U)(id + 10), " Wait_Sem", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	} else if (4 == TaskUserData[id].MyTCBStat) {    //等待消息事件
		PC_DispStr(55, (INT8U)(id + 10), " Wait_Msg", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    } else if (8 == TaskUserData[id].MyTCBStat) {    //被挂起 等待恢复事件
		PC_DispStr(55, (INT8U)(id + 10), "Suspended", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	} else if (16 == TaskUserData[id].MyTCBStat) {   //等待互斥信号量事件
		PC_DispStr(55, (INT8U)(id + 10), " Wait_Mtx", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	} else {
		PC_DispStr(55, (INT8U)(id + 10), "    Other", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	}
}

/*
*********************************************************************************************************
*                                           与APP_TCB有关函数         
*
*  1. InitAppTCBList() -- Initialize the free list of APP_TCBs
*  2. AppTCBInit()     -- 初始化一个APP_TCB块 并将其插入对应的AppTCBPrioTbl[prio]
*  3. AppSchedRR()     -- 在双向链表上实现轮转调度
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

void  AppTCBInit (INT8U  prio, INT32U  slice)
{
    APP_TCB    *ptcb;

    ptcb = AppTCBFreeList;                                  /* Get a free TCB from the free TCB list    */
    if (ptcb != (APP_TCB *)0) {
        AppTCBFreeList    = ptcb->Next;                     /* Update pointer to free TCB list          */

        ptcb->TCBPrio     = prio;                           /* Load task priority into TCB              */
        ptcb->TCBStat     = OS_STAT_RDY;                    /* Task is ready to run                     */
        ptcb->TCBDly      = 0u;                             /* Task is not delayed                      */
		ptcb->Next        = (APP_TCB *)0;  //后向指针清空
        ptcb->Ts          = slice;         //设置时间片
        ptcb->Id         += 1000u;         //每个新划分TCB的标识由四位数字组成
    }

	APPInsertListTail(&AppTCBPrioTbl[prio], ptcb);  //将新划分的TCB按优先级插入对应链表
}

//待实现
//按时间片在双向链表AppTCBPrioTbl[prio]上轮转调度
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
   
    for (i = 0; i < 5; i++) {             //仅更新用户任务数据  不打印
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
