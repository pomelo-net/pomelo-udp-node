// Standalone soak test
import { Token, Socket, Message, ChannelMode } from "../lib/pomelo-udp.js";


let client = null; // The client
let server = null; // The server
let token = null; // Connect token
let privateKey = null; // Private key

/// The connect address
const ADDRESS = "127.0.0.1:8888";
const CHANNELS = [ ChannelMode.RELIABLE ];
const PROTOCOL_ID = 125;
const MAX_CLIENTS = 1;
const CLIENT_ID = 255;
const TIMEOUT = 1; // seconds

const SEND_INTERVAL = 0.1; // seconds
const STATISTIC_INTERVAL = 1.0; // seconds
const SOAK_TEST_DURATION = 1 * 60; // seconds

const clientListener = {
    onConnected: function(session) {
        console.log(`Client session has connected ${session.id}`);
        // send a message from client to server
        this.sendInterval = setInterval(() => {
            const message = Message.acquire();
            message.writeInt32(25);
            message.writeFloat64(1.2);
            message.writeFloat64(0.5);
            message.writeInt8(1);
            session.send(0, message);
        }, SEND_INTERVAL * 1000);

        this.statisticInterval = setInterval(() => {
            const statistic = client.statistic();
            if (statistic.allocatorAllocatedBytes !== this.allocatedBytes) {
                this.allocatedBytes = statistic.allocatorAllocatedBytes;
                console.log(new Date(), "Client: ", statistic);
            }
        }, STATISTIC_INTERVAL * 1000);
    },

    onDisconnected: function(session) {
        console.log(`Client session has disconnected ${session.id}`);
    },

    onReceived: function(session, message) {
        // console.log(`Client session has received a message ${session.id}`);
    },

    onStopped: function() {
        console.log("Client has been stopped");
    }
};

const serverListener = {
    allocatedBytes: 0,

    onConnected: function(session) {
        console.log(`Server session has connected ${session.id}`);
        // send a message from client to server
        this.sendInterval = setInterval(() => {
            const message = Message.acquire();
            message.writeInt32(25);
            message.writeFloat64(1.2);
            message.writeFloat64(0.5);
            message.writeInt8(1);
            session.send(0, message);
        }, SEND_INTERVAL * 1000);

        this.statisticInterval = setInterval(() => {
            const statistic = client.statistic();
            if (statistic.allocatorAllocatedBytes !== this.allocatedBytes) {
                this.allocatedBytes = statistic.allocatorAllocatedBytes;
                console.log(new Date(), "Server: ", statistic);
            }
        }, STATISTIC_INTERVAL * 1000);
    },

    onDisconnected: function(session) {
        console.log(`Server session has disconnected ${session.id}`);
        clearInterval(this.sendInterval);
        clearInterval(this.statisticInterval);
        server.stop();
    },

    onReceived: function(session, message) {
        // console.log(`Server session has received a message ${session.id}`);
    },

    onStopped: function() {
        console.log("Server has been stopped");
    }
};


function main() {
    // Create connect token first
    createConnectToken();

    // Create sockets
    client = new Socket(CHANNELS);
    client.setListener(clientListener);

    server = new Socket(CHANNELS);
    server.setListener(serverListener);

    let ret = server.listen(
        privateKey,
        PROTOCOL_ID,
        MAX_CLIENTS,
        ADDRESS
    );
    if (!ret) {
        console.log("Failed to start server");
        return ret;
    }

    ret = client.connect(token, (result) => {
        console.log("ConnectResult:", result);
    });

    if (!ret) {
        console.log("Connect error");
    }

    setTimeout(() => {
        console.log(new Date(), "Stop testing");
        clearInterval(clientListener.sendInterval);
        clearInterval(clientListener.statisticInterval);
        clearInterval(serverListener.sendInterval);
        clearInterval(serverListener.statisticInterval);
        server.stop();
        client.stop();
    }, SOAK_TEST_DURATION * 1000);

    return true;
}


function createConnectToken() {
    const privateKeyArray = new Array(32);
    const serverToClientKeyArray = new Array(32);
    const clientToServerKeyArray = new Array(32);
    const connectTokenNonceArray = new Array(24);

    for (let i = 0; i < 32; i++) {
        privateKeyArray[i] = i;
        clientToServerKeyArray[i] = (i * 2) % 128;
        serverToClientKeyArray[i] = (i * 3) % 128;

        if (i < 24) {
            connectTokenNonceArray[i] = (i * 4) % 128;
        }
    }

    privateKey = Uint8Array.from(privateKeyArray);
    token = Token.encode(
        privateKey,
        PROTOCOL_ID,
        Date.now(),
        Date.now() + 3600 * 1000,
        Uint8Array.from(connectTokenNonceArray),
        TIMEOUT,
        [ ADDRESS ],
        Uint8Array.from(clientToServerKeyArray),
        Uint8Array.from(serverToClientKeyArray),
        CLIENT_ID,
        Uint8Array.from(new Array(Token.POMELO_USER_DATA_BYTES))
    );
}

main();
