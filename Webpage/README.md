# IoT Fall Detection and Location Monitoring System

This project utilizes a Node.js backend with Firebase Admin SDK to monitor the location of a person based on real-time updates from the Firebase Realtime Database. When a fall is detected, it triggers an SMS notification via the Twilio API. The frontend (HTML) updates a floor plan to highlight the corresponding room based on the detected location.

## Prerequisites

Before running the project, ensure you have the following installed:

- [Node.js](https://nodejs.org/) (version 12 or higher)
- [npm](https://www.npmjs.com/) (Node.js package manager)
- [Twilio account](https://www.twilio.com/) (for SMS notifications)
- Firebase project and credentials for Firebase Admin SDK (File is within the project)

## Installation

1. **Clone the repository** (or download the project files):
    ```bash
    git clone https://github.com/yhhng/iotgrp35webpage.git
    cd iotgrp35webpage
    ```

2.  **Install dependencies:**
    Run the following command to install the necessary Node.js packages (`express`, `firebase-admin`, `twilio`, and `ws`):
    ```bash
    npm install
    ```

## Running the Application

1. **Start the Node.js backend server**:
   In your terminal, run the following command to start the backend server:
   ```bash
   node backend.js
   ```
2. **Open Web Browser**
   ```
   Head to http://localhost:3000
   ```
