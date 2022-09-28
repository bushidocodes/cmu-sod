CFLAGS=-O3 -I. -Wall
LDFLAGS=-flto

WASMCC=${WASI_SDK_PATH}/bin/clang --sysroot=${WASI_SDK_PATH}/share/wasi-sysroot/
WASMCPP=${WASI_SDK_PATH}/bin/clang++ --sysroot=${WASI_SDK_PATH}/share/wasi-sysroot/
WASMCFLAGS=${CFLAGS}

# See https://lld.llvm.org/WebAssembly.html
WASMLDFLAGS=${LDFLAGS} -Wl,--allow-undefined,-z,stack-size=32768,--threads=1

# Clang 12 WebAssembly Options
# See https://clang.llvm.org/docs/ClangCommandLineReference.html#webassembly
# Disable WebAssembly Proposals aWsm does not support
WASMCFLAGS+= -mno-atomics # https://github.com/WebAssembly/threads
WASMCFLAGS+= -mno-bulk-memory # https://github.com/WebAssembly/bulk-memory-operations
WASMCFLAGS+= -mno-exception-handling # https://github.com/WebAssembly/exception-handling
WASMCFLAGS+= -mno-multivalue # https://github.com/WebAssembly/multi-value
# Mutable globals still exist, but disables the ability to import mutable globals
WASMCFLAGS+= -mno-mutable-globals # https://github.com/WebAssembly/mutable-global
WASMCFLAGS+= -mno-nontrapping-fptoint # https://github.com/WebAssembly/nontrapping-float-to-int-conversions
WASMCFLAGS+= -mno-reference-types # https://github.com/WebAssembly/reference-types
WASMCFLAGS+= -mno-sign-ext # https://github.com/WebAssembly/sign-extension-ops
WASMCFLAGS+= -mno-tail-call # https://github.com/WebAssembly/tail-call
WASMCFLAGS+= -mno-simd128 # https://github.com/webassembly/simd


build:
	mkdir -p build

out:
	mkdir -p out

build/sod.o: build sod_release_118/sod.c
	clang ${CFLAGS} -c sod_release_118/sod.c -o $@

build/sod.wasm.o: build sod_release_118/sod.c
	${WASMCC} ${WASMCFLAGS} -D_WASI_EMULATED_MMAN -o $@ -c sod_release_118/sod.c

out/original: out src/original.cpp build/sod.o
	clang++ ${CFLAGS} -fpermissive -o $@ src/original.cpp build/sod.o -Isod_release_118

out/depth_to_xyz: out src/depth_to_xyz.cpp build/sod.o
	clang++ ${CFLAGS} -fpermissive -o $@ src/depth_to_xyz.cpp build/sod.o -Isod_release_118

out/original.wasm: out src/original.cpp build/sod.wasm.o
	${WASMCPP} ${WASMCFLAGS} -D_WASI_EMULATED_MMAN -o $@ src/original.cpp build/sod.wasm.o -Isod_release_118 -lwasi-emulated-mman

out/depth_to_xyz.wasm: out src/depth_to_xyz.cpp build/sod.wasm.o
	${WASMCPP} ${WASMCFLAGS} -D_WASI_EMULATED_MMAN -o $@ src/depth_to_xyz.cpp build/sod.wasm.o -Isod_release_118 -lwasi-emulated-mman

.PHONY: run
run: out/depth_to_xyz
	cat ./images_whiteboard/0_rgb.png ./images_whiteboard/0_depth.png | ./out/depth_to_xyz 3011515 620608 > ./out/native.png

.PHONY: run-original
run-original: out/original
	./out/original

.PHONY: run-wasm
run-wasm: out/depth_to_xyz.wasm
	cat ./images_whiteboard/0_rgb.png ./images_whiteboard/0_depth.png | wasmtime ./out/depth_to_xyz.wasm 3011515 620608 > ./out/wasm.png

.PHONY: run-original-wasm
run-original-wasm: out/original.wasm
	 wasmtime --dir=. ./out/original.wasm 

.PHONY: clean
clean:
	rm -rf build
	rm -rf out
