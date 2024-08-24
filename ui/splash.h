#ifndef SPLASH_H
#define SPLASH_H

#include <stdio.h>

/** Left to right, forward, 8bit band. Reverse bit order. */

const uint8_t splash_image[] = {
  0x00, 0x78, 0x30, 0x30, 0x30, 0x30, 0x30, 0xf0, 0xf0, 0x30, 0x30, 0x30, 0x30, 0x30, 0x78, 0x80, 0x80, 0x80, 0x80,
  0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
  0xc0, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80,
  0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
  0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x99, 0x99, 0x99, 0x99, 0xbd, 0x81, 0xc3, 0x00, 0x00, 0x00, 0xff,
  0xff, 0x19, 0x19, 0x19, 0x39, 0x79, 0xf9, 0xdf, 0x0f, 0x00, 0x00, 0x00, 0xff, 0xff, 0x99, 0x99, 0x99, 0x99, 0xbd,
  0x81, 0xc3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00,
  0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x03, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00,
  0x00, 0x01, 0x01, 0x01, 0x01, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x03, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xfe, 0xfe, 0x1e, 0x3e, 0xf0, 0xc0, 0x00, 0x00, 0x00, 0x80, 0xe0, 0x3a,
  0x1e, 0xfe, 0xfe, 0x02, 0x00, 0x10, 0xf0, 0xf0, 0x10, 0x00, 0x00, 0x80, 0xc0, 0xe0, 0x70, 0x30, 0x30, 0xb0, 0x30,
  0x30, 0x60, 0xe0, 0x20, 0x00, 0x00, 0x10, 0xf0, 0xf0, 0x10, 0x00, 0x00, 0x00, 0x00, 0x10, 0xf0, 0xf0, 0x10, 0x00,
  0x78, 0x30, 0x30, 0x30, 0x30, 0xf0, 0xf0, 0x30, 0x30, 0x30, 0x30, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xfe,
  0xfe, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0xfe, 0xbc, 0x00, 0x00, 0x00, 0x10, 0xf0, 0xf0, 0x30, 0x30, 0x30,
  0x30, 0xb0, 0x30, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x20, 0x3f, 0x3f, 0x20, 0x00, 0x00, 0x03, 0x0f, 0x3c, 0x0e, 0x03, 0x00, 0x00, 0x20, 0x3f, 0x3f, 0x20, 0x00,
  0x20, 0x3f, 0x3f, 0x20, 0x00, 0x00, 0x07, 0x0f, 0x18, 0x38, 0x30, 0x30, 0x37, 0x33, 0x33, 0x33, 0x1f, 0x1f, 0x09,
  0x00, 0x20, 0x3f, 0x3f, 0x23, 0x03, 0x03, 0x03, 0x03, 0x23, 0x3f, 0x3f, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20,
  0x3f, 0x3f, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x3f, 0x3f, 0x30, 0x30, 0x30, 0x30,
  0x30, 0x30, 0x30, 0x39, 0x1f, 0x0f, 0x00, 0x00, 0x20, 0x3f, 0x3f, 0x33, 0x33, 0x33, 0x33, 0x37, 0x30, 0x78, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0xc0, 0xc0, 0xc0,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0xc0, 0xc0, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x40, 0xc0, 0xc0, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x80, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x03, 0x07, 0x0e, 0x1c, 0x38, 0x70,
  0xe0, 0xc0, 0x80, 0xff, 0xff, 0x00, 0x00, 0x00, 0xf0, 0xf8, 0x0c, 0x0e, 0x06, 0x06, 0x06, 0x06, 0x0e, 0x0c, 0xf8,
  0xf0, 0x00, 0x00, 0x02, 0xfe, 0xfe, 0x02, 0x00, 0x00, 0xb8, 0x7c, 0x66, 0x66, 0x66, 0x66, 0x66, 0xec, 0xcc, 0xc4,
  0x00, 0x00, 0x02, 0xfe, 0xfe, 0x66, 0x66, 0x66, 0x66, 0xf6, 0x06, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
  0x06, 0x1e, 0x7a, 0xe0, 0x80, 0x00, 0x80, 0xe0, 0x7a, 0x1e, 0x06, 0x02, 0x00, 0x00, 0x00, 0x03, 0x03, 0xff, 0xff,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0xff, 0x83, 0x01, 0x00, 0x00,
  0x00, 0x00, 0x01, 0x83, 0xff, 0x7c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x07, 0x07, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x07, 0x07, 0x07,
  0x04, 0x00, 0x00, 0x00, 0x01, 0x03, 0x07, 0x06, 0x06, 0x06, 0x06, 0x07, 0x03, 0x01, 0x00, 0x00, 0x00, 0x04, 0x07,
  0x07, 0x04, 0x00, 0x00, 0x02, 0x03, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x03, 0x01, 0x00, 0x00, 0x04, 0x07, 0x07,
  0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x07,
  0x06, 0x07, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x07, 0x07, 0x04, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x07, 0x07, 0x07, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x07, 0x06, 0x06, 0x06, 0x06, 0x03, 0x03, 0x01,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

#endif // SPLASH_H
