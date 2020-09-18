#ifndef  __HWCOAP_H__
#define  __HWCOAP_H__

typedef unsigned char   uint8_t;
typedef unsigned short  uint16_t;
typedef unsigned int    uint32_t;

typedef enum{
    HWOK=0,
    NETERR,
    EPSHORT,
    ACKERR,
    OBSERR,
    GETERR,
    NOTREG,
} eHWerr_t;

/*
  brief: set buffer which hwcoap used. if tx&&rx are in different thread,set tx&&rx differnt buffer,
        or you can set same one to save ram space.
  paras: txbuff:pointer to tx buffer; rxbuff:pointer to rx buffer;
         txlen rxlen: max length of buffer.
  return: null
*/
void HWCoapSetBuff(uint8_t * txbuff,uint8_t *rxbuff,uint16_t txlen,uint16_t rxlen);
/*
  brief: register to huawei iot with coap.
  paras: ep: endpoint name,string,this must be unique,can be imei.
         epLen: the length of endpoint name, must > 10.
  return: 0-register success , other-failed
*/
eHWerr_t HWRegisterWithCoap(char *ep,uint8_t epLen);
/*
  brief: send data to huawei iot with coap.
  paras: data-pointer to data to be sent,len-data length to be send.
  return: 0-success , other-failed
*/
eHWerr_t HWReportData(uint8_t *data,uint16_t len);
/*
  brief: receive data from huawei iot with coap. eg:downlink cmd
  paras: data-pointer to data receiving buffer.
         maxLen-max length of receiving buff.
  return: receiving data length
*/
uint16_t HWProcessRxData(uint8_t *data,uint16_t maxLen);

//-------------------------Interfaces that need to be implemented by user-------------------//
/*
  brief: send data through udp.
  paras: data: data to be sent
         len: data length to be sent
  return: 0-success , other -failed
*/
uint8_t UDP_Send(uint8_t *data,uint16_t len);
/*
  brief: receive data through udp.
  paras: data,receive buffer
         maxLen, max length of receiving buff.
  return: receiving data length. 
*/
uint16_t UDP_Receive(uint8_t *data,uint16_t maxLen);
/*
  brief: delay some time. unit:ms
*/
void  DelayMs(uint16_t ms);

#endif // ! __HWCOAP_H__#define  __HWCOAP_H__
