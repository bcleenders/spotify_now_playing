#include <WiFi.h>

class Network {
   public:
    static String connect(const char* ssid, const char* passphrase) {
        // Connect to Wi-Fi network with SSID and password
        Serial.print("Connecting to:");
        Serial.println(ssid);

        WiFi.begin(ssid, passphrase);

        delay(500);
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
        }
        // Print local IP address and start web server
        Serial.println("");

        String ipAddress = WiFi.localIP().toString();

        Serial.print("WiFi connected. IP address: ");
        Serial.println(ipAddress.c_str());

        return ipAddress;
    }
};
