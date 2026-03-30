# Qulk Userspace Bulk Port

This subtree ports the userspace app corpus from `/home/ubuntu/work/qulk/userspace/app` into `linux-lab`, together with the shared `lib/` layer and the vendored dependency candidates needed by that corpus.

## Layout

- `app/`: imported top-level app sources
- `lib/`: imported shared headers and support sources
- `qsc/`: imported `qsc` support library tree referenced by the original userspace build
- `vendor/`: imported third-party dependency trees used by the app corpus
- `source.Makefile`: imported top-level `qulk/userspace/Makefile` used as the source of truth for include and link metadata
- `tools/build_all.py`: bulk build driver
- `build/`: generated outputs, logs, and manifest

## Build

Run:

```bash
make -C linux-lab/examples/userspace/qulk -j4
```

Outputs land under `build/`:

- `build/bin/`: built binaries
- `build/logs/`: per-app real-build failure logs
- `build/manifest.json`: summary of built and stubbed outputs

The default target tries to compile each imported app for the current host. When a source cannot currently be built on this host or with the available dependency surface, the build falls back to a small compatibility stub so the bulk target still completes and the manifest records the real failure.

For a fail-fast run with no stub fallback:

```bash
make -C linux-lab/examples/userspace/qulk strict
```
