import { Token } from "../lib/pomelo.js";

/**
 * Test token
 * @returns {boolean}
 */
export default function testToken() {
    const privateKeyArray = new Array(Token.KEY_BYTES);
    const serverToClientKeyArray = new Array(Token.KEY_BYTES);
    const clientToServerKeyArray = new Array(Token.KEY_BYTES);
    const connectTokenNonceArray = new Array(Token.CONNECT_TOKEN_NONCE_BYTES);
    const userDataArray = new Array(Token.USER_DATA_BYTES);
    userDataArray.fill(0);

    for (let i = 0; i < Token.KEY_BYTES; i++) {
        privateKeyArray[i] = i;
        clientToServerKeyArray[i] = (i * 2) % 128;
        serverToClientKeyArray[i] = (i * 3) % 128;

        if (i < 24) {
            connectTokenNonceArray[i] = (i * 4) % 128;
        }
    }

    const privateKey = Uint8Array.from(privateKeyArray);
    
    const token = Token.encode(
        privateKey,
        1,
        Date.now(),
        Date.now() + 3600 * 1000000000,
        Uint8Array.from(connectTokenNonceArray),
        10,
        [ "127.0.0.1:8888" ],
        Uint8Array.from(clientToServerKeyArray),
        Uint8Array.from(serverToClientKeyArray),
        123,
        Uint8Array.from(userDataArray)
    );

    return token != null;
}
