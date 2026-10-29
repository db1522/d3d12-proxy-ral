# d3d12-proxy (ROG Ally Life fork)

A fork of [alxn1/d3d12-proxy](https://github.com/alxn1/d3d12-proxy) (itself based on cdozdil's d3d12-proxy), used by
the **UE5 Lighting Fix** Game Bar widget to bring back Unreal Engine 5 Lumen lighting on handheld GPUs.

Unreal Engine 5 treats a GPU that reports no non-local memory as integrated, and then turns off Lumen global
illumination, ambient occlusion and volumetric fog. The proxy reports a non-zero value
(`gpu_non_local_memory_size`), so the engine keeps them on. Everything from upstream works unchanged.

## What the fork adds

**A status file.** After each session, `dxgi_proxy.status.json` next to the game records what the proxy did:

```json
{
  "integratedGpu": true,
  "overridesActive": true,
  "nonLocalQueries": 12,
  "nonLocalOverridden": 12,
  "firstQuerySeconds": 4.2,
  "lastQuerySeconds": 63.1,
  "overridesStoppedAfterSeconds": null
}
```

`nonLocalQueries` is the key number: it counts how often the game asked for the non-local memory size, the question
Unreal uses to decide a GPU is integrated. A game that never asks can't be changed by the fix, so tools can tell
players, or turn the fix off by themselves. It's written on the first query, then at most every 5 seconds, and when
the game exits. Turn it off with `[status] enable=false`.

**`disable_after_first_query`.** Like upstream's `disable_after`, but the countdown starts at the game's first
non-local memory query instead of adapter creation. Games with long launchers, splash screens or shader compiles still
get the override when Unreal decides, and the real memory figures come back afterwards.

```ini
[override]
only_igpu=true
gpu_non_local_memory_size=2147483648
; disable_after_first_query=60

[status]
enable=true
```

## Build

Visual Studio 2022 (`d3d12-proxy.sln`), or push a tag like `v1.0.0` and GitHub builds and publishes `d3d12-proxy.zip`
(`dxgi.dll` + `dxgi_proxy.ini`, the same layout as upstream releases). It also builds with MinGW-w64:

```sh
cd dxgi-proxy
x86_64-w64-mingw32-g++ -std=c++17 -O2 -s -shared -include pch.h -o dxgi.dll dxgi_proxy*.cpp source.def \
    -static -static-libgcc -static-libstdc++ -luuid
```

## Licence and credits

MIT, like upstream (see LICENSE). Original work by **cdozdil** and **alxn1**; fork changes by ROG Ally Life.
