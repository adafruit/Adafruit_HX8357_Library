/*!
* @file Adafruit_HX8357.cpp
*
* This is the documentation for Adafruit's ILI9341 driver for the
* Arduino platform. 
*
* This library works with the Adafruit 3.5" TFT 320x480 + Touchscreen Breakout
*    http://www.adafruit.com/products/2050
*
* Adafruit TFT FeatherWing - 3.5" 480x320 Touchscreen for Feathers 
*    https://www.adafruit.com/product/3651
*
* These displays use SPI to communicate, 4 or 5 pins are required
* to interface (RST is optional).
*
* Adafruit invests time and resources providing this open source code,
* please support Adafruit and open-source hardware by purchasing
* products from Adafruit!
*
*
* This library depends on <a href="https://github.com/adafruit/Adafruit_GFX">
* Adafruit_GFX</a> being present on your system. Please make sure you have
* installed the latest version before using this library.
*
* Written by Limor "ladyada" Fried for Adafruit Industries.
*
* BSD license, all text here must be included in any redistribution.
*
*/

#ifndef _ADAFRUIT_HX8357_H
#define _ADAFRUIT_HX8357_H

#if ARDUINO >= 100
 #include "Arduino.h"
 #include "Print.h"
#else
 #include "WProgram.h"
#endif
#include <Adafruit_GFX.h>
#if defined (__AVR__) || defined(TEENSYDUINO)
  #include <avr/pgmspace.h>
#elif defined(ESP8266)
  #include <pgmspace.h>
#endif

// define here the size of a register!

#if defined (__arm__) || defined(ARDUINO_STM32_FEATHER)
 #if defined(TEENSYDUINO)
  typedef volatile uint8_t RwReg;
  #define USE_FAST_PINIO
 #elif defined(NRF52) || defined(ARDUINO_MAXIM)
  typedef volatile uint32_t RwReg;
  //#define USE_FAST_PINIO
 #elif defined(ARDUINO_STM32_FEATHER)
  typedef volatile unsigned int RwReg;
  //#define USE_FAST_PINIO
 #else
  typedef volatile uint32_t RwReg;
  #define USE_FAST_PINIO
 #endif
#elif defined (__AVR__) || defined(TEENSYDUINO)
  typedef volatile uint8_t RwReg;
  #define USE_FAST_PINIO
#elif defined (ESP8266) || defined (ESP32)
  typedef volatile uint32_t RwReg;
#elif defined (__ARDUINO_ARC__)
  typedef volatile uint32_t RwReg;
#endif


#define HX8357D 0xD
#define HX8357B 0xB

#define HX8357_TFTWIDTH  320
#define HX8357_TFTHEIGHT 480

#define HX8357_NOP     0x00
#define HX8357_SWRESET 0x01
#define HX8357_RDDID   0x04
#define HX8357_RDDST   0x09

#define HX8357_RDPOWMODE  0x0A
#define HX8357_RDMADCTL  0x0B
#define HX8357_RDCOLMOD  0x0C
#define HX8357_RDDIM  0x0D
#define HX8357_RDDSDR  0x0F

#define HX8357_SLPIN   0x10
#define HX8357_SLPOUT  0x11
#define HX8357B_PTLON   0x12
#define HX8357B_NORON   0x13

#define HX8357_INVOFF  0x20
#define HX8357_INVON   0x21
#define HX8357_DISPOFF 0x28
#define HX8357_DISPON  0x29

#define HX8357_CASET   0x2A
#define HX8357_PASET   0x2B
#define HX8357_RAMWR   0x2C
#define HX8357_RAMRD   0x2E

#define HX8357B_PTLAR   0x30
#define HX8357_TEON  0x35
#define HX8357_TEARLINE  0x44
#define HX8357_MADCTL  0x36
#define HX8357_COLMOD  0x3A

#define HX8357_SETOSC 0xB0
#define HX8357_SETPWR1 0xB1
#define HX8357B_SETDISPLAY 0xB2
#define HX8357_SETRGB 0xB3
#define HX8357D_SETCOM  0xB6

#define HX8357B_SETDISPMODE  0xB4
#define HX8357D_SETCYC  0xB4
#define HX8357B_SETOTP 0xB7
#define HX8357D_SETC 0xB9

#define HX8357B_SET_PANEL_DRIVING 0xC0
#define HX8357D_SETSTBA 0xC0
#define HX8357B_SETDGC  0xC1
#define HX8357B_SETID  0xC3
#define HX8357B_SETDDB  0xC4
#define HX8357B_SETDISPLAYFRAME 0xC5
#define HX8357B_GAMMASET 0xC8
#define HX8357B_SETCABC  0xC9
#define HX8357_SETPANEL  0xCC


#define HX8357B_SETPOWER 0xD0
#define HX8357B_SETVCOM 0xD1
#define HX8357B_SETPWRNORMAL 0xD2

#define HX8357B_RDID1   0xDA
#define HX8357B_RDID2   0xDB
#define HX8357B_RDID3   0xDC
#define HX8357B_RDID4   0xDD

#define HX8357D_SETGAMMA 0xE0

#define HX8357B_SETGAMMA 0xC8
#define HX8357B_SETPANELRELATED  0xE9



// Color definitions
#define	HX8357_BLACK   0x0000
#define	HX8357_BLUE    0x001F
#define	HX8357_RED     0xF800
#define	HX8357_GREEN   0x07E0
#define HX8357_CYAN    0x07FF
#define HX8357_MAGENTA 0xF81F
#define HX8357_YELLOW  0xFFE0  
#define HX8357_WHITE   0xFFFF


/// Class to manage hardware interface with HX8357 chipset
class Adafruit_HX8357 : public Adafruit_GFX {

 public:

  Adafruit_HX8357(int8_t _CS, int8_t _DC, int8_t _MOSI, int8_t _SCLK,
		   int8_t _RST, int8_t _MISO);
  Adafruit_HX8357(int8_t _CS, int8_t _DC, int8_t _RST = -1);

  void     begin(uint8_t type = HX8357D),
           fillScreen(uint16_t color),
           drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color),
           drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color),
           fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
             uint16_t color),
           setRotation(uint8_t r),
           invertDisplay(boolean i);
  uint16_t color565(uint8_t r, uint8_t g, uint8_t b);


  // Required Non-Transaction
  void      drawPixel(int16_t x, int16_t y, uint16_t color);
  
  // Transaction API
  void      startWrite(void);
  void      endWrite(void);
  void      writePixel(int16_t x, int16_t y, uint16_t color);
  void      writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
  void      writeFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
  void      writeFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
  
  // Transaction API not used by GFX
  void      setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
  void      writePixel(uint16_t color);
  void      writePixels(uint16_t * colors, uint32_t len);
  void      writeColor(uint16_t color, uint32_t len);
  void      pushColor(uint16_t color);
  
  void      drawRGBBitmap(int16_t x, int16_t y,
				    uint16_t *pcolors, int16_t w, int16_t h);
  //void     commandList(uint8_t *addr);

  uint8_t     readcommand8(uint8_t reg, uint8_t index = 0);

 private:
  uint8_t  tabcolor;

  int32_t  _cs, _dc, _rst, _mosi, _miso, _sclk, _freq;

#if defined (USE_FAST_PINIO)
  volatile RwReg *misoport, *mosiport, *clkport, *dcport, *csport;
  uint32_t  misopinmask, mosipinmask, clkpinmask, cspinmask, dcpinmask;
#endif

  void        writeCommand(uint8_t cmd);
  void        spiWrite(uint8_t v);
  uint8_t     spiRead(void);
  /* These are not for current use, 8-bit protocol only! */
  uint8_t     readdata(void);
  /*
  uint16_t readcommand16(uint8_t);
  uint32_t readcommand32(uint8_t);
  void     dummyclock(void);
  */  

};

#endif
