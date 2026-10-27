# Aqua_JoltPhysics 2.0

Jolt Physics through the current Aquarius native-library SDK. Import `JoltPhysics` (alias `剛體物理`); the bilingual `CreateWorld` / `建立世界` API and the world methods remain unchanged. Worlds own their bodies and reject foreign, removed, static-for-dynamic-operations, and disposed handles.

The same C++20 code runs natively on Windows, Linux and Android and as a browser WebAssembly side module. It uses Jolt's single-thread job scheduler on every target, with 4096 bodies per world, continuous collisions, 16 MiB temporary allocators and the original validation limits. The JavaScript DynamicLibrary adapter is retired.

Jolt Physics **5.3.0** is pinned in `native/vendor/`, with the official source archive SHA-256 `e7f9621e480646c434150e1fbe3a9410f4ec4b04ffe54791e0678326b741b918` verified by CMake. The source and MIT license are included. `native/vendor/PATCHES.md` documents the small WASI/modern libc++ portability changes.

Build with `aqua library build . --targets <SDK targets>` using the SDK's pinned Clang toolchains. Run `aqua run tests` for impulse, gravity and floor collision; select `validation.aqua` from the compiled test program for invalid arguments and resource ownership. Export the same test project for web and Android. The previous npm wrapper tests and historical C# tests are superseded by these executable Aquarius tests.

Current Aquarius SDK targets cover Windows x64, Linux x64, web, Android arm64 and Android x64. The old wrapper advertised macOS, but the current Aquarius distribution has no macOS runtime pack, so macOS execution is not validated by this migration.
