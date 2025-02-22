import bindings from "bindings";


if (process.versions.bun) {
    // Pomelo uses uv APIs which bun has not supported yet.
    throw new Error("Bun has not been supported yet.");
}


const DEFAULT_POOL_MESSAGE_MAX = 1000;
const DEFAULT_POOL_SESSION_MAX = 50;
const DEFAULT_POOL_CHANNEL_MAX = 200;


// Initialize pomelo binding module
const pomelo = bindings("pomelo-udp-node")({
    poolMessageMax: envOrDefault(
        process.env.POMELO_POOL_MESSAGE_MAX,
        DEFAULT_POOL_MESSAGE_MAX
    ),
    poolSessionMax: envOrDefault(
        process.env.POMELO_POOL_SESSION_MAX,
        DEFAULT_POOL_SESSION_MAX
    ),
    poolChannelMax: envOrDefault(
        process.env.POMELO_POOL_CHANNEL_MAX,
        DEFAULT_POOL_CHANNEL_MAX
    ),
    errorHandler: handleError
});


export const ChannelMode = pomelo.ChannelMode;
export const ConnectResult = pomelo.ConnectResult;
export const Message = pomelo.Message;
export const Socket = pomelo.Socket;
export const Plugin = pomelo.Plugin;
export const Token = pomelo.Token;


/**
 * @param {string | undefined} envValue
 * @param {number} defaultValue
 * @returns {number} Parsed value
 */
function envOrDefault(envValue, defaultValue) {
    return (envValue && parseInt(envValue)) || defaultValue;
}

/**
 * Handle error
 * @param {Error} error 
 */
function handleError(error) {
    console.error(error);
}
