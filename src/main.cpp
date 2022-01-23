#include <Arduino.h>
#include <WiFi.h>

#include <ConnectionHandler.cpp>
#include <SpotifyClient.cpp>
#include <ui.cpp>

// Secrets
#include <Credentials.h>

WiFiServer server(80);
// Variable to store the HTTP request
String header;

class Module {
   private:
    SpotifyClient* spClient;
    ConnectionHandler* connectionHandler;

   public:
    Module(SpotifyClient* spClient, ConnectionHandler* connectionHandler) {
        this->spClient = spClient;
        this->connectionHandler = connectionHandler;
    }

    void run_main() {
        Serial.begin(115200);
        while (!Serial) {
            delay(10);
        }

        // Connect to Wi-Fi network with SSID and password
        Serial.print("Connecting to:");
        Serial.println(WIFI_SSID);

        WiFi.begin(WIFI_SSID, WIFI_PASS);
        delay(500);
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
        }
        // Print local IP address and start web server
        Serial.println("");

        Serial.print("WiFi connected. IP address: ");
        Serial.println(WiFi.localIP().toString().c_str());
        server.begin();

        ui.show_qr_code();

        // test_function();
    }

    void test_function() {
        Track* track = new Track;

        this->spClient->get_current_playing_track(track);

        Serial.printf("Now playing: %s / %s, by %s\n",
                      track->name.c_str(),
                      track->albumName.c_str(),
                      track->artistName.c_str());
    }

    void run_loop() {
        WiFiClient client = server.available();  // Listen for incoming clients

        if (client) {  // If a new client connects,
            Serial.println("I'm here!");
            this->connectionHandler->handle(client, this->spClient);
        }
    }

   private:
    UI ui;
};

SpotifyClient* spClient = new SpotifyClient();
ConnectionHandler* connectionHandler = new ConnectionHandler();
Module* program = new Module(spClient, connectionHandler);

void setup() {
    program->run_main();
}

void loop() {
    program->run_loop();
}
