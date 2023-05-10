#define _CRT_SECURE_NO_WARNINGS 
#include<stdio.h>
#include<stdlib.h>

#define False 0
#define True 1
#define MaxLen 100

/*ʹ�õ������ݽṹ*/
char Name[MaxLen] = { 0 };				//��Դ���ƣ����п���
int Max[MaxLen][MaxLen];				//����������
int Allocation[MaxLen][MaxLen];			//ϵͳ�ѷ������
int Need[MaxLen][MaxLen];				//����Ҫ����Դ
int Available[MaxLen];					//��ǰ����Щ��Դ
int Request[MaxLen];					//������Դ
int Work[MaxLen];						//ϵͳ���ṩ�����̼����������������Դ������̬��ֵAvailable
int Finish[MaxLen];						//ϵͳ�Ƿ����㹻��Դ��������̣���ֵfalse.
int Security[MaxLen];					//��ȫ����

int process = MaxLen;					//���������
int resourse = MaxLen;					//��Դ�����


/*��ʼ�����ݣ��������ݡ���Դ���ࡢ��������Դavailable,�����̶���Դ�������������Max���������Է�������Allocation*/
void Init() {
	int flag;					//
	int temp[MaxLen] = { 0 };			//ͳ���Է������Դ
	//��������ʼ����Դ��
	printf("ϵͳ�ж����������Դ:>");
	scanf("%d", &resourse);
	for (int i = 0; i < resourse; i++){
		printf("��%d����Դ������:>", i);
		//fflush(stdin);					//������뻺����
		getchar();
		scanf("%c", &Name[i]);
		printf("��Դ%c�ĳ�ʼ����Ϊ:>", Name[i]);
		scanf("%d", &Available[i]);
	}

	//��������������
	printf("����������:>");
	scanf("%d", &process);
	printf("����������̵�����������Max��ֵ:>\n");
	printf("\t");
	for (int i = 0; i < resourse; i++) {//���ÿ����Դ������
		printf("%c\t", Name[i]);
	}
	do {
		flag = False;
		for (int i = 0; i < process; i++) {
			printf("\n%d\t", i);
			for (int j = 0; j < resourse; j++) {
				scanf("%d", &Max[i][j]);
				if (Max[i][j] > Available[j]) {	//���������������ϵͳӵ�е��������
					flag = True;
				}
				if (flag) {
					break;
				}
			}
			if (flag) {
				printf("��Դ���������������ϵͳ�е���Դ��,��������\n");
				break;
			}
		}
	} while (flag);

	//��������ø��������Ѿ���������Դ����	--Allocation
	do {
		flag = False;
		printf("����������Ѿ��������Դ�� --Allocation:>\n");
		for (int i = 0; i < process; i++) {
			for (int j = 0; j < resourse; j++) {
				scanf("%d", &Allocation[i][j]);
				if (Allocation[i][j] > Max[i][j]) {	//�Ի�õ���Դ�������˱���������������
					flag = True;	//�Ȼ�ֱ������ȥ
					break;
				}
				Need[i][j] = Max[i][j] - Allocation[i][j];	//�����������
				temp[j] += Allocation[i][j];	//ͳ���Ѿ���������̵���Դ��Ŀ
			}
			if (flag) {
				printf("������Դ���������������,�����·���\n");
				break;
			}
		}
	} while (flag);

	//����ϵͳ�����е���Դ��Ŀ
	for (int i = 0; i < resourse; i++) {
		Available[i] = Available[i] - temp[i];
	}

}

/*չʾ*/
void Show() {
	printf("**********************************************************\n");
	printf("Available:>\n");
	for (int i = 0; i < resourse; i++) {
		printf("%c\t", Name[i]);
	}
	printf("\n");
	for (int i = 0; i < resourse; i++) {
		printf("%d\t", Available[i]);
	}
	printf("\n");
	printf("ϵͳ��ǰ��Դ�������:>\n");
	printf("Name\tMax\tAllocation\tNeed\n");
	for (int i = 0; i < process; i++) {
		printf("%c\t", Name[i]);				//Name
		for (int j = 0; j < resourse; j++) {	//Max
			printf("%d\t", Max[i][j]);
		}
		printf("\t\t");
		for (int j = 0; j < resourse; j++) {	//Allocation
			printf("%d\t", Allocation[i][j]);
		}
		printf("\t\t");
		for (int j = 0; j < resourse; j++) {	//Need
			printf("%d\t", Need[i][j]);
		}
		printf("\n");
	}
}

/*������Դ*/
int Test(int index_process) {
	for (int i = 0; i < resourse; i++) {
		Available[i] -= Request[i];
		Allocation[index_process][i] += Request[i];
		Need[index_process][i] -= Request[i];
	}
	return True;
}

/*�ָ���������Դ֮ǰ*/
int Retest(int index_process) {
	for (int i = 0; i < resourse; i++) {
		Available[i] += Request[i];
		Allocation[index_process][i] -= Request[i];
		Need[index_process][i] += Request[i];
	}
	return True;
}

int Safe() {
	int apply = 0;	//�ж϶��ٸ���Դ��ͨ��
	int cnt = 0;	//��ȫ���м���
	//ͨ��work������
	for (int i = 0; i < resourse; i++) {
		Work[i] = Available[i];
	}
	for (int i = 0; i < process; i++) {	//��ʼ��Finish
		Finish[i] = False;
	}
	for (int i = 0; i < process; i++) {
		for (int j = 0; j < resourse; j++) {
			if (Finish[i] == False && Need[i][j] < Work[j]) {
				apply++;
			}
		}
		if (apply == resourse) {	//֪��ÿ����Դ������������Ϸ�(С���������е�)
			for (int k = 0; k < resourse; k++) {
				Work[k] += Allocation[i][k];	//�������ڵ��е���Դ
			}
			Finish[i] = True;	//������ok��
			Security[cnt++] = i;	//���밲ȫ����
			i = -1;	//�ٴ�ͷ��ʼ��
		}
	}
	for (int i = 0; i < process; i++) {
		if (Finish[i] == False) {
			printf("ϵͳ����ȫ\n");
			return False;
		}
	}
	printf("ϵͳ��ȫ\n���ڰ�ȫ����:>");
	for (int i = 0; i < process; i++) {
		printf("%c\t", Name[Security[i]]);
	}
	printf("\n");
	return True;
}

/*�����м��㷨��������Դ	*/
void Bank() {
	int flag = True;	//�ж����м��㷨�Ƿ��ܽ�����һ��
	int index_process;	//���������Դ�Ľ���
	printf("�������������Դ�Ľ��̺�(0-%d)\n", process - 1);
	scanf("%d", &index_process);

	//����������Ҫ�������Դ��
	printf("������%d����Ҫ�������Դ��:>\n", index_process);
	for (int i = 0; i < resourse; i++) {
		printf("%c:>", Name[i]);	//��ǰ�Ǹ���Դ
		scanf("%d", &Request[i]);	//��ǰ��Դ�������
	}

	for (int i = 0; i < resourse; i++) {
		if (Request[i] > Need[index_process][i]) {	//�������Դ���ܶ��໹��Ҫ����Դ
			printf("����%d>\n");
			flag = False;
			break;
		}
		else {	//��ʱ���������Ǻ����
			if (Request[i] > Available[i]) {
				printf("����%d������Դ����ϵͳӵ�е���Դ\n");
				flag = False;
				break;
			}
		}
	}

	//���ݺ���:�����ڱ�����Ҫ����Ŀ,ҲС��ϵͳӵ�е���Ŀ
	if (flag) {
		Test(index_process);	//�����ŷ���
		Show();					//��ʾһ��
		if (!Safe()) {			//�������䲻��ȫ,�ָ�����
			Retest(index_process);	//�ָ�
			Show();
		}
	}

	return;
}


int main() {
	char choice;
	printf("****************************************************\n");
	printf("				���м��㷨							\n");
	printf("				������								\n");
	printf("****************************************************\n");
	Init();	//��ʼ������
	Show();
	if (!Safe()) {
		exit(-1);
	}
	
	do {
		printf("************************************************\n");
		printf("			a:�������		b:�˳�				\n");
		printf("************************************************\n");
		printf("choice:>");
		fflush(stdin);
		scanf("%c", &choice);
		switch (choice) {
		case 'a':
			Bank();
			break;
		case 'b':
			exit(0);
		default:
			printf("ѡ�����\n");
		}
	} while (choice);
	return 0;
}