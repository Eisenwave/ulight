/** A wrapper class for the loaded `ulight.wasm` library.  */
export class UlightWasm {
    /**
     * @param {WebAssembly.WebAssemblyInstantiatedSource} wasm
    */
    constructor(wasm) {
        this.wasm = wasm;
    }

    /**
     * Returns an array of all available languages,
     * where the `name` field is the (unique) of the language,
     * and `id` is the numeric identifier.
     * 
     * The returned array is ordered lexicographically by `name`.
     * There may be multiple entries with the same `id`.
     * @returns {{name:string;displayName:string;id:number;}[]}
     */
    getLanguageList() {
        const baseAddress = this._exports.ulight_lang_list;
        const displayNamesAddress = this._exports.ulight_lang_display_names;
        const lengthAddress = this._exports.ulight_lang_list_length;

        const heap32 = new Int32Array(this._memory.buffer);
        const length = heap32[lengthAddress / 4];

        const result = [];
        for (let i = 0; i < length; ++i) {
            const entrySize = 12;

            const nameAddress = heap32[(baseAddress + i * entrySize + 0) / 4];
            const nameLength = heap32[(baseAddress + i * entrySize + 4) / 4];
            const name = this._loadUtf8(nameAddress, nameLength);

            const id = heap32[(baseAddress + i * entrySize + 8) / 4];
            const displayNameAddress = heap32[(displayNamesAddress + id * 8) / 4];
            const displayNameLength = heap32[(displayNamesAddress + id * 8 + 4) / 4]
            const displayName = this._loadUtf8(displayNameAddress, displayNameLength);

            result.push({ name, displayName, id });
        }

        return result;
    }

    /**
     * @returns {number} The amount of entries in the list returned by `getLanguageList`.
     * @see getLanguageList 
     */
    getLanguageListLength() {
        const length_address = this._exports.ulight_lang_list_length;
        const heap32 = new Int32Array(this._memory.buffer);
        return heap32[length_address / 4];
    }

    /**
     * Returns the numeric ID of a language by one of its short names, such as `"c++"`.
     * @param {string} name One of the short names of the language. 
     * @returns {number} The numeric ID. The special value `0` represents no valid language.
     */
    getLanguageId(name) {
        let u8string;
        try {
            u8string = this._allocUtf8(name);
            return this._exports.ulight_get_lang(u8string.address, u8string.size);
        } finally {
            this._freeUtf8(u8string);
        }
    }

    /**
     * Returns the display name of a language by one of its short names or its numeric ID.
     * @param {number|string} id The numeric ID or short name.
     * @returns {string|null}
     */
    getLanguageDisplayName(key) {
        if (typeof (key) === "string") {
            const id = this.getLanguageId(key);
            return id === 0 ? null : this.getLanguageDisplayName(id);
        }

        const tableAddress = this._exports.ulight_lang_display_names;
        const heap32 = new Int32Array(this._memory.buffer);
        const address = heap32[(tableAddress + key * 8) / 4];
        const length = heap32[(tableAddress + key * 8 + 4) / 4]

        return this._loadUtf8(address, length);
    }

    /**
     * 
     * @param {Uint8Array|string} source
     * The source code to HTML, either provided as a UTF-8-encoded `Uint8Array`,
     * or as a `string` which is encoded on the fly.
     * @param {number|string} id The language short name or numeric id. 
     * @returns {string} The highlighted HTML.
     */
    toHtml(source, id) {
        const tokenSize = 12; // sizeof(ulight_token)
        const tokenAlign = 4; // alignof(ulight_token)
        const bufferByteSize = 64 * 1024;

        const lang = typeof (id) === "string" ? this.getLanguageId(id) : id;
        const sourceData = typeof (source) === "string" ? new TextEncoder().encode(source) : source;

        this._bufferedText = "";

        let u8source = 0;
        let tokenBuffer = 0;
        let textBuffer = 0;
        let state = 0;
        try {
            u8source = this._allocBytes(sourceData);
            tokenBuffer = this._alloc(bufferByteSize, tokenAlign);
            textBuffer = this._alloc(bufferByteSize, 1);
            state = this._newState();

            const heap32 = new Int32Array(this._memory.buffer);
            heap32[state / 4 + 0] = u8source;
            heap32[state / 4 + 1] = sourceData.length;
            heap32[state / 4 + 2] = lang;
            heap32[state / 4 + 3] = 0; // TODO: flags
            heap32[state / 4 + 4] = tokenBuffer;
            heap32[state / 4 + 5] = bufferByteSize / tokenSize;
            // 6: flush_tokens_data is added automatically
            // 7: flush_tokens is added automatically
            // 8: html_tag_name stays defaulted
            // 9: html_tag_name_length stays defaulted
            // 10: html_attr_name stays defaulted
            // 11: html_attr_name_length stays defaulted
            heap32[state / 4 + 12] = textBuffer;
            heap32[state / 4 + 13] = bufferByteSize;
            // no flush_text_data necessary
            heap32[state / 4 + 15] = this._flushTextIndex;

            const result = this._exports.ulight_source_to_html(state);
            if (result != 0) {
                const errorAddress = heap32[state / 4 + 16];
                const errorLength = heap32[state / 4 + 17];
                const error = this._loadUtf8(errorAddress, errorLength);
                console.error(`ulight_source_to_html failed with status ${result}:`, error);
                throw result;
            }
            return this._bufferedText;
        } finally {
            if (state) {
                this._deleteState(state);
            }
            if (textBuffer) {
                this._free(textBuffer, bufferByteSize, 1);
            }
            if (tokenBuffer) {
                this._free(tokenBuffer, bufferByteSize, tokenAlign);
            }
            if (u8source) {
                this._free(u8source, sourceData.length, 1);
            }
        }
    }

    /**
     * Loads a `string` from UTF-8 encoded bytes residing in memory.
     * @param {number} address The address of the UTF-8 bytes.
     * @param {number} length The length of the string, in code units, i.e. the amount of bytes.
     * @returns {string} The decoded string.
     */
    _loadUtf8(address, length) {
        const wasmMemoryView = new Uint8Array(this._memory.buffer);
        const utf8Bytes = wasmMemoryView.slice(address, address + length);
        const decoder = new TextDecoder("utf-8", { fatal: true });
        return decoder.decode(utf8Bytes);
    }

    /**
     * Copies a `string` to a newly allocated UTF-8 array in memory.
     * The returned allocation should later be freed with `_freeUtf8`.
     * @param {string} str The string to encode.
     * @returns {{address:number;size:number}}
     * @see _freeUtf8
     */
    _allocUtf8(str) {
        const data = new TextEncoder().encode(str);
        const address = this._allocBytes(data);
        return { address, size: data.length };
    }

    /**
     * Allocates space for `bytes.length` bytes with `align` alignment,
     * and copies the given `bytes` to the allocated memory.
     * @param {Uint8Array} bytes The byte array.
     * @param {number} align The alignment.
     * @returns {number} The address of the allocation.
     */
    _allocBytes(bytes, align = 1) {
        const address = this._alloc(bytes.length, align);
        const heap8 = new Uint8Array(this._memory.buffer);
        heap8.set(bytes, address);
        return address;
    }

    /**
     * @param {{address:number;size:number}} param0 An allocation obtained from `_allocUtf8`. 
     */
    _freeUtf8({ address, size }) {
        this._free(address, size, 1);
    }

    /**
     * @param {number} size The requested number of bytes.
     * @param {number} alignment The memory alignment.
     * @returns {number} The address of the allocated memory.
     * @throws If allocation failed.
     */
    _alloc(size, alignment) {
        const result = this._exports.ulight_alloc(size, alignment);
        if (result === 0) {
            throw new Error(`Allocation failure in WASM with: bytes=${bytes}, align=${alignment}`);
        }
        return result;
    }

    /**
     * @param {number} address The address of the allocation.
     * @param {number} size The size of the allocation.
     * @param {number} alignment The memory alignment.
     */
    _free(address, size, alignment) {
        this._exports.ulight_free(address, size, alignment);
    }

    /**
     * @returns {number} The address of the allocated `ulight_state` object.
     * @throws If allocation failed.
     */
    _newState() {
        const result = this._exports.ulight_new();
        if (result === 0) {
            throw new Error("Allocation failure in ulight_new.");
        }
        return result;
    }

    /**
     * @param {number} state 
     */
    _deleteState(state) {
        this._exports.ulight_delete(state);
    }

    /**
     * @param {WebAssembly.ExportValue} flushText 
     */
    _addCallbacks(flushText) {
        const index = this._functionTable.length;
        this._functionTable.grow(2);
        this._functionTable.set(index, flushText);
        this._flushTextIndex = index;
    }

    async init() {
        if (this._exports._initialize) {
            this._exports._initialize();
        } else {
            console.warn('module has no export _initialize');
        }

        const flushTextFunction = (_, textAddress, textLength) => {
            const str = this._loadUtf8(textAddress, textLength);
            this._bufferedText += str;
        };
        const flushTextModule = await fetchBytes('f_i32_i32_i32_to_void.wasm')
            .then(bytes => WebAssembly.compile(bytes));
        const flushTextImports = { m: { f: flushTextFunction } };
        const flushTextInstance = await WebAssembly.instantiate(flushTextModule, flushTextImports);

        this._addCallbacks(flushTextInstance.exports.f);
    }

    /** @returns {WebAssembly.Table} */
    get _functionTable() {
        return this._exports.__indirect_function_table;
    }

    /** @returns {WebAssembly.Memory} */
    get _memory() {
        return this.wasm.instance.exports.memory;
    }

    /** @returns {WebAssembly.Instance} */
    get _instance() {
        return this.wasm.instance;
    }

    /** @returns {WebAssembly.Exports} */
    get _exports() {
        return this.wasm.instance.exports;
    }
}

/**
 * Loads the WebAssembly module.
 * @returns {Promise<UlightWasm>}
 */
export async function loadWasm() {
    // The implementations of these functions we provide don't actually match the WASI
    // requirements (https://wasix.org/docs/api-reference).
    // This is actually okay because we don't expect these functions to ever be called;
    // they are simply stuff that dead code elimination missed.
    const importObject = {
        env: {
            emscripten_notify_memory_growth() { }
        },
        wasi_snapshot_preview1: {
            clock_time_get() { },
            proc_exit() { },
            fd_close() { },
            fd_write() { },
            fd_seek() { },
            fd_read() { },
            environ_sizes_get() { },
            environ_get() { }
        }
    };
    const resultWasm = await WebAssembly.instantiateStreaming(fetch("ulight.wasm"), importObject);
    const result = new UlightWasm(resultWasm);
    await result.init();
    return result;
}

/**
 * @param {string} uri 
 * @returns {Promise<Uint8Array<ArrayBufferLike>>}
 */
async function fetchBytes(uri) {
    const response = await fetch(uri);
    return await response.bytes();
}
