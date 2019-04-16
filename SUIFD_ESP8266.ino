//#define DEV

//included in ...esp8266\hardware\
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Ticker.h>  

//used versions:
#include <ArduinoJson.h>          //version 5.12.0  "https://github.com/bblanchon/ArduinoJson.git"
#include <TFT_eSPI.h>             //version 0.16.15 "https://github.com/Bodmer/TFT_eSPI"
#include <XPT2046_Touchscreen.h>  //version 1.2     "https://github.com/PaulStoffregen/XPT2046_Touchscreen.git"
             

#include "ScreenButtons.h"
#include "JPEGPrint.h"
#include "Free_Fonts.h"

#define FS_NO_GLOBALS //required for SPIFFS/JPEGDecoder - not sure why: to find out what it does!

#define TS_CS_PIN 16
#define TS_IRQ_PIN 5

#define PAGE_STATUS 1
#define PAGE_HEAD 2
#define PAGE_TEMP 3
#define PAGE_MISC 4

TFT_eSPI tft = TFT_eSPI();
XPT2046_Touchscreen ts(TS_CS_PIN, TS_IRQ_PIN); // Param 2 - Touch IRQ Pin -

Ticker commTimer;
Ticker buttonsTimer;

IPAddress suifd_IP;

ScreenButtons FunctionKeys;

JPEGPrint prt;

String jsonData = "";

bool jsonDataReady = false;
bool timeToGetData = false;
bool timeToScanButtons = false;

float bedCurrentTemperature;
float extruderCurrentTemperature;

float bedActiveTemperature;
float extruderActiveTemperature;

int heater_status_0;
int heater_status_1;

float headPosition_X;
float headPosition_Y;
float headPosition_Z;

int speed_factor;

int extrusion_factor;

float babystep;
int tool;
int zprobe;

int fanPercent_0;
int fanPercent_1;
int fanPercent_2;
int fanPercent_3;

int fanRPM;

bool homed_x;
bool homed_y;
bool homed_z;

float print_progress;

bool timeToRefreshGraph = false;
bool timeToGetGraphData = false;
uint8_t extruderTemperatures[140] = {20};
uint8_t bedTemperatures[140] = {20};

int fraction_printed;

int btn_status, btn_head, btn_temp, btn_misc;

uint8_t lightLevel = 100;
uint8_t filamentFanLevel = 0;

const char *ssid = "YourSSID";
const char *password = "YourPassword";

void setup()
{
    tft.init();
    tft.setRotation(3);
    ts.begin();

    Serial.begin(115200);

    if (!SPIFFS.begin())
    { //Serial.println("initialisation failed!")
        ;
    }

    prt.init(&tft);

    prt.drawJpeg("/BG.jpg", 0, 0);

    FunctionKeys.init(&tft, &ts);

    btn_status = FunctionKeys.add(0, 205, 80, 39, "/btn_Status.jpg", "/pressed/btn_Status.jpg", "");
    btn_head = FunctionKeys.add(80, 205, 80, 39, "/btn_Head.jpg", "/pressed/btn_Head.jpg", "");
    btn_temp = FunctionKeys.add(160, 205, 80, 39, "/btn_Temp.jpg", "/pressed/btn_Temp.jpg", "");
    btn_misc = FunctionKeys.add(240, 205, 80, 39, "/btn_Misc.jpg", "/pressed/btn_Misc.jpg", "");
    FunctionKeys.drawButtons();

    commTimer.attach(0.10, timeToRefreshEvent);
    buttonsTimer.attach(0.05, timeToScanButtonsEvent);

    pinMode(TONE_PIN, OUTPUT);
    digitalWrite(TONE_PIN, 1);

    //OTA_setup();
}

boolean wastouched = true;
int currentPage = PAGE_STATUS;

void loop()
{
    switch (currentPage)
    {
    case PAGE_STATUS:
        currentPage = page_status();
        break;
    case PAGE_HEAD:
        currentPage = page_head();
        break;
    case PAGE_TEMP:
        currentPage = page_temperatures();
        break;
    case PAGE_MISC:
        currentPage = page_misc();
        break;
    default:
        break;
    }
}

int page_status()
{
    #define X_OFFSET 12
    #define Y_OFFSET 38
    bool onPage = true;
    int lastFunctionKey = NONE;
    int lastStatusKey = NONE;

    ScreenButtons statusKeys;
    statusKeys.init(&tft, &ts);

    int Z_babystep_inc = statusKeys.add( 240, 56, 72, 45, "/btn_Ofst_inc.jpg", "/pressed/btn_Ofst_inc.jpg", "");
    int Z_babystep_dec = statusKeys.add( 240, 101, 72, 45, "/btn_Ofst_dec.jpg", "/pressed/btn_Ofst_dec.jpg", "");
    int E_flow_inc = statusKeys.add( 163, 56, 72, 45, "/btn_Flow_inc.jpg", "/pressed/btn_Flow_inc.jpg", "");
    int E_flow_dec = statusKeys.add( 163, 101, 72, 45, "/btn_Flow_dec.jpg", "/pressed/btn_Flow_dec.jpg", "");
    
    int pause = statusKeys.add( 8, 156, 72, 33, "/btn_Pause.jpg", "/pressed/btn_Pause.jpg", "");
    int run = statusKeys.add( 85, 156, 72, 33, "/btn_Run.jpg", "/pressed/btn_Run.jpg", "");
    int slow = statusKeys.add( 163, 156, 72, 33, "/btn_Slow.jpg", "/pressed/btn_Slow.jpg", "");
    int fast = statusKeys.add( 240, 156, 72, 33, "/btn_Fast.jpg", "/pressed/btn_Fast.jpg", "");

    prt.drawJpeg("/BG_Status.jpg", 0, 0);   
    statusKeys.drawButtons();

    FunctionKeys.drawButtons();

    tft.setTextDatum(BR_DATUM);
    tft.setTextPadding(30);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString("Extr         Bed      ", 150, 44,2);
    // tft.drawString("Bed:        Extr:     ", 150, 44,2);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString("%",204,44,2);
    tft.drawString("%",252,44,2);
    uint8_t e_factor = 0;
    uint8_t s_factor = 0;
    float b_step = 0.0; 
    uint8_t padd3 = tft.textWidth("999", 2);
    uint8_t padd5 = tft.textWidth("-1.99", 2);

    tft.setTextPadding(padd3);
    tft.drawNumber(speed_factor, 192, 44, 2);
    tft.drawNumber(extrusion_factor, 240, 44, 2);
    tft.setTextPadding(padd5);
    tft.drawFloat(babystep, 2, 300, 44, 2);
    
    while (onPage)
    {
        heartBit();

        if (timeToScanButtons)
        {
            lastFunctionKey = FunctionKeys.getLastPressed();
            lastStatusKey = statusKeys.getLastPressed();
            timeToScanButtons = false;

            tft.setTextColor(TFT_CYAN, TFT_BLACK);

            
            if(speed_factor != s_factor){
                tft.setTextPadding(padd3);
                tft.drawNumber(speed_factor, 192, 44, 2);
                s_factor = speed_factor;
            }
            if(extrusion_factor != e_factor){
                tft.setTextPadding(padd3);
                tft.drawNumber(extrusion_factor, 240, 44, 2);
                e_factor = extrusion_factor;
            }
            if(babystep != b_step){
                tft.setTextPadding(padd5);
                tft.drawFloat(babystep, 2, 300, 44, 2);
                b_step = babystep;
            }

            if(lastStatusKey == Z_babystep_inc)
                Serial.print("M290 S0.05\n");
            if(lastStatusKey == Z_babystep_dec)
                Serial.print("M290 S-0.05\n");

            if(lastStatusKey == E_flow_inc)
                Serial.printf("M221 S%d\n", e_factor += 5);
            if(lastStatusKey == E_flow_dec)
                Serial.printf("M221 S%d\n", e_factor -= 5);
                
            if(lastStatusKey == slow)
                Serial.printf("M220 S%d\n", s_factor -= 5);
            if(lastStatusKey == fast)
                Serial.printf("M220 S%d\n", s_factor += 5);

            if(lastStatusKey == pause)
                Serial.print("M25\n");
            if(lastStatusKey == run)
                Serial.print("M24\n");                
        }

        if (lastFunctionKey != NONE)
        {
            if (lastFunctionKey != btn_status)
            {
                onPage = false;
            }
        }

        #ifdef DEV
            if (true)
        #else
            if (timeToRefreshGraph)
        #endif
        {
            uint16_t lineColor = tft.color565(0, 50, 0);
            uint16_t curColor;

            for (uint8_t x = 0; x < 140; x++) //for each X pixel line
            {
                if (x % 10)
                {
                    curColor = TFT_BLACK;
                }
                else
                {
                    curColor = lineColor;
                }

                tft.drawFastVLine(x + X_OFFSET, Y_OFFSET + 10, 90, curColor); //draw vertical line

                for (uint8_t s = Y_OFFSET + 10; s < 95 + Y_OFFSET; s += 10)      //draw pixels spaced 10pixel
                {
                    tft.drawPixel(x + X_OFFSET, s, lineColor);
                }


                tft.drawPixel(x + X_OFFSET, map(extruderTemperatures[139 - x], 10, 250, 95 + Y_OFFSET, 45), TFT_YELLOW);
                tft.drawPixel(x + X_OFFSET,      map(bedTemperatures[139 - x], 10, 250, 95 + Y_OFFSET, 45), TFT_CYAN);
            }

            barGraph((uint8_t)(print_progress * 100));
            tft.setTextColor(TFT_CYAN, TFT_BLACK);
            tft.drawNumber(bedTemperatures[0], 149, 45, 2);
            tft.setTextColor(TFT_YELLOW, TFT_BLACK);
            tft.drawNumber(extruderTemperatures[0], 75, 45, 2);
            timeToRefreshGraph = false;
        }
    }
    return lastFunctionKey;
}

int page_head()
{
    bool onPage = true;
    int lastFunctionKey = NONE;
    int lastHeadKey = NONE;

    prt.drawJpeg("/BG_Head.jpg", 0, 0);

    ScreenButtons HeadKeys;
    HeadKeys.init(&tft, &ts);

    int Home_X = HeadKeys.add(10, 24, 64, 25, "/btn_Home_X.jpg", "/pressed/btn_Home_X.jpg", "");
    int Home_Y = HeadKeys.add(80, 24, 64, 25, "/btn_Home_Y.jpg", "/pressed/btn_Home_Y.jpg", "");
    int Home_Z = HeadKeys.add(151, 24, 64, 25, "/btn_Home_Z.jpg", "/pressed/btn_Home_Z.jpg", "");

    int Home_All = HeadKeys.add(231, 24, 80, 34, "/btn_Home_All.jpg", "/pressed/btn_Home_All.jpg", "");

    int Retract = HeadKeys.add(239, 98, 72, 42, "/btn_Retract.jpg", "/pressed/btn_Retract.jpg", "");
    int Extrude = HeadKeys.add(239, 147, 72, 42, "/btn_Extrude.jpg", "/pressed/btn_Extrude.jpg", "");

    int X_inc = HeadKeys.add(10, 87, 64, 48, "/btn_X+.jpg", "/pressed/btn_X+.jpg", "");
    int X_dec = HeadKeys.add(10, 141, 64, 48, "/btn_X-.jpg", "/pressed/btn_X-.jpg", "");

    int Y_inc = HeadKeys.add(80, 87, 64, 48, "/btn_Y+.jpg", "/pressed/btn_Y+.jpg", "");
    int Y_dec = HeadKeys.add(80, 141, 64, 48, "/btn_Y-.jpg", "/pressed/btn_Y-.jpg", "");

    int Z_inc = HeadKeys.add(151, 87, 64, 48, "/btn_Z+.jpg", "/pressed/btn_Z+.jpg", "");
    int Z_dec = HeadKeys.add(151, 141, 64, 48, "/btn_Z-.jpg", "/pressed/btn_Z-.jpg", "");


    float displayHeadPosition_X = -2;
    float displayHeadPosition_Y = -2;
    float displayHeadPosition_Z = -2;

    HeadKeys.drawButtons();
    tft.setTextDatum(BR_DATUM);
    while (onPage)
    {
        heartBit();

        if (timeToScanButtons)
        {
            lastFunctionKey = FunctionKeys.getLastPressed();
            lastHeadKey = HeadKeys.getLastPressed();

            if (lastHeadKey == Home_All)
                Serial.print("G28\n");
            if (lastHeadKey == Home_X)
                Serial.print("G28 X\n");
            if (lastHeadKey == Home_Y)
                Serial.print("G28 Y\n");
            if (lastHeadKey == Home_Z)
                Serial.print("G28 Z\n");
            if (lastHeadKey == X_inc)
            {
                Serial.print("G91\n");
                Serial.print("G1 X10 F3600\n");
                Serial.print("G90\n");
            }
            if (lastHeadKey == X_dec)
            {
                Serial.print("G91\n");
                Serial.print("G1 X-10 S1 F3600\n");
                Serial.print("G90\n");
            }
            if (lastHeadKey == Y_inc)
            {
                Serial.print("G91\n");
                Serial.print("G1 Y10 F3600\n");
                Serial.print("G90\n");
            }
            if (lastHeadKey == Y_dec)
            {
                Serial.print("G91\n");
                Serial.print("G1 Y-10 S1 F3600\n");
                Serial.print("G90\n");
            }
            if (lastHeadKey == Z_inc)
            {
                Serial.print("G91\n"); 
                Serial.print("G1 Z-10 F3600\n"); //reversed direction to match bed movement
                Serial.print("G90\n");
            }
            if (lastHeadKey == Z_dec)
            {
                Serial.print("G91\n");
                Serial.print("G1 Z10 S1 F3600\n"); //reversed direction to match bed movement
                Serial.print("G90\n");
            }
            if (lastHeadKey == Extrude)
            {
                Serial.print("G1 E5 F300");
            }
            if (lastHeadKey == Retract)
            {
                Serial.print("G1 E-5 F2000");
            }

            timeToScanButtons = false;
        }

        if (lastFunctionKey != NONE)
        {
            if (lastFunctionKey != btn_head)
            {
                onPage = false;
            }
        }

        if (!homed_x) headPosition_X = -1;
        if (!homed_y) headPosition_Y = -1;
        if (!homed_z) headPosition_Z = -1;

        if (displayHeadPosition_X != headPosition_X)
        {
            displayHeadPosition_X = headPosition_X;
            tft.setTextColor(tft.color565(255, 0, 255), TFT_BLACK);
            tft.setTextPadding(55);
            if (headPosition_X != -1)
                tft.drawFloat(displayHeadPosition_X, 1, 63, 73, 2); 
            else
                tft.drawString("---.-", 63, 73, 2); //GFXFF);
        }

        if (displayHeadPosition_Y != headPosition_Y)
        {
            displayHeadPosition_Y = headPosition_Y;
            tft.setTextColor(TFT_GREEN, TFT_BLACK);
            tft.setTextPadding(55);
            if (headPosition_Y != -1)
                tft.drawFloat(displayHeadPosition_Y, 1, 136, 73, 2); 
            else
                tft.drawString("---.-", 136, 73, 2); //GFXFF);    
        }

        if (displayHeadPosition_Z != headPosition_Z)
        {
            displayHeadPosition_Z = headPosition_Z;
            tft.setTextColor(tft.color565(0, 128, 255), TFT_BLACK);
            tft.setTextPadding(60);
            if (headPosition_Z != -1)
                tft.drawFloat(displayHeadPosition_Z, 2, 207, 73, 2); 
            else
                tft.drawString("---.-", 207, 73, 2); 
        }
        
    }
    return lastFunctionKey;
}

int page_temperatures()
{
    bool onPage = true;
    int lastFunctionKey = NONE;
    int lastTempKey = NONE;

    prt.drawJpeg("/BG_Temp.jpg", 0, 0);
    
    ScreenButtons TempKeys;
    TempKeys.init(&tft, &ts);

    int Ext_on  = TempKeys.add( 8, 83, 64, 50, "/btn_Extruder_on.jpg",  "/pressed/btn_Extruder_on.jpg", "");
    int Ext_inc = TempKeys.add(73, 83, 80, 50, "/btn_Extruder_inc.jpg", "/pressed/btn_Extruder_inc.jpg", "");

    int Bed_on = TempKeys.add(167, 83, 64, 50, "/btn_Bed_on.jpg",  "/pressed/btn_Bed_on.jpg", "");
    int Bed_inc = TempKeys.add(233, 83, 80, 50, "/btn_Bed_inc.jpg", "/pressed/btn_Bed_inc.jpg", "");

    int Ext_off  = TempKeys.add( 8, 139, 64, 50, "/btn_Extruder_off.jpg",  "/pressed/btn_Extruder_off.jpg", "");
    int Ext_dec = TempKeys.add(73, 139, 80, 50, "/btn_Extruder_dec.jpg", "/pressed/btn_Extruder_dec.jpg", "");

    int Bed_off = TempKeys.add(167, 139, 64, 50, "/btn_Bed_off.jpg",  "/pressed/btn_Bed_off.jpg", "");
    int Bed_dec = TempKeys.add(233, 139, 80, 50, "/btn_Bed_dec.jpg", "/pressed/btn_Bed_dec.jpg", "");

    TempKeys.drawButtons();
    tft.setTextDatum(BR_DATUM);

#ifdef DEV
    float displayBedActiveTemperature = 999.9;
    float displayBedCurrentTemperature = 999.9;
    float displayExtruderActiveTemperature = 999.9;
    float displayExtruderCurrentTemperature = 999.9;
#else
    float displayBedActiveTemperature = -1; //bedActiveTemperature;
    float displayBedCurrentTemperature = -1;
    float displayExtruderActiveTemperature = -1;
    float displayExtruderCurrentTemperature = -1;
#endif

    tft.setTextColor(TFT_GREEN, TFT_BLACK);

    tft.setTextPadding(50);
    #define DISP_EXT_ACTIVE_XY       60, 67
    #define DISP_BED_ACTIVE_XY      220, 67
    #define DISP_EXT_CURRENT_XY     143, 67
    #define DISP_BED_CURRENT_XY     300, 67

    tft.drawFloat(displayExtruderActiveTemperature, 1, DISP_EXT_ACTIVE_XY, 2); //GFXFF);
    tft.drawFloat(displayBedActiveTemperature,      1, DISP_BED_ACTIVE_XY, 2); //GFXFF);

    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawFloat(displayExtruderCurrentTemperature, 1, DISP_EXT_CURRENT_XY, 2); //GFXFF);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawFloat(displayBedCurrentTemperature,     1, DISP_BED_CURRENT_XY, 2); //GFXFF);

    while (onPage)
    {
        heartBit();

        if (timeToScanButtons)
        {
            lastFunctionKey = FunctionKeys.getLastPressed();
            lastTempKey = TempKeys.getLastPressed();

            if (displayExtruderActiveTemperature != extruderActiveTemperature)
            {
                displayExtruderActiveTemperature = extruderActiveTemperature;
                tft.setTextColor(TFT_GREEN, TFT_BLACK);
                if(extruderActiveTemperature != 0){
                    tft.drawFloat(displayExtruderActiveTemperature, 1, DISP_EXT_ACTIVE_XY, 2);
                } else {
                    tft.drawString(" OFF ", DISP_EXT_ACTIVE_XY, 2);
                }
            }
            if (displayBedActiveTemperature != bedActiveTemperature)
            {
                
                displayBedActiveTemperature = bedActiveTemperature;
                tft.setTextColor(TFT_GREEN, TFT_BLACK);
                if(bedActiveTemperature != 0){
                    tft.drawFloat(displayBedActiveTemperature, 1, DISP_BED_ACTIVE_XY, 2); //GFXFF);
                } else {
                    tft.drawString(" OFF ", DISP_BED_ACTIVE_XY, 2);
                }
            }
            if (displayExtruderCurrentTemperature != extruderCurrentTemperature)
            {
                displayExtruderCurrentTemperature = extruderCurrentTemperature;
                tft.setTextColor(TFT_YELLOW, TFT_BLACK);
                tft.drawFloat(displayExtruderCurrentTemperature, 1, DISP_EXT_CURRENT_XY, 2); //GFXFF);
            }
            if (displayBedCurrentTemperature != bedCurrentTemperature)
            {
                displayBedCurrentTemperature = bedCurrentTemperature;
                tft.setTextColor(TFT_CYAN, TFT_BLACK);
                tft.drawFloat(displayBedCurrentTemperature, 1, DISP_BED_CURRENT_XY, 2); //GFXFF);
            }    
            if(lastTempKey == Ext_on)
                Serial.printf("M104 S%d\n", 170);
            if(lastTempKey == Ext_off)
                Serial.printf("M104 S%d\n", 0);
            if(lastTempKey == Bed_on)
                Serial.printf("M140 S%d\n", 60);
            if(lastTempKey == Bed_off)
                Serial.printf("M140 S%d\n", 0);

            if(lastTempKey == Ext_inc)
                Serial.printf("M104 S%d\n", (uint8_t)(extruderActiveTemperature += 5));
            if(lastTempKey == Ext_dec)
                Serial.printf("M104 S%d\n", (uint8_t)(extruderActiveTemperature -= 5));
            if(lastTempKey == Bed_inc)
                Serial.printf("M140 S%d\n", (uint8_t)(bedActiveTemperature += 5));
            if(lastTempKey == Bed_dec)
                Serial.printf("M140 S%d\n", (uint8_t)(bedActiveTemperature -= 5));

            timeToScanButtons = false;
        }

        if (lastFunctionKey != NONE)
        {
            if (lastFunctionKey != btn_temp)
            {
                onPage = false;
            }
        }

           
    }
    return lastFunctionKey;
}

int page_misc()
{
    bool onPage = true;
    int lastFunctionKey = NONE;
    int lastMiscKey = NONE;

    tft.setTextDatum(BR_DATUM);

    prt.drawJpeg("/BG_Misc.jpg", 0, 0);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("IP: " + suifd_IP.toString(), 306, 14, 1);

    ScreenButtons MiscKeys;
    MiscKeys.init(&tft, &ts);

    int Fan_inc = MiscKeys.add( 8, 82, 64, 50, "/btn_Fan_inc.jpg",  "/pressed/btn_Fan_inc.jpg", "");
    int Fan_dec = MiscKeys.add(8, 139, 80, 50, "/btn_Fan_dec.jpg", "/pressed/btn_Fan_dec.jpg", "");

    int Light_inc = MiscKeys.add(81, 82, 64, 50, "/btn_Light_inc.jpg",  "/pressed/btn_Light_inc.jpg", "");
    int Light_dec = MiscKeys.add(81, 139, 80, 50, "/btn_Light_dec.jpg", "/pressed/btn_Light_dec.jpg", "");

    int Motors_off = MiscKeys.add(248, 25, 64, 50, "/btn_Motors_off.jpg",  "/pressed/btn_Motors_off.jpg", "");
    int Level_bed  = MiscKeys.add(248, 82, 80, 50, "/btn_Level_bed.jpg", "/pressed/btn_Level_bed.jpg", "");
    int Stop       = MiscKeys.add(248, 139, 64, 50, "/btn_Stop.jpg",  "/pressed/btn_Stop.jpg", "");
    

    MiscKeys.drawButtons();

    filamentFanLevel = fanPercent_0 / 100;//map(fanPercent_0, 0, 255, 0, 100);

    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextPadding(30);
    tft.drawString("%", 61, 67, 2);
    static char outstr[5];

    tft.drawNumber(filamentFanLevel, 49, 67, 2);

    while (onPage)
    {
        heartBit();

        if (timeToScanButtons)
        {
            tft.drawNumber(fanPercent_0, 49, 67, 2);
            filamentFanLevel = fanPercent_0 / 10;
            lastFunctionKey = FunctionKeys.getLastPressed();
            lastMiscKey = MiscKeys.getLastPressed();
            
            if(lastMiscKey == Motors_off)
                Serial.print("M84\n");
            if(lastMiscKey == Level_bed)
                Serial.print("G32\n");
            if(lastMiscKey == Stop)
                Serial.print("M0\n");

            if(lastMiscKey == Fan_inc){ 
                filamentFanLevel += 1;
                if (filamentFanLevel >= 10){
                    filamentFanLevel = 10;                                                   
                    Serial.print("M106 P0 S1.0");
                } else {
                    Serial.print("M106 P0 S0.");
                    Serial.println(filamentFanLevel);    
                }

                //tft.drawNumber(fanPercent_0, 49, 67, 2);
            }
            if(lastMiscKey == Fan_dec){
                if (fanPercent_0 > 0) {
                    filamentFanLevel -= 1;
                } else {
                    filamentFanLevel = 0;
                }
                //if (filamentFanLevel <= 0) filamentFanLevel = 0;
                Serial.print("M106 P0 S0.");
                Serial.println(filamentFanLevel);
                //tft.drawNumber(fanPercent_0, 49, 67, 2);
            }

            if(lastMiscKey == Light_inc){
                if(lightLevel > 90) lightLevel = 90;
                Serial.printf("M42 P2 S%d\n", map(lightLevel += 10, 0, 100, 0, 255));
            }
            if(lastMiscKey == Light_dec){
                if(lightLevel <= 10) lightLevel = 10;
                Serial.printf("M42 P2 S%d\n", map(lightLevel -= 10, 0, 100, 0, 255));
            }


            timeToScanButtons = false;
        }

        if (lastFunctionKey != NONE)
        {
            if (lastFunctionKey != btn_misc)
            {
                //Serial.println("getting out of temp");
                onPage = false;
            }
        }
    }
    return lastFunctionKey;
}

void heartBit()
{

    while (Serial.available())
    {

        char inChar = (char)Serial.read();
        jsonData += inChar;
        if (inChar == '\n')
        {
            jsonDataReady = true;
        }
    }

    if (timeToGetData)
    {
        //ArduinoOTA.handle(); //in delayed interval to avoid too frequent calls from main loop
        Serial.println("M408 S0\n");

        if (jsonDataReady)
        {
            parseJsonData(jsonData.c_str());
            jsonDataReady = false;
            jsonData = "";
        }
        timeToGetData = false;
    }
        #ifdef DEV
            if (true)
        #else
            if (timeToGetGraphData)
        #endif
    
    {
        for (uint8_t i = 139; i > 0; i--)
        {
            extruderTemperatures[i] = extruderTemperatures[i - 1];
            bedTemperatures[i] = bedTemperatures[i - 1];
        }
        #ifdef DEV
                extruderTemperatures[0] = map(analogRead(A0), 0, 1024, 0, 250);
                bedTemperatures[0] = map(analogRead(A0), 0, 1024, 250, 0);
        #else
                extruderTemperatures[0] = (int)extruderCurrentTemperature;
                bedTemperatures[0] = (int)bedCurrentTemperature;
        #endif
        timeToGetGraphData = false;
    }

    // ArduinoOTA.handle();
    yield();
}

void parseJsonData(const char *jsonString)
{
    //StaticJsonBuffer<2000> jsonBuffer; static doesn't seem to play well when in function, stack/heap stuff
    DynamicJsonBuffer jsonBuffer(2000);

    JsonObject &root = jsonBuffer.parseObject(jsonString);

    if (!root.success())
    {
        return; //Serial.println("parseObject() failed");
    }

    bedCurrentTemperature = root["heaters"][0]; 
    extruderCurrentTemperature = root["heaters"][1];
    bedActiveTemperature = root["active"][0]; 
    extruderActiveTemperature = root["active"][1];
    headPosition_X = root["pos"][0];
    headPosition_Y = root["pos"][1];
    headPosition_Z = root["pos"][2];
    homed_x = root["homed"][0];
    homed_y = root["homed"][1];
    homed_z = root["homed"][2];
    speed_factor = root["sfactor"];
    babystep = root["babystep"];
    extrusion_factor = root["efactor"][0];
    print_progress = root["fraction_printed"];    
    fanPercent_0 = root["fanPercent"][0];
}

void timeToRefreshEvent()
{
    static uint8_t graphDelay;
    timeToGetData = true;
    graphDelay++;
    if (graphDelay > 5)
    {
        graphDelay = 0;
        timeToRefreshGraph = true;
        timeToGetGraphData = true;
    }
}

void timeToScanButtonsEvent()
{
    timeToScanButtons = true;
}

void OTA_setup()
{
    // OTA setup ==============================================================================
    //Serial.begin(115200);
    //Serial.println("Booting");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.waitForConnectResult() != WL_CONNECTED)
    {
        //Serial.println("Connection Failed! Rebooting...");
        delay(5000);
        ESP.restart();
    }
    // Port defaults to 8266
    // ArduinoOTA.setPort(8266);

    // Hostname defaults to esp8266-[ChipID]
    #ifdef DEV
        ArduinoOTA.setHostname("SUIFD_dev");
    #else
        ArduinoOTA.setHostname("SUIFD");
    #endif

    suifd_IP = WiFi.localIP();
    // No authentication by default
    // ArduinoOTA.setPassword((const char *)"123");

    ArduinoOTA.onStart([]() {
        //tft.fillScreen(ILI9341_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        //tft.setFreeFont(FSSB9);
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(80, 120);
        tft.setTextColor(ILI9341_WHITE, ILI9341_RED);
        tft.setTextSize(1);
        tft.println("Updating firmware...");
        

        //Serial.println("Start");
    });
    ArduinoOTA.onEnd([]() {
        //Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        //Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        //tft.setCursor(160, 150);
        //tft.setTextPadding(tft.textWidth("100 %"));
        //tft.printf( "%d %",(progress/(total / 100)) );
        tft.drawNumber((progress / (total / 100)), 160, 150, 4);
    });
    ArduinoOTA.onError([](ota_error_t error) {
        //Serial.printf("Error[%u]: ", error);
        //if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        //else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        //else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        //else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        //else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    ArduinoOTA.begin();
    // OTA end =================================================================================
}

void barGraph(uint8_t value){
    uint8_t x = 13;
    uint16_t ledColor;
    for(uint8_t i=0; i <= 9; i++){
        if((i * 10) < value){
            ledColor = TFT_GREEN;
        }else{
            ledColor = TFT_DARKGREEN;
        }
        tft.fillRect(x, 147, 12, 5, ledColor);
        x += 14;
    }
}
