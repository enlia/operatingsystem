/*
*********************************************************************************************************
*                                                uC/OS-II
*                                          The Real-Time Kernel
*
* 实现双向链表基本操作
*
* File : app_linked_list.c
* By   : Ahnu OS Team
*********************************************************************************************************
*/

#include <stdlib.h>
#include "app_linked_list.h"


//在链表尾部插入结点
//返回值： 1多于一个结点   0只有一个结点
int  APPInsertListTail (APP_TCB **head, APP_TCB *ptcb)
{
	APP_TCB *temp;

	if(*head == NULL){
		*head = ptcb;
		return 0;
	} else {
		temp = *head;
		while (temp->Next != NULL) {  //找到双向链中最后一个结点
			temp = temp->Next;
		};
		temp->Next = ptcb;
		ptcb->Prev = temp;
		return 1;
	}
}



//待实现
//在链表头部插入结点
//返回值： 1多于一个结点   0只有一个结点
int  APPInsertListHead (APP_TCB **head, APP_TCB *ptcb)
{
   


    return 1;
}





