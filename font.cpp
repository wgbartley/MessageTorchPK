
// Simple 7*5 dot matrix font
// ==========================
#include "application.h"

const int numGlyphs = 102; // 96 ASCII 0x20..0x7F plus 6 ������
const int bytesPerGlyph = 5;
const int rowsPerGlyph = 7;
const int glyphSpacing = 2;
static const uint8_t fontBytes[numGlyphs*bytesPerGlyph] = {
  0x00, 0x00, 0x00, 0x00, 0x00, //   0x20 (0)
  0x00, 0x00, 0x5f, 0x00, 0x00, // ! 0x21 (1)
  0x00, 0x00, 0x01, 0x00, 0x01, // " 0x22 (2)
  0x28, 0x7c, 0x28, 0x7c, 0x28, // # 0x23 (3)
  0x24, 0x2a, 0x7f, 0x2a, 0x12, // $ 0x24 (4)
  0x4c, 0x2c, 0x10, 0x68, 0x64, // % 0x25 (5)
  0x30, 0x4e, 0x55, 0x22, 0x40, // & 0x26 (6)
  0x00, 0x00, 0x00, 0x01, 0x00, // ' 0x27 (7)
  0x00, 0x1c, 0x22, 0x41, 0x00, // ( 0x28 (8)
  0x00, 0x41, 0x22, 0x1c, 0x00, // ) 0x29 (9)
  0x01, 0x03, 0x01, 0x03, 0x01, // * 0x2A (10)
  0x08, 0x08, 0x3e, 0x08, 0x08, // + 0x2B (11)
  0x50, 0x30, 0x00, 0x00, 0x00, // , 0x2C (12)
  0x08, 0x08, 0x08, 0x08, 0x08, // - 0x2D (13)
  0x60, 0x60, 0x00, 0x00, 0x00, // . 0x2E (14)
  0x40, 0x20, 0x10, 0x08, 0x04, // / 0x2F (15)

  0x3e, 0x51, 0x49, 0x45, 0x3e, // 0 0x30 (0)
  0x00, 0x42, 0x7f, 0x40, 0x00, // 1 0x31 (1)
  0x62, 0x51, 0x49, 0x49, 0x46, // 2 0x32 (2)
  0x22, 0x41, 0x49, 0x49, 0x36, // 3 0x33 (3)
  0x0c, 0x0a, 0x09, 0x7f, 0x08, // 4 0x34 (4)
  0x4f, 0x49, 0x49, 0x49, 0x31, // 5 0x35 (5)
  0x3e, 0x49, 0x49, 0x49, 0x32, // 6 0x36 (6)
  0x03, 0x01, 0x71, 0x09, 0x07, // 7 0x37 (7)
  0x36, 0x49, 0x49, 0x49, 0x36, // 8 0x38 (8)
  0x26, 0x49, 0x49, 0x49, 0x3e, // 9 0x39 (9)
  0x66, 0x66, 0x00, 0x00, 0x00, // : 0x3A (10)
  0x56, 0x36, 0x00, 0x00, 0x00, // ; 0x3B (11)
  0x00, 0x08, 0x14, 0x22, 0x41, // < 0x3C (12)
  0x24, 0x24, 0x24, 0x24, 0x24, // = 0x3D (13)
  0x00, 0x41, 0x22, 0x14, 0x08, // > 0x3E (14)
  0x02, 0x01, 0x59, 0x09, 0x06, // ? 0x3F (15)

  0x3e, 0x41, 0x5d, 0x55, 0x5e, // @ 0x40 (0)
  0x7c, 0x0a, 0x09, 0x0a, 0x7c, // A 0x41 (1)
  0x7f, 0x49, 0x49, 0x49, 0x36, // B 0x42 (2)
  0x3e, 0x41, 0x41, 0x41, 0x22, // C 0x43 (3)
  0x7f, 0x41, 0x41, 0x22, 0x1c, // D 0x44 (4)
  0x7f, 0x49, 0x49, 0x41, 0x41, // E 0x45 (5)
  0x7f, 0x09, 0x09, 0x01, 0x01, // F 0x46 (6)
  0x3e, 0x41, 0x49, 0x49, 0x7a, // G 0x47 (7)
  0x7f, 0x08, 0x08, 0x08, 0x7f, // H 0x48 (8)
  0x00, 0x41, 0x7f, 0x41, 0x00, // I 0x49 (9)
  0x30, 0x40, 0x40, 0x40, 0x3f, // J 0x4A (10)
  0x7f, 0x08, 0x0c, 0x12, 0x61, // K 0x4B (11)
  0x7f, 0x40, 0x40, 0x40, 0x40, // L 0x4C (12)
  0x7f, 0x02, 0x1c, 0x02, 0x7f, // M 0x4D (13)
  0x7f, 0x02, 0x04, 0x08, 0x7f, // N 0x4E (14)
  0x3e, 0x41, 0x41, 0x41, 0x3e, // O 0x4F (15)

  0x7f, 0x09, 0x09, 0x09, 0x06, // P 0x50 (0)
  0x3e, 0x41, 0x51, 0x61, 0x7e, // Q 0x51 (1)
  0x7f, 0x09, 0x09, 0x09, 0x76, // R 0x52 (2)
  0x26, 0x49, 0x49, 0x49, 0x32, // S 0x53 (3)
  0x01, 0x01, 0x7f, 0x01, 0x01, // T 0x54 (4)
  0x3f, 0x40, 0x40, 0x40, 0x3f, // U 0x55 (5)
  0x1f, 0x20, 0x40, 0x20, 0x1f, // V 0x56 (6)
  0x7f, 0x40, 0x38, 0x40, 0x7f, // W 0x57 (7)
  0x63, 0x14, 0x08, 0x14, 0x63, // X 0x58 (8)
  0x03, 0x04, 0x78, 0x04, 0x03, // Y 0x59 (9)
  0x61, 0x51, 0x49, 0x45, 0x43, // Z 0x5A (10)
  0x00, 0x7f, 0x41, 0x41, 0x00, // [ 0x5B (11)
  0x04, 0x08, 0x10, 0x20, 0x40, // \ 0x5C (12)
  0x00, 0x41, 0x41, 0x7f, 0x00, // ] 0x5D (13)
  0x00, 0x04, 0x02, 0x01, 0x02, // ^ 0x5E (14)
  0x40, 0x40, 0x40, 0x40, 0x40, // _ 0x5F (15)

  0x00, 0x00, 0x01, 0x02, 0x00, // ` 0x60 (0)
  0x20, 0x54, 0x54, 0x54, 0x78, // a 0x61 (1)
  0x7f, 0x44, 0x44, 0x44, 0x38, // b 0x62 (2)
  0x38, 0x44, 0x44, 0x44, 0x08, // c 0x63 (3)
  0x38, 0x44, 0x44, 0x44, 0x7f, // d 0x64 (4)
  0x38, 0x54, 0x54, 0x54, 0x18, // e 0x65 (5)
  0x08, 0x7e, 0x09, 0x09, 0x02, // f 0x66 (6)
  0x48, 0x54, 0x54, 0x54, 0x38, // g 0x67 (7)
  0x7f, 0x08, 0x08, 0x08, 0x70, // h 0x68 (8)
  0x00, 0x48, 0x7a, 0x40, 0x00, // i 0x69 (9)
  0x20, 0x40, 0x40, 0x48, 0x3a, // j 0x6A (10)
  0x7f, 0x10, 0x28, 0x44, 0x00, // k 0x6B (11)
  0x3f, 0x40, 0x40, 0x00, 0x00, // l 0x6C (12)
  0x7c, 0x04, 0x38, 0x04, 0x78, // m 0x6D (13)
  0x7c, 0x04, 0x04, 0x04, 0x78, // n 0x6E (14)
  0x38, 0x44, 0x44, 0x44, 0x38, // o 0x6F (15)

  0x7c, 0x14, 0x14, 0x14, 0x08, // p 0x70 (0)
  0x08, 0x14, 0x14, 0x7c, 0x40, // q 0x71 (1)
  0x7c, 0x04, 0x04, 0x04, 0x08, // r 0x72 (2)
  0x48, 0x54, 0x54, 0x54, 0x24, // s 0x73 (3)
  0x04, 0x04, 0x7f, 0x44, 0x44, // t 0x74 (4)
  0x3c, 0x40, 0x40, 0x40, 0x7c, // u 0x75 (5)
  0x1c, 0x20, 0x40, 0x20, 0x1c, // v 0x76 (6)
  0x7c, 0x40, 0x38, 0x40, 0x7c, // w 0x77 (7)
  0x44, 0x28, 0x10, 0x28, 0x44, // x 0x78 (8)
  0x0c, 0x50, 0x50, 0x50, 0x3c, // y 0x79 (9)
  0x44, 0x64, 0x54, 0x4c, 0x44, // z 0x7A (10)
  0x00, 0x08, 0x36, 0x41, 0x00, // { 0x7B (11)
  0x00, 0x00, 0x7f, 0x00, 0x00, // | 0x7C (12)
  0x00, 0x41, 0x36, 0x08, 0x00, // } 0x7D (13)
  0x00, 0x04, 0x02, 0x04, 0x08, // ~ 0x7E (14)
  0x7F, 0x41, 0x41, 0x41, 0x7F, //   0x7F (15)

  0x7D, 0x0a, 0x09, 0x0a, 0x7D, // � 0x41 (1)
  0x3F, 0x41, 0x41, 0x41, 0x3F, // � 0x4F (15)
  0x3D, 0x40, 0x40, 0x40, 0x3D, // � 0x55 (5)
  0x20, 0x55, 0x54, 0x55, 0x78, // � 0x61 (1)
  0x38, 0x45, 0x44, 0x45, 0x38, // � 0x6F (15)
  0x3c, 0x41, 0x40, 0x41, 0x7c, // � 0x75 (5)
};
