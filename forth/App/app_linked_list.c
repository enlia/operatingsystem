/*
*********************************************************************************************************
*                                                uC/OS-II
*                                          The Real-Time Kernel
*
* ʵ��˫�������������
*
* File : app_linked_list.c
* By   : Ahnu OS Team
*********************************************************************************************************
*/

#include <stdlib.h>
#include "app_linked_list.h"


//������β��������
//����ֵ�� 1����һ�����   0ֻ��һ�����
int  APPInsertListTail (APP_TCB **head, APP_TCB *ptcb)
{
	APP_TCB *temp;

	if(*head == NULL){
		*head = ptcb;
		return 0;
	} else {
		temp = *head;
		while (temp->Next != NULL) {  //�ҵ�˫���������һ�����
			temp = temp->Next;
		};
		temp->Next = ptcb;
		ptcb->Prev = temp;
		return 1;
	}
}



//��ʵ��
//������ͷ��������
//����ֵ�� 1����һ�����   0ֻ��һ�����
int  APPInsertListHead (APP_TCB **head, APP_TCB *ptcb)
{
   


    return 1;
}





