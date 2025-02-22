import testToken from "./token-test.js";
import testMessage from "./message-test.js";
import testSocket from "./socket-test.js";

function test() {
    let ret;

    ret = testToken();
    console.log(`Test token: ${ret ? "OK" : "Failed"}`);

    ret = testMessage();
    console.log(`Test message: ${ret ? "OK" : "Failed"}`);

    ret = testSocket();
    console.log(`Test socket: ${ret ? "OK" : "Failed"}`);
}


test();
