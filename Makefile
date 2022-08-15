build:
	mkdir -p build

out:
	mkdir -p out

build/sod.o: build sod_release_118/sod.c
	clang -c sod_release_118/sod.c -o $@

build/sod.wasm.o: build sod_release_118/sod.c
	${WASI_SDK_PATH}/bin/clang --sysroot=${WASI_SDK_PATH}/share/wasi-sysroot/ -D_WASI_EMULATED_MMAN -o $@ -c sod_release_118/sod.c

out/depth_to_xyz: out src/depth_to_xyz.cpp build/sod.o
	clang++ -fpermissive -o $@ src/depth_to_xyz.cpp build/sod.o -Isod_release_118

out/depth_to_xyz.wasm: out src/depth_to_xyz.cpp build/sod.wasm.o
	${WASI_SDK_PATH}/bin/clang++ --sysroot=${WASI_SDK_PATH}/share/wasi-sysroot/ -D_WASI_EMULATED_MMAN -o $@ src/depth_to_xyz.cpp build/sod.wasm.o -Isod_release_118 -lwasi-emulated-mman

.PHONY: run
run: out/depth_to_xyz
	cat ./images_whiteboard/0_rgb.png ./images_whiteboard/0_depth.png | ./out/depth_to_xyz 3011515 620608 > ./out/native.png

.PHONY: run-wasm
run-wasm: out/depth_to_xyz.wasm
	cat ./images_whiteboard/0_rgb.png ./images_whiteboard/0_depth.png | wasmtime ./out/depth_to_xyz.wasm 3011515 620608 > ./out/wasm.png

.PHONY: clean
clean:
	rm -f out/depth_to_xyz out/depth_to_xyz.wasm out/res.png build/sod.wasm.o build/sod.o