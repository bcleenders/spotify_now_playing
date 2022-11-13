#pragma once

#include <Arduino.h>

// Helper class to track what the URI of the ESP is (its IP address), and some helpful formatting for the URIs of its endpoints.
class Endpoint {
   private:
    String domain;

  public:
    Endpoint(String domain) {
        this->domain = domain;
    }

    /**
     * Root of the server.
     * First page you visit, when scanning the QR code.
    */
    const char* rootUri() {
        return create_server_url("http://", domain.c_str(), "/");
    }

    /**
     * URL-escaped callback URI.
     * Clients use this in OAuth to find their way back to us.
     *
     * Note: this URI (the non-escaped version) must be listed as a 'Redirect URI' on
     * https://developer.spotify.com/dashboard/applications (go to 'edit settings' under the right application)
    */
    const char* escapedCallbackUri() {
        return create_server_url("http%3A%2F%2F", domain.c_str(), "%2Fcallback");
    }

  private:
    static char* create_server_url(const char* protocol, const char* address, const char* path) {
        unsigned int const sz1 = strlen(protocol);
        unsigned int const sz2 = strlen(address);
        unsigned int const sz3 = strlen(path);

        char* url = (char*)malloc(sz1 + sz2 + sz3 + 1);

        memcpy(url, protocol, sz1);
        memcpy(url + sz1, address, sz2);
        memcpy(url + sz1 + sz2, path, sz3);
        url[sz1 + sz2 + sz3] = '\0';

        return url;
    }
};