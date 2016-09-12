MATLAB_DIST_PATH="/usr/local/MATLAB/R2014b"
CC=$(shell which gcc)
BIN_FILES=$(shell find bin -type f)
BUILD_FILES=$(shell find build -type f)

all: native mex

native: bin/testopencl LibSPH

mex: bin/mex/testinit.mexa64 bin/mex/querystate.mexa64 bin/mex/simstep.mexa64 LibSPH_mex bin/mex/testinit_wrap.m

clean:
ifneq ($(strip $(BIN_FILES)),)
	rm $(BIN_FILES)
endif
ifneq ($(strip $(BUILD_FILES)),)
	rm $(BUILD_FILES)
endif

bin/testopencl: build/testopencl.o bin/libsph.so
	$(CC) -Wl,-rpath,'$$ORIGIN' -fPIC $< -o $@ -lsph -Lbin -lm

LibSPH: bin/libsph.so bin/kernels/particle_system_kern.cl

bin/libsph.so: build/particle_system.o build/opencl/particle_system_host.o build/opencl/platforminfo.o build/opencl/clerror.o build/note.o build/build_psdata.o build/stringly.o build/3rdparty/whereami.o
	$(CC) -shared -fPIC $^ -o $@ -lOpenCL

bin/kernels/particle_system_kern.cl: src/opencl/particle_system_kern.cl
	cp $< $@

build/%.o: src/%.c
	$(CC) -fPIC -c $< -o $@

bin/mex/testinit_wrap.m: src/mex/testinit_wrap.m
	cp $< $@

LibSPH_mex: bin/mex/libsph_mex.so bin/kernels/particle_system_kern.cl

bin/mex/libsph_mex.so: build/mex/particle_system_mex.o build/mex/opencl/particle_system_host_mex.o build/mex/opencl/platforminfo_mex.o build/mex/note_mex.o build/opencl/clerror.o build/3rdparty/whereami.o build/mex/stringly_mex.o build/mex/build_psdata_mex.o
	$(CC) -shared -fPIC $^ -o $@ -lOpenCL

bin/mex/%.mexa64: build/mex/%.o bin/mex/libsph_mex.so
	$(CC) -pthread -Wl,--no-undefined -Wl,-rpath-link,$(MATLAB_DIST_PATH)/bin/glnxa64 \
		-shared  -O -Wl,--version-script,"$(MATLAB_DIST_PATH)/extern/lib/glnxa64/mexFunction.map" \
		$<  -Wl,-rpath,'$$ORIGIN' -lsph_mex   -Lbin/mex   -L"$(MATLAB_DIST_PATH)/bin/glnxa64" \
		-lmx -lmex -lmat -lm -lstdc++ -o $@

build/mex/%.o: src/mex/%.c
	mex -O CFLAGS="$$CFLAGS -fPIC -std=c99" -c $< -outdir build/mex

build/mex/%_mex.o: src/%.c
	mex -O CFLAGS="$$CFLAGS -fPIC -std=c99" -c $< -outdir build/mex/$(*D)
	mv build/mex/$*.o $@
