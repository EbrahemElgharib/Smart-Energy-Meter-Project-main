#include <Wire.h>
#include <LiquidCrystal.h>

const int voltagePin = A0;    // Voltage measurement pin
const int tempPin = A3;       // LM35 temperature sensor pin
const int buttonPin = A2;     // Button pin
const int ledPin = 6;
const int relayPin = 13;
const float voltageThreshold = 5.5; // Voltage threshold
const float tempThreshold = 30.0;   // Temperature threshold: 30°C
const float R1 = 1000.0;           // Voltage divider resistor R1
const float R2 = 1000.0;           // Voltage divider resistor R2

LiquidCrystal lcd(7, 12, 11, 10, 9, 8);

int systemState = HIGH;
bool buttonState = LOW;
bool lastButtonState = HIGH;
unsigned long lastSignalTime = 0;
unsigned long lastSendTime = 0; // To track last data send time
unsigned long lastUpdateTime = 0; // To track last LCD update
unsigned long lastSlaveDataTime = 0; // Track last valid Slave data
bool masterAlive = true;
bool isBackupMaster = false;
bool overvoltage = false;
bool overheat = false;
uint16_t voltage_int;    // Scaled voltage (voltage * 100)
uint16_t temperature_int; // Scaled temperature (temperature * 10)
float lastSlaveVoltage = 0.0; // Store last valid Slave voltage
float lastSlaveTemp = 0.0;    // Store last valid Slave temperature
String dataSource = "Backup"; // Track data source

// Buffer for receiving Slave data
#define BUFFER_SIZE 5
uint8_t dataBuffer[BUFFER_SIZE];
uint8_t bufferIndex = 0;
bool newSlaveData = false;

void setup() {
  Wire.begin(9);          // I2C address 9 for Backup Master
  Wire.setClock(50000);   // Slow I2C clock to 50kHz for reliability
  Wire.onReceive(receiveEvent); // Handle received signals
  Serial.begin(9600);

  pinMode(ledPin, OUTPUT);
  pinMode(relayPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(tempPin, INPUT);
  digitalWrite(relayPin, HIGH); // Start system ON

  lcd.begin(16, 4);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Ready");
  delay(500);
}

void receiveEvent(int bytes) {
  Serial.print("I2C Received: ");
  Serial.print(bytes);
  Serial.println(" bytes");

  if (bytes == 1 && Wire.available()) {
    uint8_t data = Wire.read();
    if (data == 1) {
      lastSignalTime = millis();
      masterAlive = true;
      isBackupMaster = false;
      newSlaveData = false;
      bufferIndex = 0;
      for (int i = 0; i < BUFFER_SIZE; i++) {
        dataBuffer[i] = 0;
      }
      Serial.println("Master signal received");
      return;
    }
  }

  if (bytes >= 5 && bufferIndex == 0) {
    while (Wire.available() && bufferIndex < BUFFER_SIZE) {
      dataBuffer[bufferIndex] = Wire.read();
      Serial.print("Byte ");
      Serial.print(bufferIndex);
      Serial.print(": ");
      Serial.println(dataBuffer[bufferIndex], HEX);
      bufferIndex++;
    }
    if (bufferIndex == BUFFER_SIZE && dataBuffer[0] == 8) {
      newSlaveData = true;
      lastSlaveDataTime = millis();
      Serial.print("Buffer: ");
      for (int i = 0; i < BUFFER_SIZE; i++) {
        Serial.print(dataBuffer[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
    } else {
      Serial.println("Invalid Slave packet");
      bufferIndex = 0;
      newSlaveData = false;
    }
  } else {
    while (Wire.available()) {
      Wire.read();
    }
    bufferIndex = 0;
    newSlaveData = false;
    Serial.println("Discarded unexpected data");
  }
}

void loop() {
  unsigned long currentTime = millis();
  float voltage = 0.0;
  float temperatureC = 0.0;
  float slaveVoltage = lastSlaveVoltage;
  float slaveTemp = lastSlaveTemp;

  // Check if Master is alive
  if (currentTime - lastSignalTime > 3000 && !isBackupMaster) {
    masterAlive = false;
    isBackupMaster = true;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Backup Master ON");
    delay(500);
    lastUpdateTime = 0;
  }

  // Read local sensors (always for LCD)
  int rawTempSum = 0;
  for (int i = 0; i < 10; i++) {
    rawTempSum += analogRead(tempPin);
    delay(5);
    
  }
  int rawVoltageSum = 0;
for (int i = 0; i < 10; i++) {
  rawVoltageSum += analogRead(voltagePin);
  delay(2);
}
  float voltageTemp = (rawTempSum / 10.0) * (5.0 / 1023.0);
  temperatureC = voltageTemp * 100;

  int rawVoltage = analogRead(voltagePin);
  voltage = rawVoltage * (5.0 / 1023.0) * (R1 + R2) / R2;
  dataSource = "Backup";

  // Process Slave data for Serial when Backup Master
  if (isBackupMaster) {
    if (newSlaveData) {
      uint16_t temp_voltage_int = (dataBuffer[1] << 8) | dataBuffer[2];
      uint16_t temp_temperature_int = (dataBuffer[3] << 8) | dataBuffer[4];
      slaveVoltage = temp_voltage_int / 100.0;
      slaveTemp = temp_temperature_int / 10.0;

      if (slaveVoltage > 0.0 && slaveVoltage < 10.0 && slaveTemp > 0.0 && slaveTemp < 100.0) {
        lastSlaveVoltage = slaveVoltage;
        lastSlaveTemp = slaveTemp;
        for (int i = 0; i < BUFFER_SIZE; i++) {
          dataBuffer[i] = 0;
        }
      } else {
        Serial.println("Invalid Slave data");
      }
      newSlaveData = false;
    }
    // Use last valid Slave data for Serial
  
  }

  // Scale local data for I2C transmission
  if (voltage < 0) {
    voltage_int = 0;
  } else if (voltage * 100 > 65535) {
    voltage_int = 65535;
  } else {
    voltage_int = (uint16_t)(voltage * 100 + 0.5);
  }
  temperature_int = (uint16_t)(temperatureC * 10 + 0.5);

  // Send data to Master every 1 second
  if (currentTime - lastSendTime >= 1000) {
    int i2cStatus = -1;
    if (masterAlive) { // Only send to Master if alive
      Wire.beginTransmission(0);
      Wire.write(9);
      Wire.write(highByte(voltage_int));
      Wire.write(lowByte(voltage_int));
      Wire.write(highByte(temperature_int));
      Wire.write(lowByte(temperature_int));
      i2cStatus = Wire.endTransmission();
      Serial.print("Sent to Master, Status: ");
      Serial.println(i2cStatus);
    }

    if (isBackupMaster) {
      Wire.beginTransmission(8);
      Wire.write(1);
      i2cStatus = Wire.endTransmission();
      Serial.print("Sent to Slave, Status: ");
      Serial.println(i2cStatus);
    }

    lastSendTime = currentTime;

    // Serial output for Backup data
    Serial.print("Data: Backup, Voltage: ");
    Serial.print(voltage, 2);
    Serial.print(" V, Temp: ");
    Serial.print(temperatureC, 1);
    Serial.println(" C");

    // Serial output for Slave data when Backup Master
    if (isBackupMaster) {
      Serial.print("Data: Slave, Voltage: ");
      Serial.print(slaveVoltage, 2);
      Serial.print(" V, Temp: ");
      Serial.print(slaveTemp, 1);
      Serial.println(" C");
    }
  }

  // Check for overvoltage or overheat (using local data)
  overvoltage = (voltage > voltageThreshold);
  overheat = (temperatureC > tempThreshold);

  // Update LCD every 1 second with Backup data
  if (currentTime - lastUpdateTime >= 1000) {
    if (systemState == HIGH) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Voltage: ");
      lcd.print(voltage, 2);
      lcd.print(" V");
      lcd.setCursor(0, 1);
      lcd.print("System: RUNNING");
      lcd.setCursor(0, 2);
      lcd.print("Temp: ");
      lcd.print(temperatureC, 1);
      lcd.print(" C");
      lcd.setCursor(0, 3);
      lcd.print("Data: Backup");

      if (overvoltage || overheat) {
        digitalWrite(ledPin, HIGH);
        digitalWrite(relayPin, LOW);
        systemState = LOW;
        lcd.setCursor(0, 3);
        if (overvoltage) {
          lcd.print("Overvoltage!");
        } else if (overheat) {
          lcd.print("Overheat!");
        }
      }
    } else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Voltage: ");
      lcd.print(voltage, 2);
      lcd.print(" V");
      lcd.setCursor(0, 1);
      lcd.print("System: STOPPED");
      lcd.setCursor(0, 2);
      lcd.print("Temp: ");
      lcd.print(temperatureC, 1);
      lcd.print(" C");
      lcd.setCursor(0, 3);
      if (overvoltage) {
        lcd.print("Overvoltage!");
      } else if (overheat) {
        lcd.print("Overheat!");
      }
    }
    lastUpdateTime = currentTime;
  }

  // Button handling to reset
  buttonState = digitalRead(buttonPin);
  if (buttonState == LOW && lastButtonState == HIGH) {
    systemState = HIGH;
    digitalWrite(relayPin, HIGH);
    digitalWrite(ledPin, LOW);
    overvoltage = false;
    overheat = false;
    lcd.setCursor(0, 3);
    lcd.print("                ");
    delay(50);
  }
  lastButtonState = buttonState;
  delay(50); // Reduced delay for responsiveness
}