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

    // Current time
    unsigned long currentTime = millis();
    // Previous time
    unsigned long previousTime = 0;
    // Define timeout time in milliseconds (example: 2000ms = 2s)
    const long timeoutTime = 2000;

    void run_loop() {
        WiFiClient client = server.available();  // Listen for incoming clients

        if (client) {  // If a new client connects,
            currentTime = millis();
            previousTime = currentTime;
            Serial.println("New Client.");
            String currentLine = "";
            String path = "";

            while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
                currentTime = millis();
                if (client.available()) {    // if there's bytes to read from the client,
                    char c = client.read();  // read a byte, then
                    Serial.write(c);         // print it out the serial monitor

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
                                Serial.println("found a GET header!" + path);
                            }
                        } else {
                            if (path != "") {
                                Serial.print("Requested path: '" + path + "'");
                            }

                            // Two consecutive newlines - end of client request!
                            // Start our response
                            client.println("HTTP/1.1 200 OK");
                            client.println("Content-type:text/html");
                            client.println("Connection: close");
                            client.println();

                            // Display the HTML web page
                            client.println("<!DOCTYPE html><html>");
                            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
                            client.println("<link rel=\"icon\" href=\"data:,\">");
                            // CSS to style the on/off buttons
                            // Feel free to change the background-color and font-size attributes to fit your preferences
                            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
                            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
                            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
                            client.println(".button2 {background-color: #555555;}</style></head>");
                            client.println("<body><h1>ESP32 Web Server</h1>");
                            client.println("<p>Your request path was: '" + path + "'</p>");
                            client.println("</body></html>");
                            client.println();
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
    // 2.9" Grayscale Featherwing or Breakout:
    ThinkInk_290_Grayscale4_T5 display = ThinkInk_290_Grayscale4_T5(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY);

    void show_qr_code() {
        QRCode qrcode;
        const char* server_url = ("http://" + WiFi.localIP().toString()).c_str();
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
