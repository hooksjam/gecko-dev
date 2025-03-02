/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

//
// THIS FILE IS AUTOGENERATED by wasm-bindgen.
//
// Generated from:
// https://github.com/mstange/profiler-get-symbols/commit/bff0bc7b6dfbbc83b084d080dd977adea6d3f661
// by following the instructions in that repository's Readme.md
//

(function() {
    const __exports = {};
    let wasm;

    let cachegetUint32Memory = null;
    function getUint32Memory() {
        if (cachegetUint32Memory === null || cachegetUint32Memory.buffer !== wasm.memory.buffer) {
            cachegetUint32Memory = new Uint32Array(wasm.memory.buffer);
        }
        return cachegetUint32Memory;
    }

    function getArrayU32FromWasm(ptr, len) {
        return getUint32Memory().subarray(ptr / 4, ptr / 4 + len);
    }

    let cachedGlobalArgumentPtr = null;
    function globalArgumentPtr() {
        if (cachedGlobalArgumentPtr === null) {
            cachedGlobalArgumentPtr = wasm.__wbindgen_global_argument_ptr();
        }
        return cachedGlobalArgumentPtr;
    }

    let cachegetUint8Memory = null;
    function getUint8Memory() {
        if (cachegetUint8Memory === null || cachegetUint8Memory.buffer !== wasm.memory.buffer) {
            cachegetUint8Memory = new Uint8Array(wasm.memory.buffer);
        }
        return cachegetUint8Memory;
    }

    function getArrayU8FromWasm(ptr, len) {
        return getUint8Memory().subarray(ptr / 1, ptr / 1 + len);
    }

    const heap = new Array(32);

    heap.fill(undefined);

    heap.push(undefined, null, true, false);

    let stack_pointer = 32;

    function addBorrowedObject(obj) {
        if (stack_pointer == 1) throw new Error('out of js stack');
        heap[--stack_pointer] = obj;
        return stack_pointer;
    }

    let WASM_VECTOR_LEN = 0;

    let cachedTextEncoder = new TextEncoder('utf-8');

    let passStringToWasm;
    if (typeof cachedTextEncoder.encodeInto === 'function') {
        passStringToWasm = function(arg) {


            let size = arg.length;
            let ptr = wasm.__wbindgen_malloc(size);
            let offset = 0;
            {
                const mem = getUint8Memory();
                for (; offset < arg.length; offset++) {
                    const code = arg.charCodeAt(offset);
                    if (code > 0x7F) break;
                    mem[ptr + offset] = code;
                }
            }

            if (offset !== arg.length) {
                arg = arg.slice(offset);
                ptr = wasm.__wbindgen_realloc(ptr, size, size = offset + arg.length * 3);
                const view = getUint8Memory().subarray(ptr + offset, ptr + size);
                const ret = cachedTextEncoder.encodeInto(arg, view);

                offset += ret.written;
            }
            WASM_VECTOR_LEN = offset;
            return ptr;
        };
    } else {
        passStringToWasm = function(arg) {


            let size = arg.length;
            let ptr = wasm.__wbindgen_malloc(size);
            let offset = 0;
            {
                const mem = getUint8Memory();
                for (; offset < arg.length; offset++) {
                    const code = arg.charCodeAt(offset);
                    if (code > 0x7F) break;
                    mem[ptr + offset] = code;
                }
            }

            if (offset !== arg.length) {
                const buf = cachedTextEncoder.encode(arg.slice(offset));
                ptr = wasm.__wbindgen_realloc(ptr, size, size = offset + buf.length);
                getUint8Memory().set(buf, ptr + offset);
                offset += buf.length;
            }
            WASM_VECTOR_LEN = offset;
            return ptr;
        };
    }
    /**
    * @param {WasmMemBuffer} binary_data
    * @param {WasmMemBuffer} debug_data
    * @param {string} breakpad_id
    * @param {CompactSymbolTable} dest
    * @returns {boolean}
    */
    __exports.get_compact_symbol_table = function(binary_data, debug_data, breakpad_id, dest) {
        const ptr2 = passStringToWasm(breakpad_id);
        const len2 = WASM_VECTOR_LEN;
        try {
            return (wasm.get_compact_symbol_table(binary_data.ptr, debug_data.ptr, ptr2, len2, dest.ptr)) !== 0;

        } finally {
            wasm.__wbindgen_free(ptr2, len2 * 1);

        }

    };

function getObject(idx) { return heap[idx]; }

function debugString(val) {
    // primitive types
    const type = typeof val;
    if (type == 'number' || type == 'boolean' || val == null) {
        return  `${val}`;
    }
    if (type == 'string') {
        return `"${val}"`;
    }
    if (type == 'symbol') {
        const description = val.description;
        if (description == null) {
            return 'Symbol';
        } else {
            return `Symbol(${description})`;
        }
    }
    if (type == 'function') {
        const name = val.name;
        if (typeof name == 'string' && name.length > 0) {
            return `Function(${name})`;
        } else {
            return 'Function';
        }
    }
    // objects
    if (Array.isArray(val)) {
        const length = val.length;
        let debug = '[';
        if (length > 0) {
            debug += debugString(val[0]);
        }
        for(let i = 1; i < length; i++) {
            debug += ', ' + debugString(val[i]);
        }
        debug += ']';
        return debug;
    }
    // Test for built-in
    const builtInMatches = /\[object ([^\]]+)\]/.exec(toString.call(val));
    let className;
    if (builtInMatches.length > 1) {
        className = builtInMatches[1];
    } else {
        // Failed to match the standard '[object ClassName]'
        return toString.call(val);
    }
    if (className == 'Object') {
        // we're a user defined class or Object
        // JSON.stringify avoids problems with cycles, and is generally much
        // easier than looping through ownProperties of `val`.
        try {
            return 'Object(' + JSON.stringify(val) + ')';
        } catch (_) {
            return 'Object';
        }
    }
    // errors
    if (val instanceof Error) {
        return `${val.name}: ${val.message}\n${val.stack}`;
    }
    // TODO we could test for more things here, like `Set`s and `Map`s.
    return className;
}

let cachedTextDecoder = new TextDecoder('utf-8');

function getStringFromWasm(ptr, len) {
    return cachedTextDecoder.decode(getUint8Memory().subarray(ptr, ptr + len));
}

let heap_next = heap.length;

function addHeapObject(obj) {
    if (heap_next === heap.length) heap.push(heap.length + 1);
    const idx = heap_next;
    heap_next = heap[idx];

    heap[idx] = obj;
    return idx;
}

function dropObject(idx) {
    if (idx < 36) return;
    heap[idx] = heap_next;
    heap_next = idx;
}

function takeObject(idx) {
    const ret = getObject(idx);
    dropObject(idx);
    return ret;
}

function handleError(e) {
    wasm.__wbindgen_exn_store(addHeapObject(e));
}
/**
*/
class CompactSymbolTable {

    free() {
        const ptr = this.ptr;
        this.ptr = 0;

        wasm.__wbg_compactsymboltable_free(ptr);
    }
    /**
    * @returns {}
    */
    constructor() {
        this.ptr = wasm.compactsymboltable_new();
    }
    /**
    * @returns {Uint32Array}
    */
    take_addr() {
        const retptr = globalArgumentPtr();
        wasm.compactsymboltable_take_addr(retptr, this.ptr);
        const mem = getUint32Memory();
        const rustptr = mem[retptr / 4];
        const rustlen = mem[retptr / 4 + 1];

        const realRet = getArrayU32FromWasm(rustptr, rustlen).slice();
        wasm.__wbindgen_free(rustptr, rustlen * 4);
        return realRet;

    }
    /**
    * @returns {Uint32Array}
    */
    take_index() {
        const retptr = globalArgumentPtr();
        wasm.compactsymboltable_take_index(retptr, this.ptr);
        const mem = getUint32Memory();
        const rustptr = mem[retptr / 4];
        const rustlen = mem[retptr / 4 + 1];

        const realRet = getArrayU32FromWasm(rustptr, rustlen).slice();
        wasm.__wbindgen_free(rustptr, rustlen * 4);
        return realRet;

    }
    /**
    * @returns {Uint8Array}
    */
    take_buffer() {
        const retptr = globalArgumentPtr();
        wasm.compactsymboltable_take_buffer(retptr, this.ptr);
        const mem = getUint32Memory();
        const rustptr = mem[retptr / 4];
        const rustlen = mem[retptr / 4 + 1];

        const realRet = getArrayU8FromWasm(rustptr, rustlen).slice();
        wasm.__wbindgen_free(rustptr, rustlen * 1);
        return realRet;

    }
}
__exports.CompactSymbolTable = CompactSymbolTable;
/**
* WasmMemBuffer lets you allocate a chunk of memory on the wasm heap and
* directly initialize it from JS without a copy. The constructor takes the
* allocation size and a callback function which does the initialization.
* This is useful if you need to get very large amounts of data from JS into
* wasm (for example, the contents of a 1.7GB libxul.so).
*/
class WasmMemBuffer {

    free() {
        const ptr = this.ptr;
        this.ptr = 0;

        wasm.__wbg_wasmmembuffer_free(ptr);
    }
    /**
    * Create the buffer and initialize it synchronously in the callback function.
    * f is called with one argument: the Uint8Array that wraps our buffer.
    * f should not return anything; its return value is ignored.
    * f must not call any exported wasm functions! Anything that causes the
    * wasm heap to resize will invalidate the typed array\'s internal buffer!
    * Do not hold on to the array that is passed to f after f completes.
    * @param {number} byte_length
    * @param {any} f
    * @returns {}
    */
    constructor(byte_length, f) {
        try {
            this.ptr = wasm.wasmmembuffer_new(byte_length, addBorrowedObject(f));

        } finally {
            heap[stack_pointer++] = undefined;

        }

    }
}
__exports.WasmMemBuffer = WasmMemBuffer;

function init(module) {

    let result;
    const imports = {};
    imports.wbg = {};
    imports.wbg.__wbindgen_debug_string = function(ret, arg0) {

        const retptr = passStringToWasm(debugString(getObject(arg0)));
        const retlen = WASM_VECTOR_LEN;
        const mem = getUint32Memory();
        mem[ret / 4] = retptr;
        mem[ret / 4 + 1] = retlen;

    };
    imports.wbg.__wbindgen_throw = function(arg0, arg1) {
        let varg0 = getStringFromWasm(arg0, arg1);
        throw new Error(varg0);
    };
    imports.wbg.__wbindgen_memory = function() {
        return addHeapObject(wasm.memory);
    };
    imports.wbg.__wbg_buffer_e04d67bf3bf41917 = function(arg0) {
        return addHeapObject(getObject(arg0).buffer);
    };
    imports.wbg.__wbg_newwithbyteoffsetandlength_9cfc37146f8a28ba = function(arg0, arg1, arg2) {
        return addHeapObject(new Uint8Array(getObject(arg0), arg1 >>> 0, arg2 >>> 0));
    };
    imports.wbg.__wbindgen_object_drop_ref = function(arg0) {
        takeObject(arg0);
    };
    imports.wbg.__wbg_call_3d24f4e03e010931 = function(arg0, arg1, arg2) {
        try {
            return addHeapObject(getObject(arg0).call(getObject(arg1), getObject(arg2)));
        } catch (e) {
            handleError(e);
        }
    };

    if (module instanceof URL || typeof module === 'string' || module instanceof Request) {

        const response = fetch(module);
        if (typeof WebAssembly.instantiateStreaming === 'function') {
            result = WebAssembly.instantiateStreaming(response, imports)
            .catch(e => {
                console.warn("`WebAssembly.instantiateStreaming` failed. Assuming this is because your server does not serve wasm with `application/wasm` MIME type. Falling back to `WebAssembly.instantiate` which is slower. Original error:\n", e);
                return response
                .then(r => r.arrayBuffer())
                .then(bytes => WebAssembly.instantiate(bytes, imports));
            });
        } else {
            result = response
            .then(r => r.arrayBuffer())
            .then(bytes => WebAssembly.instantiate(bytes, imports));
        }
    } else {

        result = WebAssembly.instantiate(module, imports)
        .then(result => {
            if (result instanceof WebAssembly.Instance) {
                return { instance: result, module };
            } else {
                return result;
            }
        });
    }
    return result.then(({instance, module}) => {
        wasm = instance.exports;
        init.__wbindgen_wasm_module = module;

        return wasm;
    });
}

self.wasm_bindgen = Object.assign(init, __exports);

})();
