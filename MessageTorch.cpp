#include "application.h"

#include "font.cpp"
#include "mtutilities.h"

#include "elapsedMills.h"

//#define	ADDCHEERLIGHTS

// Main program, torch simulation
// ==============================

#include "neopixel.h"
#define	PIXEL_COUNT		300
#define	PIXEL_PIN		D0
#define	PIXEL_TYPE		WS2812B
Adafruit_NeoPixel leds = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

const uint16_t levels = 20; // Hot fix: had to reduce number of LEDs because new spark.core FW has less RAM free for user's app, otherwise crashes.
const uint16_t ledsPerLevel = 15; // approx
const uint16_t numLeds = ledsPerLevel*levels; // total number of LEDs

const bool mirrorText = true;

// global parameters
enum {
  mode_off = 0,
  mode_torch = 1, // torch
  mode_colorcycle = 2, // moving color cycle
  mode_lamp = 3, // lamp
};

byte mode = mode_torch; // main operation mode
int brightness = 255; // overall brightness
byte fade_base = 140; // crossfading base brightness level


// text params
int text_intensity = 255; // intensity of last column of text (where text appears)
int cycles_per_px = 5;
int text_repeats = 15; // text displays until faded down to almost zero
int fade_per_repeat = 15; // how much to fade down per repeat
int text_base_line = 6;
int raise_text_by = 0; // how many rows to raise text from start to end of display
byte red_text = 0;
byte green_text = 255;
byte blue_text = 180;


// torch parameters
uint16_t cycle_wait = 1; // 0..255

byte flame_min = 100; // 0..255
byte flame_max = 220; // 0..255

byte random_spark_probability = 2; // 0..100
byte spark_min = 200; // 0..255
byte spark_max = 255; // 0..255

byte spark_tfr = 50; // 0..256 how much energy is transferred up for a spark per cycle
uint16_t spark_cap = 200; // 0..255: spark cells: how much energy is retained from previous cycle

uint16_t up_rad = 40; // up radiation
uint16_t side_rad = 30; // sidewards radiation
uint16_t heat_cap = 0; // 0..255: passive cells: how much energy is retained from previous cycle

byte red_bg = 0;
byte green_bg = 0;
byte blue_bg = 0;
byte red_bias = 5;
byte green_bias = 0;
byte blue_bias = 0;
int red_energy = 256;
int green_energy = 150;
int blue_energy = 0;


// lamp mode params
byte lamp_red = 220;
byte lamp_green = 220;
byte lamp_blue = 200;


// cheerlight params
uint8_t cheer_brightness = 100; // initial brightness
uint8_t cheer_fade_cycles = 30; // fade cheer color one brightness step every 30 cycles


// Simple 7 pixel height dot matrix font
// =====================================
// Note: the font is derived from a monospaced 7*5 pixel font, but has been adjusted a bit
//       to get rendered proportionally (variable character width, e.g. "!" has width 1, whereas "m" has 7)
//       In the fontGlyphs table below, every char has a number of pixel colums it consists of, and then the
//       actual column values encoded as a string.

typedef struct {
  uint8_t width;
  const char *cols;
} glyph_t;

// const int numGlyphs = 102; // 96 ASCII 0x20..0x7F plus 6 ÄÖÜäöü
// const int rowsPerGlyph = 7;
// const int glyphSpacing = 2;

static const glyph_t fontGlyphs[numGlyphs] = {
  { 5, "\x00\x00\x00\x00\x00" },  //   0x20 (0)
  { 1, "\x5f" },                  // ! 0x21 (1)
  { 3, "\x03\x00\x03" },          // " 0x22 (2)
  { 5, "\x28\x7c\x28\x7c\x28" },  // # 0x23 (3)
  { 5, "\x24\x2a\x7f\x2a\x12" },  // $ 0x24 (4)
  { 5, "\x4c\x2c\x10\x68\x64" },  // % 0x25 (5)
  { 5, "\x30\x4e\x55\x22\x40" },  // & 0x26 (6)
  { 1, "\x01" },                  // ' 0x27 (7)
  { 3, "\x1c\x22\x41" },          // ( 0x28 (8)
  { 3, "\x41\x22\x1c" },          // ) 0x29 (9)
  { 5, "\x01\x03\x01\x03\x01" },  // * 0x2A (10)
  { 5, "\x08\x08\x3e\x08\x08" },  // + 0x2B (11)
  { 2, "\x50\x30" },              // , 0x2C (12)
  { 5, "\x08\x08\x08\x08\x08" },  // - 0x2D (13)
  { 2, "\x60\x60" },              // . 0x2E (14)
  { 5, "\x40\x20\x10\x08\x04" },  // / 0x2F (15)

  { 5, "\x3e\x51\x49\x45\x3e" },  // 0 0x30 (0)
  { 3, "\x42\x7f\x40" },          // 1 0x31 (1)
  { 5, "\x62\x51\x49\x49\x46" },  // 2 0x32 (2)
  { 5, "\x22\x41\x49\x49\x36" },  // 3 0x33 (3)
  { 5, "\x0c\x0a\x09\x7f\x08" },  // 4 0x34 (4)
  { 5, "\x4f\x49\x49\x49\x31" },  // 5 0x35 (5)
  { 5, "\x3e\x49\x49\x49\x32" },  // 6 0x36 (6)
  { 5, "\x03\x01\x71\x09\x07" },  // 7 0x37 (7)
  { 5, "\x36\x49\x49\x49\x36" },  // 8 0x38 (8)
  { 5, "\x26\x49\x49\x49\x3e" },  // 9 0x39 (9)
  { 2, "\x66\x66" },              // : 0x3A (10)
  { 2, "\x56\x36" },              // ; 0x3B (11)
  { 4, "\x08\x14\x22\x41" },      // < 0x3C (12)
  { 4, "\x24\x24\x24\x24" },      // = 0x3D (13)
  { 4, "\x41\x22\x14\x08" },      // > 0x3E (14)
  { 5, "\x02\x01\x59\x09\x06" },  // ? 0x3F (15)

  { 5, "\x3e\x41\x5d\x55\x5e" },  // @ 0x40 (0)
  { 5, "\x7c\x0a\x09\x0a\x7c" },  // A 0x41 (1)
  { 5, "\x7f\x49\x49\x49\x36" },  // B 0x42 (2)
  { 5, "\x3e\x41\x41\x41\x22" },  // C 0x43 (3)
  { 5, "\x7f\x41\x41\x22\x1c" },  // D 0x44 (4)
  { 5, "\x7f\x49\x49\x41\x41" },  // E 0x45 (5)
  { 5, "\x7f\x09\x09\x01\x01" },  // F 0x46 (6)
  { 5, "\x3e\x41\x49\x49\x7a" },  // G 0x47 (7)
  { 5, "\x7f\x08\x08\x08\x7f" },  // H 0x48 (8)
  { 3, "\x41\x7f\x41" },          // I 0x49 (9)
  { 5, "\x30\x40\x40\x40\x3f" },  // J 0x4A (10)
  { 5, "\x7f\x08\x0c\x12\x61" },  // K 0x4B (11)
  { 5, "\x7f\x40\x40\x40\x40" },  // L 0x4C (12)
  { 7, "\x7f\x02\x04\x0c\x04\x02\x7f" },  // M 0x4D (13)
  { 5, "\x7f\x02\x04\x08\x7f" },  // N 0x4E (14)
  { 5, "\x3e\x41\x41\x41\x3e" },  // O 0x4F (15)

  { 5, "\x7f\x09\x09\x09\x06" },  // P 0x50 (0)
  { 5, "\x3e\x41\x51\x61\x7e" },  // Q 0x51 (1)
  { 5, "\x7f\x09\x09\x09\x76" },  // R 0x52 (2)
  { 5, "\x26\x49\x49\x49\x32" },  // S 0x53 (3)
  { 5, "\x01\x01\x7f\x01\x01" },  // T 0x54 (4)
  { 5, "\x3f\x40\x40\x40\x3f" },  // U 0x55 (5)
  { 5, "\x1f\x20\x40\x20\x1f" },  // V 0x56 (6)
  { 5, "\x7f\x40\x38\x40\x7f" },  // W 0x57 (7)
  { 5, "\x63\x14\x08\x14\x63" },  // X 0x58 (8)
  { 5, "\x03\x04\x78\x04\x03" },  // Y 0x59 (9)
  { 5, "\x61\x51\x49\x45\x43" },  // Z 0x5A (10)
  { 3, "\x7f\x41\x41" },          // [ 0x5B (11)
  { 5, "\x04\x08\x10\x20\x40" },  // \ 0x5C (12)
  { 3, "\x41\x41\x7f" },          // ] 0x5D (13)
  { 4, "\x04\x02\x01\x02" },      // ^ 0x5E (14)
  { 5, "\x40\x40\x40\x40\x40" },  // _ 0x5F (15)

  { 2, "\x01\x02" },              // ` 0x60 (0)
  { 5, "\x20\x54\x54\x54\x78" },  // a 0x61 (1)
  { 5, "\x7f\x44\x44\x44\x38" },  // b 0x62 (2)
  { 5, "\x38\x44\x44\x44\x08" },  // c 0x63 (3)
  { 5, "\x38\x44\x44\x44\x7f" },  // d 0x64 (4)
  { 5, "\x38\x54\x54\x54\x18" },  // e 0x65 (5)
  { 5, "\x08\x7e\x09\x09\x02" },  // f 0x66 (6)
  { 5, "\x48\x54\x54\x54\x38" },  // g 0x67 (7)
  { 5, "\x7f\x08\x08\x08\x70" },  // h 0x68 (8)
  { 3, "\x48\x7a\x40" },          // i 0x69 (9)
  { 5, "\x20\x40\x40\x48\x3a" },  // j 0x6A (10)
  { 4, "\x7f\x10\x28\x44" },      // k 0x6B (11)
  { 3, "\x3f\x40\x40" },          // l 0x6C (12)
  { 7, "\x7c\x04\x04\x38\x04\x04\x78" },  // m 0x6D (13)
  { 5, "\x7c\x04\x04\x04\x78" },  // n 0x6E (14)
  { 5, "\x38\x44\x44\x44\x38" },  // o 0x6F (15)

  { 5, "\x7c\x14\x14\x14\x08" },  // p 0x70 (0)
  { 5, "\x08\x14\x14\x7c\x40" },  // q 0x71 (1)
  { 5, "\x7c\x04\x04\x04\x08" },  // r 0x72 (2)
  { 5, "\x48\x54\x54\x54\x24" },  // s 0x73 (3)
  { 5, "\x04\x04\x7f\x44\x44" },  // t 0x74 (4)
  { 5, "\x3c\x40\x40\x40\x7c" },  // u 0x75 (5)
  { 5, "\x1c\x20\x40\x20\x1c" },  // v 0x76 (6)
  { 7, "\x7c\x40\x40\x38\x40\x40\x7c" },  // w 0x77 (7)
  { 5, "\x44\x28\x10\x28\x44" },  // x 0x78 (8)
  { 5, "\x0c\x50\x50\x50\x3c" },  // y 0x79 (9)
  { 5, "\x44\x64\x54\x4c\x44" },  // z 0x7A (10)
  { 3, "\x08\x36\x41" },          // { 0x7B (11)
  { 1, "\x7f" },                  // | 0x7C (12)
  { 3, "\x41\x36\x08" },          // } 0x7D (13)
  { 4, "\x04\x02\x04\x08" },      // ~ 0x7E (14)
  { 5, "\x7F\x41\x41\x41\x7F" },  //   0x7F (15)

  { 5, "\x7D\x0a\x09\x0a\x7D" },  // Ä 0x41 (1)
  { 5, "\x3F\x41\x41\x41\x3F" },  // Ö 0x4F (15)
  { 5, "\x3D\x40\x40\x40\x3D" },  // Ü 0x55 (5)
  { 5, "\x20\x55\x54\x55\x78" },  // ä 0x61 (1)
  { 5, "\x38\x45\x44\x45\x38" },  // ö 0x6F (15)
  { 5, "\x3c\x41\x40\x41\x7c" },  // ü 0x75 (5)
};


// Pre-declaration for compiler
int handleParams(String command);
int newMessage(String aText);
void resetText();
void crossFade(byte aFader, byte aValue, byte &aOutputA, byte &aOutputB);
void renderText();
void resetEnergy();
void calcNextEnergy();
void calcNextColors();
void injectRandom();
void initWifi();
void EEPROM_Check();
void EEPROM_SaveAll();
void EEPROM_Load();



// Cloud API
// =========

// this function automagically gets called upon a matching POST request
int handleParams(String command)
{
  //look for the matching argument "coffee" <-- max of 64 characters long
  int p = 0;
  while (p<(int)command.length()) {
    int i = command.indexOf(',',p);
    if (i<0) i = command.length();
    int j = command.indexOf('=',p);
    if (j<0) break;
    String key = command.substring(p,j);
    String value = command.substring(j+1,i);
    int val = value.toInt();

    // global params
    if (key=="wait") {
      cycle_wait = val;
      EEPROM.write(1, val);
    } else if (key=="mode") {
      mode = val;
      EEPROM.write(2, val);
    } else if (key=="brightness") {
      brightness = val;
      EEPROM.write(3, val);
    } else if (key=="fade_base") {
      fade_base = val;
      EEPROM.write(4, val);
    // cheerlight params
    } else if (key=="cheer_brightness") {
      cheer_brightness = val;
      EEPROM.write(5, val);
    } else if (key=="cheer_fade_cycles") {
      cheer_fade_cycles = val;
      EEPROM.write(6, val);
    // lamp params
    } else if (key=="lamp_red") {
      lamp_red = val;
      EEPROM.write(7, val);
    } else if (key=="lamp_green") {
      lamp_green = val;
      EEPROM.write(8, val);
    } else if (key=="lamp_blue") {
      lamp_blue = val;
      EEPROM.write(9, val);
    // text color params
    } else if (key=="red_text") {
      red_text = val;
      EEPROM.write(10, val);
    } else if (key=="green_text") {
      green_text = val;
      EEPROM.write(11, val);
    } else if (key=="blue_text") {
      blue_text = val;
      EEPROM.write(12, val);
    // text params
    } else if (key=="cycles_per_px") {
      cycles_per_px = val;
      EEPROM.write(13, val);
    } else if (key=="text_repeats") {
      text_repeats = val;
      EEPROM.write(14, val);
    } else if (key=="text_base_line") {
      text_base_line = val;
      EEPROM.write(15, val);
    } else if (key=="raise_text_by") {
      raise_text_by = val;
      EEPROM.write(16, val);
    } else if (key=="fade_per_repeat") {
      fade_per_repeat = val;
      EEPROM.write(17, val);
    } else if (key=="text_intensity") {
      text_intensity = val;
      EEPROM.write(18, val);
    // torch color params
    } else if (key=="red_bg") {
      red_bg = val;
      EEPROM.write(19, val);
    } else if (key=="green_bg") {
      green_bg = val;
      EEPROM.write(20, val);
    } else if (key=="blue_bg") {
      blue_bg = val;
      EEPROM.write(21, val);
    } else if (key=="red_bias") {
      red_bias = val;
      EEPROM.write(22, val);
    } else if (key=="green_bias") {
      green_bias = val;
      EEPROM.write(23, val);
    } else if (key=="blue_bias") {
      blue_bias = val;
      EEPROM.write(24, val);
    } else if (key=="red_energy") {
      red_energy = val;
      EEPROM.write(25, val);
    } else if (key=="green_energy") {
      green_energy = val;
      EEPROM.write(26, val);
    } else if (key=="blue_energy") {
      blue_energy = val;
      EEPROM.write(27, val);
    // torch params
    } else if (key=="spark_prob") {
      random_spark_probability = val;
      EEPROM.write(28, val);
      resetEnergy();
    }
    else if (key=="spark_cap") {
      spark_cap = val;
      EEPROM.write(29, val);
    } else if (key=="spark_tfr") {
      spark_tfr = val;
      EEPROM.write(30, val);
    } else if (key=="side_rad") {
      side_rad = val;
      EEPROM.write(31, val);
    } else if (key=="up_rad") {
      up_rad = val;
      EEPROM.write(32, val);
    } else if (key=="heat_cap") {
      heat_cap = val;
      EEPROM.write(33, val);
    } else if (key=="flame_min") {
      flame_min = val;
      EEPROM.write(34, val);
    } else if (key=="flame_max") {
      flame_max = val;
      EEPROM.write(35, val);
    } else if (key=="spark_min") {
      spark_min = val;
      EEPROM.write(36, val);
    } else if (key=="spark_max") {
      spark_max = val;
      EEPROM.write(37, val);
    }
    p = i+1;
  }
  return 1;
}


// text layer
// ==========

byte textLayer[numLeds]; // text layer
String text;

int textPixelOffset;
int textCycleCount;
int repeatCount;


// this function automagically gets called upon a matching POST request
int newMessage(String aText)
{
  // URL decode
  text = "";
  int i = 0;
  char c;
  while (i<(int)aText.length()) {
    if (aText[i]=='%') {
      if ((int)aText.length()<=i+2) break; // end of text
      // get hex
      c =(hexToInt(aText[i+1])<<4) + hexToInt(aText[i+2]);
      i += 2;
    }
    // ? = C3 84
    // ? = C3 96
    // ? = C3 9C
    // ? = C3 A4
    // ? = C3 B6
    // ? = C3 BC
    else if (aText[i]==0xC3) {
      if ((int)aText.length()<=i+1) break; // end of text
      switch (aText[i+1]) {
        case 0x84: c = 0x80; break; // ?
        case 0x96: c = 0x81; break; // ?
        case 0x9C: c = 0x82; break; // ?
        case 0xA4: c = 0x83; break; // ?
        case 0xB6: c = 0x84; break; // ?
        case 0xBC: c = 0x85; break; // ?
        default: c = 0x7F; break; // unknown
      }
      i += 1;
    }
    else {
      c = aText[i];
    }
    // put to output string
    text += String(c);
    i++;
  }
  // initiate display of new text
  textPixelOffset = -ledsPerLevel;
  textCycleCount = 0;
  repeatCount = 0;
  return 1;
}


void resetText()
{
  for(int i=0; i<numLeds; i++) {
    textLayer[i] = 0;
  }
}


void crossFade(byte aFader, byte aValue, byte &aOutputA, byte &aOutputB)
{
  byte baseBrightness = (aValue*fade_base)>>8;
  byte varBrightness = aValue-baseBrightness;
  byte fade = (varBrightness*aFader)>>8;
  aOutputB = baseBrightness+fade;
  aOutputA = baseBrightness+(varBrightness-fade);
}


// void renderText()
// {
//   // fade between rows
//   byte maxBright = text_intensity-repeatCount*fade_per_repeat;
//   byte thisBright, nextBright;
//   crossFade(255*textCycleCount/cycles_per_px, maxBright, thisBright, nextBright);
//   // generate vertical rows
//   int pixelsPerChar = bytesPerGlyph+glyphSpacing;
//   int activeCols = ledsPerLevel-2;
//   int totalTextPixels = (int)text.length()*pixelsPerChar;
//   for (int x=0; x<ledsPerLevel; x++) {
//     uint8_t column = 0;
//     // determine font row
//     if (x<activeCols) {
//       int rowPixelOffset = textPixelOffset + x;
//       if (rowPixelOffset>=0) {
//         // visible row
//         int charIndex = rowPixelOffset/pixelsPerChar;
//         if ((int)text.length()>charIndex) {
//           // visible char
//           char c = text[charIndex];
//           int glyphOffset = rowPixelOffset % pixelsPerChar;
//           if (glyphOffset<bytesPerGlyph) {
//             // fetch glyph column
//             c -= 0x20;
//             if (c>=numGlyphs) c = 95; // ASCII 0x7F-0x20
//             column = fontBytes[c*bytesPerGlyph+glyphOffset];
//           }
//         }
//       }
//     }
//     // calc base line
//     int baseLine = text_base_line + textPixelOffset*raise_text_by/totalTextPixels;
//     // now render columns
//     for (int y=0; y<levels; y++) {
//       int i = y*ledsPerLevel + x; // LED index
//       if (y>=baseLine) {
//         int glyphRow = y-baseLine;
//         if (glyphRow < rowsPerGlyph) {
//           if (column & (0x40>>glyphRow)) {
//             textLayer[i] = thisBright;
//             // also adjust pixel left to this one
//             if (x>0) {
//               increase(textLayer[i-1], nextBright, maxBright);
//             }
//             continue;
//           }
//         }
//       }
//       textLayer[i] = 0; // no text
//     }
//   }
//   // increment
//   textCycleCount++;
//   if (textCycleCount>=cycles_per_px) {
//     textCycleCount = 0;
//     textPixelOffset++;
//     if (textPixelOffset>totalTextPixels) {
//       // text shown, check for repeats
//       repeatCount++;
//       if (text_repeats!=0 && repeatCount>=text_repeats) {
//         // done
//         text = ""; // remove text
//       }
//       else {
//         // show again
//         textPixelOffset = -ledsPerLevel;
//         textCycleCount = 0;
//       }
//     }
//   }
// }


int glyphIndexForChar(const char aChar)
{
  int i = aChar-0x20;
  if (i<0 || i>=numGlyphs) i = 95; // ASCII 0x7F-0x20
  return i;
}


void renderText()
{
  // fade between rows
  byte maxBright = text_intensity-repeatCount*fade_per_repeat;
  byte thisBright, nextBright;
  crossFade(255*textCycleCount/cycles_per_px, maxBright, thisBright, nextBright);
  // generate vertical rows
  int activeCols = ledsPerLevel-2;
  // calculate text length in pixels
  int totalTextPixels = 0;
  int textLen = (int)text.length();
  for (int i=0; i<textLen; i++) {
    // sum up width of individual chars
    totalTextPixels += fontGlyphs[glyphIndexForChar(text[i])].width + glyphSpacing;
  }
  for (int x=0; x<ledsPerLevel; x++) {
    uint8_t column = 0;
    // determine font column
    if (x<activeCols) {
      int colPixelOffset = textPixelOffset + x;
      if (colPixelOffset>=0) {
        // visible column
        // - calculate character index
        int charIndex = 0;
        int glyphOffset = colPixelOffset;
        const glyph_t *glyphP = NULL;
        while (charIndex<textLen) {
          glyphP = &fontGlyphs[glyphIndexForChar(text[charIndex])];
          int cw = glyphP->width + glyphSpacing;
          if (glyphOffset<cw) break; // found char
          glyphOffset -= cw;
          charIndex++;
        }
        // now we have
        // - glyphP = the glyph,
        // - glyphOffset=column offset within that glyph (but might address a spacing column not stored in font table)
        if (charIndex<textLen) {
          // is a column of a visible char
          if (glyphOffset<glyphP->width) {
            // fetch glyph column
            column = glyphP->cols[glyphOffset];
          }
        }
      }
    }
    // now render columns
    for (int glyphRow=0; glyphRow<rowsPerGlyph; glyphRow++) {
      int i;
      int leftstep;
      if (mirrorText) {
        i = (glyphRow+1)*ledsPerLevel - 1 - x; // LED index, x-direction mirrored
        leftstep = 1;
      }
      else {
        i = glyphRow*ledsPerLevel + x; // LED index
        leftstep = -1;
      }
      if (glyphRow < rowsPerGlyph) {
        if (column & (0x40>>glyphRow)) {
          textLayer[i] = thisBright;
          // also adjust pixel left to this one
          if (x>0) {
            increase(textLayer[i+leftstep], nextBright, maxBright);
          }
          continue;
        }
      }
      textLayer[i] = 0; // no text
    }
  }
  // increment
  textCycleCount++;
  if (textCycleCount>=cycles_per_px) {
    textCycleCount = 0;
    textPixelOffset++;
    if (textPixelOffset>totalTextPixels) {
      // text shown, check for repeats
      repeatCount++;
      if (text_repeats!=0 && repeatCount>=text_repeats) {
        // done
        text = ""; // remove text
      }
      else {
        // show again
        textPixelOffset = -ledsPerLevel;
        textCycleCount = 0;
      }
    }
  }
}


// torch mode
// ==========

byte currentEnergy[numLeds]; // current energy level
byte nextEnergy[numLeds]; // next energy level
byte energyMode[numLeds]; // mode how energy is calculated for this point

enum {
  torch_passive = 0, // just environment, glow from nearby radiation
  torch_nop = 1, // no processing
  torch_spark = 2, // slowly looses energy, moves up
  torch_spark_temp = 3, // a spark still getting energy from the level below
};


void resetEnergy()
{
  for (int i=0; i<numLeds; i++) {
    currentEnergy[i] = 0;
    nextEnergy[i] = 0;
    energyMode[i] = torch_passive;
  }
}


void calcNextEnergy()
{
  int i = 0;
  for (int y=0; y<levels; y++) {
    for (int x=0; x<ledsPerLevel; x++) {
      byte e = currentEnergy[i];
      byte m = energyMode[i];
      switch (m) {
        case torch_spark: {
          // loose transfer up energy as long as the is any
          reduce(e, spark_tfr);
          // cell above is temp spark, sucking up energy from this cell until empty
          if (y<levels-1) {
            energyMode[i+ledsPerLevel] = torch_spark_temp;
          }
          break;
        }
        case torch_spark_temp: {
          // just getting some energy from below
          byte e2 = currentEnergy[i-ledsPerLevel];
          if (e2<spark_tfr) {
            // cell below is exhausted, becomes passive
            energyMode[i-ledsPerLevel] = torch_passive;
            // gobble up rest of energy
            increase(e, e2);
            // loose some overall energy
            e = ((int)e*spark_cap)>>8;
            // this cell becomes active spark
            energyMode[i] = torch_spark;
          }
          else {
            increase(e, spark_tfr);
          }
          break;
        }
        case torch_passive: {
          e = ((int)e*heat_cap)>>8;
          increase(e, ((((int)currentEnergy[i-1]+(int)currentEnergy[i+1])*side_rad)>>9) + (((int)currentEnergy[i-ledsPerLevel]*up_rad)>>8));
        }
        default:
          break;
      }
      nextEnergy[i++] = e;
    }
  }
}


void calcNextColors()
{
  for (int i=0; i<numLeds; i++) {
    if (textLayer[i]>0) {
      // text is overlaid in light green-blue
      leds.setColorDimmed(i, red_text, green_text, blue_text, (brightness*textLayer[i])>>8);
    }
    else {
      uint16_t e = nextEnergy[i];
      currentEnergy[i] = e;
  //    leds.setColorDimmed(i, 255, 170, 0, e);
      if (e>230)
        leds.setColorDimmed(i, 170, 170, e, brightness);
      else {
        //leds.setColor(i, e, (340*e)>>9, 0);
        if (e>0) {
          byte r = red_bias;
          byte g = green_bias;
          byte b = blue_bias;
          increase(r, (e*red_energy)>>8);
          increase(g, (e*green_energy)>>8);
          increase(b, (e*blue_energy)>>8);
          leds.setColorDimmed(i, r, g, b, brightness);
        }
        else {
          // background, no energy
          leds.setColorDimmed(i, red_bg, green_bg, blue_bg, brightness);
        }
      }
    }
  }
}


void injectRandom()
{
  // random flame energy at bottom row
  for (int i=0; i<ledsPerLevel; i++) {
    currentEnergy[i] = random(flame_min, flame_max);
    energyMode[i] = torch_nop;
  }
  // random sparks at second row
  for (int i=ledsPerLevel; i<2*ledsPerLevel; i++) {
    if (energyMode[i]!=torch_spark && random(100)<random_spark_probability) {
      currentEnergy[i] = random(spark_min, spark_max);
      energyMode[i] = torch_spark;
    }
  }
}

#if defined (ADDCHEERLIGHTS)
// Cheerlights interface
// =====================
// see cheerlights.com
// Code partly from https://github.com/ls6/spark-core-cheerlights licensed under MIT license

TCPClient cheerLightsAPI;
String responseLine;
unsigned long nextPoll = 0;
uint8_t cheer_red = 0;
uint8_t cheer_green = 0;
uint8_t cheer_blue = 0;
uint8_t cheer_bright = 0;
uint8_t cheer_fade_cnt = 0;


void processCheerColor(String colorName)
{
  uint8_t red, green, blue;

  if (colorName == "purple") {
    red = 128; green = 0; blue = 128;
  } else if (colorName == "red") {
    red = 255; green = 0; blue = 0;
  } else if (colorName == "green") {
    red = 0; green = 255; blue = 0;
  } else if (colorName == "blue") {
    red = 0; green = 0; blue = 255;
  } else if (colorName == "cyan") {
    red = 0; green = 255; blue = 255;
  } else if (colorName == "white") {
    red = 255; green = 255; blue = 255;
  } else if (colorName == "warmwhite") {
    red = 253; green = 245; blue = 230;
  } else if (colorName == "magenta") {
    red = 255; green = 0; blue = 255;
  } else if (colorName == "yellow") {
    red = 255; green = 255; blue = 0;
  } else if (colorName == "orange") {
    red = 255; green = 165; blue = 0;
  } else if (colorName == "pink") {
    red = 255; green = 192; blue = 203;
  } else if (colorName == "oldlace") {
    red = 253; green = 245; blue = 230;
  }
  else {
    // unknown color, do nothing
  }
  // check if cheer color is different
  if (red!=cheer_red || green!=cheer_green || blue!=cheer_blue) {
    // initiate new cheer colored background sequence
    cheer_red = red;
    cheer_green = green;
    cheer_blue = blue;
    cheer_bright = cheer_brightness; // start with configured brightness
    cheer_fade_cnt = 0;
  }
}


void updateBackgroundWithCheerColor()
{
  if (cheer_bright>0) {
    red_bg = ((int)cheer_red*cheer_bright)>>8;
    green_bg = ((int)cheer_green*cheer_bright)>>8;
    blue_bg = ((int)cheer_blue*cheer_bright)>>8;
    // check fading
    cheer_fade_cnt++;
    if (cheer_fade_cnt>=cheer_fade_cycles) {
      cheer_fade_cnt = 0;
      cheer_bright--;
      // if we reached 0 now, turn off background now
      if (cheer_bright==0) {
        red_bg = 0;
        green_bg = 0;
        blue_bg = 0;
      }
    }
  }
}


void checkCheerlights()
{
  if (cheer_brightness>0) {
    // only poll if displaye is enabled (not zero brightness)
    if (nextPoll<=millis()) {
      nextPoll = millis()+60000;
      // in case previous request wasn't answered, close connection
      cheerLightsAPI.stop();
      // issue a new request
      if (cheerLightsAPI.connect("api.thingspeak.com", 80)) {
        cheerLightsAPI.println("GET /channels/1417/field/1/last.txt HTTP/1.0");
        cheerLightsAPI.println();
      }
      responseLine = "";
    }
    if (cheerLightsAPI.available()) {
      char ch = cheerLightsAPI.read();
      responseLine += ch;
      // check for end of line (LF)
      if (ch==0x0A) {
        if (responseLine.length() == 2) {
          // empty line (CRLF only)
          // now response body (color) follows
          String colorName = "";
          while (cheerLightsAPI.available()) {
            ch = cheerLightsAPI.read();
            colorName += ch;
          };
          processCheerColor(colorName);
          cheerLightsAPI.stop();
        };
        responseLine = ""; // next line
      }
    }
  }
}
#endif


// Main program
// ============

void setup()
{
  EEPROM_Check();

  resetEnergy();
  resetText();
  leds.begin();

  // remote control
  Spark.function("params", handleParams); // parameters
  Spark.function("message", newMessage); // text message display
}



byte cnt = 0;

void loop()
{
  // check cheerlights
  //checkCheerlights();
  //updateBackgroundWithCheerColor();

  // render the text
  renderText();
  switch (mode) {
    case mode_off: {
      // off
      for(int i=0; i<leds.getNumLeds(); i++) {
        leds.setColor(i, 0, 0, 0);
      }
      break;
    }
    case mode_lamp: {
      // just single color lamp + text display
      for(int i=0; i<leds.getNumLeds(); i++) {
        if (textLayer[i]>0) {
          leds.setColorDimmed(i, red_text, green_text, blue_text, (textLayer[i]*brightness)>>8);
        }
        else {
          leds.setColorDimmed(i, lamp_red, lamp_green, lamp_blue, brightness);
        }
      }
      break;
    }
    case mode_torch: {
      // torch animation + text display + cheerlight background
      injectRandom();
      calcNextEnergy();
      calcNextColors();
      break;
    }
    case mode_colorcycle: {
      // simple color wheel animation
      cnt++;
      byte r,g,b;
      for(int i=0; i<leds.getNumLeds(); i++) {
        wheel(((i * 256 / leds.getNumLeds()) + cnt) & 255, r, g, b);
        if (textLayer[i]>0) {
          leds.setColorDimmed(i, r, g, b, (textLayer[i]*brightness)>>8);
        }
        else {
          leds.setColorDimmed(i, r, g, b, brightness>>1); // only half brightness for full area color
        }
      }
      break;
    }
  }
  // transmit colors to the leds
  leds.show();
  // wait
  delay(cycle_wait); // latch & reset needs 50 microseconds pause, at least.
}


void EEPROM_Check() {
  if(EEPROM.read(0)==137)
    EEPROM_Load();
  else
    EEPROM_SaveAll();
}


void EEPROM_SaveAll() {
  EEPROM.write(1, cycle_wait);
  EEPROM.write(2, mode);
  EEPROM.write(3, brightness);
  EEPROM.write(4, fade_base);
  EEPROM.write(5, cheer_brightness);
  EEPROM.write(6, cheer_fade_cycles);
  EEPROM.write(7, lamp_red);
  EEPROM.write(8, lamp_green);
  EEPROM.write(9, lamp_blue);
  EEPROM.write(10, red_text);
  EEPROM.write(11, green_text);
  EEPROM.write(12, blue_text);
  EEPROM.write(13, cycles_per_px);
  EEPROM.write(14, text_repeats);
  EEPROM.write(15, text_base_line);
  EEPROM.write(16, raise_text_by);
  EEPROM.write(17, fade_per_repeat);
  EEPROM.write(18, text_intensity);
  EEPROM.write(19, red_bg);
  EEPROM.write(20, green_bg);
  EEPROM.write(21, blue_bg);
  EEPROM.write(22, red_bias);
  EEPROM.write(23, green_bias);
  EEPROM.write(24, blue_bias);
  EEPROM.write(25, red_energy);
  EEPROM.write(26, green_energy);
  EEPROM.write(27, blue_energy);
  EEPROM.write(28, random_spark_probability);
  EEPROM.write(29, spark_cap);
  EEPROM.write(30, spark_tfr);
  EEPROM.write(31, side_rad);
  EEPROM.write(32, up_rad);
  EEPROM.write(33, heat_cap);
  EEPROM.write(34, flame_min);
  EEPROM.write(35, flame_max);
  EEPROM.write(36, spark_min);
  EEPROM.write(37, spark_max);
}


void EEPROM_Load() {
  /**
   * EEPROM Table
   * ============
   *  1 = wait
   *  2 = mode
   *  3 = brightness
   *  4 = fade_base
   *  5 = cheer_brightness
   *  6 = cheer_fade_cycles
   *  7 = lamp_red
   *  8 = lamp_green
   *  9 = lamp_blue
   * 10 = red_text
   * 11 = green_text
   * 12 = blue_text
   * 13 = cycles_per_px
   * 14 = text_repeats
   * 15 = text_base_line
   * 16 = raise_text_by
   * 17 = fade_per_repeat
   * 18 = text_intensity
   * 19 = red_bg
   * 20 = green_bg
   * 21 = blue_bg
   * 22 = red_bias
   * 23 = green_bias
   * 24 = blue_bias
   * 25 = red_energy
   * 26 = green_energy
   * 27 = blue_energy
   * 28 = spark_prob
   * 29 = spark_cap
   * 30 = spark_tfr
   * 31 = side_rad
   * 32 = up_rad
   * 33 = heat_cap
   * 34 = flame_min
   * 35 = flame_max
   * 36 = spark_min
   * 37 = spark_max
   **/
   cycle_wait = EEPROM.read(1);
   mode = EEPROM.read(2);
   brightness  = EEPROM.read(3);
   fade_base = EEPROM.read(4);
   cheer_brightness = EEPROM.read(5);
   cheer_fade_cycles = EEPROM.read(6);
   lamp_red = EEPROM.read(7);
   lamp_green = EEPROM.read(8);
   lamp_blue = EEPROM.read(9);
   red_text = EEPROM.read(10);
   green_text = EEPROM.read(11);
   blue_text = EEPROM.read(12);
   cycles_per_px = EEPROM.read(13);
   text_repeats = EEPROM.read(14);
   text_base_line = EEPROM.read(15);
   raise_text_by = EEPROM.read(16);
   fade_per_repeat = EEPROM.read(17);
   text_intensity = EEPROM.read(18);
   red_bg = EEPROM.read(19);
   green_bg = EEPROM.read(20);
   blue_bg = EEPROM.read(21);
   red_bias = EEPROM.read(22);
   green_bias = EEPROM.read(23);
   blue_bias = EEPROM.read(24);
   red_energy = EEPROM.read(25);
   green_energy = EEPROM.read(26);
   blue_energy = EEPROM.read(27);
   random_spark_probability = EEPROM.read(28);
   spark_cap = EEPROM.read(29);
   spark_tfr = EEPROM.read(30);
   side_rad = EEPROM.read(31);
   up_rad = EEPROM.read(32);
   heat_cap = EEPROM.read(33);
   flame_min = EEPROM.read(34);
   flame_max = EEPROM.read(35);
   spark_min = EEPROM.read(36);
   spark_max = EEPROM.read(37);
}