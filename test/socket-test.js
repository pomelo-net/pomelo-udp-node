import { Token, Socket, Message, ChannelMode } from "../lib/pomelo.js";


let client = null; // The client
let server = null; // The server
let token = null; // Connect token
let privateKey = null; // Private key

/// The connect address
const ADDRESS = "127.0.0.1:8888";
const CHANNELS = [
    ChannelMode.RELIABLE,
    ChannelMode.SEQUENCED,
    ChannelMode.UNRELIABLE
];
const PROTOCOL_ID = 128;
const MAX_CLIENTS = 20;
const CLIENT_ID = 123;
const TIMEOUT = 1; // seconds

const clientListener = {
    onConnected: function(session) {
        try {
            console.log(`Client session has connected ${session.id}`);
            session.channels.forEach(channel => {
                console.log(`Channel mode: ${channel.mode}`);
            });

            // send a message from client to server
            const message = new Message();
            message.writeInt32(25);
            message.writeFloat64(1.2);
            message.writeFloat64(0.5);
            message.writeInt8(1);
            session.send(0, message); // First channel is reliable
        } catch (ex) {
            console.error(ex);
        }
    },

    onDisconnected: function(session) {
        try {
            console.log(`Client session has disconnected ${session.id}`);
        } catch (err) {
            console.error(err);
        }
    },


    onReceived: function(session, message) {
        console.log(`Client session has received a message ${session.id}`);
    }
};

let stopped = false;

const serverListener = {
    onConnected: function(session) {
        console.log(`Server session has connected ${session.id}`);
    },

    onDisconnected: function(session) {
        try {
            console.log(`Server session has disconnected ${session.id}`);
        } catch (err) {
            console.error(err);
        }
    },

    onReceived: function(session, message) {
        console.log(`Server session has received a message ${session.id}`);
        const v0 = message.readInt32();
        console.log(`v0 = ${v0} ${v0 === 25 ? "OK" : "Failed"}`);

        const v1 = message.readFloat64();
        console.log(`v1 = ${v1} ${v1 === 1.2 ? "OK" : "Failed"}`);
        
        const v2 = message.readFloat64();
        console.log(`v2 = ${v2} ${v2 === 0.5 ? "OK" : "Failed"}`);

        const v3 = message.readInt8();
        console.log(`v3 = ${v3} ${v3 === 1 ? "OK" : "Failed"}`);

        if (stopped) {
            return;
        }

        stopped = true;

        console.log("Stopping sockets");
        client.stop();
        server.stop();
    }
};


export default function testSocket() {
    // Create connect token first
    createConnectToken();

    // Create sockets
    client = new Socket(CHANNELS);
    client.setListener(clientListener);

    server = new Socket(CHANNELS);
    server.setListener(serverListener);

    server.listen(
        privateKey,
        PROTOCOL_ID,
        MAX_CLIENTS,
        ADDRESS
    ).then(() => {
        console.log("Server is listening");
    }).catch((err) => {
        console.error("Failed to listen: ", err);
    });

    client.connect(token).then((ret) => {
        console.log("Connect result: ", ret);
    }).catch((err) => {
        console.error("Failed to connect: ", err);
    });

    return true;
}


function createConnectToken() {
    const privateKeyArray = new Array(Token.KEY_BYTES);
    const serverToClientKeyArray = new Array(Token.KEY_BYTES);
    const clientToServerKeyArray = new Array(Token.KEY_BYTES);
    const connectTokenNonceArray = new Array(Token.CONNECT_TOKEN_NONCE_BYTES);
    const userData = new Array(Token.USER_DATA_BYTES);
    userData.fill(0);

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
        Date.now() + 3600 * 1000000000,
        Uint8Array.from(connectTokenNonceArray),
        TIMEOUT,
        [ ADDRESS ],
        Uint8Array.from(clientToServerKeyArray),
        Uint8Array.from(serverToClientKeyArray),
        CLIENT_ID,
        Uint8Array.from(userData)
    );
}
