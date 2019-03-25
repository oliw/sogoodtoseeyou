#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <Adafruit_NeoPixel.h>
#include <TaskScheduler.h>

/*  
 * Title: So Good To See You!
 * Description: A wearable lighting project that intensifies in the presence of friends
 * Status: In Development
 * 
 * Technologies
 *  ESP32 Microcontroller - Detects broadcasting iBeacons via Bluetooth, and controls the lights
 *  Gimbal Beacons - Small iBeacons that broadcast their presence for months.
 *  
 *  15b87d78-d638-45c7-8dde-ace5fb6468f3
 * 
 * Author: Oliver Wilkie, oliwilks@
 */


/*
 * Beacons have 3 properties, UUID, major and minor
 * UUID should be unique in the world for your project
 * Major should be constant for everyone
 * Minor should be a unique number per person (0-65535)
 */
const String BEACON_UUID = "15b87d78d63845c78ddeace5fb6468f3"; //  UUID of this project, all iBeacons for this project should advertise with this
const int BEACON_MAX = 65535; // This is the Max value denoting People in this project, All iBeacons worn by people should wear this.

typedef struct {
  String uuid;
  unsigned int major;
  unsigned int minor;
} IBeacon;

typedef struct {
  String name;
  bool here;
} Person;

typedef struct {
  int r;
  int g;
  int b;
} Color;

typedef struct {
  Person person;
  unsigned int ibeacon_minor;
  Color color;  
} Configuration;

// Global variables
BLEClient* pClient;
BLEScan* pScan;


/*
 * Configuration Logic
 */

int num_configs = 4;
Configuration configs[] =
{
  {
    {"Oli", false}, 0, {255,0,0}
  },
  {
    {"Max", false}, 1, {0,255,0}
  },  
  {
    {"Trini", false}, 2, {0,255,255}
  },  
  {
    {"Collin", false}, 3, {255,165,0}
  }    
};

Person unknown = {"Unknown Person", false};

/*
 * Lighting Logic
 */

 // Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1
#define PIN            18

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS      8

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

/*
 * Bluetooth Logic
 */

Person* findPersonMatchingIBeacon(IBeacon iBeacon) {
  for (int i = 0; i < num_configs; i++) {
    Configuration c = configs[i];
    if (c.ibeacon_minor == iBeacon.minor) {
      return &(configs[i].person); 
    }  
  }
  return &unknown;
}


IBeacon buildFromBLEAdvertisedDevice(BLEAdvertisedDevice device) {
  // TODO: Understand why this is needed
  // Build a hex string of the Manufacturer Data
  char *pHex = BLEUtils::buildHexData(nullptr, (uint8_t*)device.getManufacturerData().data(), device.getManufacturerData().length());
  std::string data(pHex);
  free(pHex);

  IBeacon beacon;
  beacon.uuid = data.substr(8,32).c_str();
  beacon.major = (unsigned int) strtol(data.substr(40,4).c_str(), 0, 16);
  beacon.minor = (unsigned int) strtol(data.substr(44,4).c_str(), 0, 16);
  return beacon;
}


bool deviceIsAnIBeacon(BLEAdvertisedDevice device) {  
  // TODO: Understand why this is needed
  // Build a hex string of the Manufacturer Data
  char *pHex = BLEUtils::buildHexData(nullptr, (uint8_t*)device.getManufacturerData().data(), device.getManufacturerData().length());
  std::string data(pHex);
  free(pHex);

  /* 
   * data will look something like this (without spaces)
   * https://en.wikipedia.org/wiki/IBeacon
   * 4c00 02 15 15b87d78-d638-45c7-8dde-ace5fb6468f3 2332 6312 c5 
   */
   
  if (data.length() != 50) {
    return false;  
  }
  
  // bytes 0-1 should be Apple
  if (data.substr(0,4) != "4c00") {
    return false;  
  }
  // byte 2 should be iBeacon
  if (data.substr(4,2) != "02") {
    return false; 
  }
  // byte 3 should be Length (15)
  if (data.substr(6,2) != "15") {
    return false;     
  }
  // byte 4-19 should be our UUID
  if (!(String(data.substr(8,32).c_str()).equals(BEACON_UUID))) {
    return false;  
  }
  
  return true;
}

class MyDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice device){    
    if (deviceIsAnIBeacon(device)) {
      IBeacon beacon = buildFromBLEAdvertisedDevice(device);
      Serial.println("We found a beacon!");
      Serial.print("UUID: "); 
      Serial.println(beacon.uuid);  
      Serial.print("Major: "); 
      Serial.println(beacon.major); 
      Serial.print("Minor: "); 
      Serial.println(beacon.minor);
      Person *person = findPersonMatchingIBeacon(beacon);
      Serial.print("Person: "); 
      Serial.println(person->name);
      person->here = true;
      device.getScan()->stop(); 
    }
  }
};

void bluetooth() {
  Serial.println("BLE Scan started.....");
  unsigned long StartTime = millis();
  BLEScanResults scanResults = pScan->start(2);
  unsigned long CurrentTime = millis();
  unsigned long ElapsedTime = CurrentTime - StartTime;
  Serial.println("BLE Scan ended.....");
  Serial.println(ElapsedTime);
}

Color colorForPixel(int pixel) {
  Color defaultColor = {255,192,203}; // Moderately bright pink color.

  int person = pixel/2;
  Serial.print("Looking for person:");
  Serial.print(person);
  Serial.print("Is the person here?");
  Serial.println(configs[person].person.here);
  if (configs[person].person.here) {
    return configs[person].color;  
  }
  
  return defaultColor;
}

void lighting() {
  Serial.println("Running lighting sequence");
  for(int i=0;i<NUMPIXELS;i++){

    // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    Color color = colorForPixel(i);
    pixels.setPixelColor(i, pixels.Color(color.r,color.g,color.b)); // Moderately bright green color.

    pixels.show(); // This sends the updated pixel color to the hardware.

    delay(50); // Delay for a period of time (in milliseconds).

  }

    for(int i=0;i<NUMPIXELS;i++){

    // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
    pixels.setPixelColor(NUMPIXELS-1-i, pixels.Color(0,0,0)); // Moderately bright green color.

    pixels.show(); // This sends the updated pixel color to the hardware.

    delay(50); // Delay for a period of time (in milliseconds).

  }   
}

Task bluetoothTask(10000, TASK_FOREVER, &bluetooth);
Task lightingTask(0, TASK_FOREVER, &lighting);
Scheduler runner;


/*
 * Core Arduino Methods
 */

void setup() {
  // Log at this frequency
  Serial.begin(115200);
  Serial.println("Begin Setup");
  
  // Turn on Bluetooth
  BLEDevice::init("");
  pScan = BLEDevice::getScan();
  pScan->setAdvertisedDeviceCallbacks(new MyDeviceCallbacks());

  // Turn on lighting
  pixels.begin(); // This initializes the NeoPixel library.

  // Turn on tasks
  runner.init();
  runner.addTask(bluetoothTask);
  runner.addTask(lightingTask);
  bluetoothTask.enable();
  lightingTask.enable();
  
  Serial.println("Finished Setup");
}

void loop() {
  runner.execute();
}
