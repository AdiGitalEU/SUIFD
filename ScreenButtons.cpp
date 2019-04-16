#include "ScreenButtons.h"

void ScreenButtons::init(TFT_eSPI *ptrTFT,XPT2046_Touchscreen *ptrTS)
{
    _tft = ptrTFT;
    _ts = ptrTS;
    buttonsCount = 0;
    nowPressedButton = NONE;
    lastPressedButton = NONE;
    prt.init(ptrTFT);
}

int ScreenButtons::add(uint16_t x,
                       uint16_t y,
                       uint16_t w,
                       uint16_t h,
                       String depressedButtonFile,
                       String pressedButtonFile,
                       String button_text){
    
    if (buttonsCount < BUTTONS_MAX_COUNT){
        buttonsCount++;
        buttons[buttonsCount].x = x;
        buttons[buttonsCount].y = y;
        
        buttons[buttonsCount].label = button_text;
        
        buttons[buttonsCount].pressedImage = pressedButtonFile;
        buttons[buttonsCount].depressedImage = depressedButtonFile;
        
        buttons[buttonsCount].currentState = depressed;
        buttons[buttonsCount].previousState = depressed;
        
        return buttonsCount;
    }
}
void ScreenButtons::drawButtons(){
    for (int i=1; i <= buttonsCount; i++) {
        drawButton(i);
    }
}

void ScreenButtons::drawButton(int buttonIndex){
    
    int len = buttons[buttonIndex].depressedImage.length() + 2;
    char buffer[len];
    buttons[buttonIndex].depressedImage.toCharArray(buffer,len);
    this->prt.drawJpeg(buffer, buttons[buttonIndex].x, buttons[buttonIndex].y);
    
    buttons[buttonIndex].w = JpegDec.width;
    buttons[buttonIndex].h = JpegDec.height;
    //printText(buttonIndex);
}

int ScreenButtons::update(){ //checking if and what screen area was touched to determine which button.
    if (_ts->touched()) {  //if screen is touched
        
        TS_Point p = _ts->getPoint();
        
        int touch_x = map(p.x, TOUCH_X_MIN, TOUCH_X_MAX, 319, 0); //remap values
        int touch_y = map(p.y, TOUCH_Y_MIN, TOUCH_Y_MAX, 239, 0);
        nowPressedButton = NONE;
        lastPressedButton = NONE;
        
        for (int i=1; i <= buttonsCount; i++) { //check which button is pressed if any
            if ((touch_x > buttons[i].x) && (touch_x < buttons[i].x + buttons[i].w) &&
                (touch_y > buttons[i].y) && (touch_y < buttons[i].y + buttons[i].h)){
                
                nowPressedButton = i;                //this button is pressed, let's remember its id
                
                buttons[i].currentState = pressed;      //set the current state
                redrawButton(i);                        //refresh it's look
                buttons[i].previousState = pressed;     //remember it was pressed
            } else {                                 // this button is not pressed
                buttons[i].currentState = depressed;    //in case it was previously touched set the current state as no longer pressed
                redrawButton(i);                        //and redraw
                buttons[i].previousState = depressed;   //and forget it was pressed
            }
        }
    } else {                                        // the screen is not touched or not ANYMORE touched
        
        nowPressedButton = NONE;                        // as it's not touched none of the buttons are pressed
        
        for (int i=1; i <= buttonsCount; i++) {         // now go over all the buttons
            buttons[i].currentState = depressed;        // and set each button to be depressed
            redrawButton(i);                            // and redraw it
            if (buttons[i].previousState == pressed) {lastPressedButton = i; // remember which one was last touched
                //make beep
                digitalWrite(TONE_PIN, 0);
                delay(50);
                digitalWrite(TONE_PIN, 1);
            }
            buttons[i].previousState = depressed;       // last touched remembered so we can clean it.
        }
    }
}

int ScreenButtons::getLastPressed(){
    update();
    int ret = lastPressedButton;
    lastPressedButton = NONE;
    return ret;
}

void ScreenButtons::printText(int buttonIndex){
    _tft->setTextDatum(CC_DATUM);
    _tft->drawString(buttons[buttonIndex].label,
                     buttons[buttonIndex].x + (buttons[buttonIndex].w / 2),
                     buttons[buttonIndex].y + (buttons[buttonIndex].h / 2), 1);
}

void ScreenButtons::redrawButton(int buttonIndex){
    
    switch (buttons[buttonIndex].currentState) {
        case depressed:
            if (buttons[buttonIndex].previousState == pressed) {
                
                int len = buttons[buttonIndex].depressedImage.length() + 2;
                char buffer[len];
                buttons[buttonIndex].depressedImage.toCharArray(buffer,len);
                this->prt.drawJpeg(buffer, buttons[buttonIndex].x, buttons[buttonIndex].y);
            }
            break;
        case pressed:
            if (buttons[buttonIndex].previousState == depressed) {
                
                int len = buttons[buttonIndex].pressedImage.length() + 2;
                char buffer[len];
                buttons[buttonIndex].pressedImage.toCharArray(buffer,len);
                this->prt.drawJpeg(buffer, buttons[buttonIndex].x, buttons[buttonIndex].y);
            }
            break;
        default:
            break;
    }
}

