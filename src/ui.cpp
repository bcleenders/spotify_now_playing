#include <Adafruit_GFX.h>  // Needed for the font
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
#include <WiFi.h>

#include <SpotifyClient.cpp>

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
   private:
    // 2.9" Grayscale Featherwing or Breakout:
    ThinkInk_290_Grayscale4_T5 display = ThinkInk_290_Grayscale4_T5(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);

   public:
    void show_start_screen_qrcode(String ipAddress) {
        const char* server_url = create_server_url(ipAddress.c_str(), "/");

        Serial.print("Server url: ");
        Serial.println(server_url);

        QRCode qrcode;
        uint8_t qrcode_version = 3;
        uint8_t qrcodeBytes[qrcode_getBufferSize(qrcode_version)];
        qrcode_initText(&qrcode, qrcodeBytes, qrcode_version, 0, server_url);

        display.begin(THINKINK_GRAYSCALE4);
        display.clearBuffer();
        display.cp437(true);  // Enable Code Page 437-compatible charset.
        display.setCursor(0, 0);

        const int8_t scalefactor = 4;
        int8_t xOffset = (display.width() / 2) - (0.5 * scalefactor * qrcode.size);
        int8_t yOffset = (display.height() / 2) - (0.5 * scalefactor * qrcode.size);
        draw_qr_code(&qrcode, xOffset, yOffset, scalefactor);

        display.display();
        display.powerDown();
        Serial.println("Finished drawing");
    }

    void show_playing_track(Track* track) {
        display.begin(THINKINK_GRAYSCALE4);
        display.clearBuffer();
        display.cp437(true);  // Enable Code Page 437-compatible charset.
        display.setTextColor(EPD_BLACK);

        display.setFont(&FreeMono12pt7b);

        // Write to e-ink display
        display.setTextSize(1);

        display.setCursor(8, 26);  // 8 to the left, 8px + 1em down

        display.setFont(&FreeMono12pt7b);
        display.println(track->name.substring(0, 18));

        // display.setTextSize(2);
        display.setFont(&FreeMono9pt7b);

        display.setCursor(8, 48);
        display.print("Album: ");
        display.println(track->albumName);

        display.setCursor(8, 64);
        display.print("Artist: ");
        display.println(track->artistName);

        // And draw a QR code on the side
        const char* server_url = create_server_url(WiFi.localIP().toString().c_str(), "/");
        QRCode qrcode;
        uint8_t qrcode_version = 3;
        uint8_t qrcodeBytes[qrcode_getBufferSize(qrcode_version)];
        qrcode_initText(&qrcode, qrcodeBytes, qrcode_version, 0, server_url);

        const int8_t scalefactor = 2;
        int8_t xOffset = display.width() - (scalefactor * qrcode.size) - 8;
        int8_t yOffset = (display.height() / 2) - (0.5 * scalefactor * qrcode.size);
        draw_qr_code(&qrcode, xOffset, yOffset, scalefactor);

        display.display();
        display.powerDown();
        Serial.println("Finished drawing");
    }

   private:
    void draw_qr_code(QRCode* qrcode, uint8_t xOffset, uint8_t yOffset, uint8_t scalefactor) {
        for (int8_t y = 0; y < qrcode->size; y++) {
            for (int8_t x = 0; x < qrcode->size; x++) {
                uint8_t color;
                if (qrcode_getModule(qrcode, x, y)) {
                    color = 1;
                } else {
                    color = 0;
                }
                for (int8_t yCell = 0; yCell < scalefactor; yCell++) {
                    for (int8_t xCell = 0; xCell < scalefactor; xCell++) {
                        display.drawPixel(
                            xOffset + scalefactor * x + xCell,
                            yOffset + scalefactor * y + yCell,
                            color);
                    }
                }
            }
        }
    }

    static char* create_server_url(const char* address, const char* path) {
        char const* protocol = "http://";

        unsigned int const sz1 = strlen(protocol);
        unsigned int const sz2 = strlen(address);
        unsigned int const sz3 = strlen(path);

        char* url = (char*)malloc(sz1 + sz2 + sz3 + 1);

        memcpy(url, protocol, sz1);
        memcpy(url + sz1, address, sz2);
        memcpy(url + sz1 + sz2, path, sz3);
        url[sz1 + sz2 + sz3] = '\0';

        return url;
    }
};
