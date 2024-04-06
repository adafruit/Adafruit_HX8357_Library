/*!
 * @file Adafruit_HX8357.cpp
 *
 * @mainpage Adafruit HX8357 TFT Displays
 *
 * @section intro_sec Introduction
 *
 * This is the documentation for Adafruit's HX8357 driver for the Arduino
 * platform.
 *
 * This library works with the Adafruit 3.5" TFT 320x480 + Touchscreen Breakout
 *    http://www.adafruit.com/products/2050
 *
 * Adafruit TFT FeatherWing - 3.5" 480x320 Touchscreen for Feathers
 *    https://www.adafruit.com/product/3651
 *
 * These displays use SPI to communicate. This requires 4 pins (MOSI,
 * SCK, select, data/command) and optionally a reset pin. Hardware SPI
 * or 'bitbang' software SPI are both supported.
 *
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing
 * products from Adafruit!
 *
 * @section dependencies Dependencies
 *
 * This library depends on <a href="https://github.com/adafruit/Adafruit_GFX">
 * Adafruit_GFX</a> being present on your system. Please make sure you have
 * installed the latest version before using this library.
 *
 * @section author Author
 *
 * Written by Limor Fried/Ladyada for Adafruit Industries, with
 * contributions from the open source community.
 *
 * @section license License
 *
 * BSD license, all text here must be included in any redistribution.
 */

#include "Adafruit_HX8357.h"
#include <SPI.h>
#include <limits.h>

#define MADCTL_MY 0x80  ///< Bottom to top
#define MADCTL_MX 0x40  ///< Right to left
#define MADCTL_MV 0x20  ///< Reverse Mode
#define MADCTL_ML 0x10  ///< LCD refresh Bottom to top
#define MADCTL_RGB 0x00 ///< Red-Green-Blue pixel order
#define MADCTL_BGR 0x08 ///< Blue-Green-Red pixel order
#define MADCTL_MH 0x04  ///< LCD refresh right to left

// THIS REALLY SHOULD GO IN SPITFT (PART OF ADAFRUIT_GFX),
// AND CAN BE FURTHER EXPANDED (e.g. add 12 MHz for M0, 24 for M4),
// BUT TEMPORARILY FOR NOW IT'S HERE:

#if defined(ARDUINO_ARCH_ARC32)
#define SPI_DEFAULT_FREQ 16000000
#elif defined(__AVR__) || defined(TEENSYDUINO)
#define SPI_DEFAULT_FREQ 8000000
#elif defined(ESP8266) || defined(ARDUINO_MAXIM)
#define SPI_DEFAULT_FREQ 16000000
#elif defined(ESP32)
#define SPI_DEFAULT_FREQ 24000000
#elif defined(RASPI)
#define SPI_DEFAULT_FREQ 24000000
#else
#define SPI_DEFAULT_FREQ 24000000 ///< The default SPI frequency
#endif

// CONSTRUCTORS, DESTRUCTOR ------------------------------------------------

/*!
    @brief   Constructor for Adafruit_HX8357 displays, using software
             (bitbang) SPI.
    @param   cs
             Chip select pin (using Arduino pin numbering).
    @param   dc
             Data/Command pin (using Arduino pin numbering).
    @param   mosi
             SPI MOSI pin (using Arduino pin numbering).
    @param   sclk
             SPI Clock pin (using Arduino pin numbering).
    @param   rst
             Reset pin (Arduino pin numbering, optional, pass -1 if unused).
    @param   miso
             SPI MISO pin (Arduino pin #, optional, pass -1 if unused).
    @param   type
             Display type, HX8357D (default if unspecified) or HX8357B.
    @return  Adafruit_HX8357 object.
    @note    Call the object's begin() function before use.
*/
Adafruit_HX8357::Adafruit_HX8357(int8_t cs, int8_t dc, int8_t mosi, int8_t sclk,
                                 int8_t rst, int8_t miso, uint8_t type)
    : Adafruit_SPITFT(HX8357_TFTWIDTH, HX8357_TFTHEIGHT, cs, dc, mosi, sclk,
                      rst, miso),
      displayType(type) {}

/*!
    @brief   Constructor for Adafruit_HX8357 displays, using the default
             hardware SPI interface.
    @param   cs
             Chip select pin (using Arduino pin numbering).
    @param   dc
             Data/Command pin (using Arduino pin numbering).
    @param   rst
             Reset pin (Arduino pin numbering, optional, pass -1 if unused).
    @param   type
             Display type, HX8357D (default if unspecified) or HX8357B.
    @return  Adafruit_HX8357 object.
    @note    Call the object's begin() function before use.
*/
Adafruit_HX8357::Adafruit_HX8357(int8_t cs, int8_t dc, int8_t rst, uint8_t type)
    : Adafruit_SPITFT(HX8357_TFTWIDTH, HX8357_TFTHEIGHT, cs, dc, rst),
      displayType(type) {}

#if !defined(ESP8266)
/*!
    @brief   Constructor for Adafruit_HX8357 displays, using an arbitrary
             SPI interface.
    @param   spi
             SPI peripheral to use.
    @param   cs
             Chip select pin (using Arduino pin numbering).
    @param   dc
             Data/Command pin (using Arduino pin numbering).
    @param   rst
             Reset pin (Arduino pin numbering, optional, pass -1 if unused).
    @param   type
             Display type, HX8357D (default if unspecified) or HX8357B.
    @return  Adafruit_HX8357 object.
    @note    Call the object's begin() function before use.
*/
Adafruit_HX8357::Adafruit_HX8357(SPIClass *spi, int8_t cs, int8_t dc,
                                 int8_t rst, uint8_t type)
    : Adafruit_SPITFT(HX8357_TFTWIDTH, HX8357_TFTHEIGHT, spi, cs, dc, rst),
      displayType(type) {}
#endif // end !ESP8266

/**************************************************************************/
/*!
    @brief  Constructor for Adafruit_HX8357 displays, using parallel
            interface.
    @param  busWidth  If tft16 (enumeration in Adafruit_SPITFT.h), is a
                      16-bit interface, else 8-bit.
    @param  d0        Data pin 0 (MUST be a byte- or word-aligned LSB of a
                      PORT register -- pins 1-n are extrapolated from this).
    @param  wr        Write strobe pin # (required).
    @param  dc        Data/Command pin # (required).
    @param  cs        Chip select pin # (optional, pass -1 if unused and CS
                      is tied to GND).
    @param  rst       Reset pin # (optional, pass -1 if unused).
    @param  rd        Read strobe pin # (optional, pass -1 if unused).
*/
/**************************************************************************/
Adafruit_HX8357::Adafruit_HX8357(tftBusWidth busWidth, int8_t d0, int8_t wr,
                                 int8_t dc, int8_t cs, int8_t rst, int8_t rd)
    : Adafruit_SPITFT(HX8357_TFTWIDTH, HX8357_TFTHEIGHT, busWidth, d0, wr, dc,
                      cs, rst, rd) {}

/*!
    @brief   Destructor for Adafruit_HX8357 object.
    @return  None (void).
*/
Adafruit_HX8357::~Adafruit_HX8357(void) {}

// INIT DISPLAY ------------------------------------------------------------

static const uint8_t PROGMEM
    initb[] =
        {
            HX8357B_SETPOWER,
            3,
            0x44,
            0x41,
            0x06,
            HX8357B_SETVCOM,
            2,
            0x40,
            0x10,
            HX8357B_SETPWRNORMAL,
            2,
            0x05,
            0x12,
            HX8357B_SET_PANEL_DRIVING,
            5,
            0x14,
            0x3b,
            0x00,
            0x02,
            0x11,
            HX8357B_SETDISPLAYFRAME,
            1,
            0x0c, // 6.8mhz
            HX8357B_SETPANELRELATED,
            1,
            0x01, // BGR
            0xEA,
            3, // seq_undefined1, 3 args
            0x03,
            0x00,
            0x00,
            0xEB,
            4, // undef2, 4 args
            0x40,
            0x54,
            0x26,
            0xdb,
            HX8357B_SETGAMMA,
            12,
            0x00,
            0x15,
            0x00,
            0x22,
            0x00,
            0x08,
            0x77,
            0x26,
            0x66,
            0x22,
            0x04,
            0x00,
            HX8357_MADCTL,
            1,
            0xC0,
            HX8357_COLMOD,
            1,
            0x55,
            HX8357_PASET,
            4,
            0x00,
            0x00,
            0x01,
            0xDF,
            HX8357_CASET,
            4,
            0x00,
            0x00,
            0x01,
            0x3F,
            HX8357B_SETDISPMODE,
            1,
            0x00, // CPU (DBI) and internal oscillation ??
            HX8357_SLPOUT,
            0x80 + 120 / 5, // Exit sleep, then delay 120 ms
            HX8357_DISPON,
            0x80 + 10 / 5, // Main screen turn on, delay 10 ms
            0              // END OF COMMAND LIST
},
    initd[] = {
        HX8357_SWRESET,
        0x80 + 100 / 5, // Soft reset, then delay 10 ms
        HX8357D_SETC,
        3,
        0xFF,
        0x83,
        0x57,
        0xFF,
        0x80 + 500 / 5, // No command, just delay 300 ms
        HX8357_SETRGB,
        4,
        0x80,
        0x00,
        0x06,
        0x06, // 0x80 enables SDO pin (0x00 disables)
        HX8357D_SETCOM,
        1,
        0x25, // -1.52V
        HX8357_SETOSC,
        1,
        0x68, // Normal mode 70Hz, Idle mode 55 Hz
        HX8357_SETPANEL,
        1,
        0x05, // BGR, Gate direction swapped
        HX8357_SETPWR1,
        6,
        0x00, // Not deep standby
        0x15, // BT
        0x1C, // VSPR
        0x1C, // VSNR
        0x83, // AP
        0xAA, // FS
        HX8357D_SETSTBA,
        6,
        0x50, // OPON normal
        0x50, // OPON idle
        0x01, // STBA
        0x3C, // STBA
        0x1E, // STBA
        0x08, // GEN
        HX8357D_SETCYC,
        7,
        0x02, // NW 0x02
        0x40, // RTN
        0x00, // DIV
        0x2A, // DUM
        0x2A, // DUM
        0x0D, // GDON
        0x78, // GDOFF
        HX8357D_SETGAMMA,
        34,
        0x02,
        0x0A,
        0x11,
        0x1d,
        0x23,
        0x35,
        0x41,
        0x4b,
        0x4b,
        0x42,
        0x3A,
        0x27,
        0x1B,
        0x08,
        0x09,
        0x03,
        0x02,
        0x0A,
        0x11,
        0x1d,
        0x23,
        0x35,
        0x41,
        0x4b,
        0x4b,
        0x42,
        0x3A,
        0x27,
        0x1B,
        0x08,
        0x09,
        0x03,
        0x00,
        0x01,
        HX8357_COLMOD,
        1,
        0x55, // 16 bit
        HX8357_MADCTL,
        1,
        0xC0,
        HX8357_TEON,
        1,
        0x00, // TW off
        HX8357_TEARLINE,
        2,
        0x00,
        0x02,
        HX8357_SLPOUT,
        0x80 + 150 / 5, // Exit Sleep, then delay 150 ms
        HX8357_DISPON,
        0x80 + 50 / 5, // Main screen turn on, delay 50 ms
        0,             // END OF COMMAND LIST
};

/*!
    @brief   Initialize HX8357 chip. Connects to the HX8357 over SPI and
             sends initialization commands.
    @param   freq
             SPI bitrate -- default of 0 will use a (usually) platform-
             optimized value, e.g. 8 MHz on AVR, 12 MHz on M0.
    @return  None (void).
*/
void Adafruit_HX8357::begin(uint32_t freq) {
  // Older version of this library accepted a display type as the only
  // argument (HX8357D or HX8357B), but SPITFT (part of GFX lib) REQUIRES
  // the begin() function instead accept an SPI bitrate (or 0 for default),
  // so display type was moved to the constructor.  Examples will be
  // updated, but just in case there's old code around, we pull shenanigans
  // here...the values for HX8357D and HX8357B (0xD and 0xB, respectively)
  // would make for absurd SPI bitrates...so if we receive one of those
  // values, assume it's old code intending to pass the display type.
  // Override the displayType value that was set in the constructor with
  // the value passed here, and use the default SPI bitrate for platform.
  if (freq == HX8357D) {
    displayType = freq;
    freq = 0; // Use default SPI frequency
  } else if (freq == HX8357B) {
    displayType = freq;
    freq = 0; // Use default SPI frequency
  }

  if (!freq)
    freq = SPI_DEFAULT_FREQ;
  initSPI(freq);

  const uint8_t *addr = (displayType == HX8357B) ? initb : initd;
  uint8_t cmd, x, numArgs;
  while ((cmd = pgm_read_byte(addr++)) > 0) { // '0' command ends list
    x = pgm_read_byte(addr++);
    numArgs = x & 0x7F;
    if (cmd != 0xFF) { // '255' is ignored
      if (x & 0x80) {  // If high bit set, numArgs is a delay time
        sendCommand(cmd);
      } else {
        sendCommand(cmd, addr, numArgs);
        addr += numArgs;
      }
    }
    if (x & 0x80) {       // If high bit set...
      delay(numArgs * 5); // numArgs is actually a delay time (5ms units)
    }
  }

  _width = HX8357_TFTWIDTH; // Screen dimensions for default rotation 0
  _height = HX8357_TFTHEIGHT;
}

// GFX FUNCTIONS -----------------------------------------------------------

/*!
    @brief   Set origin of (0,0) and orientation of TFT display
    @param   m
             The index for rotation, from 0-3 inclusive
    @return  None (void).
*/
void Adafruit_HX8357::setRotation(uint8_t m) {
  rotation = m & 3; // can't be higher than 3
  switch (rotation) {
  case 0:
    m = MADCTL_MX | MADCTL_MY | MADCTL_RGB;
    _width = HX8357_TFTWIDTH;
    _height = HX8357_TFTHEIGHT;
    break;
  case 1:
    m = MADCTL_MV | MADCTL_MY | MADCTL_RGB;
    _width = HX8357_TFTHEIGHT;
    _height = HX8357_TFTWIDTH;
    break;
  case 2:
    m = MADCTL_RGB;
    _width = HX8357_TFTWIDTH;
    _height = HX8357_TFTHEIGHT;
    break;
  case 3:
    m = MADCTL_MX | MADCTL_MV | MADCTL_RGB;
    _width = HX8357_TFTHEIGHT;
    _height = HX8357_TFTWIDTH;
    break;
  }

  sendCommand(HX8357_MADCTL, &m, 1);
}

/*!
    @brief   Enable/Disable display color inversion
    @param   invert
             True to invert display, False for normal color.
    @return  None (void).
*/
void Adafruit_HX8357::invertDisplay(boolean invert) {
  sendCommand(invert ? HX8357_INVON : HX8357_INVOFF);
}

/*!
    @brief   Set the "address window" - the rectangle we will write to
             graphics RAM with the next chunk of SPI data writes. The
             HX8357 will automatically wrap the data as each row is filled.
    @param   x1
             Leftmost column of rectangle (screen pixel coordinates).
    @param   y1
             Topmost row of rectangle (screen pixel coordinates).
    @param   w
             Width of rectangle.
    @param   h
             Height of rectangle.
    @return  None (void).
*/
void Adafruit_HX8357::setAddrWindow(uint16_t x1, uint16_t y1, uint16_t w,
                                    uint16_t h) {
  uint16_t x2 = (x1 + w - 1), y2 = (y1 + h - 1);
  writeCommand(HX8357_CASET); // Column address set
  SPI_WRITE16(x1);
  SPI_WRITE16(x2);
  writeCommand(HX8357_PASET); // Row address set
  SPI_WRITE16(y1);
  SPI_WRITE16(y2);
  writeCommand(HX8357_RAMWR); // Write to RAM
}

/*!
    @brief   Scroll display memory
    @param   y How many pixels to scroll display by
*/
void Adafruit_HX8357::scrollTo(uint16_t y) {
  uint8_t data[2];
  data[0] = y >> 8;
  data[1] = y & 0xff;
  sendCommand(HX8357_VSCRSADD, (uint8_t *)data, 2);
}


/*!
    @brief   Set the height of the Top and Bottom Scroll Margins
    @param   top The height of the Top scroll margin
    @param   bottom The height of the Bottom scroll margin
 */
void Adafruit_HX8357::setScrollMargins(uint16_t top, uint16_t bottom) {
  // TFA+VSA+BFA must equal 320
  if (top + bottom <= HX8357_TFTHEIGHT) {
    uint16_t middle = HX8357_TFTHEIGHT - (top + bottom);
    uint8_t data[6];
    data[0] = top >> 8;
    data[1] = top & 0xff;
    data[2] = middle >> 8;
    data[3] = middle & 0xff;
    data[4] = bottom >> 8;
    data[5] = bottom & 0xff;
    sendCommand(HX8357_VSCRDEF, (uint8_t *)data, 6);
  }
}