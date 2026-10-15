# Aqua_JoltPhysics

An independent Bottle library. Core has no Jolt registration, physics engine or embedded physics assets.

```powershell
aqua bottle install C:/OfficialProjects/Aqua_JoltPhysics --global
aqua bottle install C:/OfficialProjects/Aqua_JoltPhysics --project .
aqua build . -o app.wasm
```

Import `/bottles/Aqua_JoltPhysics/main.aqua`. `CreateWorld([0,-9.81d,0])` returns a module with the former Jolt methods (and Traditional Chinese aliases). Body IDs are opaque and world-specific. Each load owns its engine, worlds and bodies, released when the runtime exits. Repeated world disposal is safe.

Windows/Linux/macOS use the generic ClearScript ES-module loader, web uses browser WebAssembly, and Android uses Javet WebAssembly. All execute the same pinned Jolt 0.24.0 Wasm, with no native ABI divergence. This maintains existing simulation behavior; it does not claim native Jolt throughput. Spheres and boxes, 4096 bodies per world, 128 worlds per load. Use a fixed 1/60 second step.

`npm ci; npm run build` rebuilds the self-contained ES module. Vendor Jolt glue/Wasm/license are pinned to 0.24.0. The final module embeds Wasm so exports work without external fetches or relative module imports.
