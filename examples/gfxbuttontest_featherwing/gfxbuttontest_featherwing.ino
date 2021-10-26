/*
 * gfxbuttontest_featherwing
 */

#include <SPI.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_HX8357.h>
#include <Adafruit_STMPE610.h>


#ifdef ESP8266
   #define STMPE_CS 16
   #define TFT_CS   0
   #define TFT_DC   15
   #define SD_CS    2
#elif defined(ESP32) && !defined(ARDUINO_ADAFRUIT_FEATHER_ESP32S2)
   #define STMPE_CS 32
   #define TFT_CS   15
   #define TFT_DC   33
   #define SD_CS    14
#elif defined(TEENSYDUINO)
   #define TFT_DC   10
   #define TFT_CS   4
   #define STMPE_CS 3
   #define SD_CS    8
#elif defined(ARDUINO_STM32_FEATHER)
   #define TFT_DC   PB4
   #define TFT_CS   PA15
   #define STMPE_CS PC7
   #define SD_CS    PC5
#elif defined(ARDUINO_NRF52832_FEATHER)  /* BSP 0.6.5 and higher! */
   #define TFT_DC   11
   #define TFT_CS   31
   #define STMPE_CS 30
   #define SD_CS    27
#elif defined(ARDUINO_MAX32620FTHR) || defined(ARDUINO_MAX32630FTHR)
   #define TFT_DC   P5_4
   #define TFT_CS   P5_3
   #define STMPE_CS P3_3
   #define SD_CS    P3_2
#else
    // Anything else, defaults!
   #define STMPE_CS 6
   #define TFT_CS   9
   #define TFT_DC   10
   #define SD_CS    5
#endif



#define TFT_RST -1


// Use hardware SPI and the above for CS/DC
Adafruit_HX8357  tft = Adafruit_HX8357( TFT_CS,  TFT_DC,  TFT_RST);
Adafruit_STMPE610 ts = Adafruit_STMPE610(STMPE_CS);

// This is calibration data for the raw touch data to the screen coordinates
// For rotation 1, these put the buttons at the top of the screen
#define TS_MINX 3800
#define TS_MAXX 100
#define TS_MINY 100
#define TS_MAXY 3750

// Redefine original colors, add additional colors to match those available with the ILI9341 library
#define HX8357_BLACK       0x0000  ///<   0,   0,   0
#define HX8357_NAVY        0x000F  ///<   0,   0, 123
#define HX8357_DARKGREEN   0x03E0  ///<   0, 125,   0
#define HX8357_DARKCYAN    0x03EF  ///<   0, 125, 123
#define HX8357_MAROON      0x7800  ///< 123,   0,   0
#define HX8357_PURPLE      0x780F  ///< 123,   0, 123
#define HX8357_OLIVE       0x7BE0  ///< 123, 125,   0
#define HX8357_LIGHTGREY   0xC618  ///< 198, 195, 198
#define HX8357_DARKGREY    0x7BEF  ///< 123, 125, 123
#define HX8357_BLUE        0x001F  ///<   0,   0, 255
#define HX8357_GREEN       0x07E0  ///<   0, 255,   0
#define HX8357_CYAN        0x07FF  ///<   0, 255, 255
#define HX8357_RED         0xF800  ///< 255,   0,   0
#define HX8357_MAGENTA     0xF81F  ///< 255,   0, 255
#define HX8357_YELLOW      0xFFE0  ///< 255, 255,   0
#define HX8357_WHITE       0xFFFF  ///< 255, 255, 255
#define HX8357_ORANGE      0xFD20  ///< 255, 165,   0
#define HX8357_GREENYELLOW 0xAFE5  ///< 173, 255,  41
#define HX8357_PINK        0xFC18  ///< 255, 130, 198

// UI Buttondetails
#define BUTTON_X 40
#define BUTTON_Y 40
#define BUTTON_W 80
#define BUTTON_H 80
#define BUTTON_TEXTSIZE 2
#define DISPLAY_XOFFSET 80
#define DISPLAY_TEXTOFFSET 90
#define DISPLAY_YOFFSET 0

enum ButtonName {
  BTN_UP,
  BTN_SELECT,
  BTN_DOWN,
  BTN_RETURN
};

#define MENU1_BTN_CNT 4
Adafruit_GFX_Button Menu1Buttons[MENU1_BTN_CNT];
char Menu1Labels[MENU1_BTN_CNT][5] = {"Up", "Sel", "Down", "Ret"};
uint16_t Menu1Colors[MENU1_BTN_CNT] = {HX8357_DARKGREY, HX8357_DARKGREY, 
                               HX8357_DARKGREY, HX8357_DARKGREY};

#define MENU2_BTN_CNT 4
Adafruit_GFX_Button Menu2Buttons[MENU2_BTN_CNT];
char Menu2Labels[MENU2_BTN_CNT][5] = {"Up", "Sel", "Down", "Ret"};
uint16_t Menu2Colors[MENU2_BTN_CNT] = {HX8357_BLUE, HX8357_BLUE, 
                               HX8357_BLUE, HX8357_BLUE};

int textSize = 2;
int textColorIndex = 0;
uint16_t textColor[7] = {
 HX8357_WHITE,
 HX8357_RED,
 HX8357_GREEN,
 HX8357_BLUE,
 HX8357_CYAN,
 HX8357_MAGENTA,
 HX8357_YELLOW
};

//-------------------------------------------------------------------------
void setTextColorIndex(int updown) {
    textColorIndex += updown;
    if (textColorIndex > 6)  
        textColorIndex = 0;
    else if (textColorIndex < 0)
        textColorIndex = 6;
     tft.setTextColor(textColor[textColorIndex]);
}

//-------------------------------------------------------------------------
void setTextSizeIndex(int updown) {
    textSize += updown;
    if (textSize > 3)  
        textSize = 1;
    else if (textSize < 1)
        textSize = 3;
     tft.setTextSize(textSize);
}

//-------------------------------------------------------------------------
bool initializeButtons(
            Adafruit_GFX_Button menuButtons[], 
            uint16_t menuColors[], 
            char menuLabels[][5], 
            int menuButtonCount) {
     tft.fillScreen(HX8357_BLACK);

    for (uint8_t row=0; row<menuButtonCount; row++) 
    {
        menuButtons[row].initButton(& tft, 
            BUTTON_X, 
            BUTTON_Y + row *(BUTTON_H),    
            BUTTON_W, 
            BUTTON_H, 
            HX8357_BLACK, 
            menuColors[row], 
            HX8357_WHITE,
            menuLabels[row], BUTTON_TEXTSIZE);    
        menuButtons[row].drawButton();
    }
    return true;
}

//-------------------------------------------------------------------------
void tftSetCenterCursor(String str, int16_t xIn, int16_t yIn) {
  int16_t xText, yText;
  uint16_t w, h;  
  
    tft.getTextBounds(str, 0, 0, &xText, &yText, &w, &h);
    tft.setCursor(xIn-w/2, yIn-h/2);  
}

//--------------------------------------------------------------------------------
void tftPrint(String str) {
    int16_t xText, yText;
    uint16_t w, h;  

      tft.getTextBounds(str, 0, 0, &xText, &yText, &w, &h);
      tft.fillRect(  tft.getCursorX(),   tft.getCursorY(), w, h, HX8357_BLACK);
      tft.print(str);
}

//-------------------------------------------------------------------------
void tftCenterPrint(String str) {
      tft.setTextSize(textSize);
      tftSetCenterCursor(str, 
                   DISPLAY_TEXTOFFSET + (  tft.width()-DISPLAY_TEXTOFFSET)/2, 
                   DISPLAY_YOFFSET + ( tft.height()-DISPLAY_YOFFSET)/2);
     tftPrint(str);
}
            
//--------------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("");

    // if the touchscreen returns true, assume we have the  tft device
    if (!ts.begin())
        return;
        
    Serial.println("Touchscreen started");
     tft.begin();
     tft.setRotation(1);
     tft.setTextWrap(false);
                              
    initializeButtons(Menu1Buttons, Menu1Colors, Menu1Labels, MENU1_BTN_CNT);
}

//--------------------------------------------------------------------------------
int  tftButtonRelease(Adafruit_GFX_Button menuButtons[], int menuButtonCount) {
    int btn = -1;
    TS_Point p;
    
    if (ts.bufferSize()) 
    {
        p = ts.getPoint(); 
    } 
    else 
    {
        // this is our way of tracking touch 'release'!
        p.x = p.y = p.z = -1;
    }
  
    // Scale from ~0->4000 to  tft.width using the calibration #'s
    if (p.z != -1) 
    {
        int16_t px = p.x;
        int16_t py = p.y;
        p.x = map(py, TS_MINY, TS_MAXY, 0,  tft.width());
        p.y = map(px, TS_MINX, TS_MAXX, 0,  tft.height()); 
    }

    // go thru all the buttons, checking if they were pressed
    for (uint8_t b=0; b<menuButtonCount; b++) 
    {
        if (menuButtons[b].contains(p.x, p.y)) 
        {
          Serial.print("Pressing: "); Serial.println(b);
          menuButtons[b].press(true);  // tell the button it is pressed
        } 
        else 
        {
          menuButtons[b].press(false);  // tell the button it is NOT pressed
        }
    }
    
    // now we can ask the buttons if their state has changed
    for (uint8_t b=0; b<menuButtonCount; b++) 
    {
        if (menuButtons[b].justReleased()) 
        {
          Serial.print("Released: "); Serial.println(b);
          menuButtons[b].drawButton();  // draw normal
          btn = b;
        }

        if (menuButtons[b].justPressed()) 
        {
            menuButtons[b].drawButton(true);  // draw invert!
            delay(100); // UI debouncing
        }
    }
    return btn;
}

//--------------------------------------------------------------------------------
void processMenu2() {
    String msg = "";
    bool exitLoop = false;

    initializeButtons(Menu2Buttons, Menu2Colors, Menu2Labels, MENU2_BTN_CNT);

    msg = "   Menu 2 Up button   ";
    Serial.println(msg);
    setTextColorIndex(0);
    setTextSizeIndex(0);
     tftCenterPrint(msg);
            
    while (!exitLoop)
    {   
        int btn =  tftButtonRelease(Menu2Buttons, MENU2_BTN_CNT);
        if (btn >= 0 && btn < MENU2_BTN_CNT)
        {
            Serial.print("btn = "); Serial.println(btn);
        }

        switch (btn)
        {
          case BTN_UP:
            msg = "Menu 2 Up button      ";
            Serial.println(msg);
            setTextColorIndex(1);
             tftCenterPrint(msg);
            break;
            
          case BTN_SELECT:
            msg = "Menu 2 Select Button  ";
            Serial.println(msg);
            setTextColorIndex(0);
             tftCenterPrint(msg);
            break;
            
          case BTN_DOWN:
            msg = "Menu 2 Down Button    ";
            Serial.println(msg);
            setTextColorIndex(-1);
             tftCenterPrint(msg);
            break;
            
          case BTN_RETURN:
            msg = "Menu 2 Ret Button     ";
            Serial.println(msg);
            Serial.println("Exiting Menu2, Returing to Menu1");

            exitLoop = true;
            break;
            
          default:
            break;
        }
    }
}

//--------------------------------------------------------------------------------
void processMenu1() {
    String msg = "";
    
    int btn =  tftButtonRelease(Menu1Buttons, MENU1_BTN_CNT);
    if (btn >= 0 && btn < MENU1_BTN_CNT)
    {
        Serial.print("btn = "); Serial.println(btn);
    }

    switch (btn)
    {
      case BTN_UP:
        msg = "Menu 1 Up Button    ";
        Serial.println(msg);
         tft.fillRect(DISPLAY_XOFFSET,DISPLAY_YOFFSET, tft.width(), tft.height(), HX8357_BLACK);
        setTextColorIndex(0);
        setTextSizeIndex(1);
         tftCenterPrint(msg);
        break;
        
      case BTN_SELECT:
        msg = "Menu 1 Select Button";
        Serial.println(msg);
        processMenu2();

        initializeButtons(Menu1Buttons, Menu1Colors, Menu1Labels, MENU1_BTN_CNT);

        msg = "Returned from Menu 2";
        Serial.println(msg);
        setTextColorIndex(0);
        setTextSizeIndex(0);
        tftCenterPrint(msg);
        break;
        
      case BTN_DOWN:
        msg = "Menu 1 Down Button  ";
        Serial.println(msg);
         tft.fillRect(DISPLAY_XOFFSET,DISPLAY_YOFFSET, tft.width(), tft.height(), HX8357_BLACK);
        setTextColorIndex(0);
        setTextSizeIndex(-1);
         tftCenterPrint(msg);
        break;
        
      case BTN_RETURN:
        msg = "Menu 1 Ret Button   ";
        Serial.println(msg);
        tftCenterPrint(msg);
        break;
        
      default:
        break;
    }
}

//--------------------------------------------------------------------------------
void loop() {

    processMenu1();
   
}
