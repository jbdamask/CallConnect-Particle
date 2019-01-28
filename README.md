## Call Connect for Particle Photon Boards
LED animations to visualize connected device "calls" and connections. Code written for Particle but only works on Photon.

## Hardware
* Particle Photon
* Neopixels
* Button (will change to FSR in near future)

### Inputs
A local sensor, such as a button, and MQTT topic subscription from Particle.io cloud. A second button is used to trigger WiFi configuration via SoftAP

### Outputs
LED animations and MQTT topic pubs


## Notes
* SoftAP works great and the extra button for WiFi configuration should make it easy for Grandma's to set up
* Note that this code WILL NOT WORK with Argon boards as they don't currently seem to support SoftAP
* I'm not happy with Particle's platform. I want to make working prototypes to hand out to people which means they need to be easy to set up on their networks. But SoftAP only exists for Photon, not the latest Argon board. I've lost a day of time trying to figure out why. Moving past Particle onto ESP32
* Requires Particle-specific libraries for buttons and NeoPixels
