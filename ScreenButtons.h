#ifndef ScreenButtons_h
#define ScreenButtons_h
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include "JPEGPrint.h"

#define FS_NO_GLOBALS //required for SPIFFS/JPEGDecoder - to find out what it does!
#define BUTTONS_MAX_COUNT 40

#define TOUCH_X_MIN 180
#define TOUCH_X_MAX 3800
#define TOUCH_Y_MIN 260
#define TOUCH_Y_MAX 3800

#define NONE -1
#define TONE_PIN D2

enum buttonState{
    pressed,
    depressed
};

typedef struct
{
    uint16_t x, y, w, h;
    String label;
    
    buttonState currentState;
    buttonState previousState;
    
    String pressedImage;
    String depressedImage;
    
} button;

class ScreenButtons
{
public:
    void init(TFT_eSPI *ptrTFT,XPT2046_Touchscreen *ptrTS);
    
    int add(uint16_t x,
            uint16_t y,
            uint16_t w,
            uint16_t h,
            String depressedButtonFile,
            String pressedButtonFile,
            String button_text);
    
    void drawButtons();
    void drawButton(int buttonIndex);
    void printText(int buttonIndex);
    int update();
    int nowPressedButton;
    int lastPressedButton;
    int getLastPressed();
    int buttonsCount;
    
    void drawJpeg(char *filename, int xpos, int ypos);
    void renderJPEG(int xpos, int ypos);
private:
    void redrawButton(int buttonIndex);
protected:
    TFT_eSPI *_tft;
    XPT2046_Touchscreen *_ts;
    button buttons[BUTTONS_MAX_COUNT];
    JPEGPrint prt;
};
#endif /* ScreenButtons_h */
