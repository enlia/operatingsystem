#define _CRT_SECURE_NO_WARNINGS 
#include<stdio.h>
#include<stdlib.h>

#define False 0
#define True 1
#define MaxLen 100

/*使用到的数据结构*/
char Name[MaxLen] = { 0 };				//资源名称，可有可无
int Max[MaxLen][MaxLen];				//最大需求矩阵
int Allocation[MaxLen][MaxLen];			//系统已分配矩阵
int Need[MaxLen][MaxLen];				//还需要的资源
int Available[MaxLen];					//当前有那些资源
int Request[MaxLen];					//请求资源
int Work[MaxLen];						//系统可提供给进程继续运行所需各类资源数，初态赋值Available
int Finish[MaxLen];						//系统是否有足够资源分配给进程，初值false.
int Security[MaxLen];					//安全队列

int process = MaxLen;					//进程最大数
int resourse = MaxLen;					//资源最大数


/*初始化数据：进程数据、资源种类、可利用资源available,各进程对资源的最大需求数量Max、各进程以分配数量Allocation*/
void Init() {
	int flag;					//
	int temp[MaxLen] = { 0 };			//统计以分配的资源
	//接下来初始化资源数
	printf("系统有多少类可用资源:>");
	scanf("%d", &resourse);
	for (int i = 0; i < resourse; i++){
		printf("第%d个资源的名称:>", i);
		//fflush(stdin);					//清空输入缓冲区
		getchar();
		scanf("%c", &Name[i]);
		printf("资源%c的初始个数为:>", Name[i]);
		scanf("%d", &Available[i]);
	}

	//输入最大需求矩阵
	printf("最大进程数量:>");
	scanf("%d", &process);
	printf("请输入各进程的最大需求矩阵Max的值:>\n");
	printf("\t");
	for (int i = 0; i < resourse; i++) {//输出每个资源的名字
		printf("%c\t", Name[i]);
	}
	do {
		flag = False;
		for (int i = 0; i < process; i++) {
			printf("\n%d\t", i);
			for (int j = 0; j < resourse; j++) {
				scanf("%d", &Max[i][j]);
				if (Max[i][j] > Available[j]) {	//请求的数量超过了系统拥有的最大数量
					flag = True;
				}
				if (flag) {
					break;
				}
			}
			if (flag) {
				printf("资源最大需求量超过了系统中的资源数,重新输入\n");
				break;
			}
		}
	} while (flag);

	//接下来获得各个进程已经分配数资源数量	--Allocation
	do {
		flag = False;
		printf("输入各进程已经分配的资源量 --Allocation:>\n");
		for (int i = 0; i < process; i++) {
			for (int j = 0; j < resourse; j++) {
				scanf("%d", &Allocation[i][j]);
				if (Allocation[i][j] > Max[i][j]) {	//以获得的资源量超过了本来需求的最大数量
					flag = True;	//等会直接跳出去
					break;
				}
				Need[i][j] = Max[i][j] - Allocation[i][j];	//计算需求矩阵
				temp[j] += Allocation[i][j];	//统计已经分配给进程的资源数目
			}
			if (flag) {
				printf("分配资源数量大于最大数量,请重新分配\n");
				break;
			}
		}
	} while (flag);

	//更新系统中现有的资源数目
	for (int i = 0; i < resourse; i++) {
		Available[i] = Available[i] - temp[i];
	}

}

/*展示*/
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
	printf("系统当前资源分配情况:>\n");
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

/*分配资源*/
int Test(int index_process) {
	for (int i = 0; i < resourse; i++) {
		Available[i] -= Request[i];
		Allocation[index_process][i] += Request[i];
		Need[index_process][i] -= Request[i];
	}
	return True;
}

/*恢复到分配资源之前*/
int Retest(int index_process) {
	for (int i = 0; i < resourse; i++) {
		Available[i] += Request[i];
		Allocation[index_process][i] -= Request[i];
		Need[index_process][i] += Request[i];
	}
	return True;
}

int Safe() {
	int apply = 0;	//判断多少个资源能通过
	int cnt = 0;	//安全队列计数
	//通过work来计算
	for (int i = 0; i < resourse; i++) {
		Work[i] = Available[i];
	}
	for (int i = 0; i < process; i++) {	//初始化Finish
		Finish[i] = False;
	}
	for (int i = 0; i < process; i++) {
		for (int j = 0; j < resourse; j++) {
			if (Finish[i] == False && Need[i][j] < Work[j]) {
				apply++;
			}
		}
		if (apply == resourse) {	//知道每类资源申请的数量都合法(小于现在已有的)
			for (int k = 0; k < resourse; k++) {
				Work[k] += Allocation[i][k];	//更新现在的有的资源
			}
			Finish[i] = True;	//这玩意ok了
			Security[cnt++] = i;	//加入安全队列
			i = -1;	//再从头开始找
		}
	}
	for (int i = 0; i < process; i++) {
		if (Finish[i] == False) {
			printf("系统不安全\n");
			return False;
		}
	}
	printf("系统安全\n存在安全序列:>");
	for (int i = 0; i < process; i++) {
		printf("%c\t", Name[Security[i]]);
	}
	printf("\n");
	return True;
}

/*用银行家算法对申请资源	*/
void Bank() {
	int flag = True;	//判断银行家算法是否能进入下一步
	int index_process;	//请求分配资源的进程
	printf("输入请求分配资源的进程号(0-%d)\n", process - 1);
	scanf("%d", &index_process);

	//接下来输入要申请的资源数
	printf("请输入%d进程要申请的资源数:>\n", index_process);
	for (int i = 0; i < resourse; i++) {
		printf("%c:>", Name[i]);	//当前那个资源
		scanf("%d", &Request[i]);	//当前资源申请多少
	}

	for (int i = 0; i < resourse; i++) {
		if (Request[i] > Need[index_process][i]) {	//申请的资源不能多余还需要的资源
			printf("进程%d>\n");
			flag = False;
			break;
		}
		else {	//此时申请数量是合理的
			if (Request[i] > Available[i]) {
				printf("进程%d申请资源大于系统拥有的资源\n");
				flag = False;
				break;
			}
		}
	}

	//数据合理:不大于本来需要的数目,也小于系统拥有的数目
	if (flag) {
		Test(index_process);	//先试着分配
		Show();					//显示一下
		if (!Safe()) {			//这样分配不安全,恢复数据
			Retest(index_process);	//恢复
			Show();
		}
	}

	return;
}


int main() {
	char choice;
	printf("****************************************************\n");
	printf("				银行家算法							\n");
	printf("				房尚鹏								\n");
	printf("****************************************************\n");
	Init();	//初始化数据
	Show();
	if (!Safe()) {
		exit(-1);
	}
	
	do {
		printf("************************************************\n");
		printf("			a:请求分配		b:退出				\n");
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
			printf("选择错误\n");
		}
	} while (choice);
	return 0;
}