#pragma once

#include <ArduinoJson.h>
#include <HTTPClient.h>

// Secrets
#include <Credentials.h>

struct SpotifyTokens {
    String token;
    unsigned long validUntil;
    String refreshToken;

    bool hasValidToken() {
        return validUntil > millis();
    }
};

struct Track {
    String id;
    String name;
    String albumName;
    String artistName;

    unsigned long durationMs;
    unsigned long startedPlaying;
};

class SpotifyClient {
   private:
    SpotifyTokens* apiTokens = new SpotifyTokens();

   public:
    bool oauth_authenticate(String spotifyOAuthCode) {
        HTTPClient http;
        http.begin("https://accounts.spotify.com/api/token");

        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
        http.setAuthorization(SPOTIFY_CLIENT_ID, SPOTIFY_CLIENT_SECRET);
        String postData = "grant_type=authorization_code&redirect_uri=http%3A%2F%2F192.168.86.66%2Fcallback&code=" + spotifyOAuthCode;

        int httpResponseCode = http.POST(postData);
        Serial.printf("(getToken) HTTP response or error code: %i\n", httpResponseCode);
        if (httpResponseCode < 200 || httpResponseCode >= 300) {
            Serial.println(http.getString());

            return false;
        }

        String json = http.getString();
        // Free resources
        http.end();

        StaticJsonDocument<512> doc;  // Measured response size is 417 characters
        DeserializationError error = deserializeJson(doc, json);

        // Test if parsing succeeds.
        if (error) {
            Serial.print("deserializeJson() in oauth_authenticate failed: ");
            Serial.println(error.f_str());
            return false;
        }

        apiTokens->token = (const char*)doc["access_token"];
        apiTokens->validUntil = millis() + doc["expires_in"].as<int>() * 1000;
        apiTokens->refreshToken = (const char*)doc["refresh_token"];

        // Print so we can use it manually for faster debugging
        Serial.println(apiTokens->token);

        return true;
    }

    bool hasValidToken() {
        return this->apiTokens->hasValidToken();
    }

    bool oauth_refresh_token() {
        // If there is no refresh token, we cannot refresh the api token yet.
        if (apiTokens->refreshToken.isEmpty()) {
            return false;
        }

        // If we have more than 10 minutes left, we don't have to refresh yet.
        if (apiTokens->validUntil - millis() > 10 * 60 * 1000) {
            return true;
        }

        HTTPClient http;
        http.begin("https://accounts.spotify.com/api/token");

        http.addHeader("Content-Type", "application/x-www-form-urlencoded");
        http.setAuthorization(SPOTIFY_CLIENT_ID, SPOTIFY_CLIENT_SECRET);
        String postData = "grant_type=refresh_token&refresh_token=" + apiTokens->refreshToken;

        int httpResponseCode = http.POST(postData);
        Serial.printf("(getToken/refresh) HTTP response or error code: %i\n", httpResponseCode);
        if (httpResponseCode < 200 || httpResponseCode >= 300) {
            return false;
        }

        String json = http.getString();
        // Free resources
        http.end();

        StaticJsonDocument<512> doc;  // Measured response size is 417 characters
        DeserializationError error = deserializeJson(doc, json);

        // Test if parsing succeeds.
        if (error) {
            Serial.print("deserializeJson() in oauth_refresh_token failed: ");
            Serial.println(error.f_str());
            return false;
        }

        apiTokens->token = (const char*)doc["access_token"];
        apiTokens->validUntil = millis() + doc["expires_in"].as<int>() * 1000;

        Serial.println("Refreshed API token");
        return true;
    }

    /*
    * Updates the currently playing track.
    * 
    * Returns true if it changed compared to last invocation.
    */
    bool get_current_playing_track(Track* track) {
// Override for easier testing - define const in Credentials.h to skip the oauth flow
#ifdef SPOTIFY_OAUTH_TOKEN
        if (apiTokens->token == "") {
            apiTokens->token = SPOTIFY_OAUTH_TOKEN;
        }
#endif

        if (apiTokens->token.isEmpty()) {
            return false;
        }

        HTTPClient http;
        http.begin("https://api.spotify.com/v1/me/player/currently-playing");

        http.addHeader("Content-Type", "application/json");
        http.addHeader("Authorization", "Bearer " + apiTokens->token);

        int httpResponseCode = http.GET();

        Serial.printf("(getTrack) HTTP response or error code: %i\n", httpResponseCode);
        if (httpResponseCode < 200 || httpResponseCode >= 300) {
            return false;
        }

        if (httpResponseCode == 204) {
            track->id = "";
            track->name = "N/A";
            track->albumName = "N/A";
            track->artistName = "N/A";
            return true;
        }

        String payload = http.getString();
        // Free resources
        http.end();

        // This allows us to only parse the fields that we're interested in.
        // The total response size (>5000 characters) is too large.
        StaticJsonDocument<200> filter;
        filter["item"]["id"] = true;
        filter["item"]["name"] = true;
        filter["item"]["album"]["name"] = true;
        filter["item"]["artists"][0]["name"] = true;
        filter["item"]["duration_ms"] = true;
        filter["item"]["progress_ms"] = true;

        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, payload, DeserializationOption::Filter(filter));

        // Test if parsing succeeds.
        if (error) {
            Serial.print("deserializeJson() in get current track failed: ");
            Serial.println(error.f_str());
            return false;
        }

        bool trackChanged = track->id != doc["item"]["id"].as<String>();

        track->id = doc["item"]["id"].as<String>();
        track->name = doc["item"]["name"].as<String>();
        track->albumName = doc["item"]["album"]["name"].as<String>();
        track->artistName = doc["item"]["artists"][0]["name"].as<String>();
        track->durationMs = doc["item"]["duration_ms"].as<unsigned long>();
        track->startedPlaying = millis() - doc["item"]["progress_ms"].as<unsigned long>();

        Serial.println("Updated currently playing track.");
        return trackChanged;
    }
};
