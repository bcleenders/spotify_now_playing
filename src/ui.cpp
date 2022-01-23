#include <WiFi.h>

#include "Adafruit_EPD.h"
#include "Adafruit_ThinkInk.h"
#include "qrcode.h"

// Pin outs for ESP32 featherwing
#ifndef EPD_PORTS
#define EPD_PORTS
#define SRAM_CS 32
#define EPD_CS 15
#define EPD_DC 33
#define EPD_RESET -1  // can set to -1 and share with microcontroller Reset!
#define EPD_BUSY -1   // can set to -1 to not use a pin (will wait a fixed delay)
#endif

class UI {
   public:
    void show_qr_code() {
        QRCode qrcode;
        const char* server_url = ("http://" + WiFi.localIP().toString() + "/").c_str();
        uint8_t qrcode_version = 3;
        uint8_t qrcodeBytes[qrcode_getBufferSize(qrcode_version)];
        qrcode_initText(&qrcode, qrcodeBytes, qrcode_version, 0, server_url);

        // Write to e-ink display
        display.begin(THINKINK_GRAYSCALE4);
        display.clearBuffer();
        display.cp437(true);  // Enable Code Page 437-compatible charset.
        display.setTextSize(1);
        display.setCursor(0, 0);

        int8_t xOffset = (display.width() / 2) - qrcode.size;
        int8_t yOffset = (display.height() / 2) - qrcode.size;

        for (int8_t y = 0; y < qrcode.size; y++) {
            for (int8_t x = 0; x < qrcode.size; x++) {
                if (qrcode_getModule(&qrcode, x, y)) {
                    display.drawPixel(xOffset + 2 * x, yOffset + 2 * y, 1);
                    display.drawPixel(xOffset + 2 * x, yOffset + 2 * y + 1, 1);
                    display.drawPixel(xOffset + 2 * x + 1, yOffset + 2 * y, 1);
                    display.drawPixel(xOffset + 2 * x + 1, yOffset + 2 * y + 1, 1);
                } else {
                    display.drawPixel(xOffset + 2 * x, yOffset + 2 * y, 0);
                    display.drawPixel(xOffset + 2 * x, yOffset + 2 * y + 1, 0);
                    display.drawPixel(xOffset + 2 * x + 1, yOffset + 2 * y, 0);
                    display.drawPixel(xOffset + 2 * x + 1, yOffset + 2 * y + 1, 0);
                }
            }
        }

        display.display();
        display.powerDown();
    }

   private:
    // 2.9" Grayscale Featherwing or Breakout:
    ThinkInk_290_Grayscale4_T5 display = ThinkInk_290_Grayscale4_T5(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);
};
