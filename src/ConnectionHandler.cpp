#pragma once

#include <Arduino.h>
#include <Credentials.h>

#include <SpotifyClient.cpp>

class ConnectionHandler {
   public:
    void handle(WiFiClient client, SpotifyClient* spClient) {
        // Current time
        unsigned long startTime = millis();
        Serial.println("New Client.");
        String currentLine = "";
        String path = "";

        while (client.connected() && millis() - startTime <= 2000) {  // loop while the client's connected
            if (client.available()) {                                 // if there's bytes to read from the client,
                char c = client.read();                               // read a byte, then
                Serial.write(c);                                      // print it out the serial monitor

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
                            Serial.println("root");
                            serve_root(client, path);
                        } else if (path.startsWith("/callback")) {  // Get token from Spotify
                            Serial.println("callback");
                            serve_callback(client, path, spClient);
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

   private:
    void serve_root(WiFiClient client, String path) {
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

    void serve_callback(WiFiClient client, String path, SpotifyClient* spClient) {
        Serial.println("In callback path: " + path);

        // Variables that we'll parse:
        String oAuthCode = extract_query_parameter(path, "code");

        if (!oAuthCode.isEmpty() && spClient->get_spotify_token(oAuthCode)) {
            Track* track = new Track;
            spClient->get_current_playing_track(track);

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

    String extract_query_parameter(String path, String needle) {
        String delimiter = "?";
        String queryParams = path.substring(path.indexOf(delimiter) + delimiter.length());

        if (queryParams.isEmpty()) {
            return "";
        }

        // delimiter = "&";
        int offset = 0;
        while (offset < queryParams.length()) {
            int delimiterPos = queryParams.indexOf("&", offset);
            if (delimiterPos < 0) {
                delimiterPos = queryParams.length();
            }
            String keyValue = queryParams.substring(offset, delimiterPos);
            offset = delimiterPos + delimiter.length();

            if (needle == keyValue.substring(0, keyValue.indexOf("="))) {
                return keyValue.substring(keyValue.indexOf("=") + 1);
            }
        }

        return "";
    }
};
