# Jolt Physics 5.3.0 portability changes

Upstream: https://github.com/jrouwe/JoltPhysics/tree/v5.3.0, MIT license in the vendored source.

- `Jolt/Core/Core.h`: include `type_traits` explicitly for modern libc++; recognize WASI as WebAssembly.
- `Jolt/Core/Mutex.h`: threadless WASI lock types for the binding's `JobSystemSingleThreaded`. Native synchronization is unchanged.
- `Jolt/Physics/Body/BodyManager.cpp`: use one body mutex on threadless WASI.
- `Jolt/Physics/PhysicsSystem.cpp`: omit OS thread yields on threadless WASI.

These changes adapt platform support. Physics, collision detection and integration algorithms are unchanged. CMake omits unused thread-pool/semaphore implementations from the WASI build.
