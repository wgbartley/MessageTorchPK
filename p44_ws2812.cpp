// Implementation (would go to .cpp file once library is separated)
// ================================================================
#include "p44_ws2812.h"

p44_ws2812::p44_ws2812(uint16_t aNumLeds)
{
  numLeds = aNumLeds;
  // allocate the buffer
  bufferSize = numLeds*3;
  if((msgBufferP = new byte[bufferSize])!=NULL) {
    memset(msgBufferP, 0, bufferSize); // all LEDs off
  }
}

p44_ws2812::~p44_ws2812()
{
  // free the buffer
  if (msgBufferP) delete msgBufferP;
}


int p44_ws2812::getNumLeds()
{
  return numLeds;
}


void p44_ws2812::begin()
{
  // begin using the driver
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV8); // System clock is 72MHz, we need 9MHz for SPI
  SPI.setBitOrder(MSBFIRST); // MSB first for easier scope reading :-)
  SPI.transfer(0); // make sure SPI line starts low (Note: SPI line remains at level of last sent bit, fortunately)
}

void p44_ws2812::show()
{
  // Note: on the spark core, system IRQs might happen which exceed 50uS
  // causing WS2812 chips to reset in midst of data stream.
  // Thus, until we can send via DMA, we need to disable IRQs while sending
  __disable_irq();
  // transfer RGB values to LED chain
  for (uint16_t i=0; i<bufferSize; i++) {
    byte b = msgBufferP[i];
    for (byte j=0; j<8; j++) {
      SPI.transfer(b & 0x80 ? 0x7E : 0x70);
      b = b << 1;
    }
  }
  __enable_irq();
}


void p44_ws2812::setColor(uint16_t aLedNumber, byte aRed, byte aGreen, byte aBlue)
{
  if (aLedNumber>=numLeds) return; // invalid LED number
  byte *msgP = msgBufferP+aLedNumber*3;
  // order in message is G-R-B for each LED
  *msgP++ = aGreen;
  *msgP++ = aRed;
  *msgP++ = aBlue;
}


void p44_ws2812::setColorScaled(uint16_t aLedNumber, byte aRed, byte aGreen, byte aBlue, byte aScaling)
{
  // scale RGB with a common brightness parameter
  setColor(aLedNumber, (aRed*aScaling)>>8, (aGreen*aScaling)>>8, (aBlue*aScaling)>>8);
}



byte brightnessToPWM(byte aBrightness)
{
  static const byte pwmLevels[16] = { 0, 1, 2, 3, 4, 6, 8, 12, 23, 36, 48, 70, 95, 135, 190, 255 };
  return pwmLevels[aBrightness>>4];
}



void p44_ws2812::setColorDimmed(uint16_t aLedNumber, byte aRed, byte aGreen, byte aBlue, byte aBrightness)
{
  setColorScaled(aLedNumber, aRed, aGreen, aBlue, brightnessToPWM(aBrightness));
}



void p44_ws2812::getColor(uint16_t aLedNumber, byte &aRed, byte &aGreen, byte &aBlue)
{
  if (aLedNumber>=numLeds) return; // invalid LED number
  byte *msgP = msgBufferP+aLedNumber*3;
  // order in message is G-R-B for each LED
  aGreen = *msgP;
  aRed = *msgP;
  aBlue = *msgP;
}
