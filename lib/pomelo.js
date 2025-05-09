import bindings from "bindings";
import { initPlatformNAPI } from "./platforms/platform-napi.js";
import { initPlatformUV } from "./platforms/platform-uv.js";


/**
 * Create the platform
 * @returns {any} The platform
 */
function createPlatform() {
    if (process.versions.bun) {
        // We use the napi bindings for bun
        return initPlatformNAPI();
    } else {
        // We use the uv bindings for node
        return initPlatformUV();
    }
}


const DEFAULT_POOL_SOCKET_MAX = 10;
const DEFAULT_POOL_MESSAGE_MAX = 1000;
const DEFAULT_POOL_SESSION_MAX = 50;
const DEFAULT_POOL_CHANNEL_MAX = 200;


/**
 * Error handler
 * @param {Error} error 
 */
let errorHandler = (error) => console.error(error);

// Create the platform
const platform = createPlatform();
const initializer = bindings({
    bindings: "pomelo-udp-node",
    module_root: process.env.POMELO_MODULE_ROOT
});

// Initialize pomelo binding module
const pomelo = initializer({
    poolMessageMax: envOrDefault(
        process.env.POMELO_POOL_MESSAGE_MAX,
        DEFAULT_POOL_MESSAGE_MAX
    ),

    poolSocketMax: envOrDefault(
        process.env.POMELO_POOL_SOCKET_MAX,
        DEFAULT_POOL_SOCKET_MAX
    ),

    poolSessionMax: envOrDefault(
        process.env.POMELO_POOL_SESSION_MAX,
        DEFAULT_POOL_SESSION_MAX
    ),

    poolChannelMax: envOrDefault(
        process.env.POMELO_POOL_CHANNEL_MAX,
        DEFAULT_POOL_CHANNEL_MAX
    ),

    /**
     * Error handler
     * @param {Error} error 
     * @returns 
     */
    errorHandler: (error) => errorHandler(error),

    platform: platform
});


export const ChannelMode = pomelo.ChannelMode;
export const ConnectResult = pomelo.ConnectResult;
export const Message = pomelo.Message;
export const Socket = pomelo.Socket;
export const Plugin = pomelo.Plugin;
export const Token = pomelo.Token;
export const statistic = pomelo.statistic;


/**
 * Set the error handler
 * @param {(error: Error) => void} handler 
 */
export function setErrorHandler(handler) {
    errorHandler = handler;
}


/**
 * @param {string | undefined} envValue
 * @param {number} defaultValue
 * @returns {number} Parsed value
 */
function envOrDefault(envValue, defaultValue) {
    return (envValue && parseInt(envValue)) || defaultValue;
}
