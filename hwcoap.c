#include "hwcoap.h"

#define CON   0X00
#define NON   0X01
#define ACK   0x10
#define RST   0X11

#define POST  0X02


typedef struct{
    uint8_t tkl : 4;
    uint8_t type: 2;
    uint8_t ver : 2;
    uint8_t code;
    uint8_t idH;
    uint8_t idL;
}sCoapHeader_t;

typedef enum{
    REGING=0,
    DATAENABLE,
} eCoapState_t;

static uint8_t *TxBuff,*RxBuff;
static uint16_t TxMaxLen,RxMaxLen;

static eCoapState_t CoapState = REGING;
uint8_t TokenNotify[16],TklNotify = 0;  //store the nofity token(observe msg token)
static uint16_t MsgID=0;
static uint8_t ObsNum=0;

/*
  brief: set buffer which hwcoap used. if tx&&rx are in different thread,set tx&&rx differnt buffer,
        or you can set same one to save ram space.
  paras: txbuff:pointer to tx buffer; rxbuff:pointer to rx buffer;
         txlen rxlen: max length of buffer.
  return: null
*/
void HWCoapSetBuff(uint8_t * txbuff,uint8_t *rxbuff,uint16_t txlen,uint16_t rxlen)
{
    TxBuff = txbuff;
    RxBuff = rxbuff;
    TxMaxLen = txlen;
    RxMaxLen = rxlen;
}
/*
  brief: receive original coap data from huawei iot.
  paras: data:   pointer to receive buff;
         maxLen: max length of receiving buff;
         timeout:receive timeou,ms.
  return: receiving data length
*/
uint16_t HWReceiveData(uint8_t *data,uint16_t maxLen,uint16_t timeout)
{
    uint16_t len = 0;
    timeout=timeout>10?timeout/10:1;
    while(timeout--){
        DelayMs(10);
        len = UDP_Receive(data,maxLen);
        if(len){
            return len;
        }
    } 
    return 0;
}
/*
  brief: register to huawei iot with coap.
  paras: ep: endpoint name,string,this must be unique,can be imei. 
         epLen: the length of endpoint name.must > 10
  return: 0(HWOK)-register success , other-failed
*/
eHWerr_t HWRegisterWithCoap(char *ep,uint8_t epLen)
{
    uint16_t i = 0x10,j=0;
    if(ep==0||epLen < 11){
        return EPSHORT;
    }
    //1.build post message,register to hw iot.
    TxBuff[0]=0x42; 
    TxBuff[1]=0x02; //code 02 post
    MsgID++;
    TxBuff[2] = MsgID>>8;TxBuff[3] = MsgID; //message ID
    TxBuff[4] = 0x55;TxBuff[5]=0xaa; //token
    TxBuff[6] = 0xB1;TxBuff[7]=0x74; // /t
    TxBuff[8] = 0x01;TxBuff[9]=0x72; // /r
    TxBuff[10] = 0x11;TxBuff[11]=0x2a; // conten-format:application/octet-stream
    TxBuff[12] = 0x3d;
    i = epLen + 3;
    TxBuff[13] = i-13;
    TxBuff[14]=0x65;TxBuff[15]=0x70;TxBuff[16]=0x3d; //ep=
    j=17;
    for(i=0;i<epLen;i++){
        TxBuff[j++]=ep[i];
    }
    i = UDP_Send(TxBuff,j); //32 Bytes
    //2.wait for ack from hw iot
    i = HWReceiveData(TxBuff,TxMaxLen,2000);
    if(TxBuff[0]!=0x62 || TxBuff[1]!=0x44){
        return ACKERR;
    }
    //3.wait for observe
    i = HWReceiveData(TxBuff,TxMaxLen,30000);
    j = (TxBuff[0]&0x0f) + 4; //observe option:tkl + 4, 0x60
    if(((TxBuff[0]&0xf0)!=0x40)||(TxBuff[1]!=0x01)||(TxBuff[j]!=0x60)){  
        return OBSERR;
    }
    //store the observe token.
    TklNotify = TxBuff[0]&0x0f;
    for(i=0;i<TklNotify;i++){
        TokenNotify[i]=TxBuff[4+i];
    }
    //4.response observe
    TxBuff[0]|=0x20;
    TxBuff[1]=0x44;
    i = UDP_Send(TxBuff,5+TklNotify); //4+Tkl+1
    //5.wait for /4/0/8
    i = HWReceiveData(TxBuff,TxMaxLen,3000);
    j = (TxBuff[0]&0x0f)+4+3+1; 
    if(TxBuff[0]&0xf0!=0x40||TxBuff[1]!=0x01||TxBuff[j]!=0x34){
        return GETERR;
    }
    //6.response /4/0/8
    TxBuff[0]|=0x20;
    TxBuff[1]=0x84;  //4.04 not found
    j = (TxBuff[0]&0x0f)+4;
    i = UDP_Send(TxBuff,j); //4+Tkl
    CoapState = DATAENABLE;
    return HWOK;
}
/*
  brief: send data to huawei iot with coap.
  paras: data-pointer to data to be sent
         len-data length to be send.
  return: 0(HWOK)-success , other-failed
*/
eHWerr_t HWReportData(uint8_t *data,uint16_t len)
{
    uint16_t i = 0x10,j=0;
    if(CoapState != DATAENABLE){
        return NOTREG;
    }
    MsgID++;  //coap msg sequence
    ObsNum++;
    TxBuff[0]=0x40|i|TklNotify;
    TxBuff[1]=0x45; //code 2.05 content
    TxBuff[2]=MsgID>>8;TxBuff[3]=MsgID; //message ID
    j=4;
    for(i=0;i<TklNotify;i++){
        TxBuff[j++]=TokenNotify[i];
    }
    TxBuff[j++]=0x61;
    TxBuff[j++]=ObsNum;
    TxBuff[j++]=0xff;
    for(i=0;i<len;i++){
        TxBuff[j++]=data[i];
    }
    i = UDP_Send(TxBuff,j);
    return i;
}
/*
  brief: receive&&process data from huawei iot with coap. eg:downlink cmd.
         This function needs to be called cyclically.
  paras: data-pointer to data receiving buffer
         maxLen-max length of receiving buff.
  return: receiving data length
*/
uint16_t HWProcessRxData(uint8_t *data,uint16_t maxLen)
{
    uint16_t i,j,rxLen;
    sCoapHeader_t *pHeader = (sCoapHeader_t *)RxBuff;
    rxLen = HWReceiveData(RxBuff,RxMaxLen,500);
    if(!rxLen){
        return 0;
    }
    if(pHeader->ver!=0x01){
        return 0;
    }
    if(pHeader->type == RST){
        CoapState = REGING;
        //TODO: call HWRegisterWithCoap.
    }else if(pHeader->code == POST){
        i=pHeader->tkl+4+3+1;
        if(RxBuff[i]=='t'&&RxBuff[i+2]=='d'&&RxBuff[i+5]==0xff){
            i+=6;
            if(maxLen >= (rxLen-i)) {
                for(j=0;i<rxLen;i++,j++){
                    data[j]=RxBuff[i];
                }
            }else {
                j = 0;
            }
        }
        RxBuff[0]|=0x20;
        RxBuff[1]|=0x44;
        i=pHeader->tkl+4;
        UDP_Send(RxBuff,i);
    }
    return j;
}