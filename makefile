MATLAB_DIST_PATH=$(shell dirname $(shell dirname $(shell which matlab)))
CC=$(shell which gcc)
BIN_FILES=$(shell find bin -type f)
BUILD_FILES=$(shell find build -type f)

native: bin/testopencl

mex: bin/mex/testentry.mexa64 bin/mex/testentry2.mexa64 bin/mex/testopencl.mexa64

clean:
ifneq ($(strip $(BIN_FILES)),)
	rm $(BIN_FILES)
endif
ifneq ($(strip $(BUILD_FILES)),)
	rm $(BUILD_FILES)
endif

bin/testopencl: build/testopencl.o bin/libsph.so
	$(CC) -Wl,-rpath,'$$ORIGIN' -fPIC $< -o $@ -lsph -Lbin

bin/libsph.so: build/particle_system.o build/opencl/particle_system_host.o
	$(CC) -shared -fPIC $^ -o $@ -lOpenCL

build/%.o: src/%.c
	$(CC) -fPIC -c $< -o $@

bin/mex/libsph_mex.so: build/mex/particle_system_mex.o build/mex/opencl/particle_system_host_mex.o
	$(CC) -shared $^ -o $@ -lOpenCL

bin/mex/%.mexa64: build/mex/%.o bin/mex/libsph_mex.so
	$(CC) -pthread -Wl,--no-undefined -Wl,-rpath-link,$(MATLAB_DIST_PATH)/bin/glnxa64 \
		-shared  -O -Wl,--version-script,"$(MATLAB_DIST_PATH)/extern/lib/glnxa64/mexFunction.map" \
		$<  -Wl,-rpath,'$$ORIGIN' -lsph_mex   -Lbin/mex   -L"$(MATLAB_DIST_PATH)/bin/glnxa64" \
		-lmx -lmex -lmat -lm -lstdc++ -o $@

build/mex/%.o: src/mex/%.c
	mex -c $< -outdir build/mex

build/mex/%_mex.o: src/%.c
	mex -c $< -outdir build/mex/$(*D)
	mv build/mex/$*.o $@
