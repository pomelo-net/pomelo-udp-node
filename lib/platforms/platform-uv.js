import bindings from "bindings";

/**
 * Node.js-specific platform implementation using libuv.
 * 
 * Note: This implementation is specific to Node.js and is not compatible with
 * Bun due to the lack of libuv implementation in Bun.
 */

/**
 * @type {Object | null}
 */
let platform = null;

/**
 * @returns {Object} The platform
 */
export function initPlatformUV() {
    if (platform) {
        return platform;
    }

    const initializer = bindings({
        bindings: "pomelo-udp-node-platform-uv",
        module_root: process.env.POMELO_MODULE_ROOT
    });
    if (!initializer || typeof initializer !== 'function') {
        throw new Error('Failed to initialize native platform module');
    }

    platform = initializer();
    if (!platform) {
        throw new Error('Failed to initialize native platform module');
    }

    return platform;
};
