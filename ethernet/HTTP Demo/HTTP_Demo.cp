#line 1 "C:/Users/Arul Prakash/Downloads/SPAR/HTTP Demo/HTTP_Demo.c"
#line 1 "c:/users/public/documents/mikroelektronika/mikroc pro for dspic/include/stdint.h"




typedef signed char int8_t;
typedef signed int int16_t;
typedef signed long int int32_t;


typedef unsigned char uint8_t;
typedef unsigned int uint16_t;
typedef unsigned long int uint32_t;


typedef signed char int_least8_t;
typedef signed int int_least16_t;
typedef signed long int int_least32_t;


typedef unsigned char uint_least8_t;
typedef unsigned int uint_least16_t;
typedef unsigned long int uint_least32_t;



typedef signed int int_fast8_t;
typedef signed int int_fast16_t;
typedef signed long int int_fast32_t;


typedef unsigned int uint_fast8_t;
typedef unsigned int uint_fast16_t;
typedef unsigned long int uint_fast32_t;


typedef signed int intptr_t;
typedef unsigned int uintptr_t;


typedef signed long int intmax_t;
typedef unsigned long int uintmax_t;
#line 1 "c:/users/arul prakash/downloads/spar/http demo/__ethenc28j60.h"
#line 119 "c:/users/arul prakash/downloads/spar/http demo/__ethenc28j60.h"
typedef struct
 {
 unsigned char valid;
 unsigned long tmr;
 unsigned char ip[4];
 unsigned char mac[6];
 } SPI_Ethernet_arpCacheStruct;

extern SPI_Ethernet_arpCacheStruct SPI_Ethernet_arpCache[];

extern unsigned char SPI_Ethernet_macAddr[6];
extern unsigned char SPI_Ethernet_ipAddr[4];
extern unsigned char SPI_Ethernet_gwIpAddr[4];
extern unsigned char SPI_Ethernet_ipMask[4];
extern unsigned char SPI_Ethernet_dnsIpAddr[4];
extern unsigned char SPI_Ethernet_rmtIpAddr[4];

extern unsigned long SPI_Ethernet_userTimerSec;

typedef struct {
 unsigned canCloseTCP: 1;
 unsigned isBroadcast: 1;
} TEthPktFlags;
#line 146 "c:/users/arul prakash/downloads/spar/http demo/__ethenc28j60.h"
extern void SPI_Ethernet_Init(unsigned char *resetPort, unsigned char resetBit, unsigned char *CSport, unsigned char CSbit, unsigned char *mac, unsigned char *ip, unsigned char fullDuplex);
extern unsigned char SPI_Ethernet_doPacket();
extern void SPI_Ethernet_putByte(unsigned char b);
extern void SPI_Ethernet_putBytes(unsigned char *ptr, unsigned int n);
extern void SPI_Ethernet_putConstBytes(const unsigned char *ptr, unsigned int n);
extern unsigned char SPI_Ethernet_getByte();
extern void SPI_Ethernet_getBytes(unsigned char *ptr, unsigned int addr, unsigned int n);
extern unsigned int SPI_Ethernet_UserUDP(unsigned char *remoteHost, unsigned int remotePort, unsigned int localPort, unsigned int reqLength);
extern unsigned int SPI_Ethernet_UserTCP(unsigned char *remoteHost, unsigned int remotePort, unsigned int localPort, unsigned int reqLength, char * canClose);
extern void SPI_Ethernet_confNetwork(char *ipMask, char *gwIpAddr, char *dnsIpAddr);
#line 21 "C:/Users/Arul Prakash/Downloads/SPAR/HTTP Demo/HTTP_Demo.c"
unsigned short normallyOpenClose, DOVAL, tempDO;
unsigned short NI2CD;
void EXPANDER_write(char chipxx, char portAddr, char dat){
 I2C2_Start();

 I2C2_Write(chipxx);
 I2C2_Write(portAddr);
 I2C2_Write(dat);
 I2C2_Stop();
}
char EXPANDER_read(char chipxx, char portAddr) {
 char dat;
 I2C2_Start();

 I2C2_Write(chipxx);
 I2C2_Write(portAddr);
 I2C2_restart();
 I2C2_Write(chipxx+1);
 dat = I2C2_Read(1);
 I2C_Stop();
 return dat;
}
void EXPANDER_init(char portA, char portB, char portC){
 EXPANDER_write ( 0x40 ,  0x00 , portA);
 EXPANDER_write ( 0x40 ,  0x01 , portB);
 EXPANDER_write ( 0x42 ,  0x00 , portC);
 EXPANDER_write ( 0x42 , 0x01 , 0xFF );
 }


void INIT1_interrupt(){
 EXPANDER_write( 0x42 , 0x02, 0xFF );
 EXPANDER_write( 0x42 , 0x03, 0x00 );
 EXPANDER_write( 0x42 , 0x04, 0x00 );
 }


void I2C_int1Chip2() iv IVT_ADDR_INT1INTERRUPT {

 NI2CD=1;
#line 70 "C:/Users/Arul Prakash/Downloads/SPAR/HTTP Demo/HTTP_Demo.c"
 IFS1.INT1IF = 0;
}



unsigned char MACAddr[6] = {0x00, 0x14, 0xa5, 0x76, 0x19, 0x3f} ;
unsigned char IpAddr[4] = {192,168,0,150};
unsigned char getRequest[10];


char gwIpAddr[4] = {192, 168, 0,1};
char dnsIpAddr[4] = {192, 168, 0,10};
char ipMask[4] = {255, 255, 255, 255};


char temp= 0xAB;
#line 98 "C:/Users/Arul Prakash/Downloads/SPAR/HTTP Demo/HTTP_Demo.c"
int jll;
#line 113 "C:/Users/Arul Prakash/Downloads/SPAR/HTTP Demo/HTTP_Demo.c"
 char maadr0=0x01;
 char maadr1=0x02;
 char maadr2=0x03;
 char maadr3=0x04;
 char maadr4=0x05;
 char maadr5=0x06;


const char httpHeader[] = "HTTP/1.1 200 OK\nContent-type: ";
const char httpMimeTypeHTML[] = "text/html\n\n";
const char httpMimeTypeScript[] = "text/plain\n\n";



char indexPage[] =
#line 136 "C:/Users/Arul Prakash/Downloads/SPAR/HTTP Demo/HTTP_Demo.c"
"<html><head><title>mikroElektronika</title></head><body><h3 align=center>MikroElektronika Home Automatization System</h3><form name=\"input\" action=\"/\" method=\"get\"><table align=center width=200 bgcolor=#4974E2 border=2><tr><td align=center colspan=2><font size=4 color=white><b>Heat Control</b></font></td></tr><tr><td align=center bgcolor=#4974E2><input name=\"tst1\" width=60 type=\"submit\" value=\"ON\"></td><td align=center bgcolor=#FFFF00><input name=\"tst2\" type=\"submit\" value=\"OFF\"></td></tr></table></form></body></html>";


 void bank_3select (void)
 {
  LATGbits.LATG0=0;  ;
 SPI1_Write( 0x1f | 0b10100000  ) ;
 SPI1_Write(0b11111111) ;
  LATGbits.LATG0=1;  ;

  LATGbits.LATG0=0;  ;
 SPI1_Write(  0x1f |  0b10000000  ) ;
 SPI1_Write(0b00000011) ;
  LATGbits.LATG0=1;  ;


 }

sbit SPI_Ethernet_CS at LATG0_bit;
sbit SPI_Ethernet_CS_Direction at TRISG0_bit;

sbit SPI_Ethernet_Rst at LATG1_bit;
sbit SPI_Ethernet_Rst_Direction at TRISG1_bit;





void int_mcu(void)
 {
 OSCCON=0x3E0;
 CLKDIV=0x006;
 PLLFBD=278;
 ANSELD=0x0000;
 ANSELA=0x0000;
 }






void SPI1_pin_config (void)
 {


 TRISG.b0= 0;

 TRISGbits.TRISG14=0;
 TRISEbits.TRISE0= 0;
 TRISEbits.TRISE1=1;


 TRISGbits.TRISG1=0;



 ANSELG.b0= 0;


 ANSELEbits.ANSE1=0;


 }






void map_io(void)
 {
 Unlock_IOLOCK();
 PPS_Mapping_NoLock(126, _OUTPUT, _SDO1);
 PPS_Mapping_NoLock(80, _OUTPUT, _SCK1OUT);
 PPS_Mapping_NoLock(81, _INPUT, _SDI1);

 Lock_IOLOCK();
 }







void SPI1_regi_config (void)
 {
 SPI1CON1 = 0x0120;
 SPI1STAT = 0x8000;

 SPI1CON2 = 0x0000;
 }




void initialize_spi1_module(void)
 {
 SPI1_pin_config();
 map_io();
 SPI1_regi_config();
 }


unsigned int SPI_Ethernet_UserTCP(unsigned char *remoteHost,
 unsigned int remotePort,
 unsigned int localPort, unsigned int reqLength, TEthPktFlags *flags)
{

 unsigned int len;
 if(localPort != 80)
 return(0);





 for(len = 0; len<15;len++)
 {
 getRequest[len] = SPI_Ethernet_getByte();
 }
 getRequest[len] = 0;

 len=0;


 if(memcmp(getRequest, "GET /", 5))
 {
 return(0);
 }

 if(!memcmp(getRequest+11, "ON", 2))
 {
 EXPANDER_write( 0x40 , 0x14, 0x80);
 }


 else if(!memcmp(getRequest+11, "OFF", 3))
 {
 {
 EXPANDER_write( 0x40 , 0x14, 0x00);
 }

 if (PORTB.F0)
 {
 memcpy(indexPage+340, "#FFFF00", 6);
 memcpy(indexPage+431, "#4974E2", 6);
 }
 else
 {
 memcpy(indexPage+340, "#4974E2", 6);
 memcpy(indexPage+431, "#FFFF00", 6);
 }
 }


 {
 len = SPI_Ethernet_putConstString(httpHeader);
 len += SPI_Ethernet_putConstString(httpMimeTypeHTML);
 len += SPI_Ethernet_putString(indexPage);
 return len;
 }



}





unsigned int SPI_Ethernet_UserUDP(unsigned char *remoteHost, unsigned int remotePort, unsigned int destPort, unsigned int reqLength, TEthPktFlags *flags)

{
 return 0;

}








void Mac_reaD(void)
{

 bank_3select();

  LATGbits.LATG0=0; ;
 SPI1_Write( 0x01 | 0b00000000 );

 SPI1_Write(temp);
  LATGbits.LATG0=1; ;

  LATGbits.LATG0=0; ;
 SPI1_Write( 0x00 | 0b00000000 );
 SPI1_Write(temp);
  LATGbits.LATG0=1; ;

  LATGbits.LATG0=0; ;
 SPI1_Write( 0x03 | 0b00000000 );
 SPI1_Write(temp);
  LATGbits.LATG0=1; ;


  LATGbits.LATG0=0; ;
 SPI1_Write( 0x02 | 0b00000000 );
 SPI1_Write(temp);
  LATGbits.LATG0=1; ;

  LATGbits.LATG0=0; ;
 SPI1_Write( 0x05 | 0b00000000 );
 SPI1_Write(temp);
  LATGbits.LATG0=1; ;

  LATGbits.LATG0=0; ;
 SPI1_Write( 0x04 | 0b00000000 );
 SPI1_Write(temp);
  LATGbits.LATG0=1; ;


 }

void main(void)
{

 int_mcu ();
 initialize_spi1_module();
 SPI_Set_Active(SPI1_Read, SPI1_Write);
 SPI_Ethernet_Init(MACAddr, IPAddr,  0x01 );
 SPI_Ethernet_confNetwork(ipMask, gwIpAddr, dnsIpAddr);
 SPI_Ethernet_Enable(_SPI_Ethernet_CRC | _SPI_Ethernet_UNICAST |_SPI_Ethernet_BROADCAST);

 I2C2_init(100000);
 EXPANDER_init(0x00, 0xFF, 0xFF);

 while(1)
 {

 SPI_Ethernet_doPacket();


 }


}
