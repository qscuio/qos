all: prepare
	cd build && cmake -DCMAKE_BUILD_TYPE=Release -DFUNCHOOK_BUILD_TESTS=ON .. && make

prepare:
	mkdir -p build

arm64: prepare
	cd build && cmake -DCMAKE_BUILD_TYPE=Release -DFUNCHOOK_CPU=arm64 -DCMAKE_TOOLCHAIN_FILE=../cmake/aarch64-linux-gnu.cmake .. && make

arm: prepare
	cd build && cmake -DCMAKE_BUILD_TYPE=Release -DFUNCHOOK_CPU=arm -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-linux-gnu.cmake .. && make

clean:
	@if [ -d build ]; then cd build && make clean; fi
	rm -rf build

