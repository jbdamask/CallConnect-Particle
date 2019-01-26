// This #include statement was automatically added by the Particle IDE.
#include <clickButton.h>

// This #include statement was automatically added by the Particle IDE.
#include <neopixel.h>
#include <math.h>

/* Variable definitions ------------------------------*/
#define BUTTON          D2
#define PIXEL_COUNT     13 // number of LEDs on strip
#define PIXEL_PIN       D6 // pin connected to the small NeoPixels strip
#define PIXEL_TYPE      SK6812RGBW  // NeoPixel RGBW
#define BRIGHTNESS      100 // Max brightness of NeoPixels
#define BUTTON_DEBOUNCE 20  // Removes button noise
#define IDLE_TIMEOUT    5000   // Milliseconds that there can be no touch or ble input before reverting to idle state

/* Button stuff -----*/
const int buttonPin1 = 2;
ClickButton button1(buttonPin1, LOW, CLICKBTN_PULLUP);
// Keep track of who calls and who receives
bool makingCall = false;
bool previouslyTouched = false;

/* MQTT stuff -----*/
#define DEBUG_TOPIC     "debug-lights"
#define TOPIC           "lights"
int previousMQTTState = 0;

/* Animation stuff -----*/
unsigned long patternInterval = 20 ; // time between steps in the pattern
// Each animation should have a value in this array
unsigned long animationSpeed [] = { 100, 50, 2, 2 } ; // speed for each animation (order counts!)
#define ANIMATIONS sizeof(animationSpeed) / sizeof(animationSpeed[0])
// Colors for sparkle
uint8_t myFavoriteColors[][3] = {{200,   0, 200},   // purple
                                 {200,   0,   0},   // red
                                 {200, 200, 200},   // white
                               };
#define FAVCOLORS sizeof(myFavoriteColors) / 3

// Connection state (0 = idle; 1 = calling; 2 = connected)
uint8_t state = 0, previousState = 0;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN);
unsigned long lastUpdate = 0, idleTimer = 0; // for millis() when last update occurred
bool gotNewMessage = false;   // Set by MQTT handler
int gotState = 0;             // Set by MQTT handler


void setup() {
    Serial.begin(9600);
    // Here we are going to subscribe to your buddy's event using Particle.subscribe
    // Particle.subscribe(TOPIC, myHandler, MY_DEVICES);
    Particle.subscribe(TOPIC, myHandler);
    pinMode(D2, INPUT_PULLUP);
    button1.debounceTime   = BUTTON_DEBOUNCE;   // Debounce timer in ms
    strip.setBrightness(BRIGHTNESS); // These things are bright!
    strip.begin(); // This initializes the NeoPixel library.
    wipe(); // wipes the LED buffers
    strip.show();

}

void loop() {
    static long countDown = 0;  // Counts down a certain number of seconds before taking action
    bool static toldUs = false; // When in state 1, we're either making or receiving a call
    bool isTouched = false; // Holds button state
    button1.Update();       // Update button state
    if(button1.clicks != 0) isTouched = true;
   // gotNewMessage = false;      // Reset

    /* Change animation speed if state changed */
    if(previousState != state) {
        // Serial.println("State change");
        wipe();
        resetBrightness();
        patternInterval = animationSpeed[state]; // set speed for this animation
        previousState = state;
    //    Serial.print("Animation speed for state: "); Serial.print(state); Serial.print(" is "); Serial.println(patternInterval);
    }


    // The various cases we can face
    switch(state){
        case 0: // Idle
            if(isTouched) {
                state = 1;
                publish(String(state));
                previouslyTouched = true;
                makingCall = true;
                Serial.println("Calling...");
                idleTimer = millis();
            } /*else if (gotNewMessage) {
                Serial.println("State 0. Got new message");
                if(gotState == 1){
                    state = 1;
                    previousMQTTState = 1; // Why aren't I setting this to 0?
                    idleTimer = millis();
                } else if(gotState == 0){
                  // This would be odd but if we are in state 0 and got a message for state 0, just ignore
                  return;
                } else {
                //   Serial.print("Expected payload 1 but got "); Serial.println(packetbuffer[2]);
                  resetState();
                }
            } */
            break;
        case 1: // calling
            if(makingCall){
                if(!toldUs) { // This is used to print once to the console
                  //Serial.println("I'm making the call");
                  toldUs = true;
                }
                if(millis() - idleTimer > IDLE_TIMEOUT){
                  resetState();       // If no answer, we reset
                  Serial.println("No one answered :-(");
                }
    //             if(gotNewMessage && previousMQTTState != 1){ // Our call has been answered. We're now connected
    //               if(gotState == 2){
    //                 //Serial.print("State 1: Received BLE. Moving to State "); Serial.println(packetbuffer[2]);
    //                 state = 2;
    //                 previousMQTTState = 3; // <-------- Is this needed? Why is it being set to 3?
    //               }else{
    // //                Serial.print("Expected payload 2 but got "); Serial.println(packetbuffer[2]);
    // //                Serial.println("Resetting state to 0");
    //                 resetState();
    //               }
    //             }
            } else if(isTouched){  // If we're receiving a call, are now are touching the local device, then we're connected
                //Serial.println("State 1. Button pushed. Moving to State 2");
                state = 2;
                publish("2");
                //bleWrite(2);
                previouslyTouched = true;
            }
            // } else if(gotNewMessage) {
            //     //Serial.println("Receiving call");
            //     if(gotState == 0){ // This device didn't answer in time so we check to see if we got a timeout signal
            //       state = 0;
            // }

            break;
        case 2:
            if(isTouched){    // Touch again to disconnect
                Serial.println("State 2. Button pushed. Moving to State 3");
                state = 3;
                publish("3");
                //bleWrite(3);
                previouslyTouched = false;
            }
            // else if( gotNewMessage ) {
            //     if(gotState == 3){
            //         Serial.print("State 2. Received MQTT message. Moving to State 3");
            //         state = 3;
            //         previousMQTTState = 3; // is this needed?????
            //     }else{
            //         //Serial.print("Expected payload 3 but got "); Serial.println(packetbuffer[2]);
            //         resetState();
            //     }
            // }
            if(state == 3) {
                Serial.println("Disconnecting. Starting count down timer.");
                countDown = millis();   // Start the timer
            }
            break;
        case 3:
            if(millis() - countDown > IDLE_TIMEOUT) {
                Serial.println("State 3. Timed out. Moving to State 0");
                resetState();
                // Reset
//                previouslyTouched = false;
//                makingCall = false;
                //bleReceived = false;
                previousState = 0;
                //previousBleState = 0;
            }
            if(isTouched && previouslyTouched == false){  // If we took our hand off but put it back on in under the time limit, re-connect
                //Serial.println("Reconnecting...");
                //Serial.println("State 3. Button pushed. The device that initiated a disconnection has reconnected. Moving to State 2");
                state = 2;
                publish("2");
//                bleWrite(2);
                previouslyTouched = true;
            }
            // else if( gotNewMessage ) {
            //     if(gotState == 2){
            //          //Serial.print("State 3. Received BLE. Moving to State "); Serial.println(packetbuffer[2]);
            //          state = 2;
            //          previousMQTTState = 2; // is this needed?????
            //     }else{
            //         //Serial.print("Expected payload 2 but got "); Serial.println(packetbuffer[2]);
            //         resetState();
            //     }
            // }
            break;
        default:
            resetState();
            break;
    }

    // Update animation frame
    if(millis() - lastUpdate > patternInterval) {
        updatePattern(state);
    }

    gotNewMessage = false;      // Reset
}


void publish(String payload){
    Particle.publish(TOPIC, payload);
}

int cnt = 0;
// Handles MQTT event listenting.
void myHandler(const char *event, const char *data)
{
    cnt++;
    Serial.print(cnt);
    Serial.print(event);
    Serial.print(", data: ");
    if (data)
    Serial.println(data);
    else
    Serial.println("NULL");

    // gotNewMessage = true; //<------ This doesn't belong here
    if(strcmp(data,"0")==0){
        gotState = 0;
        state = 0;
    }else if(strcmp(data,"1")==0){
        gotState = 1;
        state = 1;
    }else if(strcmp(data,"2")==0){
        gotState = 2;
        state = 2;
    }else if(strcmp(data,"3")==0){
        gotState = 3;
        state = 3;
    }else{
        //gotState = -1;              // This is unexpected
    }
   // Particle.publish(DEBUG_TOPIC, data);
}

// Clean house
void resetState(){
  state = 0;
  previousMQTTState = 0;
  //previousBleState = 0;
  previouslyTouched = false;
  makingCall = false;
  publish(String(state));
}

// Update the animation
void  updatePattern(int pat){
  switch(pat) {
    case 0:
      wipe();
      strip.show();
      break;
    case 1:
      wipe();
      sparkle(3);
      break;
    case 2:
      breathe(1); // Breath blue
      break;
    case 3:
      breathe(2); // Breathe red
      break;
  }
}


// LED breathing. Used for when devices are connected to one another
void breathe(int x) {
  float SpeedFactor = 0.008;
  static int i = 0;
  static int r,g,b;
  switch(x){
    case 1:
      r = 0; g = 127; b = 127;
      break;
    case 2:
      r = 255; g = 0; b = 0;
      break;
  }
  // Make the lights breathe
  float intensity = BRIGHTNESS /2.0 * (1.0 + sin(SpeedFactor * i));
  strip.setBrightness(intensity);
  for (int j=0; j<strip.numPixels(); j++) {
    strip.setPixelColor(j, r, g, b);
  }
  strip.show();
  i++;
  if(i >= 65535){
    i = 0;
  }
  lastUpdate = millis();
}

// LED sparkling. Used for when devices are "calling"
void sparkle(uint8_t howmany) {
  static int x = 0;
  static bool goingUp = true;

  for(uint16_t i=0; i<howmany; i++) {
    // pick a random favorite color!
    int c = random(FAVCOLORS);
    int red = myFavoriteColors[c][0];
    int green = myFavoriteColors[c][1];
    int blue = myFavoriteColors[c][2];

    // get a random pixel from the list
    int j = random(strip.numPixels());

    // now we will 'fade' it in 5 steps
    if(goingUp){
      if(x < 5) {
        x++;
      } else {
        goingUp = false;
      }
    } else {
      if(x>0){
        x--;
      } else {
        goingUp = true;
      }
    }

    int r = red * (x+1); r /= 5;
    int g = green * (x+1); g /= 5;
    int b = blue * (x+1); b /= 5;
    strip.setPixelColor(j, strip.Color(r,g,b));
    strip.show();
  }
  lastUpdate = millis();
}

// clear all LEDs
void wipe(){
   for(int i=0;i<strip.numPixels();i++){
     strip.setPixelColor(i, strip.Color(0,0,0));
   }
}

void resetBrightness(){
  strip.setBrightness(BRIGHTNESS);
}
