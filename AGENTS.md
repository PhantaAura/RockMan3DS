# Working rules for any agent (Claude, Codex/ChatGPT, …)

Repository: https://github.com/PhantaAura/RockMan3DS — this Git history is the
single source of truth. Do not create ZIP "checkpoints" or Recovery copies.

Start of a session
1. `git pull`, then read `README.md` and the newest `RESUME_*.md` (one page).
2. Do not re-explore the whole tree; open only the files the task names.

While working
- One logical change = one commit, pushed as you go. Message: what changed and why,
  plus which tests ran.
- Run `cmake --build <host> && ctest --test-dir <host>` before each commit that
  touches C/C++. Native build: `tools/m1_mac_build.sh` (Mac with devkitARM).
- Keep evidence labels honest (VERIFIED_1TO1 / SOURCE_BACKED_PARTIAL / CANDIDATE /
  UNKNOWN). Never invent original constants; mark guesses CANDIDATE.

Never commit
- Retail data or anything cooked from it (EXE, BIN/CUE/CHD, saves, `.l3*`,
  `.mmlpkg`, SD folders), decompiler exports, or `.3dsx` builds. `.gitignore`
  covers the usual names — check `git status` anyway.

End of a session
- Update the current `RESUME_*.md` (what's done, what's next, blockers) and
  `state/CURRENT_CHECKPOINT.json`, commit, push.
