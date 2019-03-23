#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

/*  
 * Title: So Good To See You!
 * Description: A wearable lighting project that intensifies in the presence of friends
 * Status: In Development
 * 
 * Technologies
 *  ESP32 Microcontroller - Detects broadcasting iBeacons via Bluetooth, and controls the lights
 *  Gimbal Beacons - Small iBeacons that broadcast their presence for months.
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
} Person;

typedef struct {
  Person person;
  unsigned int ibeacon_minor;  
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
    {"Oli"}, 0
  },
  {
    {"Max"}, 1
  },  
  {
    {"Trini"}, 2
  },  
  {
    {"Collin"}, 3
  }    
};

/*
 * Lighting Logic
 */


/*
 * Bluetooth Logic
 */

Person findPersonMatchingIBeacon(IBeacon iBeacon) {
  for (int i = 0; i < num_configs; i++) {
    Configuration c = configs[i];
    if (c.ibeacon_minor == iBeacon.minor) {
      return c.person; 
    }  
  }
  return Person {
    "Unknown Person"  
  };
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
      Person person = findPersonMatchingIBeacon(beacon);
      Serial.print("Person: "); 
      Serial.println(person.name);
      device.getScan()->stop(); 
    }
  }
};

void Bluetooth() {
  Serial.println("BLE Scan restarted.....");
  BLEScanResults scanResults = pScan->start(5);
  delay(10000);
}

/*
 * Core Arduino Methods
 */

void setup() {
  // put your setup code here, to run once:
  // Log at this frequency
  Serial.begin(115200);
  Serial.println("Begin Setup");
  
  // Turn on Bluetooth
  BLEDevice::init("");
  
  // Put the device in Client Mode
  //pClient = BLEDevice::createClient();
  
  // Set up callbacks for Scanner
  pScan = BLEDevice::getScan();
  pScan->setAdvertisedDeviceCallbacks(new MyDeviceCallbacks());
  
  Serial.println("Finished Setup");
}

void loop() {
  // put your main code here, to run repeatedly:
  Bluetooth();
}