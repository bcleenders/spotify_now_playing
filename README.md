# ESP32 Spotify now playing

This repo fetches the currently playing track from your Spotify account, and shows it on an e-ink display.

Clients authenticate via OAuth.

## Flow

### 1 - Start screen with QR code to log in

Scan QR code with a phone to visit the webserver on the ESP for authentication.

Note: phone must be connected to the same WiFi network as the ESP32.

![ESP start screen](docs/start.jpeg)

### 2 - Log in on Spotify

The ESP redirects to an OAuth login flow on Spotify.

![Spotify OAuth login](docs/login%20screen.jpeg)

### 3 - ESP shows currently playing track on Spotify

The ESP will show a different screen, with track info of the track you're currently playing on Spotify.

![Currently playing track](docs/now%20playing.jpeg)

Track will auto-update every few seconds.

To log in as a different user, scan the QR code on the right side of the screen to restart the OAuth flow.