#pragma once

// GC9A01 wiring (first recommendation):
// VCC->3.3  GND->G  SCL->6  SDA->7  DC->2  CS->10  RST->3
#define TFT_SCLK 6
#define TFT_MOSI 7
#define TFT_CS   10
#define TFT_DC   2
#define TFT_RST  3

// EC11 rotary encoder: CLK->0  DT->1  SW->5  +->3.3  GND->G
#define ENC_CLK 0
#define ENC_DT  1
#define ENC_SW  5

// TF / microSD (SPI shared with TFT: SCK=6 MOSI=7)
// CS->4  MISO->20  VCC->3.3  GND->G
#define SD_CS   4
#define SD_MISO 20
