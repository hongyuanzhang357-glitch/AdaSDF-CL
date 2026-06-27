# Install Validation Summary

- Source: `<source>`
- Build: `<build>`
- Install: `<install>`
- Config: `Debug`

## Results

### Configure: PASS

```bash
cmake -S '<source>' -B '<build>' -DADASDF_CL_BUILD_EXAMPLES=ON -DADASDF_CL_BUILD_TESTS=ON -DADASDF_CL_USE_EXISTING_CORE=OFF -DADASDF_CL_ENABLE_ADAPTIVE_BUILDER=ON -DADASDF_CL_ENABLE_SURROGATE_RECOMMENDER=ON '-DCMAKE_INSTALL_PREFIX=<install>'
```

```text
-- Selecting Windows SDK version 10.0.22621.0 to target Windows 10.0.26200.
--
-- AdaSDF-CL configuration:
--   Version: 0.7.0-alpha
--   Build examples: ON
--   Build tests: ON
--   Existing core requested: OFF
--   Existing core found: OFF
--   Adaptive builder: ON
--   Surrogate recommender: ON
--   CUDA: not required
--   FCL: not required
--
-- Configuring done (0.0s)
-- Generating done (0.2s)
-- Build files have been written to: <build>
```

### Build: PASS

```bash
cmake --build '<build>' --config Debug --parallel
```

```text
适用于 .NET Framework MSBuild 版本 17.14.40+3e7442088

  1>Checking Build System
  Building Custom Rule <source>/CMakeLists.txt
  adasdf_cl_runtime.vcxproj -> <build>\Debug\adasdf_cl_runtime.lib
  Building Custom Rule <source>/CMakeLists.txt
  Building Custom Rule <source>/CMakeLists.txt
  Building Custom Rule <source>/CMakeLists.txt
  Building Custom Rule <source>/CMakeLists.txt
  Building Custom Rule <source>/CMakeLists.txt
  Building Custom Rule <source>/CMakeLists.txt
  Building Custom Rule <source>/CMakeLists.txt
  Building Custom Rule <source>/CMakeLists.txt
  Building Custom Rule <source>/CMakeLists.txt
  Building Custom Rule <source>/CMakeLists.txt
  test_sdfmodel_query.vcxproj -> <build>\Debug\test_sdfmodel_query.exe
  test_pair_distance_query.vcxproj -> <build>\Debug\test_pair_distance_query.exe
  test_collision_query.vcxproj -> <build>\Debug\test_collision_query.exe
  adasdf_info.vcxproj -> <build>\Debug\adasdf_info.exe
  Building Custom Rule <source>/CMakeLists.txt
...
```

### Install: PASS

```bash
cmake --install '<build>' --config Debug --prefix '<install>'
```

```text
-- Up-to-date: <install>/lib/adasdf_cl_runtime.lib
-- Up-to-date: <install>/bin/adasdf_build.exe
-- Up-to-date: <install>/bin/adasdf_info.exe
-- Up-to-date: <install>/bin/adasdf_query.exe
-- Up-to-date: <install>/bin/adasdf_collide.exe
-- Up-to-date: <install>/include
-- Up-to-date: <install>/include/adasdf
-- Up-to-date: <install>/include/adasdf/adapters
-- Up-to-date: <install>/include/adasdf/adapters/EigenAdapter.h
-- Up-to-date: <install>/include/adasdf/adapters/FCLAdapter.h
-- Up-to-date: <install>/include/adasdf/adapters/PythonBindingPlan.h
-- Up-to-date: <install>/include/adasdf/adasdf.h
-- Up-to-date: <install>/include/adasdf/backend
-- Up-to-date: <install>/include/adasdf/backend/Backend.h
-- Up-to-date: <install>/include/adasdf/backend/CpuBackend.h
-- Up-to-date: <install>/include/adasdf/backend/GpuBackend.h
-- Up-to-date: <install>/include/adasdf/compression
-- Up-to-date: <install>/include/adasdf/compression/CompressedSDF.h
-- Up-to-date: <install>/include/adasdf/compression/CompressionOptions.h
-- Up-to-date: <install>/include/adasdf/compression/LowRankBlock.h
...
```

### Package Config Test: PASS

```bash
cmake '-DADASDF_CL_INSTALL_PREFIX=<install>' '-DADASDF_CL_PACKAGE_TEST_BUILD=<build>_pkg' -DADASDF_CL_CONFIG=Debug -P '<source>/tests/package/run_package_test.cmake'
```

```text
-- Configuring AdaSDF-CL package consumer
-- Selecting Windows SDK version 10.0.22621.0 to target Windows 10.0.26200.
-- Configuring done (0.0s)
-- Generating done (0.0s)
-- Build files have been written to: <local-path>
-- Building AdaSDF-CL package consumer
閫傜敤浜?.NET Framework MSBuild 鐗堟湰 17.14.40+3e7442088

  1>Checking Build System
  Building Custom Rule <local-path>
  test_find_package.vcxproj -> <local-path>
  Building Custom Rule <local-path>
-- Running AdaSDF-CL package consumer
Test project <local-path>
    Start 1: AdaSDFCLPackageConsumer
1/1 Test #1: AdaSDFCLPackageConsumer ..........   Passed    0.01 sec

100% tests passed, 0 tests failed out of 1

Total Test time (real) =   0.02 sec
```

### Downstream Configure: PASS

```bash
cmake -S '<source>/examples/downstream_cmake_project' -B '<workspace>/build/adasdf_cl_iv_ds' '-DCMAKE_PREFIX_PATH=<install>' -DCMAKE_BUILD_TYPE=Debug
```

```text
-- Selecting Windows SDK version 10.0.22621.0 to target Windows 10.0.26200.
-- Configuring done (0.0s)
-- Generating done (0.0s)
-- Build files have been written to: <build>_ds
```

### Downstream Build: PASS

```bash
cmake --build '<workspace>/build/adasdf_cl_iv_ds' --config Debug --parallel
```

```text
适用于 .NET Framework MSBuild 版本 17.14.40+3e7442088

  1>Checking Build System
  Building Custom Rule <source>/examples/downstream_cmake_project/CMakeLists.txt
  adasdf_downstream.vcxproj -> <build>_ds\Debug\adasdf_downstream.exe
  Building Custom Rule <source>/examples/downstream_cmake_project/CMakeLists.txt
```

### Downstream Run: PASS

```bash
'<workspace>/build/adasdf_cl_iv_ds/Debug/adasdf_downstream.exe'
```

```text
AdaSDF-CL downstream example
Version: 0.7.0-alpha
CPU backend: available
No .sdfbin supplied; package integration smoke test complete.
```
