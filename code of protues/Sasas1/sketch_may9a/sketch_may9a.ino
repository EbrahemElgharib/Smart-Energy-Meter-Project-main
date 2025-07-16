#include <Wire.h>

unsigned long lastBackupTime = 0;  // Last time Backup data was processed
unsigned long lastSlaveTime = 0;   // Last time Slave data was processed
const long interval = 2000;        // 2-second interval between operations

void setup() {
  Wire.begin(0); // Master I2C address (default 0)
  Wire.onReceive(receiveEvent); // Register event for receiving data
  Serial.begin(9600);
  Serial.println("Master Started");
}

void loop() {
  unsigned long currentTime = millis();

  // Send signal to Backup Master (address 9) to keep it alive
  Wire.beginTransmission(9);
  Wire.write(1);
  Wire.endTransmission();

  // Send signal to Slave (address 8) to keep it alive
  Wire.beginTransmission(8);
  Wire.write(1);
  Wire.endTransmission();

  // Process Backup data every 2 seconds (offset by 1 second)
  if (currentTime - lastBackupTime >= interval) {
    // No explicit request needed since Backup sends data every 1 second
    lastBackupTime = currentTime; // Update last processed time
  }

  // Process Slave data after another 1 second (total 2 seconds cycle)
  if (currentTime - lastSlaveTime >= 2 * interval) {
    // No explicit request needed since Slave sends data every 1 second
    lastSlaveTime = currentTime - interval; // Reset to align with next cycle
  }

  delay(100); // Small delay to prevent overwhelming the loop
}

// Handle incoming data from Backup or Slave
void receiveEvent(int howMany) {
  if (Wire.available() >= 5) { // Expect 5 bytes: 1 for device ID, 4 for data
    byte deviceID = Wire.read(); // First byte is device ID (8 or 9)
    byte b0 = Wire.read(); // High byte of voltage_int
    byte b1 = Wire.read(); // Low byte of voltage_int
    byte b2 = Wire.read(); // High byte of temperature_int
    byte b3 = Wire.read(); // Low byte of temperature_int

    uint16_t voltage_int = (b0 << 8) | b1; // Combine bytes for voltage
    uint16_t temperature_int = (b2 << 8) | b3; // Combine bytes for temperature

    float voltage = voltage_int / 100.0; // Convert back to float
    float temp = temperature_int / 10.0; // Convert back to float

    // Print based on device ID with timing control
    unsigned long currentTime = millis();
    if (deviceID == 9 && currentTime - lastBackupTime < interval) {
      Serial.print("Backup Voltage: ");
      Serial.print(voltage, 2);
      Serial.print(" V, Temp: ");
      Serial.print(temp, 1);
      Serial.println(" C");
    } else if (deviceID == 8 && currentTime - lastSlaveTime >= interval && currentTime - lastSlaveTime < 2 * interval) {
      Serial.print("Slave Voltage: ");
      Serial.print(voltage, 2);
      Serial.print(" V, Temp: ");
      Serial.print(temp, 1);
      Serial.println(" C");
    }
  }
}