/*
 * Project Name:
     ethV2 Demo (Ethernal clock : LCD clock that gets date & time from the internet)
 * Target Platform:
     dsPIC
 * Copyright:
     (c) mikroElektronika, 2012.
 * Revision History:
     20091030:
       - Initial release. Author: Bruno Gavand.
 *
 * description  :
 *      this program intends to show new ethernet library possibilities.
 *      it is an LCD clock with no buttons : it gets date & time from the internet with SNTP requests.
 *      the clock embeds a web server to get information on its state, and to set preferences.
 *      preferences are set on admin page. username: admin
 *                                         password: clock
 *      enc IP address can be set manually or obtained from dhcp server if such exists.
 *
 *
 *
 *      RF0 : !RESET    to ENC reset input pin
 *      RF1 : !CS       to ENC chip select input pin
 *      the ENC28J60 SPI bus CLK, SO, SI must be connected to the corresponding SPI pins of the dsPIC
 *      the INT and WOL signals from the ENC are not used
 *
 * Test configuration:
     MCU:             PIC24EP512GU810
                      http://ww1.microchip.com/downloads/en/DeviceDoc/70616g.pdf
     Dev.Board:       EasyPIC Fusion v7
                      http://www.mikroe.com/easypic-fusion/
     Oscillator:      XT-PLL, 80.00000 MHz
     Ext. Modules:    mE ac:Serial_Ethernet board
     SW:              mikroC PRO for dsPIC
                      http://www.mikroe.com/mikroc/dspic/
 * NOTES:
     - Since the ENC28J60 doesn't support auto-negotiation, full-duplex mode is
       not compatible with most switches/routers.  If a dedicated network is used
       where the duplex of the remote node can be manually configured, you may
       change this configuration.  Otherwise, half duplex should always be used.
     - Use SmartADAPT board to connect Serial Ethernet board to EasyPIC Fusion v7 development system
       http://www.mikroe.com/add-on-boards/various/smartadapt2/
     - External power supply should be used due to Serial Ethernet Board power consumption.   
     - Connect LCD over Serial LCD Adapter Board
       http://www.mikroe.com/add-on-boards/display/serial-lcd-adapter/
 */

#include "built_in.h"
#include "timelib.h"

#include "__EthEnc28j60.h"
#include "httpUtils.h"

// mE ehternet NIC pinout
sfr sbit SPI_Ethernet_Rst at LATF0_bit;  // for writing to output pin always use latch
sfr sbit SPI_Ethernet_CS  at LATF1_bit;  // for writing to output pin always use latch
sfr sbit SPI_Ethernet_Rst_Direction at TRISF0_bit;
sfr sbit SPI_Ethernet_CS_Direction  at TRISF1_bit;
// end ethernet NIC definitions

// Port Expander module connections
sbit SPExpanderRST at LATG0_bit;
sbit SPExpanderCS  at LATG1_bit;
sbit SPExpanderRST_Direction at TRISG0_bit;
sbit SPExpanderCS_Direction  at TRISG1_bit;
// End Port Expander module connections

/*
 * select either dynamic (with dhcp) network configuration, or static configuration
 */
// #define WITH_DHCP

/*
 * ts2str() mode flags
 */
#define TS2STR_DATE     1
#define TS2STR_TIME     2
#define TS2STR_TZ       4
#define TS2STR_ALL      (TS2STR_DATE | TS2STR_TIME)

/*
 * number of byte to parse in HTTP request
 */
#define HTTP_REQUEST_SIZE       128

/*
 * basic realm : admin private zone
 */
#define PRIVATE_LOGINPASSWD     "admin:clock"                           // user + password
#define ZONE_NAME               "Ethernal Clock administration"         // zone signature
#define MSG_DENIED              "Authorization Required"                // access denied message

/***********************************
 * RAM variables
 */
unsigned char   macAddr[6] = {0x00, 0x14, 0xA5, 0x76, 0x19, 0x3f};   // my MAC address

#ifdef WITH_DHCP
/*
 * if you have a DHCP server, ip address 0.0.0.0 means DCHP request,
 * otherwise, set your IP address here :
 */
unsigned char   ipAddr[4] = {0, 0, 0, 0};

/*
 * if you don't have a DHCP server, put your network settings here,
 * otherwise, leave them blank :
 */
unsigned char gwIpAddr[4];          // gateway (router) IP address
unsigned char ipMask[4];            // network mask (for example : 255.255.255.0)
unsigned char dnsIpAddr[4];         // DNS server IP address
#else
unsigned char ipAddr[4]  = {10, 101, 14, 52};                              // my IP address
unsigned char gwIpAddr[4]  = {10, 101, 14, 1};                               // gateway (router) IP address
unsigned char ipMask[4]    = {255, 255, 255, 0};                             // network mask (for example : 255.255.255.0)
unsigned char dnsIpAddr[4] = {10, 101, 14, 99};                              // DNS server IP address
#endif


TimeStruct      ts, ls;                // timestruct for now and last update
long    epoch = 0;                     // unix time now
long    lastSync = 0;                  // unix time of last sntp update
unsigned long   sntpTimer = 0;         // sntp response timer
unsigned int    presTmr = 0;           // timer prescaler

unsigned char   bufInfo[200];          // LCD buffer
unsigned char   *marquee = 0;          // marquee pointer
unsigned char   lcdEvent = 0;          // marquee event flag
unsigned int    lcdTmr   = 0;          // marquee timer

unsigned char   sntpSync = 0;          // sntp sync flag
unsigned char   reloadDNS = 1;         // dns up to date flag
unsigned char   serverStratum = 0;     // sntp server stratum
unsigned char   serverFlags = 0;       // sntp server flags
char            serverPrecision = 0;   // sntp server precision

/*
 * week day names
 */
unsigned char   *wday[] =
        {
        "Mon",
        "Tue",
        "Wed",
        "Thu",
        "Fri",
        "Sat",
        "Sun"
        };

/*
 * month names
 */
unsigned char   *mon[] =
        {
        "",     // skip number zero, time library counts months from 1 to 12
        "Jan",
        "Feb",
        "Mar",
        "Apr",
        "May",
        "Jun",
        "Jul",
        "Aug",
        "Sep",
        "Oct",
        "Nov",
        "Dec"
        };

unsigned int  httpCounter = 0;                         // number of http requests
unsigned char path_private[]    = "/admin";            // private zone path name

const unsigned char httpHeader[] = "HTTP/1.1 200 OK\nContent-type: ";  // HTTP header
const unsigned char httpMimeTypeHTML[] = "text/html\n\n";              // HTML MIME type
const unsigned char httpMimeTypeScript[] = "text/plain\n\n";           // TEXT MIME type

/*
 * configuration structure
 */
struct
        {
        unsigned char   lcdL1;                 // LCD line 1 message type
        unsigned char   lcdL2;                 // LCD line 2 message type
        short           tz;                    // time zone (diff in hours to GMT)
        unsigned char   sntpIP[4];             // SNTP ip address
        unsigned char   sntpServer[128];       // SNTP host name
        } conf =
                {
                0,
                2,
                0,
                {0, 0, 0, 0},
                "swisstime.ethz.ch"             // Zurich, Switzerland: Integrated Systems Lab, Swiss Fed. Inst. of Technology
                                                // 129.132.2.21: swisstime.ethz.ch
                                                // Service Area: Switzerland and Europe
                };

/*
 * LCD message type options
 */
const unsigned char   *LCDoption[] =
        {
        "Marquee",
        "Date",
        "Time",
        "Off"
        };

/*
 * stylesheets
 */
// clock not synchronized : red background
const char    *CSSred = "\
HTTP/1.1 200 OK\nContent-type: text/css\n\n\
body {background-color: #ffccdd;}\
";

// clock synchronized : green background
const char    *CSSgreen = "\
HTTP/1.1 200 OK\nContent-type: text/css\n\n\
body {background-color: #ddffcc;}\
";

/*
 * HTTP + HTML common header
 */
const char    *HTMLheader = "\
HTTP/1.1 200 OK\nContent-type: text/html\n\n\
<HTML><HEAD>\
<TITLE>MikroElektronika Ethernal Clock</TITLE>\
</HEAD><BODY>\
<link rel=\"stylesheet\" type=\"text/css\" href=\"/s.css\">\
<center>\
<h2>Ethernal Clock</h2>\
";

/*
 * Time info
 */
const char      *HTMLtime = "\
<h3>Time | <a href=/2>SNTP</a> | <a href=/3>Network</a> | <a href=/4>System</a> | <a href=/admin>ADMIN</a></h3>\
<script src=/a></script>\
<table border=1 style=\"font-size:20px ;font-family: terminal ;\" width=500>\
<tr><td>Date and Time</td><td align=right><script>document.write(NOW)</script></td></tr>\
<tr><td>Unix Epoch</td><td align=right><script>document.write(EPOCH)</script></td></tr>\
<tr><td>Julian Day</td><td align=right><script>document.write(EPOCH / 24 / 3600 + 2440587.5)</script></td></tr>\
<tr><td>Last sync</td><td align=right><script>document.write(LAST)</script></td></tr>";

/*
 * SNTP info
 */
const char      *HTMLsntp = "\
<h3><a href=/>Time</a> | SNTP | <a href=/3>Network</a> | <a href=/4>System</a> | <a href=/admin>ADMIN</a></h3>\
<script src=/b></script>\
<table border=1 style=\"font-size:20px ;font-family: terminal ;\" width=500>\
<tr><td>Server</td><td align=right><script>document.write(SNTP)</script></td></tr>\
<tr><td>Leap</td><td align=right><script>document.write(LEAP)</script></td></tr>\
<tr><td>Version</td><td align=right><script>document.write(VN)</script></td></tr>\
<tr><td>Mode</td><td align=right><script>document.write(MODE)</script></td></tr>\
<tr><td>Stratum</td><td align=right><script>document.write(STRATUM)</script></td></tr>\
<tr><td>Precision</td><td align=right><script>document.write(Math.pow(2, PRECISION - 256))</script></td></tr>\
";

/*
 * Network info
 */
const char      *HTMLnetwork = "\
<h3><a href=/>Time</a> | <a href=/2>SNTP</a> | Network | <a href=/4>System</a> | <a href=/admin>ADMIN</a></h3>\
<script src=/c></script>\
<table border=1 style=\"font-size:20px ;font-family: terminal ;\" width=500>\
<tr><td>Clock IP</td><td align=right><script>document.write(IP)</script></td></tr>\
<tr><td>Clock MAC</td><td align=right><script>document.write(MAC)</script></td></tr>\
<tr><td>Network Mask</td><td align=right><script>document.write(MASK)</script></td></tr>\
<tr><td>Gateway</td><td align=right><script>document.write(GW)</script></td></tr>\
<tr><td>DNS</td><td align=right><script>document.write(DNS)</script></td></tr>\
<tr><td>Your IP</td><td align=right><script>document.write(CLIENT)</script></td></tr>";

/*
 * System info
 */
const char      *HTMLsystem = "\
<h3><a href=/>Time</a> | <a href=/2>SNTP</a> | <a href=/3>Network</a> | System | <a href=/admin>ADMIN</a></h3>\
<script src=/d></script>\
<table border=1 style=\"font-size:20px ;font-family: terminal ;\" width=500>\
<tr><td>Ethernet device</td><td align=right><script>document.write(SYSTEM)</script></td></tr>\
<tr><td>Fosc</td><td align=right><script>document.write(CLK/1000)</script> Mhz</td></tr>\
<tr><td>Up Since</td><td align=right><script>document.write(UP)</script></td></tr>\
<tr><td>HTTP Request #</td><td align=right><script>document.write(REQ)</script></td></tr>";

/*
 * ADMIN section (private zone)
 */
const char      *HTMLadmin = "\
<h3><a href=/>Time</a> | <a href=/2>SNTP</a> | <a href=/3>Network</a> | <a href=/4>System</a> | ADMIN</h3>\
<script src=/admin/s></script>\
<table border=1 style=\"font-size:20px ;font-family: terminal ;\" width=500>\
<tr><td>SNTP Server</td><td align=right><script>document.write(SERVER)</script></td></tr>\
<tr><td>Update clock from SNTP</td><td align=right><a href=/admin/r>now</a></td></tr>\
<tr><td>Time Zone</td><td align=right>GMT <script>document.write(TZ)</script></td></tr>\
<tr><td>LCD Line 1</td><td align=right><script>document.write(LCD1)</script></td></tr>\
<tr><td>LCD Line 2</td><td align=right><script>document.write(LCD2)</script></td></tr>\
";

/*
 * HTML common footer
 */
const   char    *HTMLfooter = "<HTML><HEAD>\
</table>\
<br>\
See more details on <a href=http://www.micro-examples.com target=_blank>www.micro-examples.com</a>\
<br>\
<a href=http://www.mikroe.com target=_blank>www.mikroe.com</a>\
</center>\
</BODY></HTML>";

/*******************************************
 * functions
 */


/*
 * LCD marquee
 */
void    mkMarquee(unsigned char l)
        {
        unsigned char   len;
        char            marqeeBuff[17];

        if((*marquee == 0) || (marquee == 0))
                {
                marquee = bufInfo;
                }
        if((len=strlen(marquee)) < 16) {
          memcpy(marqeeBuff, marquee, len);
          memcpy(marqeeBuff+len, bufInfo, 16-len);
        }
        else
          memcpy(marqeeBuff, marquee, 16);
        marqeeBuff[16] = 0;

        SPI_Set_Active(SPI3_Read, SPI3_Write);
        SPI_Lcd_Out(l, 1, marqeeBuff);
        }
        
/*
 * put the constant string pointed to by s to the ENC transmit buffer.
 */
/*unsigned int    putConstString(const char *s)
        {
        unsigned int ctr = 0;

        while(*s)
                {
                SPI_Ethernet_putByte(*s++);
                ctr++;
                }
        return(ctr);
        } */
/*
 * it will be faster to use library SPI_Ethernet_putConstString routine
 * instead of putConstString routine above. However, the code will be a little
 * bit bigger. User should choose between size and speed and pick the implementation that
 * suites him best. If you choose to go with the putConstString definition above
 * the #define line below should be commented out.
 *
 */
#define putConstString  SPI_Ethernet_putConstString

/*
 * put the string pointed to by s to the ENC transmit buffer
 */
/*unsigned int    putString(char *s)
        {
        unsigned int ctr = 0;

        while(*s)
                {
                SPI_Ethernet_putByte(*s++);

                ctr++;
                }
        return(ctr);
        }*/
/*
 * it will be faster to use library SPI_Ethernet_putString routine
 * instead of putString routine above. However, the code will be a little
 * bit bigger. User should choose between size and speed and pick the implementation that
 * suites him best. If you choose to go with the putString definition above
 * the #define line below should be commented out.
 *
 */
#define putString  SPI_Ethernet_putString

/*
 * convert l (signed) to ascii string into s, no leading spaces
 */
void    int2str(long l, unsigned char *s)
        {
        unsigned char   i, j, n;

        if(l == 0)
                {
                s[0] = '0';
                s[1] = 0;
                }
        else
                {
                if(l < 0)
                        {
                        l *= -1;
                        n = 1;
                        }
                else
                        {
                        n = 0;
                        }
                s[0] = 0;
                i = 0;
                while(l > 0)
                        {
                        for(j = i + 1; j > 0; j--)
                                {
                                s[j] = s[j - 1];
                                }
                        s[0] = l % 10;
                        s[0] += '0';
                        i++;
                        l /= 10;
                        }
                if(n)
                        {
                        for(j = i + 1; j > 0; j--)
                                {
                                s[j] = s[j - 1];
                                }
                        s[0] = '-';
                        }
                }
        }

/*
 * convert ip to ascii string into s
 */
void    ip2str(unsigned char *s, unsigned char *ip)
        {
        unsigned char i;
        unsigned char buf[4];

        *s = 0;
        for(i = 0; i < 4; i++)
                {
                int2str(ip[i], buf);
                strcat(s, buf);
                if(i != 3)
                  strcat(s, ".");
                }
        }

/*
 * convert time struct to t to ascii into s
 * m is mode
 */
void    ts2str(unsigned char *s, TimeStruct *t, unsigned char m)
        {
        unsigned char  tmp[6];

        /*
         * convert date members
         */
        if(m & TS2STR_DATE)
                {
                strcpy(s, wday[t->wd]);        // week day
                strcat(s, " ");
                ByteToStr(t->md, tmp);         // day num
                strcat(s, tmp + 1);
                strcat(s, " ");
                strcat(s, mon[t->mo]);        // month
                strcat(s, " ");
                WordToStr(t->yy, tmp);         // year
                strcat(s, tmp + 1);
                strcat(s, " ");
                }
        else
                {
                *s = 0;
                }

        /*
         * convert time members
         */
        if(m & TS2STR_TIME)
                {
                ByteToStr(t->hh, tmp);         // hour
                strcat(s, tmp + 1);
                strcat(s, ":");
                ByteToStr(t->mn, tmp);         // minute
                if(*(tmp + 1) == ' ')
                        {
                        *(tmp + 1) = '0';
                        }
                strcat(s, tmp + 1);
                strcat(s, ":");
                ByteToStr(t->ss, tmp);         // second
                if(*(tmp + 1) == ' ')
                        {
                        *(tmp + 1) = '0';
                        }
                strcat(s, tmp + 1);
                }

        /*
         * convert time zone
         */
        if(m & TS2STR_TZ)
                {
                strcat(s, " GMT");
                if(conf.tz > 0)
                        {
                        strcat(s, "+");
                        }
                int2str(conf.tz, s + strlen(s));
                }
        }

/*
 * convert integer to hex char
 */
unsigned char nibble2hex(unsigned char n)
        {
        n &= 0x0f;
        if(n >= 0x0a)
                {
                return(n + '7');
                }
        return(n + '0');
        }

/*
 * convert byte to hex string
 */
void    byte2hex(unsigned char *s, unsigned char v)
        {
        *s++ = nibble2hex(v >> 4);
        *s++ = nibble2hex(v);
        *s = '.';
        }

/*
 * build select HTML tag with LCD options
 */
unsigned int    mkLCDselect(unsigned char l, unsigned char m)
        {
        unsigned char i;
        unsigned int len;
        
        len = putConstString("<select onChange=\\\"document.location.href = '/admin/");
        SPI_Set_Active(SPI1_Read, SPI1_Write);
        SPI_Ethernet_putByte('0' + l); len++;
        len += putConstString("/' + this.selectedIndex\\\">");
        for(i = 0; i < 4; i++)
                {
                len += putConstString("<option ");
                if(i == m)
                        {
                        len += putConstString(" selected");
                        }
                len += putConstString(">");
                len += putConstString(LCDoption[i]);
                }
        len += putConstString("</select>\";");
        return(len);
        }

/*
 * display line
 */
void mkLCDLine(unsigned char l, unsigned char m)
        {
        switch(m)
                {
                case 0:
                        // build marquee string
                        memset(bufInfo, 0, sizeof(bufInfo));
                        if(lastSync)
                                {
                                // clock is synchronized
                                strcpy(bufInfo, "Today is ");
                                ts2str(bufInfo + strlen(bufInfo), &ts, TS2STR_DATE);
                                strcat(bufInfo, ". Please visit www.micro-examples.com for more details about the Ethernal Clock. You can browse ");
                                ip2str(bufInfo + strlen(bufInfo), ipAddr);
                                strcat(bufInfo, " to set the clock preferences.    ");
                                }
                        else
                                {
                                // clock is not synchronized
                                strcpy(bufInfo, "The SNTP server did not respond, please browse ");
                                ip2str(bufInfo + strlen(bufInfo), ipAddr);
                                strcat(bufInfo, " to check clock settings.    ");
                                }
                        mkMarquee(l);           // display marquee
                        break;
                case 1:
                        // build date string
                        if(lastSync)
                                {
                                ts2str(bufInfo, &ts, TS2STR_DATE);
                                }
                        else
                                {
                                strcpy(bufInfo, "Date Not Ready !");
                                }
                        SPI_Set_Active(SPI3_Read, SPI3_Write);
                        SPI_Lcd_Out(l, 1, bufInfo);
                        break;
                case 2:
                        // build time string
                        if(lastSync)
                                {
                                ts2str(bufInfo, &ts, TS2STR_TIME);
                                SPI_Set_Active(SPI3_Read, SPI3_Write);
                                SPI_Lcd_Out(l, 4, bufInfo);
                                }
                        else
                                {
                                strcpy(bufInfo, "Time Not Ready !");
                                SPI_Set_Active(SPI3_Read, SPI3_Write);
                                SPI_Lcd_Out(l, 1, bufInfo);
                                }
                        break;
                }
        }

/*
 * make SNTP request
 */
void    mkSNTPrequest()
        {
        unsigned char sntpPkt[50];
        unsigned char * remoteIpAddr;

        SPI_Set_Active(SPI1_Read, SPI1_Write);
        if (sntpSync)
          if (SPI_Ethernet_UserTimerSec >= sntpTimer)
            if (!lastSync) {
              sntpSync = 0;
              if (!memcmp(conf.sntpIP, "\0\0\0\0", 4))
                reloadDNS = 1 ; // force to solve DNS
            }

        if(reloadDNS)   // is SNTP ip address to be reloaded from DNS ?
                {
                SPI_Set_Active(SPI3_Read, SPI3_Write);
                // yes, it can take a while, so display something
                SPI_Lcd_Out(2, 1, "Connecting...   ");

                if(isalpha(*conf.sntpServer))   // doest host name start with an alphabetic character ?
                        {
                        // yes, try to solve with DNS request
                             memset(conf.sntpIP, 0, 4);
                             SPI_Set_Active(SPI1_Read, SPI1_Write);
                              if(remoteIpAddr = SPI_Ethernet_dnsResolve(conf.sntpServer, 5))
                                {
                                // successful : save IP address
                                memcpy(conf.sntpIP, remoteIpAddr, 4);
                                }
                        }
                else
                        {
                        // host name is supposed to be an IP address, directly save it
                        unsigned char *ptr = conf.sntpServer;
                        
                        conf.sntpIP[0] = atoi(ptr);
                        ptr = strchr(ptr, '.') + 1;
                        conf.sntpIP[1] = atoi(ptr);
                        ptr = strchr(ptr, '.') + 1;
                        conf.sntpIP[2] = atoi(ptr);
                        ptr = strchr(ptr, '.') + 1;
                        conf.sntpIP[3] = atoi(ptr);
                        }

                reloadDNS = 0;         // no further call to DNS
                
                sntpSync = 0;          // clock is not sync for now
                }

        if(sntpSync)                    // is clock already synchronized from sntp ?
                {
                return;                // yes, no need to request time
                }

        /*
         * prepare buffer for SNTP request
         */
        memset(sntpPkt, 0, 48);        // clear sntp packet
        
        // FLAGS : byte 0
        sntpPkt[0] = 0b00011001;       // LI = 0; VN = 3; MODE = 1

        // STRATUM : byte 1 = 0

        // POLL : byte 2
        sntpPkt[2] = 0x0a;             // 1024 sec (arbitrary value)

        // PRECISION : byte 3
        sntpPkt[3] = 0xfa;             // 0.015625 sec (arbitrary value)

        // DELAY : bytes 4 to 7 = 0.2656 sec (arbitrary value)
        sntpPkt[6] = 0x44;

        // DISPERSION : bytes 8 to 11 = 16 sec (arbitrary value)
        sntpPkt[9] = 0x10;

        // REFERENCE ID : bytes 12 to 15 = 0 (unspecified)

        // REFERENCE TIMESTAMP : bytes 16 to 23 (unspecified)

        // ORIGINATE TIMESTAMP : bytes 24 to 31 (unspecified)

        // RECEIVE TIMESTAMP : bytes 32 to 39 (unspecified)

        // TRANSMIT TIMESTAMP : bytes 40 to 47 (unspecified)

        SPI_Set_Active(SPI1_Read, SPI1_Write);
        SPI_Ethernet_sendUDP(conf.sntpIP, 123, 123, sntpPkt, 48); // transmit UDP packet
        
        sntpSync = 1;  // done
        lastSync = 0;
        SPI_Set_Active(SPI1_Read, SPI1_Write);
        sntpTimer = SPI_Ethernet_UserTimerSec + 5;
        }

/*
 * incoming TCP request
 */
unsigned int    SPI_Ethernet_UserTCP(unsigned char *remoteHost, unsigned int remotePort, unsigned int localPort, unsigned int reqLength, TEthPktFlags *flags)
        {
        unsigned char   dyna[64];
        unsigned char   getRequest[HTTP_REQUEST_SIZE + 1];

        unsigned int    len = 0;
        int    i;
        unsigned char   admin;

        // should we close tcp socket after response is sent?
        // library closes tcp socket by default if canCloseTCP flag is not reset here
        // flags->canCloseTCP = 0; // 0 - do not close socket
                                   // otherwise - close socket

        if(localPort != 80)                     // I listen only to web request on port 80
                {
                return(0);                     // return without reply
                }

        /*
         * parse TCP frame and check for a GET request
         */
        SPI_Set_Active(SPI1_Read, SPI1_Write);
        if(HTTP_getRequest(getRequest, &reqLength, HTTP_REQUEST_SIZE) == 0)
                {
                return(0);                     // no reply if no GET request
                }

        /*
         * parse TCP frame and try to find basic realm authorization
         */
        SPI_Set_Active(SPI1_Read, SPI1_Write);
        admin = HTTP_basicRealm(reqLength, PRIVATE_LOGINPASSWD); // admin is set if login+password matches, cleared otherwise

        if(memcmp(getRequest, path_private, sizeof(path_private) - 1) == 0)   // is path under private section ?
                {
                unsigned char   *ptr;
                
                // yes, points to sub path
                ptr = getRequest + sizeof(path_private) - 1;

                // check if access is granted
                if(admin == 0)
                        {
                        SPI_Set_Active(SPI1_Read, SPI1_Write);
                        // no, reply with access denied message
                        len = HTTP_accessDenied(ZONE_NAME, MSG_DENIED);
                        }
                else
                        {
                        // yes, parse request
                        if(getRequest[sizeof(path_private)] == 's')
                                {
                                // request for javascript variables
                                len = putConstString(httpHeader);              // HTTP header
                                len += putConstString(httpMimeTypeScript);     // with script MIME type

                                // LCD line 1 select + options
                                len += putConstString("var LCD1=\"");
                                len += mkLCDselect(1, conf.lcdL1);

                                // LCD line 2 select + options
                                len += putConstString("var LCD2=\"");
                                len += mkLCDselect(2, conf.lcdL2);

                                // SNTP host name
                                len += putConstString("var SERVER=\"");
                                len += putConstString("<input onChange=\\\"document.location.href = '/admin/n/' + this.value\\\" value=\\\"");
                                len += putString(conf.sntpServer);
                                len += putConstString("\\\">\";");

                                // time zone select + options
                                len += putConstString("var TZ=\"");
                                len += putConstString("<select onChange=\\\"document.location.href = '/admin/t/' + this.selectedIndex\\\">");
                                for(i = -11; i < 12; i++)
                                        {
                                        len += putConstString("<option ");
                                        if(i == conf.tz)
                                                {
                                                len += putConstString(" selected");
                                                }
                                        len += putConstString(">");
                                        int2str(i, dyna);
                                        len += putString(dyna);
                                        }
                                len += putConstString("</select>\";");
                                }
                        else
                                {
                                // parse path to find parameters
                                switch(getRequest[sizeof(path_private)])
                                        {
                                        case '1' :
                                                SPI_Set_Active(SPI3_Read, SPI3_Write);
                                                // LCD line 1 selection
                                                SPI_Lcd_Cmd(_LCD_CLEAR);
                                                conf.lcdL1 = getRequest[sizeof(path_private) + 2] - '0';
                                                break;
                                        case '2':
                                                SPI_Set_Active(SPI3_Read, SPI3_Write);
                                                // LCD line 2 selection
                                                SPI_Lcd_Cmd(_LCD_CLEAR);
                                                conf.lcdL2 = getRequest[sizeof(path_private) + 2] - '0';
                                                break;
                                        case 'r':
                                                // force to renew SNTP request
                                                sntpSync = 0;
                                                if (!memcmp(conf.sntpIP, "\0\0\0\0", 4))
                                                  reloadDNS = 1 ; // force to solve DNS
                                                break;
                                        case 'n':
                                                {
                                                // sntp server host name
                                                unsigned char *src, *dst, i;

                                                src = &getRequest[sizeof(path_private) + 2];
                                                dst = conf.sntpServer;
                                                for(i = 0; i < 128; i++)
                                                        {
                                                        *dst = *src++;
                                                        if(*dst == ' ')
                                                                {
                                                                *dst = 0;
                                                                break;
                                                                }
                                                        dst++;
                                                        }
                                                reloadDNS = 1; // force to solve DNS
                                                }
                                                break;
                                        case 't':
                                                // time zone
                                                conf.tz = atoi(&getRequest[sizeof(path_private) + 2]);
                                                conf.tz -= 11;
                                                break;
                                        }

                                // reply to browser with admin HTML page
                                len += putConstString(HTMLheader);
                                len += putConstString(HTMLadmin);
                                len += putConstString(HTMLfooter);
                                }
                        }
                }
        else switch(getRequest[1])
                {
                // not in private zone, parse request path
                case 's':
                        // reply with stylesheet
                        if(lastSync == 0)
                                {
                                len = putConstString(CSSred);          // not sync
                                }
                        else
                                {
                                len = putConstString(CSSgreen);        // sync
                                }
                        break;
                case 'a':
                        // reply with clock info javascript variables
                        len = putConstString(httpHeader);
                        len += putConstString(httpMimeTypeScript);

                        // add date to reply
                        ts2str(dyna, &ts, TS2STR_ALL | TS2STR_TZ);
                        len += putConstString("var NOW=\"");
                        len += putString(dyna);
                        len += putConstString("\";");

                        // add epoch to reply
                        int2str(epoch, dyna);
                        len += putConstString("var EPOCH=");
                        len += putString(dyna);
                        len += putConstString(";");

                        // add last sync date
                        if(lastSync == 0)
                                {
                                strcpy(dyna, "???");
                                }
                        else
                                {
                                Time_epochToDate(lastSync + conf.tz * 3600, &ls);
                                ts2str(dyna, &ls, TS2STR_ALL | TS2STR_TZ);
                                }
                        len += putConstString("var LAST=\"");
                        len += putString(dyna);
                        len += putConstString("\";");

                        break;

                case 'b':
                        // reply with sntp info javascript variables
                        len = putConstString(httpHeader);
                        len += putConstString(httpMimeTypeScript);

                        // reply is made of the IP MASK in human readable format
                        ip2str(dyna, conf.sntpIP);
                        len += putConstString("var SNTP=\"");
                        len += putString(conf.sntpServer);
                        len += putConstString(" (");
                        len += putString(dyna);
                        len += putConstString(")");
                        len += putConstString("\";");

                        // add sntp stratum to reply
                        if(serverStratum == 0)
                                {
                                strcpy(dyna, "Unspecified");
                                }
                        else if(serverStratum == 1)
                                {
                                strcpy(dyna, "1 (primary)");
                                }
                        else if(serverStratum < 16)
                                {
                                int2str(serverStratum, dyna);
                                strcat(dyna, "(secondary)");
                                }
                        else
                                {
                                int2str(serverStratum, dyna);
                                strcat(dyna, " (reserved)");
                                }
                        len += putConstString("var STRATUM=\"");
                        len += putString(dyna);
                        len += putConstString("\";");

                        // add sntp flags to reply
                        switch(serverFlags & 0b11000000)
                                {
                                case 0b00000000: strcpy(dyna, "No warning"); break;
                                case 0b01000000: strcpy(dyna, "Last minute has 61 seconds"); break;
                                case 0b10000000: strcpy(dyna, "Last minute has 59 seconds"); break;
                                case 0b11000000: strcpy(dyna, "SNTP server not synchronized"); break;
                                }
                        len += putConstString("var LEAP=\"");
                        len += putString(dyna);
                        len += putConstString("\";");

                        int2str(serverPrecision, dyna);
                        len += putConstString("var PRECISION=\"");
                        len += putString(dyna);
                        len += putConstString("\";");

                        switch(serverFlags & 0b00111000)
                                {
                                case 0b00011000: strcpy(dyna, "IPv4 only"); break;
                                case 0b00110000: strcpy(dyna, "IPv4, IPv6 and OSI"); break;
                                default: strcpy(dyna, "Undefined"); break;
                                }
                        len += putConstString("var VN=\"");
                        len += putString(dyna);
                        len += putConstString("\";");

                        switch(serverFlags & 0b00000111)
                                {
                                case 0b00000000: strcpy(dyna, "Reserved"); break;
                                case 0b00000001: strcpy(dyna, "Symmetric active"); break;
                                case 0b00000010: strcpy(dyna, "Symmetric passive"); break;
                                case 0b00000011: strcpy(dyna, "Client"); break;
                                case 0b00000100: strcpy(dyna, "Server"); break;
                                case 0b00000101: strcpy(dyna, "Broadcast"); break;
                                case 0b00000110: strcpy(dyna, "Reserved for NTP control message"); break;
                                case 0b00000111: strcpy(dyna, "Reserved for private use"); break;
                                }
                        len += putConstString("var MODE=\"");
                        len += putString(dyna);
                        len += putConstString("\";");

                        break;

                case 'c':
                        // reply with network info javascript variables
                        len = putConstString(httpHeader);              // HTTP header
                        len += putConstString(httpMimeTypeScript);     // with text MIME type

                        // reply is made of the remote host IP address in human readable format
                        ip2str(dyna, ipAddr);
                        len += putConstString("var IP=\"");
                        len += putString(dyna);
                        len += putConstString("\";");

                        byte2hex(dyna, macAddr[0]);
                        byte2hex(dyna + 3, macAddr[1]);
                        byte2hex(dyna + 6, macAddr[2]);
                        byte2hex(dyna + 9, macAddr[3]);
                        byte2hex(dyna + 12, macAddr[4]);
                        byte2hex(dyna + 15, macAddr[5]);
                        *(dyna + 17) = 0;
                        len += putConstString("var MAC=\"");
                        len += putString(dyna);
                        len += putConstString("\";");

                        // reply is made of the remote host IP address in human readable format
                        ip2str(dyna, remoteHost);
                        len += putConstString("var CLIENT=\"");
                        len += putString(dyna);
                        len += putConstString("\";");

                        // reply is made of the ROUTER address in human readable format
                        ip2str(dyna, gwIpAddr);
                        len += putConstString("var GW=\"");
                        len += putString(dyna);
                        len += putConstString("\";");

                        // reply is made of the IP MASK in human readable format
                        ip2str(dyna, ipMask);
                        len += putConstString("var MASK=\"");
                        len += putString(dyna);
                        len += putConstString("\";");

                        // reply is made of the IP MASK in human readable format
                        ip2str(dyna, dnsIpAddr);
                        len += putConstString("var DNS=\"");
                        len += putString(dyna);
                        len += putConstString("\";");
                        
                        break;

                case 'd':
                        {
                        // reply with system info javascript variables
                        TimeStruct t;
                        
                        len = putConstString(httpHeader);              // HTTP header
                        len += putConstString(httpMimeTypeScript);     // with text MIME type

                        len += putConstString("var SYSTEM=\"ENC28J60\";");

                        int2str(Clock_kHz(), dyna);
                        len += putConstString("var CLK=\"");
                        len += putString(dyna);
                        len += putConstString("\";");

                        // add HTTP requests counter to reply
                        int2str(httpCounter, dyna);
                        len += putConstString("var REQ=");
                        len += putString(dyna);
                        len += putConstString(";");

                        SPI_Set_Active(SPI1_Read, SPI1_Write);
                        Time_epochToDate(epoch - SPI_Ethernet_UserTimerSec + conf.tz * 3600, &t);
                        ts2str(dyna, &t, TS2STR_ALL | TS2STR_TZ);
                        len += putConstString("var UP=\"");
                        len += putString(dyna);
                        len += putConstString("\";");


                        break;
                        }
                        
                case '4':
                        // reply with system info HTML page
                        len += putConstString(HTMLheader);
                        len += putConstString(HTMLsystem);
                        len += putConstString(HTMLfooter);
                        break;

                case '3':
                        // reply with network info HTML page
                        len += putConstString(HTMLheader);
                        len += putConstString(HTMLnetwork);
                        len += putConstString(HTMLfooter);
                        break;

                case '2':
                        // reply with sntp info HTML page
                        len += putConstString(HTMLheader);
                        len += putConstString(HTMLsntp);
                        len += putConstString(HTMLfooter);
                        break;

                case '1':
                default:
                        // reply with clock info HTML page
                        len += putConstString(HTMLheader);
                        len += putConstString(HTMLtime);
                        len += putConstString(HTMLfooter);
                }

        httpCounter++;                             // one more request done

        return(len);                               // return to the library with the number of bytes to transmit
        }

/*
 * incoming UDP request
 */
unsigned int    SPI_Ethernet_UserUDP(unsigned char *remoteHost, unsigned int remotePort, unsigned int destPort, unsigned int reqLength, TEthPktFlags *flags)
        {
        unsigned char   i;

        if(destPort == 123)             // check SNTP port number
                {
                unsigned long tts;

                serverFlags = SPI_Ethernet_getByte();
                serverStratum = SPI_Ethernet_getByte();
                SPI_Set_Active(SPI1_Read, SPI1_Write);
                SPI_Ethernet_getByte();        // skip poll
                serverPrecision = SPI_Ethernet_getByte();

                for(i = 0; i < 36; i++)
                        {
                        SPI_Set_Active(SPI1_Read, SPI1_Write);
                        SPI_Ethernet_getByte(); // skip all unused fileds
                        }

                // store transmit timestamp
                SPI_Set_Active(SPI1_Read, SPI1_Write);
                Highest(tts) = SPI_Ethernet_getByte();
                SPI_Set_Active(SPI1_Read, SPI1_Write);
                Higher(tts) = SPI_Ethernet_getByte();
                SPI_Set_Active(SPI1_Read, SPI1_Write);
                Hi(tts) = SPI_Ethernet_getByte();
                SPI_Set_Active(SPI1_Read, SPI1_Write);
                Lo(tts) = SPI_Ethernet_getByte();

                // convert sntp timestamp to unix epoch
                epoch = tts - 2208988800;

                // save last synchronization timestamp
                lastSync = epoch;

                // update display
                marquee = bufInfo;
                SPI_Set_Active(SPI3_Read, SPI3_Write);
                SPI_Lcd_Cmd(_LCD_CLEAR);
                }
        return(0);
        }

/*
 * ISR : triggered by timer 0 rollover
 */
void Timer1Int() iv IVT_ADDR_T1INTERRUPT
       {
            presTmr++ ;                        // increment prescaler
            if(presTmr == 5)                   // timer1 overflows 5 times per second
              {
                SPI_Set_Active(SPI1_Read, SPI1_Write);
                SPI_Ethernet_UserTimerSec++ ;  // increment ethernet library counter
                epoch++ ;                      // epoch is RTC counter
                presTmr = 0 ;                  // reset prescaler
              }
            lcdEvent = 1;                      // set marquee event
            T1IF_bit = 0 ;                     // clear timer0 overflow flag
        }

/*
 * main entry
 */
void    main()
        {
          PLLPRE_0_bit = 0;
          PLLPRE_1_bit = 0;
          PLLPRE_2_bit = 0;
          PLLPRE_3_bit = 0;
          PLLPRE_4_bit = 0;

          PLLFBD = 38;             // PLL multiplier M=38

          PLLPOST_0_bit = 0;
          PLLPOST_1_bit = 0;

          ANSELA = 0x00;    // Convert all I/O pins to digital
          ANSELB = 0x00;
          ANSELC = 0x00;
          ANSELD = 0x00;
          ANSELE = 0x00;
          ANSELG = 0x00;

          PPS_Mapping(65, _INPUT, _SDI1);
          PPS_Mapping(66, _OUTPUT, _SDO1);
          PPS_Mapping(67, _OUTPUT, _SCK1OUT);


          PPS_Mapping(71, _INPUT, _SDI3);
          PPS_Mapping(69, _OUTPUT, _SDO3);
          PPS_Mapping(70, _OUTPUT, _SCK3OUT);
        /*
         * configure TIMER 1 and interrupts
         */
        TMR1 = 0;
        PR1 = 31250;            // Set Period Register
        IPC0 = IPC0 | 0x1000;   // Interrupt priority level = 1
        T1CON = 0x8030;         // Timer1 ON, internal clock FCY, prescaler 1:128
        T1IF_bit = 0 ;          // clear TMR1 overflow interrupt flag
        T1IE_bit = 1 ;          // enable interrupt on TMR0 overflow

        /*
         * init LCD
         */
        SPI3_Init();

        SPI_Set_Active(SPI3_Read, SPI3_Write);
        SPI_Lcd_Config(0);                     // Initialize Lcd over SPI interface
        SPI_Lcd_Cmd(_LCD_CLEAR);
        SPI_Lcd_Cmd(_LCD_CURSOR_OFF);

        SPI_Lcd_Out(1, 1, "EthClock Welcome");
        SPI_Lcd_Out(2, 1, "Ethernet init...");

        /*
         * init Ethernet
         */
        SPI1_Init();
        // for faster SPI communication use Spi1_Init_Advanced settings
        //SPI1_Init_Advanced(_SPI_MASTER, _SPI_8_BIT, _SPI_PRESCALE_SEC_1, _SPI_PRESCALE_PRI_4,
        //                   _SPI_SS_DISABLE, _SPI_DATA_SAMPLE_MIDDLE, _SPI_CLK_IDLE_LOW, _SPI_IDLE_2_ACTIVE);
        SPI_Set_Active(SPI1_Read, SPI1_Write);
        SPI_Ethernet_Init(macAddr, ipAddr, SPI_Ethernet_FULLDUPLEX);

        /*
         * try to get an IP address
         */
        SPI_Set_Active(SPI3_Read, SPI3_Write);
        SPI_Lcd_Out(2, 1, "Registering...  ");
        
        SPI_Set_Active(SPI1_Read, SPI1_Write);
        if(SPI_Ethernet_getIpAddress()[0] == 0)        // is IP address null ?
                {
                  SPI_Set_Active(SPI1_Read, SPI1_Write);
                  while(SPI_Ethernet_initDHCP(5) == 0); // try to get one from DHCP until it works
                  /*
                   * read network parameters if neccessary
                   * by copying them into your globals,
                   * do NOT modify the contents of the locations pointed to
                   * by pointers which are returned by the below SPI_Ethernet_get routines
                   */
                  SPI_Set_Active(SPI1_Read, SPI1_Write);
                  memcpy(ipAddr,    SPI_Ethernet_getIpAddress(),    4); // get assigned IP address
                  SPI_Set_Active(SPI1_Read, SPI1_Write);
                  memcpy(ipMask,    SPI_Ethernet_getIpMask(),       4); // get assigned IP mask
                  SPI_Set_Active(SPI1_Read, SPI1_Write);
                  memcpy(gwIpAddr,  SPI_Ethernet_getGwIpAddress(),  4); // get assigned gateway IP address
                  SPI_Set_Active(SPI1_Read, SPI1_Write);
                  memcpy(dnsIpAddr, SPI_Ethernet_getDnsIpAddress(), 4); // get assigned dns IP address
                }
        else
                {
                SPI_Set_Active(SPI1_Read, SPI1_Write);
                SPI_Ethernet_confNetwork(ipMask, gwIpAddr, dnsIpAddr); // use configured IP address
                }

        while(1)                                // forever
                {
                /*
                 * if necessary, test the return value to get error code
                 */
                SPI_Set_Active(SPI1_Read, SPI1_Write);
                SPI_Ethernet_doPacket();       // process incoming Ethernet packets
                

                mkSNTPrequest();               // do sntp request if needed
                
#ifdef WITH_DHCP
                /*
                 * take care of the lease time
                 */
                SPI_Set_Active(SPI1_Read, SPI1_Write);
                if(SPI_Ethernet_doDHCPLeaseTime())
                        { // is the lease time expired
                        SPI_Set_Active(SPI3_Read, SPI3_Write);
                        SPI_Lcd_Out(2, 1, "DHCPrenewing...  "); // display status
                        
                        SPI_Set_Active(SPI1_Read, SPI1_Write);
                        while (!SPI_Ethernet_renewDHCP(5))   // try to renew until it works
                          ;
                        SPI_Set_Active(SPI3_Read, SPI3_Write);
                        SPI_Lcd_Cmd(_LCD_CLEAR);
                        }
#endif

                /*
                 * update display
                 */
                Time_epochToDate(epoch + conf.tz * 3600l, &ts);
                if(lcdEvent)
                        { // is it time for lcd update
                        mkLCDLine(1, conf.lcdL1); // update lcd: first row
                        mkLCDLine(2, conf.lcdL2); // update lcd: second row
                        lcdEvent = 0;             // clear lcd update flag
                        marquee++;                // set marquee pointer
                        }
                }
        }