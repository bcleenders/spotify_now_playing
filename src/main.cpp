#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include "Adafruit_EPD.h"
#include "Adafruit_ThinkInk.h"
#include "qrcode.h"

// Secrets
#include <Credentials.h>

// Pin outs for ESP32 featherwing
#ifndef EPD_PORTS
#define EPD_PORTS
#define SRAM_CS 32
#define EPD_CS 15
#define EPD_DC 33
#define EPD_RESET -1  // can set to -1 and share with microcontroller Reset!
#define EPD_BUSY -1   // can set to -1 to not use a pin (will wait a fixed delay)
#endif

WiFiServer server(80);
// Variable to store the HTTP request
String header;

struct SpotifyTokens {
    String token;
    unsigned long validUntil;
    String refreshToken;
};

class Module {
   public:
    void run_main() {
        Serial.begin(115200);
        while (!Serial) {
            delay(10);
        }

        // Connect to Wi-Fi network with SSID and password
        Serial.printf("Connecting to: %s\n", WIFI_SSID);

        WiFi.begin(WIFI_SSID, WIFI_PASS);
        delay(500);
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
        }
        // Print local IP address and start web server
        Serial.println("");

        Serial.println("WiFi connected.");
        Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
        server.begin();

        show_qr_code();
    }

    // Define timeout time in milliseconds (example: 2000ms = 2s)
    const long timeoutTime = 2000;

    void run_loop() {
        WiFiClient client = server.available();  // Listen for incoming clients

        if (client) {  // If a new client connects,
            // Current time
            unsigned long startTime = millis();
            Serial.println("New Client.");
            String currentLine = "";
            String path = "";

            while (client.connected() && millis() - startTime <= timeoutTime) {  // loop while the client's connected
                if (client.available()) {                                        // if there's bytes to read from the client,
                    char c = client.read();                                      // read a byte, then
                    Serial.write(c);                                             // print it out the serial monitor

                    if (c == '\r') {
                        // Skip carriage returns
                    } else if (c != '\n') {
                        currentLine += c;  // add it to the end of the currentLine
                    } else {
                        // End of line!
                        if (currentLine.length() > 0) {
                            // Parse line to extract useful fields
                            if (currentLine.startsWith("GET")) {
                                // Parse the / out of `GET / HTTP/1.1`
                                int pathStart = currentLine.indexOf("/");
                                path = currentLine.substring(pathStart, currentLine.indexOf(" ", pathStart));
                            }
                        } else {
                            // Two consecutive newlines - end of client request!
                            // Start our response
                            if (path == "/") {  // Send to Spotify for OAuth consent screen
                                serve_redirect(client, path);
                            } else if (path.startsWith("/callback")) {  // Get token from Spotify
                                serve_callback(client, path);
                            } else {
                                Serial.println("404");
                            }

                            // Break out of the while loop
                            break;
                        }
                        currentLine = "";
                    }
                }
            }
            // Close the connection
            client.stop();
            Serial.println("Client disconnected.");
            Serial.println("");
        }
    }

   private:
    void serve_redirect(WiFiClient client, String path) {
        client.println("HTTP/1.1 302 Moved Temporarily");
        client.printf(
            "Location: https://accounts.spotify.com/authorize?response_type=code"
            "&client_id=%s"
            "&scope=user-read-email,user-read-playback-state"
            "&redirect_uri=%s\n",
            SPOTIFY_CLIENT_ID,
            "http%3A%2F%2F192.168.86.188%2Fcallback");
        // client.println("Connection: close");
        client.println();
    }

    void serve_callback(WiFiClient client, String path) {
        Serial.println("In callback path: " + path);

        String delimiter = "?";
        String queryParams = path.substring(path.indexOf(delimiter) + delimiter.length());

        delimiter = "&";
        int offset = 0;
        int iteration = 0;
        while (offset < queryParams.length() && iteration < 5) {
            iteration++;

            int delimiterPos = queryParams.indexOf("&", offset);
            if (delimiterPos < 0) {
                delimiterPos = queryParams.length();
            }
            String keyValue = queryParams.substring(offset, delimiterPos);
            offset = delimiterPos + delimiter.length();

            // Now split key and value. Remember that value is optional (/callback?key=value&key2 is valid)
            Serial.printf("kv=%s\n\n", keyValue.c_str());
            int splitPos = keyValue.indexOf("=");
            String key, value;
            if (splitPos < 0) {
                key = keyValue;
            } else {
                key = keyValue.substring(0, splitPos);
                value = keyValue.substring(splitPos + 1);
            }

            if (key == "code") {
                get_spotify_token(value);
            }
        }

        client.println("HTTP/1.1 200 OK");
        client.println("Content-type:text/html");
        client.println("Connection: close");
        client.println();
    }

    SpotifyTokens apiTokens;

    void get_spotify_token(String spotifyOAuthCode) {
        Serial.println("TODO: use the OAuth code to get a token + refresh token");

        apiTokens.token = "token";
        apiTokens.validUntil = millis() + 3600 * 1000;
        apiTokens.refreshToken = "foo";
    }

    // 2.9" Grayscale Featherwing or Breakout:
    ThinkInk_290_Grayscale4_T5 display = ThinkInk_290_Grayscale4_T5(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);

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

    HTTPClient http;
};

Module* program = new Module();

void setup() {
    program->run_main();
}

void loop() {
    program->run_loop();
}
