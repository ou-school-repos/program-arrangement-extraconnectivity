#ifndef STAR_SWEEP_CHECKSUM_HPP
#define STAR_SWEEP_CHECKSUM_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

#if defined(__SSE4_2__) && (defined(__x86_64__) || defined(__i386__))
#include <nmmintrin.h>
#define STAR_SWEEP_CRC32C_HW 1
#endif

namespace star_sweep {

// CRC32C with the SSE4.2 instruction when the target enables it, and a
// dependency-free table fallback otherwise. This protects metadata against
// accidental truncation/corruption; it is not an adversarial hash.
class Crc32c {
  public:
    void reset() { value_ = 0xffffffffU; }

    void update(const void *data, std::size_t length) {
        const auto *bytes = static_cast<const std::uint8_t *>(data);
#if defined(STAR_SWEEP_CRC32C_HW)
        while (length >= sizeof(std::uint64_t)) {
            std::uint64_t word = 0;
            std::memcpy(&word, bytes, sizeof(word));
            value_ = static_cast<std::uint32_t>(_mm_crc32_u64(value_, word));
            bytes += sizeof(word);
            length -= sizeof(word);
        }
#endif
        while (length != 0) {
            value_ = table()[(value_ ^ *bytes) & 0xffU] ^ (value_ >> 8);
            ++bytes;
            --length;
        }
    }

    std::uint32_t digest() const { return value_ ^ 0xffffffffU; }

  private:
    static const std::array<std::uint32_t, 256> &table() {
        static const std::array<std::uint32_t, 256> values = [] {
            std::array<std::uint32_t, 256> generated{};
            for (std::size_t index = 0; index < generated.size(); ++index) {
                std::uint32_t value = static_cast<std::uint32_t>(index);
                for (int bit = 0; bit < 8; ++bit)
                    value = (value & 1U) != 0 ? (value >> 1) ^ 0x82f63b78U
                                              : value >> 1;
                generated[index] = value;
            }
            return generated;
        }();
        return values;
    }

    std::uint32_t value_ = 0xffffffffU;
};

// Small dependency-free streaming XXH64 implementation for WAL payloads.
class Xxh64 {
  public:
    explicit Xxh64(std::uint64_t seed = 0) : seed_(seed) { reset(); }

    void reset() {
        total_length_ = 0;
        memory_size_ = 0;
        v1_ = seed_ + prime1_ + prime2_;
        v2_ = seed_ + prime2_;
        v3_ = seed_;
        v4_ = seed_ - prime1_;
    }

    void update(const void *data, std::size_t length) {
        const auto *bytes = static_cast<const std::uint8_t *>(data);
        total_length_ += length;

        if (memory_size_ + length < memory_.size()) {
            std::memcpy(memory_.data() + memory_size_, bytes, length);
            memory_size_ += length;
            return;
        }

        std::size_t offset = 0;
        if (memory_size_ != 0) {
            const std::size_t needed = memory_.size() - memory_size_;
            std::memcpy(memory_.data() + memory_size_, bytes, needed);
            consume(memory_.data());
            offset += needed;
            memory_size_ = 0;
        }

        if (length >= memory_.size()) {
            while (offset + memory_.size() <= length) {
                std::memcpy(memory_.data(), bytes + offset, memory_.size());
                consume(memory_.data());
                offset += memory_.size();
            }
        }

        if (offset < length) {
            memory_size_ = length - offset;
            std::memcpy(memory_.data(), bytes + offset, memory_size_);
        }
    }

    std::uint64_t digest() const {
        std::uint64_t result;
        if (total_length_ >= memory_.size()) {
            result = rotate_left(v1_, 1) + rotate_left(v2_, 7) +
                     rotate_left(v3_, 12) + rotate_left(v4_, 18);
            result = merge_round(result, v1_);
            result = merge_round(result, v2_);
            result = merge_round(result, v3_);
            result = merge_round(result, v4_);
        } else {
            result = seed_ + prime5_;
        }

        result += total_length_;
        const auto *bytes = memory_.data();
        std::size_t offset = 0;
        while (offset + sizeof(std::uint64_t) <= memory_size_) {
            const std::uint64_t value = read64(bytes + offset);
            result ^= round(0, value);
            result = rotate_left(result, 27) * prime1_ + prime4_;
            offset += sizeof(std::uint64_t);
        }
        while (offset + sizeof(std::uint32_t) <= memory_size_) {
            result ^=
                static_cast<std::uint64_t>(read32(bytes + offset)) * prime1_;
            result = rotate_left(result, 23) * prime2_ + prime3_;
            offset += sizeof(std::uint32_t);
        }
        while (offset < memory_size_) {
            result ^= bytes[offset] * prime5_;
            result = rotate_left(result, 11) * prime1_;
            ++offset;
        }
        result ^= result >> 33;
        result *= prime2_;
        result ^= result >> 29;
        result *= prime3_;
        result ^= result >> 32;
        return result;
    }

  private:
    static constexpr std::uint64_t prime1_ = 11400714785074694791ULL;
    static constexpr std::uint64_t prime2_ = 14029467366897019727ULL;
    static constexpr std::uint64_t prime3_ = 1609587929392839161ULL;
    static constexpr std::uint64_t prime4_ = 9650029242287828579ULL;
    static constexpr std::uint64_t prime5_ = 2870177450012600261ULL;

    static std::uint64_t rotate_left(std::uint64_t value, unsigned count) {
        return (value << count) | (value >> (64 - count));
    }

    static std::uint64_t read64(const std::uint8_t *bytes) {
        std::uint64_t value;
        std::memcpy(&value, bytes, sizeof(value));
        return value;
    }

    static std::uint32_t read32(const std::uint8_t *bytes) {
        std::uint32_t value;
        std::memcpy(&value, bytes, sizeof(value));
        return value;
    }

    static std::uint64_t round(std::uint64_t accumulator, std::uint64_t input) {
        accumulator += input * prime2_;
        accumulator = rotate_left(accumulator, 31);
        return accumulator * prime1_;
    }

    static std::uint64_t merge_round(std::uint64_t accumulator,
                                     std::uint64_t value) {
        accumulator ^= round(0, value);
        return accumulator * prime1_ + prime4_;
    }

    void consume(const std::uint8_t *bytes) {
        v1_ = round(v1_, read64(bytes));
        v2_ = round(v2_, read64(bytes + 8));
        v3_ = round(v3_, read64(bytes + 16));
        v4_ = round(v4_, read64(bytes + 24));
    }

    std::uint64_t seed_ = 0;
    std::uint64_t total_length_ = 0;
    std::uint64_t memory_size_ = 0;
    std::uint64_t v1_ = 0;
    std::uint64_t v2_ = 0;
    std::uint64_t v3_ = 0;
    std::uint64_t v4_ = 0;
    std::array<std::uint8_t, 32> memory_{};
};

} // namespace star_sweep

#endif
