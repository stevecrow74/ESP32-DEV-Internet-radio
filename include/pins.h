/******************************************************************************
 *
 * pins.h
 *
 ******************************************************************************/

#pragma once

//==================================================
// ST7789 TFT Display
//==================================================

constexpr uint8_t TFT_CS   = 5;
constexpr uint8_t TFT_DC   = 2;
constexpr uint8_t TFT_RST  = 4;

constexpr uint8_t TFT_MOSI = 23;
constexpr uint8_t TFT_SCLK = 18;

//==================================================
// PCM5102A I2S DAC
//==================================================

constexpr uint8_t I2S_BCLK = 26;
constexpr uint8_t I2S_LRC  = 25;
constexpr uint8_t I2S_DOUT = 22;

//==================================================
// Encoder 1 (Volume)
//==================================================

constexpr uint8_t ENC1_CLK = 32;
constexpr uint8_t ENC1_DT  = 33;
constexpr uint8_t ENC1_SW  = 27;

//==================================================
// Encoder 2 (Station / Menu)
//==================================================

constexpr uint8_t ENC2_CLK = 14;
constexpr uint8_t ENC2_DT  = 13;
constexpr uint8_t ENC2_SW  = 21;