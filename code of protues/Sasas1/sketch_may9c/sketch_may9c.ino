#include <Wire.h>
#include <LiquidCrystal.h>

const int voltagePin = A0;    // Voltage measurement pin
const int tempPin = A3;       // LM35 temperature sensor pin
const int buttonPin = A2;     // Button pin (distinct from voltagePin)
const int ledPin = 6;
const int relayPin = 13;
const float voltageThreshold = 2.90; // Voltage threshold: 4.5V (adjust as needed)
const float tempThreshold = 30.0;   // Temperature threshold: 30°C

LiquidCrystal lcd(7, 12, 11, 10, 9, 8);

int systemState = HIGH;
bool buttonState = LOW;
bool lastButtonState = HIGH;
unsigned long lastSignalTime = 0;
unsigned long lastSendTime = 0; // To track last data send time
bool masterAlive = true;
bool overvoltage = false;
bool overheat = false;
uint16_t voltage_int;    // Scaled voltage (voltage * 100)
uint16_t temperature_int; // Scaled temperature (temperature * 10)
const String dataSource = "Slave"; // Always Slave data

void setup() {
  Wire.begin(8);          // I2C address 8 for Slave
  Wire.onReceive(receiveEvent); // Handle received signals
  Serial.begin(9600);

  pinMode(ledPin, OUTPUT);
  pinMode(relayPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(tempPin, INPUT); // Set temperature pin as input
  digitalWrite(relayPin, HIGH); // Start system ON (relay closed)

  lcd.begin(16, 4);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Ready");
  delay(500);
}

void receiveEvent(int bytes) {
  if (Wire.available()) {
    int signal = Wire.read();
    if (signal == 1) {
      lastSignalTime = millis();
      masterAlive = true;
    }
  }
}

void loop() {
  unsigned long currentTime = millis();

  // Check if Master is alive
  if (currentTime - lastSignalTime > 3000) {
    masterAlive = false;
  }

  // Read temperature from LM35 with averaging for stability
  int rawTempSum = 0;
  for (int i = 0; i < 10; i++) {
    rawTempSum += analogRead(tempPin);
    delay(10); // Small delay for sampling
  }
  float voltageTemp = (rawTempSum / 10.0) * (5.0 / 1023.0); // Average voltage
  float temperatureC = voltageTemp * 100; // LM35: 10mV per °C

  // Read voltage
  int rawVoltage = analogRead(voltagePin);
  float voltage = rawVoltage * (5.0 / 1023.0); // Convert to voltage (0-5V range)

  // Scale data for I2C transmission
  if (voltage < 0) {
    voltage_int = 0; // Handle negative voltage
  } else if (voltage * 100 > 65535) {
    voltage_int = 65535; // Handle overflow
  } else {
    voltage_int = (uint16_t)(voltage * 100 + 0.5); // Scale and round
  }
  temperature_int = (uint16_t)(temperatureC * 10 + 0.5); // Scale temperature by 10

  // Send data every 500ms
  if (currentTime - lastSendTime >= 500) {
    if (masterAlive) {
      Wire.beginTransmission(0); // Master address
      Wire.write(8); // Send device ID (8 for Slave)
      Wire.write(highByte(voltage_int));
      Wire.write(lowByte(voltage_int));
      Wire.write(highByte(temperature_int));
      Wire.write(lowByte(temperature_int));
      Wire.endTransmission();
    } else {
      Wire.beginTransmission(9); // Backup Master address
      Wire.write(8); // Send device ID (8 for Slave)
      Wire.write(highByte(voltage_int));
      Wire.write(lowByte(voltage_int));
      Wire.write(highByte(temperature_int));
      Wire.write(lowByte(temperature_int));
      Wire.endTransmission();
    }
    lastSendTime = currentTime;

    // Debug output to Serial monitor
    Serial.print("Data: ");
    Serial.print(dataSource);
    Serial.print(", Voltage: ");
    Serial.print(voltage, 2);
    Serial.print(" V, Temp: ");
    Serial.print(temperatureC, 1);
    Serial.println(" C");
  }

  // Check for overvoltage or overheat
  overvoltage = (voltage > voltageThreshold);
  overheat = (temperatureC > tempThreshold);

  // Update LCD
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
    lcd.print("Data: ");
    lcd.print(dataSource);

    if (overvoltage || overheat) {
      digitalWrite(ledPin, HIGH);
      digitalWrite(relayPin, LOW); // Turn relay OFF
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

  // Button handling to reset
  buttonState = digitalRead(buttonPin);
  if (buttonState == LOW && lastButtonState == HIGH) {
    systemState = HIGH;
    digitalWrite(relayPin, HIGH); // Turn relay ON
    digitalWrite(ledPin, LOW);    // Turn LED OFF
    overvoltage = false;
    overheat = false;
    lcd.setCursor(0, 3);
    lcd.print("                ");
    delay(50);
  }
  lastButtonState = buttonState;

  delay(500);
}