#include <Arduino.h>

#include <ConnectionHandler.cpp>
#include <Network.cpp>
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

        String ipAddress = Network::connect(WIFI_SSID, WIFI_PASS);
        server.begin();
        ui->show_start_screen_qrcode(ipAddress);

        // ui->show_start_screen_qrcode("1.2.3.4");
    }

    void test_function() {
        Serial.begin(115200);
        while (!Serial) {
            delay(10);
        }
        // this->spClient->get_current_playing_track(track);

        track->id = "trackid";
        track->name = "Space-Dye Vest567890";
        track->albumName = "Awake";
        track->artistName = "Dream Theater";

        // Serial.printf("Now playing: %s / %s, by %s\n",
        //               track->name.c_str(),
        //               track->albumName.c_str(),
        //               track->artistName.c_str());

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

        // Poll if the currently playing track has changed.
        bool shouldRefresh = (millis() > track->startedPlaying + track->durationMs && millis() < track->startedPlaying + track->durationMs + 500) ||  // Track ended (or was pauzed)
                             (millis() > last_track_refresh + 10 * 1000);                                                                             // 10 seconds passed since last refresh
        if (this->spClient->hasValidToken() && shouldRefresh) {
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
#ifdef TEST_FUNCTION
    // Easier testing of independent functions
    program->test_function();
#else
    // Run the actual program
    program->run_main();
#endif
}

void loop() {
#ifdef TEST_FUNCTION
    // Disable running loops periodically - just the test loop once.
    Serial.println("Test mode - not running loop.");
    delay(1000);
#else
    program->run_loop();
#endif
}
