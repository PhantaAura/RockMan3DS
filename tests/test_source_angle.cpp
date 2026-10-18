#include "legends3ds/source_angle.h"
#include "legends3ds/actor_runtime.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
int32_t signed_word(uint32_t bits) {
    return bits <= INT32_MAX ? static_cast<int32_t>(bits)
        : static_cast<int32_t>(static_cast<int64_t>(bits) - INT64_C(4294967296));
}
l3::SourceAngleSteps synthetic_steps(int kind) {
    l3::SourceAngleSteps table{};
    require(kind >= 0 && kind <= 3, "unknown synthetic table kind");
    for (size_t i = 0; i != table.size(); ++i) {
        if (kind == 1) table[i] = static_cast<int32_t>(i + 1);
        else if (kind == 2) table[i] = static_cast<int32_t>(i + 1) * ((i & 1U) != 0 ? -1 : 1);
        else if (kind == 3) table[i] = signed_word(static_cast<uint32_t>(i + 1) * UINT32_C(0x13579bdf));
    }
    return table;
}
l3::SourceAngleSteps owner_steps(const char* path) {
    std::ifstream exe(path, std::ios::binary);
    require(static_cast<bool>(exe), "owner executable unavailable");
    // Profile/hash is checked by the original-code fixture generator. This
    // standalone comparison reads only the known12 words; never serializes them.
    exe.seekg(0x80092b64 - 0x80010000 + 0x800);
    l3::SourceAngleSteps steps{};
    for (auto& value : steps) {
        unsigned char bytes[4]{};
        exe.read(reinterpret_cast<char*>(bytes), 4);
        require(static_cast<bool>(exe), "truncated owner angle table");
        value = signed_word(static_cast<uint32_t>(bytes[0]) |
            static_cast<uint32_t>(bytes[1]) << 8 |
            static_cast<uint32_t>(bytes[2]) << 16 |
            static_cast<uint32_t>(bytes[3]) << 24);
    }
    return steps;
}
size_t compare(const char* path, const l3::SourceAngleSteps* owner) {
    std::ifstream file(path);
    require(static_cast<bool>(file), "angle source fixture unavailable");
    std::string line;
    std::getline(file, line);
    size_t count = 0;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::istringstream row(line);
        int operation, table_kind;
        int16_t ax, az, tx, tz;
        int32_t ratio;
        uint32_t expected;
        require(static_cast<bool>(row >> operation >> table_kind >> ax >> az >> tx >> tz >> ratio >> expected), "malformed angle case");
        std::string extra;
        require(!(row >> extra), "unexpected angle case field");
        l3::SourceAngleSteps table{};
        if (table_kind == -1) {
            require(owner != nullptr, "owner table must be explicitly supplied");
            table = *owner;
        } else table = synthetic_steps(table_kind);
        uint32_t actual = 0;
        if (operation == 0) actual = static_cast<uint32_t>(l3::source_catan_8007a414(ratio, table));
        else {
            require(operation == 1, "unknown angle operation");
            const auto heading = l3::source_heading_80031198(ax, az, tx, tz, &table);
            require(heading.has_value(), "supported angle query rejected");
            actual = *heading;
            l3::ImportedActorInstance actor;
            actor.source_class = 0x20;
            actor.subtype = 3;
            actor.source_origin = {{ax, 0, az}};
            const auto query = l3::sample_imported_actor_target(actor, tx, tz, &table);
            require(query.has_value() && query->source_heading_80031198 == expected,
                    "imported actor target query changed original heading");
        }
        if (actual != expected) {
            std::cerr << "first angle divergence case " << count << ": expected " << expected << " actual " << actual << '\n';
            throw std::runtime_error("source angle mismatch");
        }
        ++count;
    }
    require(count != 0, "empty angle fixture");
    return count;
}

void package_checks() {
    const auto reject = [](const uint8_t* bytes, size_t size) {
        bool failed = false;
        try { static_cast<void>(l3::load_l3actor_angle(bytes, size)); }
        catch (const std::runtime_error&) { failed = true; }
        require(failed, "malformed angle package accepted");
    };
    reject(nullptr, l3::kActorAnglePackageBytes);
    for (int kind = 0; kind != 4; ++kind) {
        const auto table = synthetic_steps(kind);
        const auto package = l3::encode_l3actor_angle(table);
        require(l3::load_l3actor_angle(package.data(), package.size()) == table,
                "angle package signed words changed");
        for (size_t size = 0; size != package.size(); ++size) reject(package.data(), size);
        reject(package.data(), package.size() + 1);
        for (size_t offset = 0; offset != package.size(); ++offset) {
            auto malformed = package;
            malformed[offset] ^= 1;
            reject(malformed.data(), malformed.size());
        }
    }
    l3::ImportedActorInstance actor;
    actor.source_class = 0x20;
    actor.subtype = 3;
    const auto unknown = l3::sample_imported_actor_target(actor, 1, 1, nullptr);
    require(unknown.has_value() && !unknown->source_heading_80031198.has_value(),
            "missing angle table must keep target heading unknown");
    require(!l3::sample_imported_actor_target(actor, INT16_MAX + 1, 0, nullptr).has_value(),
            "out-of-range player X accepted");
    require(!l3::sample_imported_actor_target(actor, 0, INT16_MIN - 1, nullptr).has_value(),
            "out-of-range player Z accepted");
    actor.subtype = 0x35;
    require(!l3::sample_imported_actor_target(actor, 0, 0, nullptr).has_value(),
            "unproved diagnostic subtype target query accepted");
    actor.source_class = 0x40;
    actor.subtype = 3;
    require(!l3::sample_imported_actor_target(actor, 0, 0, nullptr).has_value(),
            "unsupported actor class target query accepted");
}
} // namespace

int main(int argc, char** argv) {
    try {
        require(argc == 2 || argc == 4 || argc == 5, "synthetic fixture, optionally owner fixture/executable and cooked package required");
        package_checks();
        const size_t synthetic = compare(argv[1], nullptr);
        require(!l3::source_heading_80031198(0, 0, 1, 1, nullptr).has_value(), "missing owner table must remain unknown");
        require(l3::source_heading_80031198(0, 0, 0, 0, nullptr) == 0xc00U, "source zero-vector branch changed");
        require(l3::source_heading_80031198(0, 0, -1, 0, nullptr) == 0x400U, "source negative axis branch changed");
        std::cout << synthetic << " original-code synthetic-table angle cases; missing-table guards passed\n";
        if (argc >= 4) {
            const auto owner = owner_steps(argv[3]);
            std::cout << compare(argv[2], &owner) << " original-owner-table angle cases passed\n";
            if (argc == 5) {
                std::ifstream cooked(argv[4], std::ios::binary);
                require(static_cast<bool>(cooked), "cooked actor angle package unavailable");
                std::array<uint8_t, l3::kActorAnglePackageBytes + 1> bytes{};
                cooked.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
                const auto count = cooked.gcount();
                require(count >= 0, "failed angle package read");
                require(l3::load_l3actor_angle(bytes.data(), static_cast<size_t>(count)) == owner,
                        "verified cooked table changed original owner words");
                std::cout << "verified owner angle cooker/loader passed\n";
            }
        }
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << '\n';
        return 1;
    }
}
