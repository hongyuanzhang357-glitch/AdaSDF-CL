# Install Validation Summary

- Source: `<source>`
- Build: `<build>`
- Install: `<install>`
- Config: `Release`

## Results

### Configure: PASS

```bash
cmake -S '<source>' -B '<build>' -DADASDF_CL_BUILD_EXAMPLES=ON -DADASDF_CL_BUILD_TESTS=ON -DADASDF_CL_BUILD_BENCHMARKS=ON -DADASDF_CL_USE_EXISTING_CORE=OFF -DADASDF_CL_ENABLE_ADAPTIVE_BUILDER=ON -DADASDF_CL_ENABLE_SURROGATE_RECOMMENDER=ON -DADASDF_CL_ENABLE_DEMO_BACKEND=ON -DADASDF_CL_ENABLE_DEMO_SURROGATE=ON -DADASDF_CL_ENABLE_COLLISION_VIEWER=ON -DADASDF_CL_ENABLE_CUDA=OFF '-DCMAKE_INSTALL_PREFIX=<install>'
```

```text
-- Building for: Visual Studio 17 2022
-- Selecting Windows SDK version 10.0.22621.0 to target Windows 10.0.26200.
-- The CXX compiler identification is MSVC 19.42.34444.0
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Check for working CXX compiler: C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.42.34433/bin/Hostx64/x64/cl.exe - skipped
-- Detecting CXX compile features
-- Detecting CXX compile features - done
--
-- AdaSDF-CL configuration:
--   Version: 1.0.1-alpha
--   Build examples: ON
--   Build tests: ON
--   Benchmarks: ON
--   Existing core requested: OFF
--   Existing core found: OFF
--   Adaptive builder: ON
--   Surrogate recommender: ON
--   Demo backend: ON
--   Demo surrogate: ON
...
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
  CudaQueryBackend.cpp
  GpuBackend.cpp
  PointCloudGenerator.cpp
  CompressedSDF.cpp
  AdaptiveSDFBuilder.cpp
  DemoAdaptiveSDFBuilder.cpp
  DemoSurrogateRecommender.cpp
  ExistingBuilderBridge.cpp
  SDFBuilder.cpp
  SurrogateRecommender.cpp
  AnalyticSDFModel.cpp
  DemoAdaptiveSDFModel.cpp
  MeshModel.cpp
  SDFModel.cpp
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
-- Installing: <install>/bin/adasdf_make_demo_box.exe
-- Installing: <install>/bin/adasdf_recommend_demo.exe
-- Installing: <install>/bin/adasdf_build_demo_adaptive.exe
-- Installing: <install>/bin/adasdf_collide_boxes_demo.exe
-- Installing: <install>/bin/adasdf_query_mode_demo.exe
-- Installing: <install>/bin/adasdf_benchmark_batch_query.exe
-- Installing: <install>/include
-- Installing: <install>/include/adasdf
-- Installing: <install>/include/adasdf/adapters
-- Installing: <install>/include/adasdf/adapters/EigenAdapter.h
-- Installing: <install>/include/adasdf/adapters/FCLAdapter.h
-- Installing: <install>/include/adasdf/adapters/PythonBindingPlan.h
-- Installing: <install>/include/adasdf/adasdf.h
-- Installing: <install>/include/adasdf/backend
-- Installing: <install>/include/adasdf/backend/Backend.h
...
```

### Package Configure: PASS

```bash
cmake -S '<source>/tests/package' -B '<local-path>' '-DCMAKE_PREFIX_PATH=<install>'
```

```text
-- Building for: Visual Studio 17 2022
-- Selecting Windows SDK version 10.0.22621.0 to target Windows 10.0.26200.
-- The CXX compiler identification is MSVC 19.42.34444.0
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Check for working CXX compiler: C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.42.34433/bin/Hostx64/x64/cl.exe - skipped
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- Configuring done (1.9s)
-- Generating done (0.0s)
-- Build files have been written to: <build>_pkg
```

### Package Build: PASS

```bash
cmake --build '<local-path>' --config Release --parallel
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
'<local-path>'
```

```text
AdaSDF-CL package consumer
Version: 1.0.1-alpha
Point: 1 2 3
Demo signed distance at origin: -0.5
Demo adaptive blocks: 7
Batch query points: 2
CUDA batch backend available: false
CPU backend available: true
```

### Downstream Configure: PASS

```bash
cmake -S '<source>/examples/downstream_cmake_project' -B '<local-path>' '-DCMAKE_PREFIX_PATH=<install>'
```

```text
-- Building for: Visual Studio 17 2022
-- Selecting Windows SDK version 10.0.22621.0 to target Windows 10.0.26200.
-- The CXX compiler identification is MSVC 19.42.34444.0
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Check for working CXX compiler: C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.42.34433/bin/Hostx64/x64/cl.exe - skipped
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- Configuring done (1.6s)
-- Generating done (0.0s)
-- Build files have been written to: <build>_ds
```

### Downstream Build: PASS

```bash
cmake --build '<local-path>' --config Release --parallel
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
'<local-path>'
```

```text
AdaSDF-CL downstream example
Version: 1.0.1-alpha
CPU backend: available
No .sdfbin supplied; running core-free demo adaptive path.
Demo signed distance at origin: -0.5
Demo adaptive blocks: 7
Demo batch query points: 3
CUDA batch backend: unavailable
Demo colliding: true
Demo contacts: 4
```
