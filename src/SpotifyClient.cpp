#pragma once

#include <ArduinoJson.h>
#include <HTTPClient.h>

// Secrets
#include <Credentials.h>

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

class SpotifyClient {
   public:
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
            return true;
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

   private:
    SpotifyTokens apiTokens = {"", 0, ""};
};
