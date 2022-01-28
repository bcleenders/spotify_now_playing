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
    UI* ui;
    SpotifyClient* spClient;
    ConnectionHandler* connectionHandler;

   public:
    Module(UI* ui, SpotifyClient* spClient, ConnectionHandler* connectionHandler) {
        this->ui = ui;
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

        ui->show_start_screen_qrcode();

        // test_function();
    }

    void test_function() {
        this->spClient->get_current_playing_track(track);

        Serial.printf("Now playing: %s / %s, by %s\n",
                      track->name.c_str(),
                      track->albumName.c_str(),
                      track->artistName.c_str());

        ui->show_playing_track(track);
    }

   private:
    Track* track = new Track;
    unsigned long last_track_refresh = 0;

   public:
    void run_loop() {
        // If there is a client, serve it
        if (WiFiClient client = server.available()) {
            this->connectionHandler->handle(client, this->spClient);
        }

        // Refresh the API token (only if needed)
        this->spClient->oauth_refresh_token();

        if (millis() > last_track_refresh + 10 * 1000) {
            last_track_refresh = millis();

            bool trackChanged = this->spClient->get_current_playing_track(track);
            if (trackChanged) {
                ui->show_playing_track(track);
            }
        }

        delay(10);
    }
};

UI* ui = new UI();
SpotifyClient* spClient = new SpotifyClient();
ConnectionHandler* connectionHandler = new ConnectionHandler();
Module* program = new Module(ui, spClient, connectionHandler);

void setup() {
    program->run_main();
}

void loop() {
    program->run_loop();
}
