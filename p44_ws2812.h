/*
 * Spark Core library to control WS2812 based RGB LED devices
 * using SPI to create bitstream.
 * Future plan is to use DMA to feed the SPI, so WS2812 bitstream
 * can be produced without CPU load and without blocking IRQs
 *
 * (c) 2014 by luz@plan44.ch (GPG: 1CC60B3A)
 * Licensed as open source under the terms of the MIT License
 * (see LICENSE.TXT)
 */


// Declaration (would go to .h file once library is separated)
// ===========================================================

#include "application.h"

class p44_ws2812 {

  uint16_t numLeds; // number of LEDs
  size_t bufferSize; // number of bytes in the buffer
  byte *msgBufferP; // the message buffer

public:
  /// create driver for a WS2812 LED chain
  /// @param aNumLeds number of LEDs in the chain
  p44_ws2812(uint16_t aNumLeds);

  /// destructor
  ~p44_ws2812();

  /// begin using the driver
  void begin();

  /// transfer RGB values to LED chain
  /// @note this must be called to update the actual LEDs after modifying RGB values
  /// with setColor() and/or setColorDimmed()
  void show();

  /// set color of one LED
  /// @param aRed intensity of red component, 0..255
  /// @param aGreen intensity of green component, 0..255
  /// @param aBlue intensity of blue component, 0..255
  void setColor(uint16_t aLedNumber, byte aRed, byte aGreen, byte aBlue);

  /// set color of one LED, scaled by a factor
  /// @param aRed intensity of red component, 0..255
  /// @param aGreen intensity of green component, 0..255
  /// @param aBlue intensity of blue component, 0..255
  /// @param aScaling scaling factor for RGB (PWM duty cycle), 0..255
  void setColorScaled(uint16_t aLedNumber, byte aRed, byte aGreen, byte aBlue, byte aScaling);

  /// set color of one LED, scaled by a visible brightness (non-linear) factor
  /// @param aRed intensity of red component, 0..255
  /// @param aGreen intensity of green component, 0..255
  /// @param aBlue intensity of blue component, 0..255
  /// @param aBrightness brightness, will be converted non-linear to PWM duty cycle for uniform brightness scale, 0..255
  void setColorDimmed(uint16_t aLedNumber, byte aRed, byte aGreen, byte aBlue, byte aBrightness);

  /// get current color of LED
  /// @param aRed set to intensity of red component, 0..255
  /// @param aGreen set to iintensity of green component, 0..255
  /// @param aBlue set to iintensity of blue component, 0..255
  /// @note for LEDs set with setColorDimmed()/setColorScaled(), this returns the scaled down RGB values,
  ///   not the original r,g,b parameters
  void getColor(uint16_t aLedNumber, byte &aRed, byte &aGreen, byte &aBlue);

  /// @return number of LEDs
  int getNumLeds();

};

