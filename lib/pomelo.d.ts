/**
 * The delivery mode of message sending
 */
export enum ChannelMode {
    UNRELIABLE,
    SEQUENCED,
    RELIABLE
}

/**
 * The connect result (For client only)
 */
export enum ConnectResult {
    SUCCESS,
    DENIED,
    TIMED_OUT
}

/**
 * Message
 */
export class Message {
    /**
     * Get the size of message
     */
    size(): number;

    /**
     * Write data to the buffer
     * @param value The uint8 typed array to write
     */
    write(value: Uint8Array): void;

    /**
     * Write Uint8 value to buffer
     * @param value Value to write
     */
    writeUint8(value: number | bigint): void;

    /**
     * Write Uint16 value to buffer
     * @param value Value to write
     */
    writeUint16(value: number | bigint): void;

    /**
     * Write Uint32 value to buffer
     * @param value Value to write
     */
    writeUint32(value: number | bigint): void;

    /**
     * Write Uint64 value to buffer
     * @param value Value to write
     */
    writeUint64(value: number | bigint): void;

    /**
     * Write Int8 value to buffer
     * @param value Value to write
     */
    writeInt8(value: number | bigint): void;

    /**
     * Write Int16 value to buffer
     * @param value Value to write
     */
    writeInt16(value: number | bigint): void;

    /**
     * Write Int32 value to buffer
     * @param value Value to write
     */
    writeInt32(value: number | bigint): void;

    /**
     * Write Int64 to buffer
     * @param value Value to write
     */
    writeInt64(value: number | bigint): void

    /**
     * Write Float32 to buffer
     * @param value 
     */
    writeFloat32(value: number | bigint): void;

    /**
     * Write Float64 to buffer
     * @param value Value to write
     */
    writeFloat64(value: number | bigint): void;

    /**
     * Read the message with specific length
     * @param length Length to read
     */
    read(length: number | bigint): Uint8Array;

    /**
     * Read Uint8 value from buffer
     * @returns Retrieved value from buffer
     */
    readUint8(): number;

    /**
     * Read Uint16 value from buffer
     * @returns Retrieved value from buffer
     */
    readUint16(): number;

    /**
     * Read Uint32 value from buffer
     * @returns Retrieved value from buffer
     */
    readUint32(): number;

    /**
     * Read Uint64 value from buffer
     * @returns Retrieved value from buffer
     */
    readUint64(): bigint;

    /**
     * Read Int8 value from buffer
     * @returns Retrieved value from buffer
     */
    readInt8(): number;

    /**
     * Read Int16 value from buffer
     * @returns Retrieved value from buffer
     */
    readInt16(): number;

    /**
     * Read Int32 value from buffer
     * @returns Retrieved value from buffer
     */
    readInt32(): number;

    /**
     * Read Int64 from buffer
     * @returns Retrieved value from buffer
     */
    readInt64(): bigint;

    /**
     * Read the Float32 from buffer
     * @returns Retrieved value from buffer
     */
    readFloat32(): number;

    /**
     * Read Float64 from buffer
     * @returns Retrieved value from buffer
     */
    readFloat64(): number;
}


/**
 * Round trip time
 */
export interface RTT {
    /**
     * The mean of round trip time
     */
    mean: bigint;

    /**
     * The variance of round trip time
     */
    variance: bigint;
}


/**
 * The specific channel of a session
 */
export interface Channel {
    /**
     * The channel mode
     */
    mode: ChannelMode;

    /**
     * Send message by specific channel
     * @param message The message to send
     * @returns Returns a promise which will resolve to a number value
     * indicating the number of sent messages.
     */
    send(message: Message): Promise<number>;
}


/**
 * The connected session
 */
export interface Session {
    /**
     * The session ID
     */
    readonly id: bigint;

    /**
     * The channels of session
     */
    readonly channels: Channel[];

    /**
     * The custom data for session
     */
    data?: any;

    /**
     * Send message to the peer connected by this session
     * @param channelIndex The channel to send
     * @param message The message to send
     * @returns Returns a promise which will resolve to a number value
     * indicating the number of sent messages.
     */
    send(channelIndex: number, message: Message): Promise<number>;

    /**
     * Set mode for specific channel of a session
     * This is equivalent to getting channel and setting channel mode.
     * @param channelIndex Index of channel
     * @param mode Channel mode
     * @returns True on success, false on failure
     */
    setChannelMode(channelIndex: number, mode: ChannelMode): boolean;

    /**
     * Get the channel mode of a channel by index
     * @param channelIndex Channel index
     * @returns Mode of channel
     */
    getChannelMode(channelIndex: number): ChannelMode;
    
    /**
     * Disconnect this session.
     * @returns Returns false if session has been disconnected before
     */
    disconnect(): boolean;

    /**
     * Get the round trip time information of session
     */
    rtt(): RTT;
}


/**
 * The socket listener
 */
export interface SocketListener {
    /**
     * Peer connected callback.
     * @param session Created session
     */
    onConnected(session: Session): void;

    /**
     * Peer disconected callback
     * @param session 
     */
    onDisconnected(session: Session): void;

    /**
     * This callback is called when a message has been arrived to this peer.
     * The message WILL BE INVALID after this callback.
     * @param session The sender
     * @param message The incoming message
     */
    onReceived(session: Session, message: Message): void;
}


/**
 * Statistic
 */
export interface Statistic {
    /**
     * The allocator statistic
     */
    allocator: {
        /**
         * The number of allocated bytes
         */
        allocatedBytes: BigInt
    }

    /**
     * The API module statistic
     */
    api: {
        /**
         * The number of active messages
         */
        messages: number;

        /**
         * The number of active builtin sessions
         */
        builtinSessions: number;

        /**
         * The number of active plugin sessions
         */
        pluginSessions: number;

        /**
         * The number of active builtin channels
         */
        builtinChannels: number;

        /**
         * The number of active plugin channels
         */
        pluginChannels: number;

        /**
         * The number of active session disconnecting requests
         */
        sessionDisconnectRequests: number;
    }

    /**
     * The data context statistic
     */
    buffer: {
        /**
         * The number of active buffers
         */
        buffers: number;
    }
    
    /**
     * The platform module statistic
     */
    platform: {
        /**
         * The number of scheduled timers
         */
        timers: number;

        /**
         * The number of scheduled works
         */
        workerTasks: number;

        /**
         * The number of active work groups
         */
        workersGroups: number;

        /**
         * The number of active deferred works
         */
        deferredTasks: number;

        /**
         * The number of active sending commands
         */
        sendCommands: number;

        /**
         * The number of bytes which are sent by platform
         */
        sentBytes: BigInt;

        /**
         * The number of bytes which are recv by platform
         */
        recvBytes: BigInt;
    }

    /**
     * The protocol module statistic
     */
    protocol: {
        /**
         * The number of active senders
         */
        senders: number;

        /**
         * The number of active receivers
         */
        receivers: number;

        /**
         * The number of active packets
         */
        packets: number;

        /**
         * The number of active peers
         */
        peers: number;

        /**
         * The number of active servers
         */
        servers: number;

        /**
         * The number of active clients
         */
        clients: number;

        /**
         * The number of active crypto contexts
         */
        cryptoContexts: number;
        
        /**
         * The number of accepted connections
         */
        acceptances: number;

        /**
         * The number of peer disconnect requests
         */
        peerDisconnectRequests: number;
    }

    /**
     * The delivery module statistic
     */
    delivery: {
        /**
         * The number of active dispatchers
         */
        dispatchers: number;

        /**
         * The number of active senders
         */
        senders: number;

        /**
         * The number of active receivers
         */
        receivers: number;

        /**
         * The number of active transporters
         */
        transporters: number;

        /**
         * The number of active endpoints
         */
        endpoints: number;

        /**
         * The number of active buses
         */
        buses: number;

        /**
         * The number of active recv records
         */
        recvRecords: number;

        /**
         * The number of active send results
         */
        sendResults: number;

        /**
         * The number of active send records
         */
        sendRecords: number;

        /**
         * The number of active parcels
         */
        parcels: number;
    },

    /**
     * The binding informations
     */
    binding: {
        /**
         * The number of binding messages
         */
        messages: number;

        /**
         * The number of binding sockets
         */
        sockets: number;

        /**
         * The number of binding sessions
         */
        sessions: number;

        /**
         * The size of message pool
         */
        messagePoolSize: number;

        /**
         * The size of channels array pool
         */
        channelsArrayPoolSize: number;
    }
}


/**
 * The socket.
 * Passing listener of socket is not so convinient
 */
export class Socket {
    /**
     * Create new socket
     * @param channelModes Initial channel modes
     */
    constructor(channelModes: ChannelMode[]);

    /**
     * Set the socket listener
     * @param listener Socket listener
     */
    setListener(listener: SocketListener): void;

    /**
     * Start the socket as server
     * @param privateKey The private key
     * @param protocolID The protocol ID
     * @param maxClients The maximum number of clients
     * @param address The bind address
     * @returns Returns a promise which will resolve if socket starts listening
     * successfully, or reject on error.
     */
    listen(
        privateKey: Uint8Array,
        protocolID: number | bigint,
        maxClients: number,
        address: string
    ): Promise<void>;

    /**
     * Start the socket as a client.
     * @param connectToken The connect token. If a string is passed, it must be
     * a base64 representation of binary connect token.
     * @returns Return a project which will resolve the connection result and
     * reject on error
     */
    connect(connectToken: Uint8Array | string): Promise<ConnectResult>;

    /**
     * Stop the socket
     */
    stop(): void;

    /**
     * Send a message to multiple recipients.
     * @param channelIndex The sending channel index
     * @param message The message
     * @param recipients List of recipients
     * @returns Returns a promise which will resolve to the number of sent
     * messages
     */
    send(
        channelIndex: number,
        message: Message,
        recipients: Session[]
    ): Promise<number>;

    /**
     * Get synchronized socket time
     */
    time(): bigint;
}


/**
 * The token namespace
 */
export namespace Token {
    /**
     * Encode the connect token
     * @param privateKey The private key
     * @param protocolID 64 bit value unique to this particular game/application
     * @param createTimestamp 64 bit unix timestamp when this connect token was created
     * @param expireTimestamp 64 bit unix timestamp when this connect token expires
     * @param connectTokenNonce (24 bytes) The nonce of private connect token
     * @param timeout timeout in seconds. negative values disable timeout (dev only)
     * @param addresses Array of server addresses in strings
     * @param clientToServerKey The key for sending data from client to server
     * @param serverToClientKey The key for sending data from server to client
     * @param clientID globally unique identifier for an authenticated client
     * @param userData The custom user data
     */
    function encode(
        privateKey: Uint8Array,
        protocolID: number | bigint,
        createTimestamp: number | bigint,
        expireTimestamp: number | bigint,
        connectTokenNonce: Uint8Array,
        timeout: number,
        addresses: string[],
        clientToServerKey: Uint8Array,
        serverToClientKey: Uint8Array,
        clientID: number | bigint,
        userData: Uint8Array
    ): Uint8Array;

    /**
     * Random a buffer
     * @param length Length of output buffer
     */
    function randomBuffer(length: number | bigint): Uint8Array;

    /**
     * The number of bytes of pomelo keys
     */
    const KEY_BYTES: number;

    /**
     * The number of bytes of token nonce
     */
    const CONNECT_TOKEN_NONCE_BYTES: number;

    /**
     * The number of bytes of user data
     */
    const USER_DATA_BYTES: number;
}


/**
 * Plugin manager
 */
export namespace Plugin {
    /**
     * Register plugin by name
     * @param name Name of plugin
     * @returns True if success or false if failure
     */
    function registerPluginByName(name: string): boolean;

    /**
     * Register plugin by path
     * @param path Path to plugin
     * @returns True if success or false if failure
     */
    function registerPluginByPath(path: string): boolean;
}


/**
 * Get the statistic of pomelo
 */
export function statistic(): Statistic;

/**
 * Set the error handler
 * @param {function} handler 
 */
export function setErrorHandler(handler: (error: Error) => void): void;
