import testToken from "./token-test.js";
import testMessage from "./message-test.js";
import testSocket from "./socket-test.js";
import { statistic } from "../lib/pomelo.js";

function test() {
    let ret = testToken();
    console.log(`Test token: ${ret ? "OK" : "Failed"}`);

    ret = testMessage();
    console.log(`Test message: ${ret ? "OK" : "Failed"}`);

    ret = testSocket();
    console.log(`Test socket: ${ret ? "OK" : "Failed"}`);

    // Check statistic
    console.log(statistic());
}


test();
