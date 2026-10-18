# Validation commands

Full native/host pipeline:
python3 -B tools/build_hardware_test.py --pack ../pass-20261014-worldalive-01440/pack-runtime-r1 --out ../pass-20261014-worldalive-01441/native-r1

PASS:138/138 host tests in27.49s; actualARMv6K build. Existing verified pack reused, no cookers/extractors.

Native selector addition inr2 and corrected camera-binding order inr3:
make -C ../pass-20261014-worldalive-01441/native-r3/native-source/platform/3ds -j4
ctest --test-dir ../pass-20261014-worldalive-01441/native-r1/host --output-on-failure

FinalPASS:138/138 in41.79s, native868008-byte3dsx. Only main.cpp changed; matching earlier objects were reused from the same toolchain and the changed object/final link were forced. All279currentnative source hashes match the final compilation snapshot. Old iterations/binaries/logs preserved.

Actual Old-modelAzahar verification: automaticCPUbars, GPUcyan with no shader draw, explicitXfallback and full player/bottommap. Finaloriginalcaptures in capture-validation.json. No physical14.41proof or fullWCAclaim. Final14.41UV/texture-off modes were not separately re-exercised; underlying renderers are retained from14.40.
