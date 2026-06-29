# Install Validation Summary

- Source: `<source>`
- Build: `<build>`
- Install: `<install>`
- Config: `Release`

## Results

### Configure: PASS

```bash
cmake -S '<source>' -B '<build>' -DADASDF_CL_BUILD_EXAMPLES=ON -DADASDF_CL_BUILD_TESTS=ON -DADASDF_CL_USE_EXISTING_CORE=OFF -DADASDF_CL_ENABLE_ADAPTIVE_BUILDER=ON -DADASDF_CL_ENABLE_SURROGATE_RECOMMENDER=ON '-DCMAKE_INSTALL_PREFIX=<install>'
```

```text
-- Selecting Windows SDK version 10.0.22621.0 to target Windows 10.0.26200.
--
-- AdaSDF-CL configuration:
--   Version: 0.7.0-alpha.1
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
cmake --build '<build>' --config Release --parallel
```

```text
适用于 .NET Framework MSBuild 版本 17.14.40+3e7442088

  1>Checking Build System
  Building Custom Rule <source>/CMakeLists.txt
  FCLAdapter.cpp
  Backend.cpp
  GpuBackend.cpp
  CompressedSDF.cpp
  AdaptiveSDFBuilder.cpp
  ExistingBuilderBridge.cpp
  SDFBuilder.cpp
  SurrogateRecommender.cpp
  MeshModel.cpp
  SDFModel.cpp
  Transform.cpp
  ContactOnlySDFBin.cpp
  ExistingSDFBridge.cpp
  SDFBinReader.cpp
  SDFBinWriter.cpp
  CandidatePointSampler.cpp
...
```

### Install: PASS

```bash
cmake --install '<build>' --config Release --prefix '<install>'
```

```text
-- Installing: <install>/lib/adasdf_cl_runtime.lib
-- Installing: <install>/bin/adasdf_build.exe
-- Installing: <install>/bin/adasdf_info.exe
-- Installing: <install>/bin/adasdf_query.exe
-- Installing: <install>/bin/adasdf_collide.exe
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

### Package Configure: PASS

```bash
cmake -S '<source>/tests/package' -B '<workspace>/build/adasdf_cl_iv_pkg' '-DCMAKE_PREFIX_PATH=<install>'
```

```text
-- Selecting Windows SDK version 10.0.22621.0 to target Windows 10.0.26200.
-- Configuring done (0.0s)
-- Generating done (0.0s)
CMake Warning:
  Manually-specified variables were not used by the project:

    CMAKE_PREFIX_PATH


-- Build files have been written to: <build>_pkg
```

### Package Build: PASS

```bash
cmake --build '<workspace>/build/adasdf_cl_iv_pkg' --config Release --parallel
```

```text
适用于 .NET Framework MSBuild 版本 17.14.40+3e7442088

  1>Checking Build System
  Building Custom Rule <source>/tests/package/CMakeLists.txt
  test_find_package.cpp
  test_find_package.vcxproj -> <build>_pkg\Release\test_find_package.exe
  Building Custom Rule <source>/tests/package/CMakeLists.txt
```

### Package Run: PASS

```bash
'<workspace>/build/adasdf_cl_iv_pkg/Release/test_find_package.exe'
```

```text
AdaSDF-CL package consumer
Version: 0.7.0-alpha.1
Point: 1 2 3
CPU backend available: true
```

### Downstream Configure: PASS

```bash
cmake -S '<source>/examples/downstream_cmake_project' -B '<workspace>/build/adasdf_cl_iv_ds' '-DCMAKE_PREFIX_PATH=<install>'
```

```text
-- Selecting Windows SDK version 10.0.22621.0 to target Windows 10.0.26200.
-- Configuring done (0.0s)
-- Generating done (0.0s)
-- Build files have been written to: <build>_ds
```

### Downstream Build: PASS

```bash
cmake --build '<workspace>/build/adasdf_cl_iv_ds' --config Release --parallel
```

```text
适用于 .NET Framework MSBuild 版本 17.14.40+3e7442088

  1>Checking Build System
  Building Custom Rule <source>/examples/downstream_cmake_project/CMakeLists.txt
  main.cpp
  adasdf_downstream.vcxproj -> <build>_ds\Release\adasdf_downstream.exe
  Building Custom Rule <source>/examples/downstream_cmake_project/CMakeLists.txt
```

### Downstream Run: PASS

```bash
'<workspace>/build/adasdf_cl_iv_ds/Release/adasdf_downstream.exe'
```

```text
AdaSDF-CL downstream example
Version: 0.7.0-alpha.1
CPU backend: available
No .sdfbin supplied; package integration smoke test complete.
```
