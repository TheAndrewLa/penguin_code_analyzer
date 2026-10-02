## Penguin Code Analyzer

<img src="frontend/public/assets/logo.png" alt="Penguin Code Analyzer" width="96" align="right" />

A machine code analyzer

#### Features

- Multiple source tabs, compiled with gcc or clang
- Upload prebuilt `.o`, `.obj` or `.elf` files
- Control-flow graph with per-instruction details:
  μOps, latency, reciprocal throughput,
  may load / may store, unmodeled side effects
- Aggregate stats: total cycles, IPC, μOps / cycle
- Pipeline timeline for simulated blocks
- Microarchitecture presets: Haswell, Skylake, Ice Lake,
  Rocket Lake, Zen 2, Zen 3
- Compile and simulation errors reported inline

#### Build & Run

```bash
docker compose build
docker compose up -d
```

Application will be at `localhost:3000`
