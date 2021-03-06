

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


#define PORT 21

struct sockaddr_in dataChannel;

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

char* sendCommand(char str[100])
{
	//sprintf(buf,"USER %s\r\n",info);
	return NULL;
}
 
int main(int argc,char *argv[])
{
	struct sockaddr_in *remote;
	struct stat obj;
	int sock;
	int choice;
	int tmpres, size, status;
	int filehandle;
	char buf[BUFSIZ+1];
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock == -1)
	{
		printf("socket creation failed");
		exit(1);
	}
	char *ip = "127.0.0.1";
	remote = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in *));
	remote->sin_family = AF_INET;
	tmpres = inet_pton(AF_INET, ip, (void *)(&(remote->sin_addr.s_addr)));
	if( tmpres < 0)  
	{
		perror("Can't set remote->sin_addr.s_addr");
		exit(1);
	}else if(tmpres == 0)
	{
		fprintf(stderr, "%s is not a valid IP address\n", ip);
		exit(1);
	}
	remote->sin_port = htons(PORT);

	tmpres = connect(sock,(struct sockaddr*)remote, sizeof(struct sockaddr));
	if(tmpres == -1)
	{
		printf("Connect Error");
		exit(1);
	}

	/*
	Connection Establishment
	   120
		  220
	   220
	   421
	Login
	   USER
		  230
		  530
		  500, 501, 421
		  331, 332
	   PASS
		  230
		  202
		  530
		  500, 501, 503, 421
		  332
	*/
	char * str;
	int codeftp;
	printf("Connection established, waiting for welcome message...\n");
	//How to know the end of welcome message: http://stackoverflow.com/questions/13082538/how-to-know-the-end-of-ftp-welcome-message
	while((tmpres = recv(sock, buf, BUFSIZ, 0)) > 0){
		sscanf(buf,"%d", &codeftp);
		printf("%s", buf);
		if(codeftp != 220) //120, 240, 421: something wrong
		{
			replylogcode(codeftp);
			exit(1);
		}

		str = strstr(buf, "\r\n");//Why ???
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
		tmpres = send(sock, buf, strlen(buf), 0);

		memset(buf, 0, sizeof buf);
		tmpres = recv(sock, buf, BUFSIZ, 0);

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
		tmpres = send(sock, buf, strlen(buf), 0);

		memset(buf, 0, sizeof buf);
		tmpres = recv(sock, buf, BUFSIZ, 0);

		sscanf(buf,"%d", &codeftp);
		if(codeftp != 230)
		{
			replylogcode(codeftp);
			continue;
		}
		printf("%s", buf); 
		break;
	}
	//Active mode
	int x1, x2,port;
	int dataSock;
	char temp[16];
	
	x1 = 78;
	x2 = 123;
	
	for(int i=0; i<16; i++)
		if(ip[i] != '.')
			temp[i] = ip[i];
		else
			temp[i] = ',';
	memset(buf, 0, sizeof buf);
	sprintf(buf,"PORT %s,%d,%d\r\n",temp,x1,x2);
	send(sock, buf, strlen(buf), 0);
	memset(buf, 0, sizeof buf);
	recv(sock, buf, BUFSIZ, 0);
	puts(buf);
	
	port = x1 * 256 + x2;
	printf("%i\n",port);
	
	dataChannel.sin_family=AF_INET;
	dataChannel.sin_port=htons(port);
	dataChannel.sin_addr.s_addr=inet_addr(ip);
	memset(dataChannel.sin_zero, '\0', sizeof dataChannel.sin_zero);
	
	int welcomeSocket = socket(AF_INET, SOCK_STREAM, 0);
	bind(welcomeSocket, (struct sockaddr *) &dataChannel, sizeof(struct sockaddr));
	
	memset(buf, 0, sizeof buf);
	sprintf(buf,"LIST\r\n");
	tmpres = send(sock, buf, strlen(buf), 0);
	
	listen(welcomeSocket,5);
	
	struct sockaddr_storage serverStorage;
	socklen_t addr_size = sizeof serverStorage;
	dataSock = accept(welcomeSocket, (struct sockaddr *) &serverStorage, &addr_size);
	
	memset(buf, 0, sizeof buf);
	tmpres = recv(dataSock, buf, BUFSIZ, 0);
	puts(buf);
}

