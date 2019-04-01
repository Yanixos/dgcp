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


/*peer B starts*/
int main()
{
	printf("Peer B \n");
	/*Declaring the variables*/
	int udpSocket;//after the peer B creates a socket it is saved in this variable
	char buffer[MAXLINE];//a char array to store the message
	struct sockaddr_in peerAAddr;//a structure for storing the peer A address
	struct sockaddr_in peerBAddr;//a structure for storing the peer B address
	socklen_t addr_size;
	addr_size=sizeof(peerAAddr);
	

	/*Peer B creates a socket*/
	udpSocket=socket(PF_INET,SOCK_DGRAM,0);
	if(udpSocket==-1)
	{
		perror("Error creating the Peer B socket");
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
	
	/*Binding the created socket to the peer B address*/
	if(bind(udpSocket,(const struct sockaddr *)&peerBAddr,sizeof(peerBAddr))==-1)
	{
		perror("Error Binding the PEER B socket");
		exit(1);	
	}
	
	/*keep listening to the data*/
	while(1)
	{		
		/*Now PEER B sents a message*/
		printf("Peer B:");
		fgets(buffer,sizeof(buffer)/sizeof(buffer[0]),stdin);
		if(sendto(udpSocket,(char *)buffer,sizeof(buffer)/sizeof(buffer[0]),0,(const struct sockaddr *)&peerAAddr,sizeof(peerAAddr))==-1)
		{
			perror("Error Sending Message to Peer A");
			exit(1);	
		}	
		/*Wait for the peer A to sent the data*/
		if(recvfrom(udpSocket,(char *)buffer,sizeof(buffer)/sizeof(buffer[0]),0,(struct sockaddr *)&peerAAddr,&addr_size)==-1)
		{
			perror("Error Recieving Message from Peer A");
			exit(1);	
		}
		printf("Peer A:%s\n",buffer);	
	}
	return(0);
}
