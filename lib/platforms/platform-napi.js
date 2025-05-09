import { hrtime } from 'node:process';
import dgram from 'node:dgram';
import bindings from "bindings";

/**
 * Cross-platform implementation using Node.js N-API.
 * 
 * The implementation is designed to work across different JavaScript runtimes,
 * including Node.js and Bun, by using standard Node.js APIs and N-API bindings.
 */

const CLIENT_SOCKET_SNDBUF_SIZE = 256 * 1024;
const CLIENT_SOCKET_RCVBUF_SIZE = 256 * 1024;
const SERVER_SOCKET_SNDBUF_SIZE = 4 * 1024 * 1024;
const SERVER_SOCKET_RCVBUF_SIZE = 4 * 1024 * 1024;

const SOCKET_MODE_SERVER = 0;
const SOCKET_MODE_CLIENT = 1;


/**
 * UDP socket class
 */
class UDPSocket {
    /** 
     * @param {string} host The host
     * @param {number} port The port
     * @param {Function} recvCallback The receive callback
     * @param {Function} sendCallback The send callback
     */
    constructor(host, port, recvCallback, sendCallback) {
        this.host = host;
        this.port = port;
        this.recvCallback = recvCallback;
        this.sendCallback = sendCallback;
        this.running = false;
    }


    /**
     * Create the socket
     * @param {number} mode The mode
     * @returns {dgram.Socket} The socket
     */
    createSocket(mode) {
        if (this.socket) {
            return this.socket;
        }

        this.mode = mode;
        this.socket = dgram.createSocket({
            type: this.host.indexOf(':') === -1 ? 'udp4' : 'udp6',
            reuseAddr: true,
            recvBufferSize: mode === SOCKET_MODE_SERVER
                ? SERVER_SOCKET_RCVBUF_SIZE
                : CLIENT_SOCKET_RCVBUF_SIZE,
            sendBufferSize: mode === SOCKET_MODE_SERVER
                ? SERVER_SOCKET_SNDBUF_SIZE
                : CLIENT_SOCKET_SNDBUF_SIZE,
        });

        this.socket.on('message', (message, remote) => {
            this.recvCallback(this, message, remote.address, remote.port);
        });

        return this.socket;
    }


    /**
     * Bind the socket to the host and port
     * @return {boolean} True if the socket is bound, false otherwise
     */
    bind() {
        if (this.socket) {
            return false;
        }

        const socket = this.createSocket(SOCKET_MODE_SERVER);
        socket.bind(this.port, this.host);
        this.running = true;
    
        return true;
    }


    /**
     * Connect the socket to the host and port
     * @return {boolean} True if the socket is connected, false otherwise
     */
    connect() {
        if (this.socket) {
            return false;
        }

        const socket = this.createSocket(SOCKET_MODE_CLIENT);
        socket.connect(this.port, this.host);
        this.running = true;

        return true;
    }
    

    /**
     * Stop the socket
     * @return {boolean} True if the socket is stopped, false otherwise
     */
    stop() {
        if (!this.socket) {
            return false;
        }

        this.socket.close();
        this.running = false;
        this.socket = null;
        return true;
    }


    /**
     * Send a message to the socket
     * @param {Buffer[]} messages The messages to send
     * @param {string} host The host
     * @param {number} port The port
     * @param {Object} callbackData The callback native data
     * @param {Object} callbackFunc The callback native function
     * @return {boolean} True if the message is sent, false otherwise
     */
    send(messages, host, port, callbackData, callbackFunc) {
        if (!this.socket) {
            return false;
        }

        if (this.mode === SOCKET_MODE_SERVER) {
            this.socket.send(messages, port, host, (error) => {
                if (!this.running) {
                    return;
                }
                this.sendCallback(callbackFunc, callbackData, error ? -1 : 0);
            });
        } else {
            this.socket.send(messages, (error) => {
                if (!this.running) {
                    return;
                }
                this.sendCallback(callbackFunc, callbackData, error ? -1 : 0);
            });
        }

        return true;
    }
};


/**
 * Timer class
 */
class Timer {
    /**
     * 
     * @param {Function} callback The entry function
     * @param {number} timeoutMS The timeout in milliseconds
     * @param {number} repeatMS The repeat in milliseconds
     */
    constructor(callback, timeoutMS, repeatMS) {
        this.callback = callback;
        this.timeoutMS = timeoutMS;
        this.repeatMS = repeatMS;
        this.timeout = null;
        this.interval = null;
    }

    /**
     * Start the timer
     */
    start() {
        this.stop();
        this.timeout = setTimeout(() => {
            this.callback(this);
            if (this.repeatMS > 0) {
                this.interval = setInterval(() => {
                    this.callback(this);
                }, this.repeatMS);
            }
        }, this.timeoutMS);
    }

    /**
     * Stop the timer
     */
    stop() {
        if (this.timeout !== null) {
            clearTimeout(this.timeout);
            this.timeout = null;
        }
        if (this.interval !== null) {
            clearInterval(this.interval);
            this.interval = null;
        }
    }
};


/**
 * Platform for N-API
 */
const options = {};


/* -------------------------------------------------------------------------- */
/*                            Platform Time APIs                              */
/* -------------------------------------------------------------------------- */

/**
 * Get the high resolution time
 * @returns {bigint} The high resolution time
 */
options.hrtime = () => hrtime.bigint();


/**
 * Get the current time
 * @returns {bigint} The current time
 */
options.now = () => BigInt(Date.now());

/* -------------------------------------------------------------------------- */
/*                             Platform UDP APIs                              */
/* -------------------------------------------------------------------------- */

/**
 * Create a UDP socket
 * @param {string} host The host
 * @param {number} port The port
 * @param {Function} recvCallback The receive callback
 * @param {Function} sendCallback The send callback
 * @returns {UDPSocket | null} The UDP socket object
 */
options.udpCreate = (host, port, recvCallback, sendCallback) => {
    return new UDPSocket(host, port, recvCallback, sendCallback);
};


/**
 * Bind the UDP socket to the host and port
 * @param {UDPSocket} socket The UDP socket object
 * @returns {boolean} True if the socket is bound, false otherwise
 */
options.udpBind = (socket) => {
    return socket.bind();
};


/**
 * Connect the UDP socket to the host and port
 * @param {UDPSocket} socket The UDP socket object
 * @returns {boolean} True if the socket is connected, false otherwise
 */
options.udpConnect = (socket) => {
    return socket.connect();
};


/**
 * Stop the UDP socket
 * @param {UDPSocket} socket The UDP socket object
 * @returns {boolean} True if the socket is stopped, false otherwise
 */
options.udpStop = (socket) => {
    return socket.stop();
};


/**
 * Send a message to the UDP socket
 * @param {UDPSocket} socket The UDP socket object
 * @param {Buffer[]} messages The messages to send
 * @param {string} host The host
 * @param {number} port The port
 * @param {Object} callbackData The callback native data
 * @param {Object} callbackFunc The callback native function
 * @returns {boolean} True if the message is sent, false otherwise
 */
options.udpSend = (
    socket, messages, host, port, callbackData, callbackFunc
) => {
    return socket.send(messages, host, port, callbackData, callbackFunc);
}


/* -------------------------------------------------------------------------- */
/*                            Platform Timer APIs                              */
/* -------------------------------------------------------------------------- */

/**
 * Create a timer
 * @param {Function} callback The callback function
 * @param {number} timeoutMS The timeout in milliseconds
 * @param {number} repeatMS The repeat in milliseconds
 * @returns {Timer} The timer object
 */
options.timerCreate = (callback, timeoutMS, repeatMS) => {
    return new Timer(callback, timeoutMS, repeatMS);
};


/**
 * Start a timer
 * @param {Timer} timer The timer object
 */
options.timerStart = (timer) => {
    timer.start();
};


/**
 * Stop a timer
 * @param {Timer} timer The timer object
 */
options.timerStop = (timer) => {
    timer.stop();
};


/* -------------------------------------------------------------------------- */
/*                            Platform Statistic APIs                           */
/* -------------------------------------------------------------------------- */

/**
 * Get the statistic of the platform
 * @returns {Object} The statistic of the platform
 */
options.statistic = () => {
    return {};
};


/**
 * @type {Object | null}
 */
let platform = null;


/**
 * Initialize the platform
 * @returns {Object} The platform
 */
export function initPlatformNAPI() {
    if (platform) {
        return platform;
    }

    // We use the napi bindings for bun
    const initializer = bindings({
        bindings: "pomelo-udp-node-platform-napi",
        module_root: process.env.POMELO_MODULE_ROOT
    });
    if (!initializer || typeof initializer !== 'function') {
        throw new Error('Failed to initialize native platform module');
    }

    platform = initializer(options);
    if (!platform) {
        throw new Error('Failed to initialize native platform module');
    }

    return platform;
}
