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
	} 
	
	temp = *head;
	while (temp->Next != NULL) {  //找到双向链中最后一个结点
		temp = temp->Next;
	};
	temp->Next = ptcb;
	ptcb->Prev = temp;
	return 1;
}




//待实现
//在链表头部插入结点
//返回值： 1多于一个结点   0只有一个结点
//int  APPInsertListHead (APP_TCB **head, APP_TCB *ptcb)
//{
//   
//
//
//    return 1;
//}



//获取双向链表长度（结点数）
int  APPGetListLen (APP_TCB **head)
{
	int      count;
	APP_TCB *temp;

	if (*head == NULL)
		return 0;

	temp  = *head;
	count = 1;
	while (temp->Next != NULL) {
		temp = temp->Next;
		count++;
	}
	return count;
}

