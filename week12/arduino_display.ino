
#include <Wire.h>
#include <ArduinoBLE.h>

#define DISPLAY_ADDRESS 0x72

const char* peripheralServiceUUID = "90019001-9001-9001-9001-900190019001";
const char* peripheralTempUUID = "90019001-FFFF-FFFF-FFFF-900190019001";
const char* peripheralHumUUID = "90019001-0000-0000-0000-900190019001";

void setup() {
  Serial.begin(9600);
  //while(!Serial);
  delay(500);

  Wire.begin();

  if (!BLE.begin()) {
    Serial.println("failed to start BLE...");
    while (1);
  }

  BLE.setLocalName("BLE 33 hehe");
  BLE.advertise();

  Wire.beginTransmission(DISPLAY_ADDRESS);
  Wire.write('|');
  Wire.write('-');
  Wire.write("Init");
  Wire.endTransmission();

  delay(500);

  Serial.println("BLE active, avaiting connections");

}

void i2cUpdateScreen(uint8_t tempVal, uint8_t humVal) {
  char temp[2];
  char hum[2];
  itoa(tempVal, temp, DEC);
  itoa(humVal, hum, DEC);

  Wire.beginTransmission(DISPLAY_ADDRESS);

  Wire.write('|'); //Put LCD into setting mode
  Wire.write('-'); //Send clear display command
  Wire.endTransmission();

  Wire.beginTransmission(DISPLAY_ADDRESS);

  Wire.write("Temperature: ");
  Wire.print(temp);
  Wire.write("CHumidity: ");
  Wire.print(hum);
  Wire.print("% RH");

  Wire.endTransmission();
}

void connectToPeripheral() {
  BLEDevice peripheral;

  Serial.println("Discovering peripheral...");

  do {
    BLE.scanForUuid(peripheralServiceUUID);
    peripheral = BLE.available();
  } while (!peripheral);

  if (peripheral) {
    Serial.println("* Peripheral device found!");
    Serial.print("* Device MAC address: ");
    Serial.println(peripheral.address());
    Serial.print("* Device name: ");
    Serial.println(peripheral.localName());
    Serial.print("* Advertised service UUID: ");
    Serial.println(peripheral.advertisedServiceUuid());
    Serial.println(" ");
    BLE.stopScan();
    controlPeripheral(peripheral);
  }
}

void controlPeripheral(BLEDevice peripheral) {
  //i2cWaitingScreen(0);

  if (peripheral.connect()) {
    //i2cWaitingScreen(1);
    Serial.println("* Connected to peripheral device!");
  } else {
    Serial.println("* Connection to peripheral device failed!");
    return;
  }

  if (peripheral.discoverAttributes()) {
    //i2cWaitingScreen(2);
    Serial.println("* Peripheral device attributes discovered!");
  } else {
    Serial.println("* Peripheral device attributes discovery failed!");
    return;
  }

  BLECharacteristic tempCharacteristic = peripheral.characteristic(peripheralTempUUID);
  BLECharacteristic humCharacteristic = peripheral.characteristic(peripheralHumUUID);

  //i2cWaitingScreen(3);

  if (!tempCharacteristic || !humCharacteristic) {
    Serial.println("* Peripheral device does not have temp/hum characteristic!");
    peripheral.disconnect();
    return;
  } else if (!tempCharacteristic.canRead() || !humCharacteristic.canRead()) {
    Serial.println("* Peripheral does not have a readable characteristic!");
    peripheral.disconnect();
    return;
  }

  uint8_t tempValue[1] = {99};
  uint8_t humValue[1] = {99};
  
  while (peripheral.connected()) {
    delay(1112);

    tempCharacteristic.readValue(tempValue, 1);
    humCharacteristic.readValue(humValue, 1);

    Serial.print("temp [");
    Serial.println(tempValue[0]);

    i2cUpdateScreen(tempValue[0], humValue[0]);
    //i2cUpdateScreen();
  }
}

void loop() {
  connectToPeripheral();
  delay(1);
}
