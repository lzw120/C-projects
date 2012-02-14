#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include<errno.h>
#include<netinet/in.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include <time.h>
#include <sys/time.h>

#define BUF 1024
#define random(x) (rand()%x)
#define propablity 0.1

int main(int argc , char *argv[])
{
    int sockfd;
    int len;
    socklen_t his_len;
    int z;
    int size;
    int lost=0;
    
    struct sockaddr_in my_addr;
    struct sockaddr_in his_addr;
    
    char lbuf[BUF] = "";
    char sbuf[BUF] = "";
    char last = 'Y';

    timeval tim;
    gettimeofday(&tim, NULL);
    double t1=tim.tv_sec+(tim.tv_usec/1000000.0);
    
    
    if(argc < 4 )
    {
        printf("[!]Usage %s ip port filename\n",argv[0]);
        _exit(0);
    }
    printf("Client initialized\n");
    printf("[+]Creating Socket\n");
    sockfd = socket(PF_INET , SOCK_DGRAM , 0);
    if(sockfd < 0)
    {
        perror("[!]Socket()");
        _exit(1);
    }
    
    memset(&my_addr , 0 , sizeof(my_addr));
    memset(&his_addr , 0 , sizeof(his_addr));
    
    printf("[+]Forming Address\n");
    
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(2000);
	my_addr.sin_addr.s_addr = INADDR_ANY;
    
    his_addr.sin_family = AF_INET;
    his_addr.sin_port = htons(atoi(argv[2]));
    his_addr.sin_addr.s_addr = inet_addr(argv[1]);
    
    his_len = sizeof(his_addr);
    len = sizeof(my_addr);
    
    printf("[+]Binding Socket\n");
    z = bind(sockfd , (struct sockaddr*)&my_addr , len);
    if(z < 0)
    {
        perror("[!]Bind");
        _exit(1);
    }

    

    //1. send the filename
    // please specify your documents location, just for test.
    unsigned int expected = 1;
    char packet = 'Y';
    char filename[BUF];
    strcpy (filename, argv[3]);
    
    strcpy(sbuf, filename);
    if (random(10) >= (int)10*propablity) {
        sendto(sockfd, filename, sizeof(filename) / sizeof(filename[0]), 0, (struct sockaddr*)&his_addr, his_len);
    }

    
    FILE *fp = fopen(argv[3],"wb");
    if(NULL == fp )
        {
            printf("File:/t%s Can Not Open To Write/n", filename);
            exit(1);
        }
    //2. receive a package.
    recvfrom(sockfd, lbuf , BUF , 0 , (struct sockaddr*)&his_addr , &his_len);
    packet = lbuf[0];        
    if (packet == 'x') 
	{ 
		printf ("File not found.\n");
		return 0;
	} 
	else
	{
		size = atoi (lbuf);
		printf ("Download in progress: ");  ;
	}
        
    
    while (true) {
        
        // add a file pointer
        if (expected%(size/14)==0) printf ("#"); 
        
        
        recvfrom(sockfd, lbuf , BUF , 0 , (struct sockaddr*)&his_addr , &his_len);
	packet = lbuf[0];        
	//printf ("Received: %s with header %s \n",lbuf+1,&packet);
	    //3. get the #id of package and compare with expected.
        char ackmsg[4] = "ACK";
	/*char temp[1];
	temp[0] = lbuf[0];*/
	
	
        if (packet == last) {
            // first get the content of file
            // second send ACK expected
            // ... get content()
            fputs(lbuf+1, fp);
            
            //fwrite(lbuf+1, 1, 1023, fp);
            
            // send ACK, initialize sbuf
            memset(sbuf,0,sizeof(sbuf));
            
            // initialize lbuf
            memset(lbuf,0,sizeof(lbuf));
            
            //set acksmg[3] with value of expected
            memset(ackmsg+3, expected, 1);
            //or we can use memcpy(sbuf, strcat(ackmsg, &expected),4); when expected is a char
            
            //ok send message ACK associate with the number expected
            if (random(10) >= (int)10*propablity) {
                sendto(sockfd, sbuf, BUF, 0, (struct sockaddr*)&his_addr , his_len);
            }
            //expect the next
            expected++;
	    if (expected % 2 == 0) {last = 'N';} else {last = 'Y';}
		//printf ("Expecting Next: %d with header %s...\n",expected,&last);
            
        }
        else
        {
            //last package finsih transfering
            if (packet=='0') {
		printf ("\nDownload complete. \n");
		gettimeofday(&tim, NULL);
             	double t2=tim.tv_sec+(tim.tv_usec/1000000.0);
             	printf("%.6lf seconds elapsed\n", t2-t1);
		printf("Average delay is %.6lf\n",(t2-t1)/expected);
		printf("Throughput is %.6lf Kbps \n", size/(t2-t1));
		printf("Packets lost %d", lost);	
		fputs(lbuf+1, fp);
                fclose(fp);                
		return 0;
            }
            
            //clear sbuf first
            memset(sbuf, 0, sizeof(sbuf));
            
            // initialize lbuf
            memset(lbuf,0,sizeof(lbuf));
            
            //set ACK and #
            memset(ackmsg+3, expected-1+'0', 1);
            if (random(10) >= (int)10*propablity) {
                sendto(sockfd, sbuf, BUF, 0, (struct sockaddr*)&his_addr, his_len);
            } lost++;

		//printf ("Lost information exp: %s packet: %s \n ",&last,&packet);
        }
        
    }
    fclose(fp);
    return 0;
}


