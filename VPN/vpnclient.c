#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <linux/ip.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
//#include <netinet/ip.h>
//#include <netinet/ip_icmp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/stat.h>
#include <netinet/in_systm.h>
#include <netinet/udp.h>


#define BUF 1024
#define MSGSIZ BUF
#define PACKET_SIZE BUF

// should be real ip address that client can ping to
char *serveraddress = "192.168.1.10";
// should be ip of myself, as used for inject into raw socket 
char *Myaddress = "192.168.1.11";
char lbuf[BUF] = "";
char sbuf[BUF] = "";

char buf[PACKET_SIZE];
char ifname[IFNAMSIZ];
int nread, fd;
int sockfd;
socklen_t his_len, my_len;
int z;
struct sockaddr_in my_addr;
struct sockaddr_in his_addr;
int clientport = 3333;
int serverport = 3333;
short int packet_length;



int tun_alloc(char *dev)
{
    struct ifreq ifr;
    int fd, err;
    
    if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
        perror("open");
        return -1;
    }
    
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    
    if (*dev)
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    
    if ((err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0) {
        perror("ioctl");
        close(fd);
        return err;
    }
    strcpy(dev, ifr.ifr_name);
    
    return fd;
}

int main()
{
    
//Step1. client mode start:
    // start tun, get from client "ping", 
    
    printf("Client initialized\n");
    printf("[+]Creating Socket\n");
    sockfd = socket(PF_INET , SOCK_DGRAM , 0);
    if(sockfd < 0)
    {
        perror("[!]Socket()");
        _exit(1);
    }
    
    // initialize
    memset(&my_addr , 0 , sizeof(my_addr));
    memset(&his_addr , 0 , sizeof(his_addr));
    
 	printf("[+]Forming Address\n");
    
   	my_addr.sin_family = AF_INET;
    //!!!!!!!!!!!!!I'm client, so using clientport
    my_addr.sin_port = htons(clientport);
	my_addr.sin_addr.s_addr = INADDR_ANY;
    
	
    //!!!!!!!!!!!!! He's server, so using serverport
	his_addr.sin_port = htons(serverport);
	his_addr.sin_addr.s_addr = inet_addr(serveraddress); 
    his_addr.sin_family = AF_INET;
    
	his_len = sizeof(his_addr);
    my_len = sizeof(my_addr);
	
	printf("[+]Binding Socket\n");
    z = bind(sockfd , (struct sockaddr*)&my_addr , my_len);
    if(z < 0)
    {
		perror("[!]Bind");
		_exit(1);
    }
    



    strcpy(ifname, "tun%d");
    if ((fd = tun_alloc(ifname)) < 0) {
        fprintf(stderr, "tunnel interface allocation failed\n");
        exit(1);
    }


    printf("allocted tunnel interface %s\n", ifname);
    
    // you can set for loop infinite or not.
//for (;;) {
    if ((nread = read(fd, buf, sizeof(buf))) < 0) {
        perror("read");
        close(fd);
        exit(1);
    }
    

    //send buf to server.
	sendto(sockfd, buf, sizeof(buf), 0, (struct sockaddr*)&his_addr, his_len);

	printf("client sends to server successfully! \n");

  
//Step2. We're gonna change to server mode!!    
    // receive response from server
    z  = recvfrom(sockfd, lbuf , MSGSIZ , 0 , (struct sockaddr*)&my_addr , &my_len ); 
    printf("received from server\n");
    // Get it, extract packet from lbuf now.
    // we need accurate packet length, as lbuf is filled with 0 to its final
    //char a = buf[3];
	//printf("buf 3 is:%d\n", a);
    //struct iphdr *ip = (struct iphdr*) lbuf;
    packet_length = buf[3];//ntohs(ip->tot_len);

    //struct ip ip; 
  
    
    //long tmp = ntohl(inet_addr(Myaddress));
    //memcpy(&(ip->daddr), &tmp, 4);
    //struct in_addr src;

    //memcpy(&src, &ip->saddr, 4);
    //memcpy(&ip, lbuf, sizeof(ip));
    //packet_length = ntohs(ip.ip_len);
    char a[4];
    buf[15];
    printf("read %d bytes from: %d.%d.%d.%d \n", packet_length, buf[16], buf[17], buf[18], buf[19]); ////inet_ntoa(src));

   // ok, copy it to sbuf, we'll inject it into raw socket
   // memcpy(sbuf, lbuf, packet_length);
//}

/*
//Step3. Raw Socekt start working
    	int sd;
	u_char *packet;
	const int on = 1;
	struct sockaddr_in sin;
	packet = (u_char *)malloc(packet_length);
    
 	ip.ip_hl = 0x5;
	ip.ip_v = 0x4;
	ip.ip_tos = 0x0;
	ip.ip_len = htons(packet_length);
	ip.ip_id = htons(12830);
	ip.ip_off = 0x0;
	ip.ip_ttl = 64;
	ip.ip_p = IPPROTO_ICMP;
	ip.ip_sum = 0x0;
	ip.ip_src.s_addr = inet_addr("10.0.0.2");
	ip.ip_dst.s_addr = inet_addr("192.168.1.11");
	ip.ip_sum = in_cksum((unsigned short *)&ip, sizeof(ip));
	memcpy(packet, &ip, sizeof(ip));
	icmp.icmp_type = ICMP_ECHOREPLY;
	icmp.icmp_code = 0;
	icmp.icmp_id = 1000;
	icmp.icmp_seq = 0;
	icmp.icmp_cksum = 0;
	icmp.icmp_cksum = in_cksum((unsigned short *)&icmp, 8);
	memcpy(packet + 20, &icmp, 8);  



    // copy from sbuf, to inject into raw socket
    memcpy(packet, sbuf, packet_length);


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
	sin.sin_addr.s_addr = inet_addr(Myaddress);//("55.55.55.155");//ip.ip_dst.s_addr;
	
    if (sendto(sd, packet, packet_length, 0, (struct sockaddr *)&sin, sizeof(struct sockaddr)) < 0)  {
		perror("sendto error");
		exit(1);
	}
	printf("send raw socket successfully! \n");
*/    
    
    return 0;
}

