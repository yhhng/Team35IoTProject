const express = require('express');
const firebaseAdmin = require('firebase-admin');
const WebSocket = require('ws');
const path = require('path');
const twilio = require('twilio');

const app = express();
const port = 3000;

// Save the server start time
const serverStartTime = new Date();

// Firebase Admin SDK setup
firebaseAdmin.initializeApp({
    credential: firebaseAdmin.credential.cert('./service-account-file.json'),
    databaseURL: 'https://iot-project-fall-detection-default-rtdb.firebaseio.com/'
});
const db = firebaseAdmin.database();

// Twilio setup
const accountSid = 'AC294f15d994e80d7858eb677e9182b6f4';
const authToken = '41b6b683d77d572a5c5fdb09790c1663';
const twilioClient = new twilio(accountSid, authToken);
const twilioNumber = '+17853926561';
const recipientNumber = '<+6597973374>'; 

// Serve static files from 'public' folder
app.use(express.static(path.join(__dirname, 'public')));

// Route to serve floorplan.html
app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, 'public', 'floorplan.html'));
});

// Route to serve history.html
app.get('/history', (req, res) => {
    res.sendFile(path.join(__dirname, 'public', 'history.html'));
});

// Route to get all fall incidents
app.get('/fall_incidents', async (req, res) => {
    try {
        const snapshot = await db.ref('fall_incidents').once('value');
        const incidents = snapshot.val() || {};
        res.json(incidents);
    } catch (error) {
        console.error('Error fetching fall incidents:', error);
        res.status(500).json({ error: 'Failed to fetch fall incidents' });
    }
});

// WebSocket setup
const wss = new WebSocket.Server({ noServer: true });

wss.on('connection', (ws) => {
    console.log('New client connected');
    ws.send(JSON.stringify({ type: 'info', message: 'Connected to WebSocket server' }));

    // Send initial fall incidents data
    db.ref('fall_incidents').once('value', (snapshot) => {
        const incidents = snapshot.val() || {};
        ws.send(JSON.stringify({ type: 'history', data: incidents }));
    });

    // Listen to current_location changes
    db.ref('current_location').on('value', (snapshot) => {
        const location = snapshot.val();
        console.log('Current Location:', location);

        const payload = JSON.stringify({ type: 'location', data: location });

        wss.clients.forEach(client => {
            if (client.readyState === WebSocket.OPEN) {
                client.send(payload);
            }
        });
    });

    ws.on('message', (message) => {
        console.log('Received from client:', message);
    });

    ws.on('close', () => {
        console.log('Client disconnected');
    });
});

// Listen to current_location changes
db.ref('current_location').on('value', (snapshot) => {
    const location = snapshot.val();
    console.log('Current Location:', location);

    const payload = JSON.stringify({ type: 'location', data: location });

    wss.clients.forEach(client => {
        if (client.readyState === WebSocket.OPEN) {
            client.send(payload);
        }
    });
});

// Listen to fall_incidents changes and broadcast to all clients
db.ref('fall_incidents').on('value', (snapshot) => {
    const incidents = snapshot.val() || {};
    const payload = JSON.stringify({ type: 'history', data: incidents });

    wss.clients.forEach(client => {
        if (client.readyState === WebSocket.OPEN) {
            client.send(payload);
        }
    });
});

// Listen to new fall_incidents and send Twilio SMS only for NEW entries
db.ref('fall_incidents').on('child_added', (snapshot) => {
    const data = snapshot.val();
    const incidentTime = new Date(data.timestamp); 

    const payload = JSON.stringify({
        type: 'fall',
        data: {
            location: data.location,
            timestamp: data.timestamp
        }
    });

    console.log('Fall Detected:', payload);

    // Broadcast to WebSocket clients
    wss.clients.forEach(client => {
        if (client.readyState === WebSocket.OPEN) {
            client.send(payload);
        }
    });

    // Only send SMS if fall was detected AFTER the server started
    if (incidentTime > serverStartTime) {
        const messageBody = `⚠️ Fall detected at ${data.location} on ${data.timestamp}`;
        twilioClient.messages
            .create({
                body: messageBody,
                from: twilioNumber,
                to: recipientNumber
            })
            .then(message => console.log('✅ Twilio SMS sent, SID:', message.sid))
            .catch(err => console.error('❌ Error sending SMS:', err));
    } else {
        console.log('⏳ Skipped old fall entry (before server started)');
    }
});

// Start HTTP + WebSocket server
app.server = app.listen(port, '0.0.0.0', () => {
    console.log(`Server running on http://localhost:${port}`);
});

app.server.on('upgrade', (request, socket, head) => {
    wss.handleUpgrade(request, socket, head, (ws) => {
        wss.emit('connection', ws, request);
    });
});
