import http from "node:http";
import { Message, Socket, Plugin } from "../lib/pomelo.js";
import { ADDRESS_HOST, ADDRESS_PORT, CHANNEL_MODES, SERVICE_HOST, SERVICE_PORT } from "./shared.js";


async function main() {
    const plugin = process.argv[2];
    if (plugin) {
        const result = Plugin.registerPluginByPath(plugin);
        console.log(`Register plugin ${plugin}: ${result ? "OK" : "Failed"}`);
    }

    console.log("Requesting connect token");
    const tokenB64 = await requestConnectToken();
    const token = Buffer.from(tokenB64, 'base64url');
    console.log(`Got connect token ${token.length} bytes`);
    console.log("Channel mode: ", CHANNEL_MODES);
    
    const socket = new Socket(CHANNEL_MODES);
    socket.setListener({
        onConnected: function(session) {
            console.log(`Connected ID = ${session.id}`);

            console.log("Writing message");
            const message = new Message();
            message.writeInt32(1234);
            message.writeUint64(999888);
            const result = session.send(0, message);
            console.log("Send result: " + result);
        },

        onDisconnected: function(session) {
            console.log(`Disonnected ID = ${session.id}`);
        },

        onReceived: function(session, message) {
            console.log(`Received a message ID = ${session.id}`);
            console.log(`Message size: ${message.size()}`);

            const v1 = message.readInt32();
            const v2 = message.readUint64();
            console.log(`Received: ${v1} ${v2}`);

            console.log("Disconnecting from server");
            session.disconnect();
        }
    });

    console.log("Connecting to socket server");
    const result = await socket.connect(token);
    console.log(`Connect result: ${result}`);
}


function requestConnectToken() {
    return new Promise((resolve, reject) => {
        const chunks = [];
        const address = `http://${SERVICE_HOST}:${SERVICE_PORT}`;
        const req = http.request(address, function(res) {
            res.setEncoding('utf8');
            res.on('data', chunk => chunks.push(chunk));
            res.on('end', () => resolve(chunks.join()));
        });

        req.on('error', err => reject(err));
        req.end();
    });
}


main().catch(err => {
    console.error(err);
    process.exit(-1);
});
