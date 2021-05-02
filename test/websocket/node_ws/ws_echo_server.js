const WebSocket = require('ws');

const server = new WebSocket.Server({ port: 45458 });

server.on('connection', (client) => {
    client.on('message', (message) => {
        console.log('received: %s', message);
        client.send(message);
    });
});

process.on('SIGTERM', () => {
    console.info('SIGTERM signal received.');
    server.close();
});

process.on('exit', () => {
    console.log('bye');
})