#ifndef JPEGPrint_h
#define JPEGPrint_h

#include <TFT_eSPI.h>
#include <JPEGDecoder.h>  //version 1.7.7 "https://github.com/Bodmer/JPEGDecoder.git"
/*
 * modified JPEGDecoder.cpp from line 359 if not working try with the default version
 * 
int JPEGDecoder::decodeSdFile(const String& pFilename) {
#if !defined (ARDUINO_ARCH_SAM)
// File pInFile = SD.open( pFilename, FILE_READ);
  int len = pFilename.length() + 2;
    char buffer[len];
    pFilename.toCharArray(buffer,len);
    File pInFile = SD.open( buffer, FILE_READ);
//File pInFile = SD.open( pFilename, FILE_READ);
  return decodeSdFile(pInFile);
#else
  return -1;
#endif
}
 */

class JPEGPrint
{
public:
    void init(TFT_eSPI *ptrTFT);
    void drawJpeg(char *filename, int xpos, int ypos);
private:
    void jpegRender(int xpos, int ypos);
    TFT_eSPI *_tft;
    //JPEGDecoder *_ptrDecoder;
};
#endif /* JPEGPrint_h */
