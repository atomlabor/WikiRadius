# WikiRadius v1.0.0

WikiRadius is a contextual awareness application designed for Pebble smartwatches (specifically optimized for the Emery Pebble Time 2). It enables users to discover historical landmarks, points of interest, and geographical curiosities in their immediate vicinity using the Wikipedia Geosearch API.

## Core Functionality

The application operates as a bridge between your physical location and the vast repository of human knowledge. Upon activation, the app performs the following steps:

1.  **Geographic Discovery**: Utilizing the smartphone's GPS via PebbleKit JS, the app identifies the closest Wikipedia-indexed location within a 5km radius.
2.  **Knowledge Transfer**: Descriptive snippets of the Wikipedia article are transmitted to the watch, allowing the user to read about their surroundings directly on their wrist.


* **Interactive Scroll Interface**: A dedicated ScrollLayer allows users to read extended Wikipedia extracts (up to 550 characters) using the watch's physical hardware buttons.
* **Special Character Support**: Full UTF-8 integration ensures that umlauts and special characters in various languages are rendered correctly.
* **Arrival Haptics**: A double-pulse vibration alert triggers once the user is within 50 meters of the target location.
* **Persistence**: The last discovered location and its description are saved to the watch's internal storage for offline reference.
  

## Purpose and Use Case

WikiRadius was developed to enhance the experience of urban exploration. Traditional mobile applications require the user to constantly look at a smartphone screen, which disconnects them from their environment. 

WikiRadius solves this by providing a "glanceable" interface. It answers the question "What is this place?" through a non-intrusive, hands-free experience. Whether you are a tourist in a new city or a local history enthusiast, WikiRadius turns a simple walk into an educational journey.

## Technical Specifications

* **Platform Support**: Pebble Emery (200x228).
* **Communication**: AppMessage protocol (1024 bytes inbox / 128 bytes outbox).
* **Assets**: Features a custom splashscreen and system menu icon.



## License

This project is part of the continuous mindfuck series, focusing on modern, scientifically sound, and innovative wearable concepts. Developed by https://atomlabor.de
