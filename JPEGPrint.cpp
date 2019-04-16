#include "JPEGPrint.h"

#define minimum(a,b)     (((a) < (b)) ? (a) : (b))
#define USE_SPI_BUFFER // Comment out to use slower 16 bit pushColor()

void JPEGPrint::init(TFT_eSPI *ptrTFT){
    _tft = ptrTFT;
}

void JPEGPrint::drawJpeg(char *filename, int xpos, int ypos){
        JpegDec.decodeFile(filename);
        jpegRender(xpos, ypos);
}

void JPEGPrint::jpegRender(int xpos, int ypos) {
    
    // retrieve infomration about the image
    uint16_t  *pImg;
    uint16_t mcu_w = JpegDec.MCUWidth;
    uint16_t mcu_h = JpegDec.MCUHeight;
    uint32_t max_x = JpegDec.width;
    uint32_t max_y = JpegDec.height;
    
    // Jpeg images are draw as a set of image block (tiles) called Minimum Coding Units (MCUs)
    // Typically these MCUs are 16x16 pixel blocks
    // Determine the width and height of the right and bottom edge image blocks
    uint32_t min_w = minimum(mcu_w, max_x % mcu_w);
    uint32_t min_h = minimum(mcu_h, max_y % mcu_h);
    
    // save the current image block size
    uint32_t win_w = mcu_w;
    uint32_t win_h = mcu_h;
    
    // record the current time so we can measure how long it takes to draw an image
    //uint32_t drawTime = millis();
    
    // save the coordinate of the right and bottom edges to assist image cropping
    // to the screen size
    max_x += xpos;
    max_y += ypos;
    
    // read each MCU block until there are no more
#ifdef USE_SPI_BUFFER
    while( JpegDec.readSwappedBytes()){ // Swap byte order so the SPI buffer can be used
#else
        while ( JpegDec.read()) { // Normal byte order read
#endif
            // save a pointer to the image block
            pImg = JpegDec.pImage;
            
            // calculate where the image block should be drawn on the screen
            int mcu_x = JpegDec.MCUx * mcu_w + xpos;
            int mcu_y = JpegDec.MCUy * mcu_h + ypos;
            
            // check if the image block size needs to be changed for the right and bottom edges
            if (mcu_x + mcu_w <= max_x) win_w = mcu_w;
            else win_w = min_w;
            if (mcu_y + mcu_h <= max_y) win_h = mcu_h;
            else win_h = min_h;
            
            // calculate how many pixels must be drawn
            uint32_t mcu_pixels = win_w * win_h;
            
            // draw image MCU block only if it will fit on the screen
            if ( ( mcu_x + win_w) <= _tft->width() && ( mcu_y + win_h) <= _tft->height())
            {
#ifdef USE_SPI_BUFFER
                // Now set a MCU bounding window on the TFT to push pixels into (x, y, x + width - 1, y + height - 1)
                _tft->setWindow(mcu_x, mcu_y, mcu_x + win_w - 1, mcu_y + win_h - 1);
                // Write all MCU pixels to the TFT window
                uint8_t *pImg8 = (uint8_t*)pImg;     // Convert 16 bit pointer to an 8 bit pointer
                _tft->pushColors(pImg8, mcu_pixels*2); // Send bytes via 64 byte SPI port buffer
#else
                // Now set a MCU bounding window on the TFT to push pixels into (x, y, x + width - 1, y + height - 1)
                _tft->setAddrWindow(mcu_x, mcu_y, mcu_x + win_w - 1, mcu_y + win_h - 1);
                // Write all MCU pixels to the TFT window
                while (mcu_pixels--) _tft->pushColor(*pImg++);
#endif
            }
            
            else if ( ( mcu_y + win_h) >= _tft->height()) JpegDec.abort();
            
        }
        
        // calculate how long it took to draw the image
        //drawTime = millis() - drawTime; // Calculate the time it took
        
        // print the results to the serial port
        //Serial.print  ("Total render time was    : "); Serial.print(drawTime); Serial.println(" ms");
        //Serial.println("=====================================");
        
    }
