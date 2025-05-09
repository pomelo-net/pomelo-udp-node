import http from "node:http";
import { Message, Socket, Token, Plugin } from "../lib/pomelo.js";
import { CHANNEL_MODES, ADDRESS_HOST, ADDRESS_PORT, SERVICE_PORT, SERVICE_HOST } from "./shared.js";


const MAX_CLIENTS = 20;
const TIMEOUT = 5; // seconds
const EXPIRE = 3600; // seconds


async function main() {
    // Load plugin
    const plugin = process.argv[2];
    if (plugin) {
        const result = Plugin.registerPluginByPath(plugin);
        console.log(`Register plugin ${plugin}: ${result ? "OK" : "Failed"}`);
    }

    // Random initial values
    const privateKey = Token.randomBuffer(Token.KEY_BYTES);
    const protocolID = Math.floor(Math.random() * 1000);

    // Create UDP socket
    const socket = new Socket(CHANNEL_MODES);

    socket.setListener({
        onConnected: function(session) {
            console.log(`Session ${session.id} has connected`);
        },

        onDisconnected: function(session) {
            console.log(`Session ${session.id} has disconnected`);
        },

        onReceived: function(session, message) {
            const size = message.size();

            console.log(`Session ${session.id} has sent a message`);
            console.log(`Message size: ${size}`);

            // Reply with the same data
            const reply = new Message();
            reply.write(message.read(size));
            session.send(0, reply);
        }
    });

    const address = `${ADDRESS_HOST}:${ADDRESS_PORT}`;
    await socket.listen(privateKey, protocolID, MAX_CLIENTS, address);
    console.log(`Socket is listening on ${address}`);

    // Variables for generating client tokens
    let clientID = 1;

    // Create HTTP server
    const server = http.createServer(function(_req, res) {
        console.log(`Generating token for client: ${clientID}`);

        // Generate new token here and response it as base64
        const time = Date.now();
        const token = Token.encode(
            privateKey,
            protocolID,
            time,
            time + EXPIRE * 1000,
            Token.randomBuffer(Token.CONNECT_TOKEN_NONCE_BYTES),
            TIMEOUT,
            [ address ],
            Token.randomBuffer(Token.KEY_BYTES), // client key
            Token.randomBuffer(Token.KEY_BYTES), // server key
            clientID,
            new Uint8Array(Token.USER_DATA_BYTES)
        );

        const tokenB64 = Buffer.from(token).toString('base64url');

        res.writeHead(200, {
            'Content-Type': 'text/plain',
            'Content-Length': tokenB64.length,
            'Connection': 'close',
            'Access-Control-Allow-Origin': '*'
        });

        res.end(tokenB64);

        // Update values
        clientID++;
    });

    server.listen(SERVICE_PORT, SERVICE_HOST, () => {
        console.log(
            `HTTP server is started: http://${SERVICE_HOST}:${SERVICE_PORT}`
        );
    });
}


main().catch(err => {
    console.error(err);
    process.exit(-1);
});
