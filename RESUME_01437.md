# Resume 0.0.14.37 first-boundary diagnostics

Exact source: /Users/emeraldhunter/.codex/.chatgpt-projects/g-p-6a7e25b6218c8191abf8733e250834ca/work/Legends3DS_0.0.14.37_WorldComesAlive
NoGit. Frozen parent.36 preserved. Syncedsources read-only. Noreset used.

Only gameplay-codechange is read-only logging of the first strict RNG mismatch after session.tick and beforeeffectcleanup/maintail. Print rawfixedXYZ28/32/36, halfXYZ20/22/24 andknownbits; state08/09/sub10, requestAC, sequenceA4/countdownA5; allocation/pendingknownflags and occupiedslotraw00/02/08/0D. Latchtape generation to avoid repeatedlogging. No importedstate injection or callbacksemantics invented.

Build nowrunning: python3 -B work/Legends3DS_0.0.14.37_WorldComesAlive/tools/build_hardware_test.py --pack work/pass-20261012-worldalive-01434/pack-runtime-r1 --out work/pass-20261013-worldalive-01437/native-r2 --host-build work/pass-20261013-worldalive-01437/host
Execsession13549;stdoutwork/pass-20261013-worldalive-01437/build-run-r1.stdout. Inspectnative-r2/build-result.json;script0doesnotprovesuccess. Currenthostbuild progresses. No.37compiled or emulatorclaimyet.

Next:finishbuild;fixfailureifany;run newbinaryinAzaharOld-modelusing.36privateSD/config in a freshprofile. Capturefirstmismatchactor/timer/pool beforedrift, compare existingPS1record. Thenport missing800172F4→table8011A604[2]→80116E68 subtype2 lifecycle with actualsourceRNGreceipts. Original.36research and privateproposalmuzzle remain inpass-20261013-worldalive-01436.

Originalparent.36131tests/nativePASS;14.37tests/buildpending. WCAunfinished, physicalOld3DSuntested, PHYSICS_1TO1=false. This recoveryZIPcontains completecumulativecode plus inheriteddocumentation but no retail/cooked/buildbinarypayloads.

Review correction: actor snapshot selects validated live event+0x24 pointer, logs expected pointer separately, and records all four pointer knowledge bits. No source RNG or gameplay semantics changed. Native-r1 PASS precedes this correction; native-r2 is authoritative. Emulator run still pending.

Final native-r2 result: PASS;131/131hostchecksPASS;all269compiledsource hashesmatch. ActualbinarySHA8fadfbc4f02c348c8a51210f749f1d7a86aab5f038e5e9c8e33da22e0dd98ad0. No01437emulatortest. Next: run newbinaryinfreshOld-modelAzaharprofileusingexistinglocal.36SD/config, capturefirstholdonce, then implement proven subtype2callback. SourceZIPdeliverables/Legends3DS_0.0.14.37_Source.zip.
