#include "legends3ds/actor_detection_math.h"
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <new>
#include <stdexcept>

namespace { size_t allocations = 0; bool count_allocations = false; }
void* operator new(std::size_t size) {
    if (count_allocations) ++allocations;
    if (void* pointer = std::malloc(size ? size : 1)) return pointer;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* pointer) noexcept { std::free(pointer); }
void operator delete[](void* pointer) noexcept { std::free(pointer); }
void operator delete(void* pointer, std::size_t) noexcept { std::free(pointer); }
void operator delete[](void* pointer, std::size_t) noexcept { std::free(pointer); }
namespace {
void package_tests() {
    l3::SourceNormalizationLookup table{}; for (size_t i = 0; i != table.size(); ++i) table[i] = static_cast<int16_t>(int32_t(i) - 96);
    const auto bytes = l3::encode_l3normalization_lookup(table);
    assert(l3::load_l3normalization_lookup(bytes.data(), bytes.size()) == table);
    const auto reject = [](const uint8_t* b, size_t n) {
        bool rejected = false;
        try { static_cast<void>(l3::load_l3normalization_lookup(b, n)); }
        catch (const std::runtime_error&) { rejected = true; }
        assert(rejected);
    };
    reject(nullptr, bytes.size()); reject(bytes.data(), bytes.size() - 1); reject(bytes.data(), bytes.size() + 1);
    for (const size_t offset : {size_t(0), size_t(4), size_t(6), size_t(8), size_t(12), size_t(16), size_t(20), size_t(24), size_t(28), size_t(32), size_t(415)}) {
        auto malformed = bytes; malformed[offset] ^= 1; reject(malformed.data(), malformed.size());
    }
    assert((l3::source_normalize_8007a694({{0, 0, 0}}, nullptr) == std::optional<std::array<int16_t, 3>>({{0, 0, 0}})));
    assert(!l3::source_normalize_8007a694({{1, 0, 0}}, nullptr));
    assert(!l3::source_normalize_8007a694({{32767, 32767, 32767}}, &table));
}
void normalize_fixture(const char* path, const l3::SourceNormalizationLookup* owner = nullptr) {
    std::ifstream file(path); if (!file) throw std::runtime_error("normalization fixture missing");
    int factor = 0; int32_t x = 0, y = 0, z = 0; int ex = 0, ey = 0, ez = 0; size_t count = 0;
    while ((owner != nullptr || bool(file >> factor)) && (file >> x >> y >> z >> ex >> ey >> ez)) {
        l3::SourceNormalizationLookup table{}; table.fill(static_cast<int16_t>(factor)); if (owner != nullptr) table = *owner;
        count_allocations = true; const auto out = l3::source_normalize_8007a694({{x, y, z}}, &table); count_allocations = false;
        if (!out || *out != std::array<int16_t, 3>{{static_cast<int16_t>(ex), static_cast<int16_t>(ey), static_cast<int16_t>(ez)}}) {
            std::cerr << "normalization divergence case " << count << " vector " << x << ',' << y << ',' << z << '\n';
            if (out) std::cerr << " actual " << (*out)[0] << ',' << (*out)[1] << ',' << (*out)[2] << " expected " << ex << ',' << ey << ',' << ez << '\n';
            throw std::runtime_error("original/native normalization mismatch");
        }
        ++count;
    }
    assert(count != 0 && allocations == 0); std::cout << "original/native normalization cases " << count << '\n';
}
void interpolate_fixture(const char* path, const l3::SourceNormalizationLookup* owner = nullptr) {
    std::ifstream file(path); if (!file) throw std::runtime_error("interpolation fixture missing");
    int factor = 0; int nx = 0, ny = 0, nz = 0, px = 0, py = 0, pz = 0, ex = 0, ey = 0, ez = 0; size_t count = 0;
    while ((owner != nullptr || bool(file >> factor)) && (file >> nx >> ny >> nz >> px >> py >> pz >> ex >> ey >> ez)) {
        l3::SourceNormalizationLookup table{}; table.fill(static_cast<int16_t>(factor)); if (owner != nullptr) table = *owner;
        const std::array<int16_t, 3> n{{static_cast<int16_t>(nx), static_cast<int16_t>(ny), static_cast<int16_t>(nz)}};
        const std::array<int16_t, 3> p{{static_cast<int16_t>(px), static_cast<int16_t>(py), static_cast<int16_t>(pz)}};
        count_allocations = true; const auto out = l3::source_detection_interpolate_80033f04(n, p, &table); count_allocations = false;
        if (!out || *out != std::array<int16_t, 3>{{static_cast<int16_t>(ex), static_cast<int16_t>(ey), static_cast<int16_t>(ez)}}) {
            std::cerr << "interpolation divergence case " << count << " previous " << px << ',' << py << ',' << pz << '\n';
            if (out) std::cerr << " actual " << (*out)[0] << ',' << (*out)[1] << ',' << (*out)[2] << " expected " << ex << ',' << ey << ',' << ez << '\n';
            throw std::runtime_error("original/native interpolation mismatch");
        }
        ++count;
    }
    assert(count != 0 && allocations == 0); std::cout << "original/native interpolation cases " << count << "; allocations " << allocations << '\n';
}
l3::SourceNormalizationLookup owner_lookup(const char* path) {
    std::ifstream file(path, std::ios::binary); if (!file) throw std::runtime_error("owner lookup input unavailable");
    // The source probe hash-gates the owner EXE profile. This read-only helper
    // loads precisely the known reference records and never writes owner files.
    file.seekg(0x80092d38 - 0x80010000 + 0x800);
    l3::SourceNormalizationLookup table{};
    for (auto& value : table) {
        unsigned char bytes[2]{}; file.read(reinterpret_cast<char*>(bytes), 2);
        if (!file) throw std::runtime_error("truncated owner normalization lookup");
        const uint32_t bits = uint32_t(bytes[0]) | uint32_t(bytes[1]) << 8;
        value = bits < 32768 ? static_cast<int16_t>(bits) : static_cast<int16_t>(int32_t(bits) - 65536);
    }
    return table;
}
} // namespace
int main(int argc, char** argv) {
    package_tests();
    if (argc > 1) normalize_fixture(argv[1]);
    if (argc > 2) interpolate_fixture(argv[2]);
    if (argc > 5) {
        const auto owner = owner_lookup(argv[3]);
        normalize_fixture(argv[4], &owner); interpolate_fixture(argv[5], &owner);
    }
    std::cout << "detector source math/codec passed; retail parity false\n";
}
