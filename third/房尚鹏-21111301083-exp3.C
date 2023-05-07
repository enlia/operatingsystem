/*
*********************************************************************************************************
*                                                uC/OS-II
*                                          The Real-Time Kernel
*
*                                               EXAMPLE #3
*********************************************************************************************************
*/
#define _CRT_SECURE_NO_WARNINGS

#include "includes.h"
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



/*保存被查询任务的相关信息  陆续完善  */
typedef struct {
	INT8U   MyTCBStat;
	INT8U   MyTCBPrio;
    INT32U  MyTCBDly;
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

OS_TCB  tcb_copy;                   //定义用来复制被查询任务的TCB

TASK_USER_DATA  TaskUserData[5];    //对5个用户任务定义结构体数组

OS_EVENT       *Semp;               //定义信号量
OS_EVENT       *MsgQ;               //定义消息队列
OS_EVENT       *Mutex;
OS_EVENT       *Empty;
OS_EVENT	   *Full;
OS_EVENT       *cnt;
void           *MsgQStr[20];        //定义消息指针数组

APP_QUEUE       Buffer;             //定义缓冲池

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
void  DispTaskData(INT8U id);
void  UpdateTaskData(INT8U id);

/*$PAGE*/
/*
*********************************************************************************************************
*                                                  MAIN
*********************************************************************************************************
*/

int main (void)
{
	INT8U err;

    PC_DispClrScr(DISP_BGND_BLACK);                        /* Clear the screen                         */

    OSInit();                                              /* Initialize uC/OS-II                      */
	
	
	
	
	APPInitQueue(&Buffer);
	/* ToDo: Create semaphores, mailboxes etc. here						       */
	Semp = OSSemCreate(0);                   //创建初值为0的信号量
	MsgQ = OSQCreate(&MsgQStr[0], 20);       //创建消息队列
	Mutex = OSMutexCreate(1, &err);
	Empty = OSSemCreate(128);
	Full = OSSemCreate(0);

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
    INT8U  err;
	char  s[40];
	INT16S     key;
	
    pdata = pdata;                                         /* Prevent compiler warning                 */

    /* Setup the display  */
	PC_DispStr( 0,  0, "                           实时多任务操作系统uC/OS-II                           ", DISP_FGND_WHITE + DISP_BGND_RED);
    PC_DispStr( 0,  1, " 版权声明:                                                                      ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  2, "  1.内核源码由Jean J. Labrosse提供                                              ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  3, "  2.x86-Win32平台移植由Werner Zimmermann提供                                    ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  4, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  5, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  6, "  CH3 进程同步问题                                                              ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  7, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  8, "    UserTask    OSTCBStat   OSTCBDly   OSTCBPrio          Status                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  9, "-------------  ----------  ---------  ----------    ------------                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
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
	PC_DispStr( 0, 20, "          211111301083   房尚鹏                                                 ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 21, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 22, "    总任务数  :          CPU利用率:      %                                      ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
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
		OSTimeDly(200);          //间隔4秒阻塞和恢复Task5
		err = OSTaskSuspend(14);
		OSTimeDly(200);
        err = OSTaskResume(14);
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
    INT8U  err, count = 0;
	char  s[40] = "";
	pdata = pdata;                            /* Prevent compiler warning      */
	PC_DispStr(10, 16, "生产者 MyTask1 产品已入池  ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    while (1) {
		OSSemPend(Empty,0,&err);
		OSMutexPend(Mutex, 0, &err);
		APPEnQueue(&Buffer,++cnt);count++;
		sprintf(s, "%3d", count);
		PC_DispStr(38, 16, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);
		err = OSMutexPost(Mutex);
		OSSemPost(Full);

        OSTimeDly(150);                       /* Wait 150 ticks                  */
    }
}
void MyTask2(void *pdata)
{ 
    INT8U  err, count = 0;
	char  s[40] = "";
	
	pdata = pdata;                            /* Prevent compiler warning      */
	PC_DispStr(10, 17, "生产者 MyTask2 产品已入池 ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	while (1) { 
		OSSemPend(Empty,0,&err);
		OSMutexPend(Mutex, 0, &err);
		APPEnQueue(&Buffer,++cnt);count++;
		sprintf(s, "%3d", count);
		PC_DispStr(38, 17, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);
		err = OSMutexPost(Mutex);
		OSSemPost(Full);
        OSTimeDly(120);                       /* Wait 120 ticks                  */
    }
}
void MyTask3(void *pdata)
{
	INT8U  err,count = 0;
	char  s[40] = "";
	pdata = pdata;                            /* Prevent compiler warning      */
	PC_DispStr(10, 18, "消费者 MyTask3 已消费  ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
	while (1) {
		OSSemPend(Full,0,&err);
		OSMutexPend(Mutex, 0, &err);
		APPDeQueue(&Buffer);count++;
		sprintf(s, "%3d", count);
		PC_DispStr(38, 18, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);
		err = OSMutexPost(Mutex);
		OSSemPost(Empty);

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
	char  *msg;

	pdata = pdata;                            /* Prevent compiler warning      */
    while (1) { 
        msg = (char *)OSQPend(MsgQ, 0, &err);    //请求消息
        PC_DispStr(70, 13, msg, DISP_FGND_YELLOW + DISP_BGND_BLUE);

		err = OSQFlush(MsgQ);                    //清洗消息队列

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
	char  msg[20];

	pdata = pdata;                            /* Prevent compiler warning      */
    strcpy(&msg[0], "任务5消息!");
	while (1) { 
        err = OSQPost(MsgQ, (void *)&msg[0]);

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
		sprintf(s, "%5d", OSTaskCtr);
		PC_DispStr(18, 22, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);      /* Total tasks      */
		sprintf(s, "%3d", OSCPUUsage);
		PC_DispStr(38, 22, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);      /* CPU usage        */

		PC_GetDateTime(s);
        PC_DispStr(18, 24, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);      /* System time      */

		PC_DispStr(70, 13, "          ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
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
        
		for (i = 0; i < 5; i++) {             //只打印5个用户任务
            DispTaskData(i);                                
        }

        OSTimeDly(35);                        //值过小更新频率太快  过大不易捕捉状态变化
    }
}


/*$PAGE*/
/*
*********************************************************************************************************
*                                      UPDATE & DISPLAY TASK RELATED DATA
*********************************************************************************************************
*/

void  UpdateTaskData (INT8U id)
{
    INT8U err;

	err = OSTaskQuery (id + 10, &tcb_copy);
	TaskUserData[id].MyTCBStat = tcb_copy.OSTCBStat;
	TaskUserData[id].MyTCBPrio = tcb_copy.OSTCBPrio;
	TaskUserData[id].MyTCBDly = tcb_copy.OSTCBDly;
}

void  DispTaskData (INT8U id)
{
	char  s[80];
	sprintf(s, "         %02x        %3d        %5d", TaskUserData[id].MyTCBStat, TaskUserData[id].MyTCBDly, TaskUserData[id].MyTCBPrio);
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
*                                           STATISTIC TASK HOOK
*********************************************************************************************************
*/
void  OSTaskStatHook (void)
{
    INT8U   i;
   
    for (i = 0; i < 5; i++) {             //统计任务仅更新用户任务数据  不打印
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
