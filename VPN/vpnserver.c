#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <errno.h>
#include <netinet/in_systm.h>
#include <netinet/udp.h>

#include <linux/netfilter.h>
#include <libipq/libipq.h>

#define BUF 1024
#define MSGSIZ BUF
#define PACKET_SIZE BUF
#define BUFSIZE BUF

// should be real ip address that client can ping to
char *serveraddress = "192.168.1.10";//"192.168.21.42";////"127.0.0.1";
char *clientaddress = "192.168.1.11";
// should be ip of myself, as used for inject into raw socket 
char *Myaddress = "192.168.1.10";//"127.0.0.1";
char lbuf[BUF] = "";
char sbuf[BUF] = "";

char buf[PACKET_SIZE];

int nread, fd;
int sockfd;
socklen_t his_len, my_len;
int z;
struct sockaddr_in my_addr;
struct sockaddr_in his_addr;
int clientport = 3333;
int serverport = 3333;
short int packet_length;



unsigned char srcip[4];
unsigned char dstip[4];
int sd;
u_char *packet;
const int on = 1;
struct sockaddr_in sin;



// iptable_queue
static void die(struct ipq_handle *h)
{
    ipq_perror("passer");
    ipq_destroy_handle(h);
    exit(1);
}



int main (int argc, const char * argv[]) {
    
    
    
    
    //Step1. 
    //Server Mode start
    //receive udp packets from client using normal socket
    printf("Server Initialized\n");
    
    //I'm server, so using serverport!
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(serverport);
    my_addr.sin_addr.s_addr = inet_addr(Myaddress);//INADDR_ANY; /*automaticaly pick local address (machine's)*/
    my_len = sizeof(my_addr);            /*structure len*/
    
    
	/*create a socket*/
    printf("[+]Creating Socket\n");
    sockfd = socket(PF_INET , SOCK_DGRAM , 0);
    
   	if(sockfd < 0)
		perror("socket()");
	printf("[+]Binding Socket\n");
	
	z = bind(sockfd , (struct sockaddr*)&my_addr , my_len); /*bind socket to our local address*/	    
	if(z < 0)
		perror("bind()");
    
    printf("[+]Waiting For Messages\n");
    printf("-------------------------------------------------\n");
    
	
	z  = recvfrom(sockfd, lbuf , MSGSIZ , 0 , (struct sockaddr*)&his_addr , &his_len );  
    
    
    
    
/////////////////////////////////////////////    
    //Step2. Change to Client Mode
    // we are gonna extract icmp packet from udp packet content
    
    //////////may be help
	// get source ip address
	//memcpy(srcip, &lmsg[12],4);
	// get destination ip address
	//memcpy(dstip, lmsg+16,4);
    
    
    
    
	// buffer[2-3] are length of packet
	// but using memcpy would be little-Endian,
	// so need htons(s for short, since 2 bytes)
	// to reverse.
    
	memcpy(&packet_length, lbuf+2,2);
	packet_length = htons(packet_length);	
	printf("packet length is: %d bytes.\n", packet_length);
    
    	packet = (u_char *)malloc(packet_length);
/*    	
	char a1, a2, a3, a4;
	memcpy(&a1, lbuf+12, 1);
	memcpy(&a2, lbuf+13, 1);
	memcpy(&a3, lbuf+14, 1);
	memcpy(&a4, lbuf+15, 1);
    	printf("dst:%d, %d, %d, %d\n",a1, a2, a3, a4);
*/	
	memset(lbuf+15, 1, 1);
    //OK we got packet and copy it into packet, and exactly the same length
	memcpy(packet, lbuf, packet_length);
    
    
	if ((sd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
		perror("raw socket");
		exit(1);
	}	
    
	if (setsockopt(sd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0) {
		perror("setsockopt");
		exit(1);
	}
    
	printf("set socket ready! \n");
    
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr("10.0.0.2");//ip.ip_dst.s_addr;
	
    //inject into raw socket and send it out.
    if (sendto(sd, packet, packet_length, 0, (struct sockaddr *)&sin, sizeof(struct sockaddr)) < 0)  {
		perror("sendto error");
		exit(1);
	}
	printf("send raw socket successfully! \n");
	
    //exit(0);
//////////////////////////////    
    //Step3. now get response from someone like VM2
    //Now funcion as iptable_queue
    int status;
    unsigned char buf[BUFSIZE];
    struct ipq_handle *h;
    
    h = ipq_create_handle(0, NFPROTO_IPV4);//PF_INET);
    if (!h)
        die(h);
    
    status = ipq_set_mode(h, IPQ_COPY_PACKET, BUFSIZE);
    if (status < 0)
        die(h);
    

    //iptable_queue loop
    //do{
        status = ipq_read(h, buf, BUFSIZE, 0);
        if (status < 0)
            die(h);
        
        switch (ipq_message_type(buf)) {
            case NLMSG_ERROR:
                fprintf(stderr, "Received error message %d\n",
                        ipq_get_msgerr(buf));
                break;
                
            case IPQM_PACKET: {
                ipq_packet_msg_t *m = ipq_get_packet(buf);
                printf("received one icmp\n");
/*				struct nlmsghdr *nlh; 
				nlh = (struct nlmsghdr *)buf; 

				printf("recv bytes =%d, nlmsg_len=%d, indev=%s, datalen=%d, packet_id=%x\n", nlh->nlmsg_len,m->indev_name, m->data_len, m->packet_id); 
                */
                /////////////////////////////////////////////////////////////////////////////////
				// now we'll send this packet to client
				// first I stored it in sbuf
				struct iphdr *ip = (struct iphdr*) m->payload;
				// clear sbuf
                memset(sbuf, 0, BUF);
                
                //get length of total packet (ip level, so with ip header)
                memcpy(sbuf, m->payload, ntohs(ip->tot_len));


                
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
                
                //already binded before
                //my_addr.sin_family = AF_INET;
                //my_addr.sin_port = htons(2000);
                //my_addr.sin_addr.s_addr = INADDR_ANY;
                
                
                // He's client, so using clientport
                // We already receive from client, so just use his.
                his_addr.sin_port = htons(clientport);
                his_addr.sin_addr.s_addr = inet_addr(clientaddress); 
                his_addr.sin_family = AF_INET;
                his_len = sizeof(his_addr);
                

                sendto(sockfd, buf, sizeof(buf), 0, (struct sockaddr*)&his_addr, his_len);
                
                printf("send successfully! \n");
                
                
                fflush(stdout);
                
                
                status = ipq_set_verdict(h, m->packet_id,
                                         NF_ACCEPT, 0, NULL);
                break; // just break, currently I do not want loop
                
                if (status < 0)
                    die(h);
                break;
            }
                
            default:
                fprintf(stderr, "Unknown message type!\n");
                break;
        }
   // } while (1);
    
    ipq_destroy_handle(h);
    
    
	return 0;
}

