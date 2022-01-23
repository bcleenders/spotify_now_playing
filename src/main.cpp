#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>

#include <ui.cpp>
// #include <WiFiUdp.h>

// #include "base64.h"

// extern "C" {
// #include "crypto/base64.h"
// }

// Secrets
#include <Credentials.h>

WiFiServer server(80);
// Variable to store the HTTP request
String header;

struct SpotifyTokens {
    String token;
    unsigned long validUntil;
    String refreshToken;
};

struct Track {
    String name;
    String albumName;
    String artistName;
};

class Module {
   public:
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

        test_function();
    }

    void test_function() {
        Track* track = new Track;

        get_current_playing_track(track);

        Serial.printf("Now playing: %s / %s, by %s\n",
                      track->name.c_str(),
                      track->albumName.c_str(),
                      track->artistName.c_str());
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
    UI ui;
    void serve_redirect(WiFiClient client, String path) {
        client.println("HTTP/1.1 302 Moved Temporarily");
        client.printf(
            "Location: https://accounts.spotify.com/authorize?response_type=code"
            "&client_id=%s"
            "&scope=user-read-email,user-read-playback-state"
            "&redirect_uri=%s\n",
            SPOTIFY_CLIENT_ID,
            "http%3A%2F%2F192.168.86.188%2Fcallback");
        client.println();
    }

    void serve_callback(WiFiClient client, String path) {
        Serial.println("In callback path: " + path);

        String delimiter = "?";
        String queryParams = path.substring(path.indexOf(delimiter) + delimiter.length());

        // Variables that we'll parse:
        String oAuthCode = "";  // URL key is 'code'

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

            if (key == "code") {  // ?code=$OAUTH_TOKEN
                oAuthCode = value;
            }
        }

        if (!oAuthCode.isEmpty() && get_spotify_token(oAuthCode)) {
            Track* track = new Track;
            get_current_playing_track(track);

            client.println("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
            client.println("<html><body>");
            client.println("<h1>Logged in succesfully</h1>");
            client.println("<p>Now playing:</p>");
            client.println("<p>Track: ");
            client.println(track->name.c_str());
            client.println("</p>");
            client.println("<p>Album: ");
            client.println(track->albumName.c_str());
            client.println("</p>");
            client.println("<p>Artists: ");
            client.println(track->artistName.c_str());
            client.println("</p>");
            client.println("</body></html>");

            // TODO - start token refresh loop & start display updates
        }
    }

    SpotifyTokens apiTokens = {"", 0, ""};

    bool get_spotify_token(String spotifyOAuthCode) {
        HTTPClient http;
        http.begin("https://accounts.spotify.com/api/token");

        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
        http.setAuthorization(SPOTIFY_CLIENT_ID, SPOTIFY_CLIENT_SECRET);
        String postData = "grant_type=authorization_code&redirect_uri=http%3A%2F%2F192.168.86.188%2Fcallback&code=" + spotifyOAuthCode;

        int httpResponseCode = http.POST(postData);
        Serial.printf("(getToken) HTTP response or error code: %i\n", httpResponseCode);
        if (httpResponseCode < 200 || httpResponseCode >= 300) {
            return false;
        }

        String payload = http.getString();
        // Free resources
        http.end();

        Serial.println(payload);

        set_apiTokens(payload);

        return true;
    }

    /**
     * Parses the JSON return string to extract access token & refresh token, and set
     * it as class property.
     */
    bool set_apiTokens(String json) {
        StaticJsonDocument<512> doc;  // Measured response size is 417 characters
        DeserializationError error = deserializeJson(doc, json);

        // Test if parsing succeeds.
        if (error) {
            Serial.print("deserializeJson() in set_apiTokens failed: ");
            Serial.println(error.f_str());
            return false;
        }

        apiTokens.token = (const char*)doc["access_token"];
        apiTokens.validUntil = millis() + doc["expires_in"].as<int>() * 1000;
        apiTokens.refreshToken = (const char*)doc["refresh_token"];
        return true;
    }

    bool get_current_playing_track(Track* track) {
        HTTPClient http;

// Override for easier testing - define const in Credentials.h to skip the oauth flow
#ifdef SPOTIFY_OAUTH_TOKEN
        if (apiTokens.token == "") {
            apiTokens.token = SPOTIFY_OAUTH_TOKEN;
        }
#endif

        http.begin("https://api.spotify.com/v1/me/player/currently-playing");

        http.addHeader("Content-Type", "application/json");
        http.addHeader("Authorization", "Bearer " + apiTokens.token);

        int httpResponseCode = http.GET();

        Serial.printf("(getTrack) HTTP response or error code: %i\n", httpResponseCode);
        if (httpResponseCode < 200 || httpResponseCode >= 300) {
            Serial.println("(getTrack) return false");
            return false;
        }

        if (httpResponseCode == 204) {
            track->name = "N/A";
            track->albumName = "N/A";
            track->artistName = "N/A";
        }

        Serial.println("(getTrack) end of if");
        String payload = http.getString();
        Serial.println("(getTrack) getString");
        // Free resources
        http.end();
        Serial.println("(getTrack) http end");

        // This allows us to only parse the fields that we're interested in.
        // The total response size (>5000 characters) is too large.
        StaticJsonDocument<200> filter;
        filter["item"]["name"] = true;
        filter["item"]["album"]["name"] = true;
        filter["item"]["artists"][0]["name"] = true;

        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, payload, DeserializationOption::Filter(filter));

        // Test if parsing succeeds.
        if (error) {
            Serial.print("deserializeJson() in get current track failed: ");
            Serial.println(error.f_str());
            return false;
        }

        track->name = doc["item"]["name"].as<String>();
        track->albumName = doc["item"]["album"]["name"].as<String>();
        track->artistName = doc["item"]["artists"][0]["name"].as<String>();

        return true;
    }
};

Module* program = new Module();

void setup() {
    program->run_main();
}

void loop() {
    program->run_loop();
}
