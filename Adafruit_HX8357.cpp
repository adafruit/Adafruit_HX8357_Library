/***************************************************
  This is our library for the Adafruit HX8357D Breakout
  ----> http://www.adafruit.com/products/2050

  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/

#include <SPI.h>
#include "Adafruit_HX8357.h"
#include <limits.h>
#ifdef __AVR
  #include <avr/pgmspace.h>
#elif defined(ESP8266)
  #include <pgmspace.h>
#endif
#if !defined(ARDUINO_STM32_FEATHER)
 #include "pins_arduino.h"
 #include "wiring_private.h"
#endif



#define MADCTL_MY  0x80
#define MADCTL_MX  0x40
#define MADCTL_MV  0x20
#define MADCTL_ML  0x10
#define MADCTL_RGB 0x00
#define MADCTL_BGR 0x08
#define MADCTL_MH  0x04


/*
 * Control Pins
 * */

#ifdef USE_FAST_PINIO
#define SPI_DC_HIGH()           *dcport |=  dcpinmask
#define SPI_DC_LOW()            *dcport &= ~dcpinmask
#define SPI_CS_HIGH()           *csport |= cspinmask
#define SPI_CS_LOW()            *csport &= ~cspinmask
#else
#define SPI_DC_HIGH()           digitalWrite(_dc, HIGH)
#define SPI_DC_LOW()            digitalWrite(_dc, LOW)
#define SPI_CS_HIGH()           digitalWrite(_cs, HIGH)
#define SPI_CS_LOW()            digitalWrite(_cs, LOW)
#endif

/*
 * Software SPI Macros
 * */

#ifdef USE_FAST_PINIO
#define SSPI_MOSI_HIGH()        *mosiport |=  mosipinmask
#define SSPI_MOSI_LOW()         *mosiport &= ~mosipinmask
#define SSPI_SCK_HIGH()         *clkport |=  clkpinmask
#define SSPI_SCK_LOW()          *clkport &= ~clkpinmask
#define SSPI_MISO_READ()        ((*misoport & misopinmask) != 0)
#else
#define SSPI_MOSI_HIGH()        digitalWrite(_mosi, HIGH)
#define SSPI_MOSI_LOW()         digitalWrite(_mosi, LOW)
#define SSPI_SCK_HIGH()         digitalWrite(_sclk, HIGH)
#define SSPI_SCK_LOW()          digitalWrite(_sclk, LOW)
#define SSPI_MISO_READ()        digitalRead(_miso)
#endif

#define SSPI_BEGIN_TRANSACTION()
#define SSPI_END_TRANSACTION()
#define SSPI_WRITE(v)           spiWrite(v)
#define SSPI_WRITE16(s)         SSPI_WRITE((s) >> 8); SSPI_WRITE(s)
#define SSPI_WRITE32(l)         SSPI_WRITE((l) >> 24); SSPI_WRITE((l) >> 16); SSPI_WRITE((l) >> 8); SSPI_WRITE(l)
#define SSPI_WRITE_PIXELS(c,l)  for(uint32_t i=0; i<(l); i+=2){ SSPI_WRITE(((uint8_t*)(c))[i+1]); SSPI_WRITE(((uint8_t*)(c))[i]); }

/*
 * Hardware SPI Macros
 * */

#define SPI_OBJECT  SPI

#if defined (__AVR__) ||  defined(TEENSYDUINO) ||  defined(ARDUINO_ARCH_STM32F1)
    #define HSPI_SET_CLOCK() SPI_OBJECT.setClockDivider(SPI_CLOCK_DIV2);
#elif defined (__arm__)
    #define HSPI_SET_CLOCK() SPI_OBJECT.setClockDivider(11);
#elif defined(ESP8266) || defined(ESP32)
    #define HSPI_SET_CLOCK() SPI_OBJECT.setFrequency(_freq);
#elif defined(RASPI)
    #define HSPI_SET_CLOCK() SPI_OBJECT.setClock(_freq);
#elif defined(ARDUINO_ARCH_STM32F1)
    #define HSPI_SET_CLOCK() SPI_OBJECT.setClock(_freq);
#else
    #define HSPI_SET_CLOCK()
#endif

#ifdef SPI_HAS_TRANSACTION
    #define HSPI_BEGIN_TRANSACTION() SPI_OBJECT.beginTransaction(SPISettings(_freq, MSBFIRST, SPI_MODE0))
    #define HSPI_END_TRANSACTION()   SPI_OBJECT.endTransaction()
#else
    #define HSPI_BEGIN_TRANSACTION() HSPI_SET_CLOCK(); SPI_OBJECT.setBitOrder(MSBFIRST); SPI_OBJECT.setDataMode(SPI_MODE0)
    #define HSPI_END_TRANSACTION()
#endif

#ifdef ESP32
    #define SPI_HAS_WRITE_PIXELS
#endif
#if defined(ESP8266) || defined(ESP32)
    // Optimized SPI (ESP8266 and ESP32)
    #define HSPI_READ()              SPI_OBJECT.transfer(0)
    #define HSPI_WRITE(b)            SPI_OBJECT.write(b)
    #define HSPI_WRITE16(s)          SPI_OBJECT.write16(s)
    #define HSPI_WRITE32(l)          SPI_OBJECT.write32(l)
    #ifdef SPI_HAS_WRITE_PIXELS
        #define SPI_MAX_PIXELS_AT_ONCE  32
        #define HSPI_WRITE_PIXELS(c,l)   SPI_OBJECT.writePixels(c,l)
    #else
        #define HSPI_WRITE_PIXELS(c,l)   for(uint32_t i=0; i<((l)/2); i++){ SPI_WRITE16(((uint16_t*)(c))[i]); }
    #endif
#else
    // Standard Byte-by-Byte SPI

    #if defined (__AVR__) || defined(TEENSYDUINO)
static inline uint8_t _avr_spi_read(void) __attribute__((always_inline));
static inline uint8_t _avr_spi_read(void) {
    uint8_t r = 0;
    SPDR = r;
    while(!(SPSR & _BV(SPIF)));
    r = SPDR;
    return r;
}
        #define HSPI_WRITE(b)            {SPDR = (b); while(!(SPSR & _BV(SPIF)));}
        #define HSPI_READ()              _avr_spi_read()
    #else
        #define HSPI_WRITE(b)            SPI_OBJECT.transfer((uint8_t)(b))
        #define HSPI_READ()              HSPI_WRITE(0)
    #endif
    #define HSPI_WRITE16(s)          HSPI_WRITE((s) >> 8); HSPI_WRITE(s)
    #define HSPI_WRITE32(l)          HSPI_WRITE((l) >> 24); HSPI_WRITE((l) >> 16); HSPI_WRITE((l) >> 8); HSPI_WRITE(l)
    #define HSPI_WRITE_PIXELS(c,l)   for(uint32_t i=0; i<(l); i+=2){ HSPI_WRITE(((uint8_t*)(c))[i+1]); HSPI_WRITE(((uint8_t*)(c))[i]); }
#endif

/*
 * Final SPI Macros
 * */
#if defined (ARDUINO_ARCH_ARC32)
  #define SPI_DEFAULT_FREQ         16000000
#elif defined (__AVR__) || defined(TEENSYDUINO)
  #define SPI_DEFAULT_FREQ         8000000
#elif defined(ESP8266)
  #define SPI_DEFAULT_FREQ         16000000
#elif defined(ESP32)
  #define SPI_DEFAULT_FREQ         24000000
#elif defined(RASPI)
  #define SPI_DEFAULT_FREQ         24000000
#else
  #define SPI_DEFAULT_FREQ         24000000
#endif

#define SPI_BEGIN()             if(_sclk < 0){SPI_OBJECT.begin();}
#define SPI_BEGIN_TRANSACTION() if(_sclk < 0){HSPI_BEGIN_TRANSACTION();}
#define SPI_END_TRANSACTION()   if(_sclk < 0){HSPI_END_TRANSACTION();}
#define SPI_WRITE16(s)          if(_sclk < 0){HSPI_WRITE16(s);}else{SSPI_WRITE16(s);}
#define SPI_WRITE32(l)          if(_sclk < 0){HSPI_WRITE32(l);}else{SSPI_WRITE32(l);}
#define SPI_WRITE_PIXELS(c,l)   if(_sclk < 0){HSPI_WRITE_PIXELS(c,l);}else{SSPI_WRITE_PIXELS(c,l);}


// Pass 8-bit (each) R,G,B, get back 16-bit packed color
uint16_t Adafruit_HX8357::color565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// Constructor when using software SPI.  All output pins are configurable.
Adafruit_HX8357::Adafruit_HX8357(int8_t cs, int8_t dc, int8_t mosi, int8_t sclk, int8_t rst, int8_t miso) 
  : Adafruit_GFX(HX8357_TFTWIDTH, HX8357_TFTHEIGHT) {
  _cs   = cs;
  _dc   = dc;
  _mosi  = mosi;
  _miso = miso;
  _sclk = sclk;
  _rst  = rst;

  _freq = SPI_DEFAULT_FREQ;

#ifdef USE_FAST_PINIO
    csport    = portOutputRegister(digitalPinToPort(_cs));
    cspinmask = digitalPinToBitMask(_cs);
    dcport    = portOutputRegister(digitalPinToPort(_dc));
    dcpinmask = digitalPinToBitMask(_dc);
    clkport     = portOutputRegister(digitalPinToPort(_sclk));
    clkpinmask  = digitalPinToBitMask(_sclk);
    mosiport    = portOutputRegister(digitalPinToPort(_mosi));
    mosipinmask = digitalPinToBitMask(_mosi);
    if(miso >= 0) {
        misoport    = portInputRegister(digitalPinToPort(_miso));
        misopinmask = digitalPinToBitMask(_miso);
    } else {
        misoport    = 0;
        misopinmask = 0;
    }
#endif
}


// Constructor when using hardware SPI.  Faster, but must use SPI pins
// specific to each board type (e.g. 11,13 for Uno, 51,52 for Mega, etc.)
Adafruit_HX8357::Adafruit_HX8357(int8_t cs, int8_t dc, int8_t rst) : Adafruit_GFX(HX8357_TFTWIDTH, HX8357_TFTHEIGHT) {
  _cs   = cs;
  _dc   = dc;
  _rst  = rst;
  _mosi = _sclk = -1;

  _freq = SPI_DEFAULT_FREQ;

#ifdef USE_FAST_PINIO
    csport    = portOutputRegister(digitalPinToPort(_cs));
    cspinmask = digitalPinToBitMask(_cs);
    dcport    = portOutputRegister(digitalPinToPort(_dc));
    dcpinmask = digitalPinToBitMask(_dc);
    clkport     = 0;
    clkpinmask  = 0;
    mosiport    = 0;
    mosipinmask = 0;
    misoport    = 0;
    misopinmask = 0;
#endif
}


void Adafruit_HX8357::begin(uint8_t type) {
  // Control Pins
  pinMode(_dc, OUTPUT);
  digitalWrite(_dc, LOW);
  pinMode(_cs, OUTPUT);
  digitalWrite(_cs, HIGH);

  // Software SPI
  if(_sclk >= 0){
    pinMode(_mosi, OUTPUT);
    digitalWrite(_mosi, LOW);
    pinMode(_sclk, OUTPUT);
    digitalWrite(_sclk, HIGH);
    if(_miso >= 0){
      pinMode(_miso, INPUT);
    }
  }
  
  // Hardware SPI
  SPI_BEGIN();
  
  // toggle RST low to reset
  if (_rst >= 0) {
    pinMode(_rst, OUTPUT);
    digitalWrite(_rst, HIGH);
    delay(100);
    digitalWrite(_rst, LOW);
    delay(100);
    digitalWrite(_rst, HIGH);
    delay(200);
  }

  startWrite();
  
  if (type == HX8357B) {
    //Serial.println("linux HX8357B"); 
    // seqpower
    writeCommand(HX8357B_SETPOWER);
    spiWrite(0x44);
    spiWrite(0x41); 
    spiWrite(0x06);
    // seq_vcom
    writeCommand(HX8357B_SETVCOM);
    spiWrite(0x40); 
    spiWrite(0x10);
    // seq_power_normal
    writeCommand(HX8357B_SETPWRNORMAL);
    spiWrite(0x05); 
    spiWrite(0x12);
    // seq_panel_driving
    writeCommand(HX8357B_SET_PANEL_DRIVING);
    spiWrite(0x14); 
    spiWrite(0x3b);
    spiWrite(0x00);
    spiWrite(0x02);
    spiWrite(0x11);
    // seq_display_frame
    writeCommand(HX8357B_SETDISPLAYFRAME);
    spiWrite(0x0c);  // 6.8mhz
    // seq_panel_related
    writeCommand(HX8357B_SETPANELRELATED);
    spiWrite(0x01);  // BGR
    // seq_undefined1
    writeCommand(0xEA);
    spiWrite(0x03);
    spiWrite(0x00);
    spiWrite(0x00); 
    // undef2
    writeCommand(0xEB);
    spiWrite(0x40);
    spiWrite(0x54);
    spiWrite(0x26); 
    spiWrite(0xdb);
    // seq_gamma
    writeCommand(HX8357B_SETGAMMA); // 0xC8
    spiWrite(0x00);
    spiWrite(0x15);
    spiWrite(0x00); 
    spiWrite(0x22);
    spiWrite(0x00);
    spiWrite(0x08);
    spiWrite(0x77); 
    spiWrite(0x26);
    spiWrite(0x66);
    spiWrite(0x22);
    spiWrite(0x04); 
    spiWrite(0x00);

    // seq_addr mode
    writeCommand(HX8357_MADCTL);
    spiWrite(0xC0);
    // pixel format
    writeCommand(HX8357_COLMOD);
    spiWrite(0x55);
    
    // set up whole address box
    // paddr
    writeCommand(HX8357_PASET);
    spiWrite(0x00);
    spiWrite(0x00);
    spiWrite(0x01); 
    spiWrite(0xDF);
    // caddr
    writeCommand(HX8357_CASET);
    spiWrite(0x00);
    spiWrite(0x00);
    spiWrite(0x01); 
    spiWrite(0x3F);

    // display mode
    writeCommand(HX8357B_SETDISPMODE);
    spiWrite(0x00); // CPU (DBI) and internal oscillation ??
    // exit sleep
    writeCommand(HX8357_SLPOUT);

    delay(120);
    // main screen turn on
    writeCommand(HX8357_DISPON);
    delay(10);
  } else if (type == HX8357D) {
    writeCommand(HX8357_SWRESET);
    delay(10);

    // setextc
    writeCommand(HX8357D_SETC);
    spiWrite(0xFF);
    spiWrite(0x83);
    spiWrite(0x57);
    delay(300);

    // setRGB which also enables SDO
    writeCommand(HX8357_SETRGB); 
    spiWrite(0x80);  //enable SDO pin!
//    spiWrite(0x00);  //disable SDO pin!
    spiWrite(0x0);
    spiWrite(0x06);
    spiWrite(0x06);

    writeCommand(HX8357D_SETCOM);
    spiWrite(0x25);  // -1.52V
    
    writeCommand(HX8357_SETOSC);
    spiWrite(0x68);  // Normal mode 70Hz, Idle mode 55 Hz
    
    writeCommand(HX8357_SETPANEL); //Set Panel
    spiWrite(0x05);  // BGR, Gate direction swapped
    
    writeCommand(HX8357_SETPWR1);
    spiWrite(0x00);  // Not deep standby
    spiWrite(0x15);  //BT
    spiWrite(0x1C);  //VSPR
    spiWrite(0x1C);  //VSNR
    spiWrite(0x83);  //AP
    spiWrite(0xAA);  //FS

    writeCommand(HX8357D_SETSTBA);  
    spiWrite(0x50);  //OPON normal
    spiWrite(0x50);  //OPON idle
    spiWrite(0x01);  //STBA
    spiWrite(0x3C);  //STBA
    spiWrite(0x1E);  //STBA
    spiWrite(0x08);  //GEN
    
    writeCommand(HX8357D_SETCYC);  
    spiWrite(0x02);  //NW 0x02
    spiWrite(0x40);  //RTN
    spiWrite(0x00);  //DIV
    spiWrite(0x2A);  //DUM
    spiWrite(0x2A);  //DUM
    spiWrite(0x0D);  //GDON
    spiWrite(0x78);  //GDOFF

    writeCommand(HX8357D_SETGAMMA); 
    spiWrite(0x02);
    spiWrite(0x0A);
    spiWrite(0x11);
    spiWrite(0x1d);
    spiWrite(0x23);
    spiWrite(0x35);
    spiWrite(0x41);
    spiWrite(0x4b);
    spiWrite(0x4b);
    spiWrite(0x42);
    spiWrite(0x3A);
    spiWrite(0x27);
    spiWrite(0x1B);
    spiWrite(0x08);
    spiWrite(0x09);
    spiWrite(0x03);
    spiWrite(0x02);
    spiWrite(0x0A);
    spiWrite(0x11);
    spiWrite(0x1d);
    spiWrite(0x23);
    spiWrite(0x35);
    spiWrite(0x41);
    spiWrite(0x4b);
    spiWrite(0x4b);
    spiWrite(0x42);
    spiWrite(0x3A);
    spiWrite(0x27);
    spiWrite(0x1B);
    spiWrite(0x08);
    spiWrite(0x09);
    spiWrite(0x03);
    spiWrite(0x00);
    spiWrite(0x01);
    
    writeCommand(HX8357_COLMOD);
    spiWrite(0x55);  // 16 bit
    
    writeCommand(HX8357_MADCTL);  
    spiWrite(0xC0); 
    
    writeCommand(HX8357_TEON);  // TE off
    spiWrite(0x00); 
    
    writeCommand(HX8357_TEARLINE);  // tear line
    spiWrite(0x00); 
    spiWrite(0x02);

    writeCommand(HX8357_SLPOUT); //Exit Sleep
    delay(150);
    
    writeCommand(HX8357_DISPON);  // display on
    delay(50);
  } else {
    //Serial.println("unknown type");
  }

  endWrite();
  _width  = HX8357_TFTWIDTH;
  _height = HX8357_TFTHEIGHT;
}



void Adafruit_HX8357::setRotation(uint8_t m) {

  rotation = m % 4; // can't be higher than 3
  switch (rotation) {
   case 0:
     m = MADCTL_MX | MADCTL_MY | MADCTL_RGB;
     _width  = HX8357_TFTWIDTH;
     _height = HX8357_TFTHEIGHT;
     break;
   case 1:
     m = MADCTL_MV | MADCTL_MY | MADCTL_RGB;
     _width  = HX8357_TFTHEIGHT;
     _height = HX8357_TFTWIDTH;
     break;
  case 2:
    m = MADCTL_RGB;
     _width  = HX8357_TFTWIDTH;
     _height = HX8357_TFTHEIGHT;
    break;
   case 3:
     m = MADCTL_MX | MADCTL_MV | MADCTL_RGB;
     _width  = HX8357_TFTHEIGHT;
     _height = HX8357_TFTWIDTH;
     break;
  }

  startWrite();
  writeCommand(HX8357_MADCTL);
  spiWrite(m);
  endWrite();
}



void Adafruit_HX8357::invertDisplay(boolean i) {
  startWrite();
  writeCommand(i ? HX8357_INVON : HX8357_INVOFF);
  endWrite();
}


uint8_t Adafruit_HX8357::spiRead() {
    if(_sclk < 0){
        return HSPI_READ();
    }
    if(_miso < 0){
        return 0;
    }
    uint8_t r = 0;
    for (uint8_t i=0; i<8; i++) {
        SSPI_SCK_LOW();
        SSPI_SCK_HIGH();
        r <<= 1;
        if (SSPI_MISO_READ()){
            r |= 0x1;
        }
    }
    return r;
}



void Adafruit_HX8357::spiWrite(uint8_t b) {
    if(_sclk < 0){
        HSPI_WRITE(b);
        return;
    }
    for(uint8_t bit = 0x80; bit; bit >>= 1){
        if((b) & bit){
            SSPI_MOSI_HIGH();
        } else {
            SSPI_MOSI_LOW();
        }
        SSPI_SCK_LOW();
        SSPI_SCK_HIGH();
    }
}



/*
 * Transaction API
 * */

void Adafruit_HX8357::startWrite(void){
    SPI_BEGIN_TRANSACTION();
    SPI_CS_LOW();
}

void Adafruit_HX8357::endWrite(void){
    SPI_CS_HIGH();
    SPI_END_TRANSACTION();
}


void Adafruit_HX8357::writeCommand(uint8_t c) {
    SPI_DC_LOW();
    spiWrite(c);
    SPI_DC_HIGH();
}


void Adafruit_HX8357::setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
  uint32_t xa = ((uint32_t)x << 16) | (x+w-1);
  uint32_t ya = ((uint32_t)y << 16) | (y+h-1);
  writeCommand(HX8357_CASET); // Column addr set
  SPI_WRITE32(xa);

  writeCommand(HX8357_PASET); // Row addr set
  SPI_WRITE32(ya);

  writeCommand(HX8357_RAMWR); // write to RAM
}


void Adafruit_HX8357::pushColor(uint16_t color) {
  startWrite();
  SPI_WRITE16(color);
  endWrite();
}


void Adafruit_HX8357::writePixel(uint16_t color){
    SPI_WRITE16(color);
}


void Adafruit_HX8357::writePixels(uint16_t * colors, uint32_t len){
    SPI_WRITE_PIXELS((uint8_t*)colors , len * 2);
}

void Adafruit_HX8357::writeColor(uint16_t color, uint32_t len){
#ifdef SPI_HAS_WRITE_PIXELS
    if(_sclk >= 0){
        for (uint32_t t=0; t<len; t++){
            writePixel(color);
        }
        return;
    }
    static uint16_t temp[SPI_MAX_PIXELS_AT_ONCE];
    size_t blen = (len > SPI_MAX_PIXELS_AT_ONCE)?SPI_MAX_PIXELS_AT_ONCE:len;
    uint16_t tlen = 0;

    for (uint32_t t=0; t<blen; t++){
        temp[t] = color;
    }

    while(len){
        tlen = (len>blen)?blen:len;
        writePixels(temp, tlen);
        len -= tlen;
    }
#else
    uint8_t hi = color >> 8, lo = color;
    if(_sclk < 0){ //AVR Optimization
        for (uint32_t t=len; t; t--){
            HSPI_WRITE(hi);
            HSPI_WRITE(lo);
        }
        return;
    }
    for (uint32_t t=len; t; t--){
        spiWrite(hi);
        spiWrite(lo);
    }
#endif
}

void Adafruit_HX8357::writePixel(int16_t x, int16_t y, uint16_t color) {
    if((x < 0) ||(x >= _width) || (y < 0) || (y >= _height)) return;
    setAddrWindow(x,y,1,1);
    writePixel(color);
}

void Adafruit_HX8357::writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color){
    if((x >= _width) || (y >= _height)) return;
    int16_t x2 = x + w - 1, y2 = y + h - 1;
    if((x2 < 0) || (y2 < 0)) return;

    // Clip left/top
    if(x < 0) {
        x = 0;
        w = x2 + 1;
    }
    if(y < 0) {
        y = 0;
        h = y2 + 1;
    }

    // Clip right/bottom
    if(x2 >= _width)  w = _width  - x;
    if(y2 >= _height) h = _height - y;

    int32_t len = (int32_t)w * h;
    setAddrWindow(x, y, w, h);
    writeColor(color, len);
}

void Adafruit_HX8357::writeFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color){
    writeFillRect(x, y, 1, h, color);
}

void Adafruit_HX8357::writeFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color){
    writeFillRect(x, y, w, 1, color);
}



void Adafruit_HX8357::drawPixel(int16_t x, int16_t y, uint16_t color) {
    startWrite();
    writePixel(x, y, color);
    endWrite();
}


void Adafruit_HX8357::drawFastVLine(int16_t x, int16_t y, int16_t h,
 uint16_t color) {
    startWrite();
    writeFastVLine(x, y, h, color);
    endWrite();
}

void Adafruit_HX8357::drawFastHLine(int16_t x, int16_t y, int16_t w,
  uint16_t color) {
    startWrite();
    writeFastHLine(x, y, w, color);
    endWrite();
}

// fill a rectangle
void Adafruit_HX8357::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    startWrite();
    writeFillRect(x,y,w,h,color);
    endWrite();
}

void Adafruit_HX8357::fillScreen(uint16_t color) {
#if defined(ESP8266)
  ESP.wdtDisable();
#endif
  fillRect(0, 0, _width, _height, color);
#if defined(ESP8266)
  ESP.wdtEnable(WDTO_4S);
#endif
}



// Adapted from https://github.com/PaulStoffregen/ILI9341_t3
// by Marc MERLIN. See examples/pictureEmbed to use this.
// 5/6/2017: function name and arguments have changed for compatibility
// with current GFX library and to avoid naming problems in prior
// implementation.  Formerly drawBitmap() with arguments in different order.
void Adafruit_HX8357::drawRGBBitmap(int16_t x, int16_t y,
  uint16_t *pcolors, int16_t w, int16_t h) {

    int16_t x2, y2; // Lower-right coord
    if(( x             >= _width ) ||      // Off-edge right
       ( y             >= _height) ||      // " top
       ((x2 = (x+w-1)) <  0      ) ||      // " left
       ((y2 = (y+h-1)) <  0)     ) return; // " bottom

    int16_t bx1=0, by1=0, // Clipped top-left within bitmap
            saveW=w;      // Save original bitmap width value
    if(x < 0) { // Clip left
        w  +=  x;
        bx1 = -x;
        x   =  0;
    }
    if(y < 0) { // Clip top
        h  +=  y;
        by1 = -y;
        y   =  0;
    }
    if(x2 >= _width ) w = _width  - x; // Clip right
    if(y2 >= _height) h = _height - y; // Clip bottom

    pcolors += by1 * saveW + bx1; // Offset bitmap ptr to clipped top-left
    startWrite();
    setAddrWindow(x, y, w, h); // Clipped area
    while(h--) { // For each (clipped) scanline...
      writePixels(pcolors, w); // Push one (clipped) row
      pcolors += saveW; // Advance pointer by one full (unclipped) line
    }
    endWrite();
}



////////// stuff not actively being used, but kept for posterity


 uint8_t Adafruit_HX8357::readdata(void) {
   digitalWrite(_dc, HIGH);
   digitalWrite(_cs, LOW);
   uint8_t r = spiRead();
   digitalWrite(_cs, HIGH);
   
   return r;
}
 

uint8_t Adafruit_HX8357::readcommand8(uint8_t c, uint8_t index) {
   digitalWrite(_dc, LOW);
   digitalWrite(_cs, LOW);

   spiWrite(c);
 
   digitalWrite(_dc, HIGH);
   uint8_t r = spiRead();
   digitalWrite(_cs, HIGH);
   return r;
}


 
/*

 uint16_t Adafruit_HX8357::readcommand16(uint8_t c) {
 digitalWrite(_dc, LOW);
 if (_cs)
 digitalWrite(_cs, LOW);
 
 spiwrite(c);
 pinMode(_sid, INPUT); // input!
 uint16_t r = spiread();
 r <<= 8;
 r |= spiread();
 if (_cs)
 digitalWrite(_cs, HIGH);
 
 pinMode(_sid, OUTPUT); // back to output
 return r;
 }
 
 uint32_t Adafruit_HX8357::readcommand32(uint8_t c) {
 digitalWrite(_dc, LOW);
 if (_cs)
 digitalWrite(_cs, LOW);
 spiwrite(c);
 pinMode(_sid, INPUT); // input!
 
 dummyclock();
 dummyclock();
 
 uint32_t r = spiread();
 r <<= 8;
 r |= spiread();
 r <<= 8;
 r |= spiread();
 r <<= 8;
 r |= spiread();
 if (_cs)
 digitalWrite(_cs, HIGH);
 
 pinMode(_sid, OUTPUT); // back to output
 return r;
 }
 
 */
