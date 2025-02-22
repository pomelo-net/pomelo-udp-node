import { Message } from "../lib/pomelo.js"


/**
 * Test message
 * @returns {boolean}
 */
export default function testMessage() {
    const message = new Message();
    let valid = false;
    try {
        // This message must be read-only
        // This should throw an error
        message.readUint8();
    } catch (err) {
        valid = true;
    }

    if (!valid) {
        return false;
    }

    // Test writing message
    // These should not throw errors
    message.writeUint8(1);
    message.writeUint16(2n);
    message.writeUint32(3);
    message.writeUint64(4n);
    message.writeInt8(5);
    message.writeInt16(6n);
    message.writeInt32(7);
    message.writeInt64(8n);
    message.writeFloat32(0.12);
    message.writeFloat64(123.456);

    return true;
}
