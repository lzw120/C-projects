#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<errno.h>
#include <ifaddrs.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>

#define BUF 1024
#define MSGSIZ  1024
#define ERROR(X) perror(X);
#define MAXPATH 1024

#define random(x) (rand()%x)
#define propablity 0

// as timer need it so static global
int static to_send = 1;
int sockfd; /*socket file descriptor*/
int z; /*function statues*/
int socklen;
int PORT;
int conn_num;
int count=1;
socklen_t him_len; /*remote length*/
socklen_t bcast_len; /*broadcast length*/
struct sockaddr_in i; /*local address*/
struct sockaddr_in him; /*remote address*/
struct sockaddr_in bcast; /*broadcast address*/
char lmsg[MSGSIZ] = "";
char smsg[MSGSIZ] = "";
char *quit = "Bye Bye\n";
char *remote_addr;
char *adrss[100];
int timer_send = 0;
char last = 'Y';
char size[10];



void thread(void){
    printf("This is a pthread.\n");
}


int check(const char *string)
{
    if(strstr(string , "QUIT") != NULL)
    {
        return 0;
    }
    return 1;
}

//alarm
void catch_alarm ( int sig_num) 
{ 
    //printf ("Sorry,time limit reached. /n");
    if (timer_send == to_send) {
        if (random(10) >= (int)10*propablity) {
            sendto(sockfd, smsg, MSGSIZ, 0, (struct sockaddr*)&him , him_len);
            alarm(1);
        }
        
    }
    else
        alarm(0);
    //clear timer
    
    //exit (0); 
}

int main(int argc , char *argv[])
{
    srand((int)time(0));
    
	conn_num = 0;
    
    if(argc < 1)
    {
        printf("[Usage..] <%s> portno\n",argv[0]);
        _exit(1);
    }
    
    
    printf("Server Initialized\n");
    i.sin_family = AF_INET;
    i.sin_port = htons(atoi(argv[1]));
    i.sin_addr.s_addr = INADDR_ANY; /*automaticaly pick local address (machine's)*/
    socklen = sizeof(i);            /*structure len*/
    
    
	/*create a socket*/
    printf("[+]Creating Socket\n");
    sockfd = socket(PF_INET , SOCK_DGRAM , 0);
    
    if(sockfd < 0)
        ERROR("socket()");
    printf("[+]Binding Socket\n");
    z = bind(sockfd , (struct sockaddr*)&i , socklen); /*bind socket to our local address*/
    
    if(z < 0)
        ERROR("bind()");
    
    //printf("[+]Local Connection %s:%u\n",inet_ntoa(i.sin_addr) , ntohs(i.sin_port));
    printf("[+]Waiting For Messages\n");
    printf("-------------------------------------------------\n");
    
    //1. receive the filename
    char filename[MAXPATH] = "";
    him_len = sizeof(him);
    z  = recvfrom(sockfd, lmsg , MSGSIZ , 0 , (struct sockaddr*)&him , &him_len );  
    printf("Download request for %s requested by %s\n", lmsg, inet_ntoa(him.sin_addr));
    
    
    
    //2. send the first piece of file to client
    
    //2.1 read the first buffer of file
    // ...
    
    // create file
    // test file
    
    memcpy(filename, lmsg, sizeof(lmsg));
    FILE * fp = fopen(filename,"rb");
    
    if(NULL == fp )
    {
        printf("%s file not found \n", filename);
	sendto(sockfd,"x", 3, 0, (struct sockaddr*)&him, him_len);
        exit(1);
    }

	//Calculating no. of packets
	while (fgets(smsg+1, 1023, fp) ) count++;
	sprintf (size, "%d" , count);
	sendto(sockfd, size , sizeof(size), 0, (struct sockaddr*)&him, him_len);
	

    
    // start file transfer UI display
//    pthread_t id;
//    ret=pthread_create(&id,NULL,(void *) thread,NULL);
//    pthread_join(id,NULL);
    
    fseek(fp, 0, SEEK_SET);
    uint start = ftell(fp);     
    fseek(fp,0,SEEK_END); 
    uint end   =  ftell(fp); 
    uint len = end - start;
    fseek(fp, 0, SEEK_SET);
//    long proccess = 0;
    // start to read file into buffer
    //fgets(smsg+1, 1023, fp);
    //int i = 1;
    //start with piece 0
    //test smsg = "0"+filename;
    //memcpy(smsg+1, filename, sizeof(filename));
    
    printf ("Download in progress: ");    

    while (fgets(smsg+1, 1023, fp) ) {
	if (to_send%(count/14)==0) printf ("#");
        //len = ftell(fp);
//        fread(smsg+1, 1, 1023, fp);
//        proccess += 1023;
        //set the smsg[0] the number of the piece
        memset(smsg, last, 1);    
        if (random(10) >= (int)10*propablity) {
            sendto(sockfd, smsg, MSGSIZ, 0, (struct sockaddr*)&him, him_len);
        }
        
        //3. backup file piece with number to_send
        char back_up[MSGSIZ] = "";
        memcpy(back_up, smsg, sizeof(smsg));
        
        
        //4.set timer and wait for ACK+to_send
        
        //4.1 set timer here...
        signal ( SIGALRM, catch_alarm); 
        //when timer is set, record to_send
        timer_send = to_send;
        //alarm 1 sec
        alarm(1);
        
        //4.2 wait for ACK
        //clear lmsg
        memset(lmsg, 0, sizeof(lmsg));
        recvfrom(sockfd, lmsg, MSGSIZ, 0,(struct sockaddr*)&him , &him_len);
        alarm(0);
        //5. ack received, change to_send, and send next package
        to_send++;
        if (to_send % 2 == 0) {last = 'N';} else {last = 'Y';}
        
        //5.1 read next buffer
        // clear smsg
        memset(smsg, 0, sizeof(smsg));
        //        // ...................
        //        memcpy(smsg+1, filename, sizeof(filename));
        //        // set new to_send tag
        //        memset(smsg, to_send+'0', 1);
        //        //5.2 send out
        //        sendto(sockfd, smsg, MSGSIZ, 0, (struct sockaddr*)&him , him_len);
    }   
    
    
    // final send id "0"
    memset(smsg, 0+'0', 1);
    printf ("\nDownload complete.\n");
    if (random(10) >= (int)10*propablity) {
        sendto(sockfd, smsg, MSGSIZ, 0, (struct sockaddr*)&him, him_len);
    }
    
    return 0;
}
