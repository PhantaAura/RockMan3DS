#pragma once
#include <cstdint>
#include <memory>
#include <vector>

namespace l3 {

// Immutable reads are shared across area registrations and the active area.
// Explicit replacement detaches; no writable pointer can change another owner.
class SharedBytePayload {
public:
    using Storage = std::vector<uint8_t>;
    using const_iterator = Storage::const_iterator;
    size_t size() const { return storage().size(); }
    bool empty() const { return storage().empty(); }
    const uint8_t* data() const { return storage().data(); }
    const_iterator begin() const { return storage().begin(); }
    const_iterator end() const { return storage().end(); }
    uint8_t back() const { return storage().back(); }
    template<class Iterator> void assign(Iterator first, Iterator last) {
        bytes_ = std::make_shared<const Storage>(first, last);
    }
    void clear() { bytes_.reset(); }
    // Used by malformed-payload construction as well as explicit replacement.
    void resize(size_t count) {
        auto replacement = std::make_shared<Storage>(storage());
        replacement->resize(count);
        bytes_ = std::move(replacement);
    }
    const void* storage_identity() const { return bytes_.get(); }
    std::weak_ptr<const Storage> lifetime() const { return bytes_; }
    bool operator==(const Storage& other) const { return storage() == other; }
    bool operator==(const SharedBytePayload& other) const {
        return bytes_ == other.bytes_ || storage() == other.storage();
    }
private:
    const Storage& storage() const {
        static const Storage empty;
        return bytes_ ? *bytes_ : empty;
    }
    std::shared_ptr<const Storage> bytes_;
};
} // namespace l3
