
#include <stdint.h>
#include "__EthEnc28j60.h"


#define SPI_Ethernet_HALFDUPLEX     0x00
#define SPI_Ethernet_FULLDUPLEX     0x01

//-----------------------W/R Control bytes--------------------------------------
#define Chip1W      0x40  //0b0100 + 0b000(Slave address) + 0b0(Write) => 0x40
#define Chip2W      0x42  //0b0100 + 0b001(Slave address) + 0b0(Write) => 0x42
//-----------------------I/O Direction Registers(TRIS)--------------------------
#define IODIRA     0x00
#define IODIRB     0x01
#define IODIRC     0x00 // this is related to 23008
//-----------------------Output Latches-----------------------------------------
#define OLATA      0x14   //Writing to the OLATn register Writes to the port.

//--------------------------------------------------------------------------------  -

unsigned short normallyOpenClose, DOVAL, tempDO;
unsigned short  NI2CD;
void EXPANDER_write(char chipxx, char portAddr, char dat){
        I2C2_Start();
        //while(I2C2_Is_Idle());
        I2C2_Write(chipxx);
        I2C2_Write(portAddr);
        I2C2_Write(dat);
        I2C2_Stop();
}
char EXPANDER_read(char chipxx, char portAddr) {
        char dat;
        I2C2_Start();
        //while(I2C2_Is_Idle());
        I2C2_Write(chipxx);
        I2C2_Write(portAddr);
        I2C2_restart();
        I2C2_Write(chipxx+1);
        dat = I2C2_Read(1);
        I2C_Stop();
        return dat;
}
void EXPANDER_init(char portA, char portB, char portC){
        EXPANDER_write           (Chip1W, IODIRA, portA);
        EXPANDER_write           (Chip1W, IODIRB, portB);
        EXPANDER_write           (Chip2W, IODIRC, portC);
        EXPANDER_write           (Chip2W, 0x01 ,  0xFF );
        } // Input Polarity

//------------------Interrupt Initialize----------------------------------------
void INIT1_interrupt(){
        EXPANDER_write(Chip2W, 0x02, 0xFF ); //GPINTEN  change on interrupt
        EXPANDER_write(Chip2W, 0x03, 0x00 ); //DEFVAL  default value
        EXPANDER_write(Chip2W, 0x04, 0x00 );
        } //INTCON is to compare values

//-----------------------------Init Section-------------------------------------
void I2C_int1Chip2() iv IVT_ADDR_INT1INTERRUPT {
        //  LATE.b8=0;
        NI2CD=1;
        // T9CON=0x0030;
        // TMR9=0;
        // T9CON = 0x8030;
        /*while (I2C2_Is_Idle());
        LATE.b5=1;
        DIVAL = EXPANDER_read(Chip2W, 0x08);  // don't need it here
        EXPANDER_write(Chip1W, OLATA, DIVAL);*/

        // LATE.b8=1;
        IFS1.INT1IF = 0;
}


//global variables
unsigned char MACAddr[6] = {0x00, 0x14, 0xa5, 0x76, 0x19, 0x3f} ;   //my MAC adress
unsigned char IpAddr[4] = {192,168,0,150};  // Static Ip adress privided to ENC28J60
unsigned char getRequest[10];


char   gwIpAddr[4]  = {192, 168, 0,1};                         // static gateway IP address of the router
char   dnsIpAddr[4] = {192, 168, 0,10};                         // static dns IP address
char   ipMask[4]    = {255, 255, 255, 255};                         // static subnet mask





#define ECON1           0x1f


//commands
#define WCRCMD          0b01000000                                 // write control register
#define RCRCMD          0b00000000                                // read control register
#define BFCCMD          0b10100000                               // bit field clear
#define BFSCMD          0b10000000                              // bit field set


int jll;
// To select and deselect an ENC28J60
#define Select()           LATGbits.LATG0=0;                   //making cs=0, to start communication
#define Deselect()         LATGbits.LATG0=1;                      //making cs=1, to stop communication



//mac address
 #define MAADR0  0x01
 #define MAADR1  0x00
 #define MAADR2  0x03
 #define MAADR3  0x02
 #define MAADR4  0x05
 #define MAADR5  0x04

 char maadr0=0x01;
 char maadr1=0x02;
 char maadr2=0x03;
 char maadr3=0x04;
 char maadr4=0x05;
 char maadr5=0x06;


const char httpHeader[] = "HTTP/1.1 200 OK\nContent-type: "; // HTTP header
const char httpMimeTypeHTML[] = "text/html\n\n";             // HTML MIME type
const char httpMimeTypeScript[] = "text/plain\n\n";          // TEXT MIME type


// default html page
char    indexPage[] =
"<html><head><title>mikroElektronika</title></head><body>\
<h3 align=center>MikroElektronika Home Automatization System</h3>\
<form name=\"input\" action=\"/\" method=\"get\">\
<table align=center width=200 bgcolor=#4974E2 border=2><tr>\
<td align=center colspan=2><font size=4 color=white><b>Heat Control</b></font>\
</td></tr><tr><td align=center bgcolor=#4974E2><input name=\"tst1\" width=60 \
type=\"submit\" value=\"ON\"></td><td align=center bgcolor=#FFFF00>\
<input name=\"tst2\" type=\"submit\" value=\"OFF\"></td></tr></table>\
</form></body></html>";


 void bank_3select (void)
        {
         Select() ;
         SPI1_Write(ECON1|BFCCMD ) ;
         SPI1_Write(0b11111111) ;
         Deselect() ;

         Select() ;
         SPI1_Write( ECON1| BFSCMD ) ;
         SPI1_Write(0b00000011) ;
         Deselect() ;


        }

sbit SPI_Ethernet_CS at LATG0_bit;
sbit SPI_Ethernet_CS_Direction at TRISG0_bit;

sbit SPI_Ethernet_Rst at LATG1_bit;
sbit SPI_Ethernet_Rst_Direction at TRISG1_bit;



//Discription: oscillator confuguration

void int_mcu(void)
        {
                OSCCON=0x3E0;
                CLKDIV=0x006;
                PLLFBD=278;
                ANSELD=0x0000;
                ANSELA=0x0000;
        }




//SPI1 configuration

void SPI1_pin_config (void)
      {
       //setup for direction control

      TRISG.b0= 0;                       //cs pin , as an output

      TRISGbits.TRISG14=0;          //SO, serial data output
      TRISEbits.TRISE0= 0;         //Sck, serial clock out
      TRISEbits.TRISE1=1;          //SI, serial data input


      TRISGbits.TRISG1=0;       // reset to ENC28J60

      //setup for ,as digital pins

       ANSELG.b0= 0;   // configuring cs pin as a digital
       
       //caution: This bit ust be 0 all the time.
       ANSELEbits.ANSE1=0; //makng SDI pin as a digital


      }




// Mapping the SPI1 module for ENC28J60

void map_io(void)
     {
                Unlock_IOLOCK();
                PPS_Mapping_NoLock(126, _OUTPUT, _SDO1);     // map pin 95 as a SDO1
                PPS_Mapping_NoLock(80, _OUTPUT, _SCK1OUT);  // map pin 93 as SCK1
                PPS_Mapping_NoLock(81, _INPUT,  _SDI1);     // map pin 94 as a SDI1
                       //PPS_Mapping_NoLock(89, _INPUT, _INT1);
                        Lock_IOLOCK();
        }





//Discription: Coniguration of SPI in MODE-00, as ENC28J60 need MODE-0 configuration to work with

void SPI1_regi_config (void)
        {
        SPI1CON1 = 0x0120; // (120) device frequency= 140Mhz, FCY=Fosc/2=70Mhz,SPI freuency = FCY/( primary * secondry prescale )= 16Mhz/ (4:1 * 1:1) =17.5Mhz
        SPI1STAT = 0x8000; // SPIx module is enabled and configures the SCK1, SDO1, SDI1 and SS1 pins as serial port pins

        SPI1CON2 = 0x0000; //defualt value
        }


// putting all function of SPI1 togather to initialize SPI1 module

void initialize_spi1_module(void)
        {
            SPI1_pin_config();
            map_io();
            SPI1_regi_config();
        }


unsigned int  SPI_Ethernet_UserTCP(unsigned char *remoteHost,
                                    unsigned int remotePort,
                                    unsigned int localPort, unsigned int reqLength, TEthPktFlags *flags)
{

            unsigned int len;                            // my reply length
            if(localPort != 80)
            return(0);             // I listen only to web request on port 80




         //get 10 first bytes only of the request, the rest does not matter here
          for(len = 0; len<15;len++)
                      {
                              getRequest[len] = SPI_Ethernet_getByte();
                      }
             getRequest[len] = 0;

        len=0;


         if(memcmp(getRequest, "GET /", 5))
                  {
                          return(0);  // only GET method
                   }

         if(!memcmp(getRequest+11, "ON", 2))        // do we have ON command
                 {
                       EXPANDER_write(Chip1W, 0x14, 0x80);
                  }


            else if(!memcmp(getRequest+11, "OFF", 3))
        {
                            {                     // do we have OFF command
                                 EXPANDER_write(Chip1W, 0x14, 0x00);
                            }

                  if (PORTB.F0)
                            {
                                     memcpy(indexPage+340, "#FFFF00", 6);        // highlight (yellow) ON
                                     memcpy(indexPage+431, "#4974E2", 6);        // clear OFF
                            }
                 else
                            {
                                     memcpy(indexPage+340, "#4974E2", 6);        // clear ON
                                      memcpy(indexPage+431, "#FFFF00", 6);        // highlight (yellow) OFF
                           }
        }


                        {
  len =  SPI_Ethernet_putConstString(httpHeader);         // HTTP header
  len += SPI_Ethernet_putConstString(httpMimeTypeHTML);  // with HTML MIME type
  len += SPI_Ethernet_putString(indexPage);                           // HTML page first part
  return len; // return to the library with the number of bytes to transmit
                        }



}



// user UDP function, not using this so returnig zero

unsigned int  SPI_Ethernet_UserUDP(unsigned char *remoteHost, unsigned int remotePort, unsigned int destPort, unsigned int reqLength, TEthPktFlags *flags)

{
  return 0; // back to the library with the length of the UDP reply

}








void Mac_reaD(void)
{
//code lines below is used to read MAC adress form MAADRXx 0f bank 03
                 bank_3select();
               //reading back MAC address
                  Select();
                  SPI1_Write(MAADR0|RCRCMD);

                  SPI1_Write(temp);
                  Deselect();

                  Select();
                  SPI1_Write(MAADR1|RCRCMD);
                  SPI1_Write(temp);
                  Deselect();

                  Select();
                  SPI1_Write(MAADR2|RCRCMD);
                  SPI1_Write(temp);
                  Deselect();


                  Select();
                  SPI1_Write(MAADR3|RCRCMD);
                  SPI1_Write(temp);
                  Deselect();

                  Select();
                  SPI1_Write(MAADR4|RCRCMD);
                  SPI1_Write(temp);
                  Deselect();

                  Select();
                  SPI1_Write(MAADR5|RCRCMD);
                  SPI1_Write(temp);
                  Deselect();


 }

void main(void)
{

        int_mcu ();
        initialize_spi1_module();
        SPI_Set_Active(SPI1_Read, SPI1_Write);
        SPI_Ethernet_Init(MACAddr, IPAddr, SPI_Ethernet_FULLDUPLEX);
        SPI_Ethernet_confNetwork(ipMask, gwIpAddr, dnsIpAddr);
        SPI_Ethernet_Enable(_SPI_Ethernet_CRC | _SPI_Ethernet_UNICAST |_SPI_Ethernet_BROADCAST);

        I2C2_init(100000);                    // Give speed in KBps
        EXPANDER_init(0x00, 0xFF, 0xFF);     //  PortA all Outputs, PortB all Inputs.

         while(1)
         {

                SPI_Ethernet_doPacket();


         }


}