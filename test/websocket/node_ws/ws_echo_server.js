import ControlPrinter from './common/ctrl_message';
import WebSocket from 'ws';
import {createServer} from 'https';

const printer = new ControlPrinter();

const secure = process.argv.findIndex(elem => elem === "--secure=true") != -1;

const fixCert = (cert, data) => {
    let split = data.split('\n');
    split = split.map(element => {
        return element.trim();
    });
    let res;
    if (cert)
    {
        res = '-----BEGIN CERTIFICATE-----\n' + split.join('') + '\n-----END CERTIFICATE-----';
    }
    else
    {
        res = '-----BEGIN RSA PRIVATE KEY-----\n' + split.join('') + '\n-----END RSA PRIVATE KEY-----';
    }
    return res;
}

let ws;
let http;
if (!secure) {
    const port = 0;
    ws = new WebSocket.Server({ port: port });
} else {
    http = createServer({
        cert: fixCert(true, `
        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`),
        
        key: fixCert(false, `
        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`)
    });
    
    ws = new WebSocket.Server({ server: http });
    http.listen(0);
}

printer.sendVariable("port", ws.address().port);

ws.on('connection', (client) => {
    client.on('message', (message) => {
        printer.sendVariable("recv", message);
        client.send(message);
    });
});

ws.on('error', (error) => {
})

ws.on('SIGTERM', () => {
    server.close();
});

ws.on('exit', () => {
})