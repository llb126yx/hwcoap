#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>
#include"hwcoap.h"

#define DEBUG  1

/*
set socket non-block
1.
    int flag = fcntl(client_fd, F_GETFL, 0);
    fcntl(client_fd, F_SETFL, flag | O_NONBLOCK);
2.
    struct timeval timeout;
    timeout.tv_sec = 1;//秒
    timeout.tv_usec = 0;//微秒
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
        printf("setsockopt failed:");
    }
*/

#define SERVER_PORT 5683
#define SERVER_IP   "119.3.250.80"  //"192.168.1.136" //

int client_fd = -1;
struct sockaddr_in ser_addr;

uint8_t CoapTxBuff[512],CoapRxBuff[512];

uint8_t AppBuff[255];

uint8_t UDP_Send(uint8_t *data,uint16_t len)
{
    int res;
    if(client_fd < 0){
        return 1;
    }
    #if DEBUG
    printf("UDP send:");
    for(res = 0; res < len;res++){
        printf("%.2x ",data[res]);
    }
    printf("\r\n");
    #endif
    res = sendto(client_fd, data, len, 0, (struct sockaddr *)&ser_addr, sizeof(ser_addr));
    if(res == -1){
        return 1;
    }
    return 0;
}
uint16_t UDP_Receive(uint8_t *data,uint16_t maxLen)
{
    int cnt;
    struct sockaddr_in src;
    socklen_t slen;
    if(client_fd < 0){
        return 0;
    }
    cnt=recvfrom(client_fd, data, maxLen, 0, (struct sockaddr*)&src, &slen);
    if(cnt == -1){
        return 0;
    }
    #if DEBUG
    printf("UDP receive:");
    for(maxLen = 0; maxLen < cnt;maxLen++){
        printf("%.2x ",data[maxLen]);
    }
    printf("\r\n");
    #endif
    return cnt;
}
void DelayMs(uint16_t ms)
{
    usleep(ms*1000);
}


int main(int argc, char* argv[])
{
    int i=0,j,flag;
    uint16_t rxLen=0;
    char *epname;
    //0.creat socket
    client_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(client_fd < 0)
    {
        printf("create socket fail!\n");
        return -1;
    }
    //1.set non-block
    flag = fcntl(client_fd, F_GETFL, 0);
    fcntl(client_fd, F_SETFL, flag | O_NONBLOCK);
    //2.init sockaddr_in
    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    ser_addr.sin_port = htons(SERVER_PORT);

    //3.start test: register -- report data -- receving data frome iot
    HWCoapSetBuff(CoapTxBuff,CoapRxBuff,512,512);
    printf("start reg HW-IoT...\r\n");
    //Note: you should change the endpoint name to your own.you need to register the device on the HW IOT platform first.
    if(argc > 1){
        epname = argv[1];
    }else {
        epname = "868681049496159";
    }
    i = HWRegisterWithCoap(epname,strlen(epname));
    if(i!=HWOK){
        printf("Reg HW-IoT faild,errcode:%d\r\n",i);
        return -1;
    }
    printf("Reg HW-IoT success,start reporting data every 10s.\r\n");
    while(1)
    {
		printf("Report data to HW-IoT:%u\r\n",i);
        AppBuff[0]=00;
        AppBuff[1]=i;
        i++;
        HWReportData(AppBuff,2);
        rxLen = HWProcessRxData(AppBuff,255);
        if(rxLen){
            printf("Rx Data frome HW-IoT:");
            for(j=0;j<rxLen;j++){
                printf("%.2x ",AppBuff[j]);
            }
            printf("\r\n");
        }
        DelayMs(10000);
    }
    return 1;
}