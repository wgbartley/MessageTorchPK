#include "application.h"

//#include "p44_ws2812.h"
#include "font.cpp"
#include "mtutilities.h"

#include "elapsedMills.h"

//#define	ADDCHEERLIGHTS

// Main program, torch simulation
// ==============================
boolean demoActive;
elapsedMillis	demoTime;
enum demoStates {FLAMES, TEXT, RAINBOW, RAINBOWTEXT};
demoStates demoMode;

#include "neopixel.h"
#define	PIXEL_COUNT		300
#define	PIXEL_PIN		D0
#define	PIXEL_TYPE		WS2812B
Adafruit_NeoPixel leds = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

const uint16_t levels = 20; // Hot fix: had to reduce number of LEDs because new spark.core FW has less RAM free for user's app, otherwise crashes.
const uint16_t ledsPerLevel = 15; // approx
const uint16_t numLeds = ledsPerLevel*levels; // total number of LEDs

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
    if (key=="wait")
      cycle_wait = val;
    else if (key=="mode")
      mode = val;
    else if (key=="brightness")
      brightness = val;
    else if (key=="fade_base")
      fade_base = val;
    // cheerlight params
    else if (key=="cheer_brightness")
      cheer_brightness = val;
    else if (key=="cheer_fade_cycles")
      cheer_fade_cycles = val;
    // lamp params
    else if (key=="lamp_red")
      lamp_red = val;
    else if (key=="lamp_green")
      lamp_green = val;
    else if (key=="lamp_blue")
      lamp_blue = val;
    // text color params
    else if (key=="red_text")
      red_text = val;
    else if (key=="green_text")
      green_text = val;
    else if (key=="blue_text")
      blue_text = val;
    // text params
    else if (key=="cycles_per_px")
      cycles_per_px = val;
    else if (key=="text_repeats")
      text_repeats = val;
    else if (key=="text_base_line")
      text_base_line = val;
    else if (key=="raise_text_by")
      raise_text_by = val;
    else if (key=="fade_per_repeat")
      fade_per_repeat = val;
    else if (key=="text_intensity")
      text_intensity = val;
    // torch color params
    else if (key=="red_bg")
      red_bg = val;
    else if (key=="green_bg")
      green_bg = val;
    else if (key=="blue_bg")
      blue_bg = val;
    else if (key=="red_bias")
      red_bias = val;
    else if (key=="green_bias")
      green_bias = val;
    else if (key=="blue_bias")
      blue_bias = val;
    else if (key=="red_energy")
      red_energy = val;
    else if (key=="green_energy")
      green_energy = val;
    else if (key=="blue_energy")
      blue_energy = val;
    // torch params
    else if (key=="spark_prob") {
      random_spark_probability = val;
      resetEnergy();
    }
    else if (key=="spark_cap")
      spark_cap = val;
    else if (key=="spark_tfr")
      spark_tfr = val;
    else if (key=="side_rad")
      side_rad = val;
    else if (key=="up_rad")
      up_rad = val;
    else if (key=="heat_cap")
      heat_cap = val;
    else if (key=="flame_min")
      flame_min = val;
    else if (key=="flame_max")
      flame_max = val;
    else if (key=="spark_min")
      spark_min = val;
    else if (key=="spark_max")
      spark_max = val;
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


void renderText()
{
  // fade between rows
  byte maxBright = text_intensity-repeatCount*fade_per_repeat;
  byte thisBright, nextBright;
  crossFade(255*textCycleCount/cycles_per_px, maxBright, thisBright, nextBright);
  // generate vertical rows
  int pixelsPerChar = bytesPerGlyph+glyphSpacing;
  int activeCols = ledsPerLevel-2;
  int totalTextPixels = (int)text.length()*pixelsPerChar;
  for (int x=0; x<ledsPerLevel; x++) {
    uint8_t column = 0;
    // determine font row
    if (x<activeCols) {
      int rowPixelOffset = textPixelOffset + x;
      if (rowPixelOffset>=0) {
        // visible row
        int charIndex = rowPixelOffset/pixelsPerChar;
        if ((int)text.length()>charIndex) {
          // visible char
          char c = text[charIndex];
          int glyphOffset = rowPixelOffset % pixelsPerChar;
          if (glyphOffset<bytesPerGlyph) {
            // fetch glyph column
            c -= 0x20;
            if (c>=numGlyphs) c = 95; // ASCII 0x7F-0x20
            column = fontBytes[c*bytesPerGlyph+glyphOffset];
          }
        }
      }
    }
    // calc base line
    int baseLine = text_base_line + textPixelOffset*raise_text_by/totalTextPixels;
    // now render columns
    for (int y=0; y<levels; y++) {
      int i = y*ledsPerLevel + x; // LED index
      if (y>=baseLine) {
        int glyphRow = y-baseLine;
        if (glyphRow < rowsPerGlyph) {
          if (column & (0x40>>glyphRow)) {
            textLayer[i] = thisBright;
            // also adjust pixel left to this one
            if (x>0) {
              increase(textLayer[i-1], nextBright, maxBright);
            }
            continue;
          }
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

  if ((demoActive == true) && (demoTime >= 4000)) {
	demoTime = 0;

	switch (demoMode) {
	case FLAMES:
		newMessage("");
		mode = mode_torch;
		demoMode = TEXT;
		break;
	case TEXT:
		newMessage("SPARK");
		demoMode = RAINBOW;
		break;
	case RAINBOW:
		newMessage("");
		mode = mode_colorcycle;
		demoMode = RAINBOWTEXT;
		break;
	case RAINBOWTEXT:
		newMessage("SPARK");
		demoMode = FLAMES;
		break;
	}
  }

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
