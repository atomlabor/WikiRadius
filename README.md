# WikiRadius v3.0.0

Welcome to **WikiRadius**! This is a contextual awareness application crafted for Pebble smartwatches (specifically optimised for the rectangular display of the Emery / Pebble Time 2). It transforms your wrist into a gateway for discovery, uncovering historical landmarks, fascinating points of interest, and geographical curiosities right in your immediate vicinity, powered by the Wikipedia Geosearch API.

## Core Functionality

Think of this application as a bridge between your physical location and the vast repository of human knowledge. The moment you activate the app, it seamlessly executes the following:

1. **Geographic Discovery**: By harnessing your smartphone's GPS via PebbleKit JS, the app pinpoint-locates the closest Wikipedia-indexed landmark within a 5-kilometre radius.
2. **Knowledge Transfer**: Engaging, bite-sized snippets of the relevant Wikipedia article are beamed straight to your watch, allowing you to absorb the history of your surroundings without ever reaching for your phone.

### Key Features
* **Interactive Scroll Interface**: Dive deeper into the lore. A dedicated `ScrollLayer` lets you read extended Wikipedia extracts (up to 550 characters) comfortably using the watch's tactile hardware buttons.
* **Flawless Typography**: Full UTF-8 integration ensures that umlauts, accents, and special characters across various languages are rendered impeccably.
* **Arrival Haptics**: Feel your destination. A satisfying double-pulse vibration alert triggers the moment you are within a 50-metre radius of your target location.
* **Smart Persistence**: Your last discovered location and its historical snippet are safely tucked away in the watch's internal storage for offline reading later.

## Purpose & Use Case

WikiRadius was dreamt up to elevate the brilliant experience of urban exploration. Let's face it: traditional mobile apps demand that you constantly stare at a glowing screen, ripping you out of the moment and disconnecting you from the environment you're trying to explore.

WikiRadius elegantly solves this. By offering a "glanceable", hands-free interface, it effortlessly answers the lingering question, *"What on earth is this place?"* Whether you're a curious traveller navigating a new city or a local history buff wandering your own neighbourhood, WikiRadius turns a simple stroll into an educational, modern journey.

## Technical Specifications

* **Platform**: Optimised exclusively for the rectangular Pebble Emery (200x228).
* **Framework**: Built firmly upon **Pebble SDK 4.0**, leveraging all its modern capabilities.
* **Communication**: AppMessage protocol (1024 bytes inbox / 128 bytes outbox).
* **Visual Assets**: Features a bespoke custom splash screen and a beautifully crisp system menu icon.

## ☕ Support & More Apps

If you enjoy using WikiRadius and find it useful on your urban adventures, I would be absolutely delighted if you gave me a mention! If you'd like to help fuel my late-night coding sessions, you can pop a donation into my tip jar here:
👉 **[Buy me a coffee on Ko-fi](https://ko-fi.com/atomlabor)**

Curious about what else I've been building? You can find my entire collection of Pebble apps right here:
👉 **[Atomlabor on the Pebble Store](https://apps.repebble.com/apps/dev/atomlabor_5c1e0bc226259d0001915df5)**

## Legal Disclaimer

Please note that WikiRadius is a completely independent project. I do not collaborate with, nor is this application affiliated with, endorsed by, or sponsored by Wikipedia or the Wikimedia Foundation. The application solely acts as a client that queries the public Wikipedia Geosearch API to provide geographical data.

## Licence 

This project is proudly open-source under the MIT Licence.

**MIT License**

Copyright (c) 2026 Atomlabor (https://atomlabor.de)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
