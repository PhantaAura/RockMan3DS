# 0.0.14.51 validation commands

```sh
ctest --test-dir /private/tmp/legends3ds-01450-host -R 'actor_defeat_runtime|source_combat_profile|cook_actor_combat' --output-on-failure
python3 tools/build_mml3ds.py OWNER_EXTRACTION --out NEW_OUTPUT --cache LOCAL_CACHE --tutorial-only
```

The owner-local command verified source, built host tools, passed 144/144 tests,
cooked/validated eleven tutorial packages, ran deterministic replays, compiled
the ARMv6K target, and produced the SD layout. Owner and cooked payload paths are
intentionally omitted from the distributable source report.

