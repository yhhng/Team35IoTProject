# IoT Group 35

This project implements a comprehensive IoT-based smart home monitoring system using various sensors and communication protocols. The system is designed to monitor different areas of a home and detect potential falls or emergencies.

## Project Structure

The project is organized into three main components:

### 1. Webpage
A Node.js-based web application that serves as the central monitoring interface.
- `backend.js`: Handles server-side logic and data processing
- `frontend.js`: Manages the user interface and real-time updates
- `public/`: Contains static assets and frontend resources
- `package.json`: Lists project dependencies and scripts

### 2. ESP-IDF
Contains ESP32-based bluetooth mesh network implementations:
- `bluetooth_mesh_sensor_server/`: Server implementation for the mesh network
- `bluetooth_mesh_sensor_client/`: Client implementation for the mesh network

### 3. Arduino
Contains various sensor implementations for different rooms:
- `LivingRoomIRSensor/`: Infrared sensor for living room monitoring
- `BedroomIRSensor/`: Infrared sensor for bedroom monitoring
- `KitchenToiletFallDetector/`: Fall detection system for kitchen toilet area
- `KitchenIRSensor/`: Infrared sensor for kitchen monitoring
- `BedroomToiletFallDetector/`: Fall detection system for bedroom toilet area

### 4. MQTT Broker
Contains the modified version of MQTT broker which uses TLS/PSK Auth and Username/Password Auth. CA Cert Auth was tested but not in use.

## System Architecture

The system consists of:
1. Multiple Arduino-based sensor nodes placed throughout the home
2. ESP32-based bluetooth mesh network for reliable communication
3. Web-based monitoring interface for real-time data visualization

## Getting Started

### Prerequisites
- Node.js and npm (for the web interface)
- Arduino IDE (for sensor programming)
- ESP-IDF (for bluetooth mesh network implementation)

### Installation
1. Web Interface:
   ```bash
   cd Webpage
   npm install
   ```

2. Arduino Sensors:
   - Open each sensor project in the Arduino IDE
   - Install required libraries
   - Upload to respective Arduino boards

3. ESP-IDF Mesh Network:
   - Set up ESP-IDF development environment
   - Select port, build and flash the server and client implementations

## Usage
1. Start the web server:
   ```bash
   cd Webpage
   node backend.js
   ```

2. Access the web interface through your browser
3. Monitor sensor data and alerts in real-time

## P.S.
Due to security reason, Google/Github does not allow to upload the secret key to our firebase aka service-account-file.json. You may drop any of us an email should you require the key file to test run the project.
