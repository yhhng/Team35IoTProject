const ws = new WebSocket(`ws://${window.location.host}`);

ws.onmessage = (event) => {
    const location = event.data;
    console.log('Received location:', location);
    highlightRoom(location);
};

function highlightRoom(location) {
    document.querySelectorAll('.room').forEach(room => {
        room.classList.remove('active');
    });

    const activeRoom = document.getElementById(location);
    if (activeRoom) {
        activeRoom.classList.add('active');
    }
}
