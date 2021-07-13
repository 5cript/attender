import ControlPrinter from './common/ctrl_message';
import WebSocket from 'ws';

const printer = new ControlPrinter();

const port = 0;
const server = new WebSocket.Server({ port: port });
printer.sendVariable("port", server.address().port);

server.on('connection', (client) => {
    client.on('message', (message) => {
        printer.sendVariable("recv", message);
        client.send(message);
    });
});

process.on('SIGTERM', () => {
    server.close();
});

process.on('exit', () => {
})

console.log("unrelated");