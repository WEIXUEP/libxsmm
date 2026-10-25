# Windows CMake build

The `win_ea0b2049` branch starts at
`ea0b20499a41377bab148257240adbbfe1b4a333`. Build an x64 static library with
Visual Studio 2022 MSVC and jom from an initialized Intel oneAPI prompt:

```bat
call F:\Intel\oneAPI\setvars.bat intel64
cmake --preset win
cmake --build --preset win
ctest --test-dir build-win --output-on-failure
cmake --install build-win
```

The preset uses the DLL MSVC runtime (`/MD`), enables JIT code generation,
and installs `xsmm.lib`, public headers, and a relocatable CMake package into
`build-win/install`. No BLAS library is needed by this core static library.
Libraries using BLAS fallback, including libCEED, supply their own BLAS linkage.
Windows shared-library builds are not supported by this CMake configuration.
`XSMM_STATIC` selects static/shared linkage (static is required on Windows).
`XSMM_TRACE=ON` enables runtime tracing; Palace enables it for Debug builds.
Standard CMake compiler, configuration flags, and MSVC runtime settings apply.

Consumers can use:

```cmake
find_package(LIBXSMM CONFIG REQUIRED)
target_link_libraries(my_target PRIVATE LIBXSMM::xsmm)
```

Set `CMAKE_PREFIX_PATH` to the installation prefix. The focused CTest suite
checks the existing kernel registry and executes generated FP64 GEMM kernels
with beta=0 and beta=1 against a reference matrix multiplication. All three tests
pass with MSVC x64. The JIT check includes non-square transposed GEMV, which
regresses a Win64 register-save bug: the transpose setup must retain the new
A pointer in RCX instead of restoring its original value.
The large-transpose test uses 256 KB of scratch and verifies page-by-page
Win64 stack growth, without precommitting those pages through input buffers.
The Makefile and Visual Studio project build paths remain
available.
