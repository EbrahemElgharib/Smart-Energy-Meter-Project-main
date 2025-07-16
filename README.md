
Smart Energy Meter Project

This project simulates a Smart Energy Meter System using Proteus for circuit design and Virtuino for real-time data monitoring. The goal is to measure electrical current and voltage, calculate power consumption, and log the data into an Excel sheet for further analysis and record-keeping.

✅ Features

⚡ Measures voltage, current, and power in real time

📲 Sends data from Arduino to Virtuino via serial communication (or virtual COM)

📊 Logs live data to Excel using a serial-to-Excel interface

🔁 Can be extended to support energy cost calculations, overload protection, and relay control

📐 Fully simulated using Proteus (no physical hardware required)

🛠 Technologies Used
🔌 Proteus: Circuit design and simulation

🤖 Arduino UNO: Main controller for sensing and processing

📱 Virtuino: Visual interface to display live data on PC or Android

📈 Excel Logging: Using PLX-DAQ or custom Python script for serial-to-Excel communication

📁 Project Components

Arduino Code (.ino) for reading sensor data (voltage, current)

Proteus simulation file (.pdsprj)

Virtuino setup (.csv or screen config, if applicable)

Excel data logger interface

🚀 How It Works

Simulate circuit in Proteus with sensors like ACS712 (current) and voltage dividers.

Arduino calculates real-time values of voltage, current, and power.

Data is sent via serial communication.

Virtuino visualizes the readings.

Optionally, data is logged into Excel using a tool like PLX-DAQ or a Python script.
Screenshots
<img width="1048" height="558" alt="image" src="https://github.com/user-attachments/assets/ebd9d3c8-8313-44d3-ade1-54ffb90af985" />

https://github.com/user-attachments/assets/54d0f891-813d-47d5-8766-d5f9cd5103a8


