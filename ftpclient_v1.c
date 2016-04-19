

/*FTP Client*/
 
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
 
/*for getting file size using stat()*/
#include<sys/stat.h>
 
/*for sendfile()*/
#include<sys/sendfile.h>
 
/*for O_RDONLY*/
#include<fcntl.h>


#define cmdPort 21
struct sockaddr_in cmdAddr, dataAddr;
int cmdSock, dataSock;
char mode[4];
char buf[BUFSIZ+1];
char ip[16];
struct sockaddr_storage serverStorage;
socklen_t addr_size = sizeof serverStorage;
int listenSock;
int status;

void replylogcode(int code)
{
	switch(code){
		case 200:
			printf("Command okay");
			break;
		case 500:
			printf("Syntax error, command unrecognized.");
			printf("This may include errors such as command line too long.");
			break;
		case 501:
			printf("Syntax error in parameters or arguments.");
			break;
		case 202:
			printf("Command not implemented, superfluous at this site.");
			break;
		case 502:
			printf("Command not implemented.");
			break;
		case 503:
			printf("Bad sequence of commands.");
			break;
		case 530:
			printf("Not logged in.");
			break;
	}
	printf("\n");
}

void ModePassive()
{
	//Passive mode
	char s1[5], s2[5];
	int x1, x2, dataPort;
	char* temp;
	
	memset(buf, 0, sizeof buf);
	sprintf(buf,"PASV\r\n");
	send(cmdSock, buf, strlen(buf), 0);
	
	memset(buf, 0, sizeof buf);
	recv(cmdSock, buf, BUFSIZ, 0);
	printf("%s", buf);
	
	//Calc port
	temp = strtok(buf,"(");
	/*for(int i=0;i<5;i++){
		temp = strtok(NULL,",");
		if (i == 4)
			strcpy(s1,temp);
	}*/
	
	int i=0;
	do {
		temp = strtok(NULL,",");
		if (i == 4)
			strcpy(s1,temp);
		i++;
	}while(i<5);
	
	temp = strtok(NULL,")");
	strcpy(s2,temp);

	x1 = atoi(s1);
	x2 = atoi(s2);
	dataPort = x1 * 256 + x2;
	
	dataAddr.sin_port=htons(dataPort);

	dataSock = socket(AF_INET, SOCK_STREAM, 0);
	if(dataSock == -1)
	{
		puts("Socket creation failed");
		exit(1);
	}
	if( connect(dataSock, (struct sockaddr *) &dataAddr, sizeof(struct sockaddr)) == -1 )
	{
		puts("Connect Error");
		exit(1);
	}
}

void ModeActive()
{
	int x1, x2, dataPort;
	char temp[16];
	
	x1 = rand() % 200 + 1;
	x2 = rand() % 200 + 1;
	
	/*for(int i=0; i<16; i++)
		if(ip[i] != '.')
			temp[i] = ip[i];
		else
			temp[i] = ',';*/
	int i=0;	
	do {
		if(ip[i] != '.')
			temp[i] = ip[i];
		else
			temp[i] = ',';
		i++;
	}while(i<16);
	
	memset(buf, 0, sizeof buf);
	sprintf(buf,"PORT %s,%d,%d\r\n",temp,x1,x2);
	send(cmdSock, buf, strlen(buf), 0);
	
	memset(buf, 0, sizeof buf);
	recv(cmdSock, buf, BUFSIZ, 0);
	puts(buf);
	
	dataPort = x1 * 256 + x2;
	
	dataAddr.sin_port=htons(dataPort);
	
	listenSock = socket(AF_INET, SOCK_STREAM, 0);
	bind(listenSock, (struct sockaddr *) &dataAddr, sizeof(struct sockaddr));
	listen(listenSock,5);
}

void CheckMode()
{
	if (strcmp(mode,"on") == 0)
		ModePassive();
	else 
		ModeActive();
}

void Accept()
{
	if (strcmp(mode,"off") == 0)
		dataSock = accept(listenSock, (struct sockaddr *) &serverStorage, &addr_size);
}

void CmdLineHandle(char* cmdLine)
{
	char* temp = strtok(cmdLine," ");
	//passive
	if (strcmp(temp,"passive") == 0)
	{
		if (strcmp(mode,"off") == 0)
		{
			strcpy(mode,"on");
			puts("Passive mode on!");
		}
		else {
			strcpy(mode,"off");
			puts("Passive mode off!");
		}
	}
	//ls
	else if (strcmp(temp,"ls") == 0)
	{
		CheckMode();
				
		memset(buf, 0, sizeof buf);
		sprintf(buf,"LIST\r\n");
		send(cmdSock, buf, strlen(buf), 0);
		
		Accept();
	
		memset(buf, 0, sizeof buf);
		recv(cmdSock, buf, BUFSIZ, 0);
		puts(buf);
		memset(buf, 0, sizeof buf);
		recv(cmdSock, buf, BUFSIZ, 0);
		puts(buf);
		memset(buf, 0, sizeof buf);
		recv(dataSock, buf, BUFSIZ, 0);
		puts(buf);
	}
	//exit
	else if (strcmp(cmdLine,"exit") == 0)
	{
		status = 1;
		memset(buf, 0, sizeof buf);
		sprintf(buf,"QUIT\r\n");
		send(cmdSock, buf, strlen(buf), 0);
		recv(cmdSock, buf, BUFSIZ, 0);
		puts(buf);
	}
}
 
int main(int argc,char *argv[])
{	
	int tmpres;
	char cmdLine[1024];
	status = 0;
	
	//Create socket	
	strcpy(ip,argv[1]);

	cmdAddr.sin_family = AF_INET;
	cmdAddr.sin_addr.s_addr=inet_addr(ip);
	cmdAddr.sin_port = htons(cmdPort);
	memset(cmdAddr.sin_zero, '\0', sizeof cmdAddr.sin_zero);
	
	dataAddr.sin_family=AF_INET;
	dataAddr.sin_addr.s_addr=inet_addr(ip);
	memset(dataAddr.sin_zero, '\0', sizeof dataAddr.sin_zero);
	
	cmdSock = socket(AF_INET, SOCK_STREAM, 0);
	if(cmdSock == -1)
	{
		printf("Socket creation failed");
		exit(1);
	}

	tmpres = connect(cmdSock,(struct sockaddr*)&cmdAddr, sizeof(struct sockaddr));
	if(tmpres == -1)
	{
		printf("Connect Error");
		exit(1);
	}

	char * str;
	int codeftp;
	printf("Connection established, waiting for welcome message...\n");
	
	while((tmpres = recv(cmdSock, buf, BUFSIZ, 0)) > 0){
		sscanf(buf,"%d", &codeftp);
		printf("%s", buf);
		if(codeftp != 220)
		{
			replylogcode(codeftp);
			exit(1);
		}

		str = strstr(buf, "\r\n");
		if(str != NULL){
			break;
		}
		memset(buf, 0, tmpres);
	}
	while(true) {
		//Send Username
		char info[50];
		printf("Name (%s): ", ip);
		memset(buf, 0, sizeof buf);
		scanf("%s", info);

		sprintf(buf,"USER %s\r\n",info);
		tmpres = send(cmdSock, buf, strlen(buf), 0);

		memset(buf, 0, sizeof buf);
		tmpres = recv(cmdSock, buf, BUFSIZ, 0);

		sscanf(buf,"%d", &codeftp);
		if(codeftp != 331)
		{
			replylogcode(codeftp);
			continue;
		}
		printf("%s", buf);

		//Send Password
		memset(info, 0, sizeof info);
		printf("Password: ");
		memset(buf, 0, sizeof buf);
		scanf("%s", info);

		sprintf(buf,"PASS %s\r\n",info);
		tmpres = send(cmdSock, buf, strlen(buf), 0);

		memset(buf, 0, sizeof buf);
		tmpres = recv(cmdSock, buf, BUFSIZ, 0);

		sscanf(buf,"%d", &codeftp);
		if(codeftp != 230)
		{
			replylogcode(codeftp);
			continue;
		}
		printf("%s", buf); 
		break;
	}
	strcpy(mode,"off");
	do {
	printf("ftp>");
	fflush(stdin);
	scanf("%s",cmdLine);
	CmdLineHandle(cmdLine);
	}while(status == 0);
	/*flush(stdin);
	scanf("%s",cmdLine);
	CmdLineHandle(cmdLine);*/
}

