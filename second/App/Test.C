/*
*********************************************************************************************************
*                                                uC/OS-II
*                                          The Real-Time Kernel
*
*					WIN32 PORT & LINUX PORT
*                          (c) Copyright 2004, Werner.Zimmermann@fht-esslingen.de
*                                           All Rights Reserved
*
*                                               EXAMPLE #2
*********************************************************************************************************
*/
#define _CRT_SECURE_NO_WARNINGS
#include "includes.h"

/*
*********************************************************************************************************
*                                              CONSTANTS
*********************************************************************************************************
*/

#define          TASK_STK_SIZE     512                /* Size of each task's stacks (# of WORDs) */

#define          TASK_START_PRIO     9                 /* Tasks Priorities */
#define          TASK_CLK_PRIO      10
#define          TASK_1_PRIO        11
#define          TASK_2_PRIO        12
#define          TASK_3_PRIO        13
#define          TASK_4_PRIO        14
#define          TASK_5_PRIO        15

//#define          MSG_QUEUE_SIZE     20                /* Size of message queue used */


/*
*********************************************************************************************************
*                                              VARIABLES
*********************************************************************************************************
*/

OS_STK          TaskStartStk[TASK_STK_SIZE];          /* Startup     task stack                        */
OS_STK          TaskClkStk[TASK_STK_SIZE];            /* Clock       task stack                        */
OS_STK          Task1Stk[TASK_STK_SIZE];              /* MyTask1    task stack                         */
OS_STK          Task2Stk[TASK_STK_SIZE];              /* MyTask2    task stack                         */
OS_STK          Task3Stk[TASK_STK_SIZE];              /* MyTask3    task stack                         */
OS_STK          Task4Stk[TASK_STK_SIZE];              /* MyTask4    task stack                         */
OS_STK          Task5Stk[TASK_STK_SIZE];              /* MyTask5    task stack                         */

OS_TCB  tcb_copy;                                     /* Copy the TCB data of the task to be queried    */

//OS_EVENT       *MsgQueue;                             /* Message queue pointer                        */
//void           *MsgQueueTbl[20];                      /* Storage for messages                         */

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

void  TaskStart(void *data);
void  TaskClock(void *data);
void  MyTask1(void *data);
void  MyTask2(void *data);
void  MyTask3(void *data);
void  MyTask4(void *data);
void  MyTask5(void *data);

void  DispTaskStat(INT8U id);

/*$PAGE*/
/*
*********************************************************************************************************
*                                                  MAIN
*********************************************************************************************************
*/

int main (void)
{
    PC_DispClrScr(DISP_BGND_BLACK);                        /* Clear the screen                         */

    OSInit();                                              /* Initialize uC/OS-II                      */

	/* ToDo: Create semaphores, mailboxes etc. here						       */

    OSTaskCreate(TaskStart, (void *)0, &TaskStartStk[TASK_STK_SIZE - 1], TASK_START_PRIO);	/* Create the startup task */
	OSTaskCreate(TaskClock, (void *)0, &TaskClkStk[TASK_STK_SIZE - 1], TASK_CLK_PRIO);      /* Create the clock task */        
    
	/* Create user tasks */
	OSTaskCreate(MyTask1, (void *)0, &Task1Stk[TASK_STK_SIZE - 1], TASK_1_PRIO);
	OSTaskCreate(MyTask2, (void *)0, &Task2Stk[TASK_STK_SIZE - 1], TASK_2_PRIO);
	OSTaskCreate(MyTask3, (void *)0, &Task3Stk[TASK_STK_SIZE - 1], TASK_3_PRIO);
	OSTaskCreate(MyTask4, (void *)0, &Task4Stk[TASK_STK_SIZE - 1], TASK_4_PRIO);
	OSTaskCreate(MyTask5, (void *)0, &Task5Stk[TASK_STK_SIZE - 1], TASK_5_PRIO);
    
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
    char  s[40];
	INT16S     key;

    pdata = pdata;                                         /* Prevent compiler warning                 */

    /* Setup the display  */
	PC_DispStr( 0,  0, "                           实时多任务操作系统uC/OS-II                           ", DISP_FGND_WHITE + DISP_BGND_RED);
    PC_DispStr( 0,  1, " 版权申明:                                                                      ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  2, "  1.内核源码由Jean J. Labrosse提供                                              ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  3, "  2.x86-Win32平台移植由Werner Zimmermann提供                                    ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  4, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  5, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  6, "  CH2 任务状态转换                                                              ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  7, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  8, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0,  9, "   Task Name    OSTCBStat            OSTCBDly                Status             ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 10, "-------------  ----------  ----------------------------  ------------           ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 11, "   TaskStart                                                                    ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 12, "   TaskClock                                                                    ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 13, "     MyTask1                                                                    ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 14, "     MyTask2                                                                    ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 15, "     MyTask3                                                                    ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 16, "     MyTask4                                                                    ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 17, "     MyTask5                                                                    ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 18, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 19, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 20, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 21, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 22, "    总任务数  :          CPU占用率:      %                                      ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 23, "    内核版本  :                                                                 ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr( 0, 24, "    当前时间  :                                       <--按‘ESC’键退出-->     ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);

    OSStatInit();                                          /* Initialize uC/OS-II's statistics         */

	/* Display the version of OS */
	sprintf(s, "uC/OS-II V%1d.%02d", OSVersion() / 100, OSVersion() % 100);
    PC_DispStr(18, 23, s, DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);

    //MsgQueue = OSQCreate(&MsgQueueTbl[0], MSG_QUEUE_SIZE); /* Create a message queue                   */
	

    for (;;) {
        /* Update the display of total tasks and CPU usage */
		sprintf(s, "%5d", OSTaskCtr);
		PC_DispStr(18, 22, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);
		sprintf(s, "%3d", OSCPUUsage);
		PC_DispStr(38, 22, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);

        if (PC_GetKey(&key)) {                             /* See if key has been pressed              */
            if (key == 0x1B) {                             /* Yes, see if it's the ESCAPE key          */
                exit(0);                                   /* Yes, return to DOS                       */
            }
        }

        /* ToDo: Don't forget to call the uCOS scheduler with OSTimeDly etc., to give other tasks a chance to run */
		OSTimeDly(100);                                    /* Wait 100 ticks                           */
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
    pdata = pdata;                            /* Prevent compiler warning      */
    while (1) { 
        

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
    pdata = pdata;                            /* Prevent compiler warning      */
    while (1) { 
        

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
    pdata = pdata;                            /* Prevent compiler warning      */
    while (1) { 


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
    pdata = pdata;                            /* Prevent compiler warning      */
    while (1) { 
        

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

    pdata = pdata;
    for (;;) {
        PC_GetDateTime(s);
        PC_DispStr(18, 24, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);

        OSTimeDly(OS_TICKS_PER_SEC);
    }
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                      DISPLAY TASK RELATED STATISTICS
*********************************************************************************************************
*/

void  DispTaskStat (INT8U id)
{
    INT8U err;
	char  s[80];

	err = OSTaskQuery (id+9, &tcb_copy);
    sprintf(s, "         %02x             %5d", tcb_copy.OSTCBStat, tcb_copy.OSTCBDly);
    PC_DispStr(13, (INT8U) (id+11), s, DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
}

/*
*********************************************************************************************************
*                                           STATISTIC TASK HOOK
*********************************************************************************************************
*/
void  OSTaskStatHook (void)
{
    INT8U   i;
   
    for (i = 0; i < 7; i++) {                               /* Display task user data                  */
        DispTaskStat(i);                                
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
