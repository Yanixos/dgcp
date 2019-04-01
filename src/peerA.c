#include<stdio.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include<stdlib.h>
#include<pthread.h>
#include<unistd.h>
#include<arpa/inet.h>

/*Macros*/
#define PORT_A 7891
#define PORT_B 7892
#define MAXLINE 1024


/*peer A starts*/
int main()
{
	printf("Peer A \n");
	/*Declaring the variables*/
	int udpSocket;//after the peer A creates a socket it is saved in this variable
	char buffer[MAXLINE];//a char array to store the message
	struct sockaddr_in peerAAddr;//a structure for storing the peer A address
	struct sockaddr_in peerBAddr;//a structure for storing the peer B address
	socklen_t addr_size;
	addr_size=sizeof(peerBAddr);
	

	/*Peer A creates a socket*/
	udpSocket=socket(PF_INET,SOCK_DGRAM,0);
	if(udpSocket==-1)
	{
		perror("Error creating the Peer A socket");
		exit(1);
	}
	
	/*Stating the Peer A Address*/
	peerAAddr.sin_family=AF_INET;
	peerAAddr.sin_port=htons(PORT_A);
	peerAAddr.sin_addr.s_addr=inet_addr("127.0.0.1");
	memset(peerAAddr.sin_zero,'\0',sizeof(peerAAddr.sin_zero));	

	/*Stating the Peer B Address*/
	peerBAddr.sin_family=AF_INET;
	peerBAddr.sin_port=htons(PORT_B);
	peerBAddr.sin_addr.s_addr=inet_addr("127.0.0.1");
	memset(peerBAddr.sin_zero,'\0',sizeof(peerBAddr.sin_zero));	
	
	/*Binding the created socket to the peer A address*/
	if(bind(udpSocket,(const struct sockaddr *)&peerAAddr,sizeof(peerAAddr))==-1)
	{
		perror("Error Binding the peer A socket");
		exit(1);	
	}
	
	/*keep listening to the data*/
	while(1)
	{
		/*Wait for the peer B to sent the data*/
		if(recvfrom(udpSocket,(char *)buffer,sizeof(buffer)/sizeof(buffer[0]),0,(struct sockaddr *)&peerBAddr,&addr_size)==-1)
		{
			perror("Error Recieving Message from Peer B");
			exit(1);	
		}
		printf("Peer B:%s\n",buffer);
		/*Now peer A sents a message*/
		printf("Peer A:");
		fgets(buffer,sizeof(buffer)/sizeof(buffer[0]),stdin);
		if(sendto(udpSocket,(char *)buffer,sizeof(buffer)/sizeof(buffer[0]),0,(const struct sockaddr *)&peerBAddr,sizeof(peerBAddr))==-1)
		{
			perror("Error Sending Message to Peer B");
			exit(1);	
		}		
	}
	return(0);
}

