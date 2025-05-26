module;

#include <array>
#include <vector>
#include <string>
#include <ranges>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <cstdint>

module wire.serialization:sha256;

namespace wire {

    constexpr uint32_t c_SHA256Constants[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
        0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
        0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
        0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
        0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
        0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
        0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
        0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    constexpr static inline uint32_t rotr(uint32_t x, uint32_t n)
    {
        return (x >> n) | (x << (32 - n));
    }

    constexpr static inline uint32_t choose(uint32_t e, uint32_t f, uint32_t g)
    {
        return (e & f) ^ (~e & g);
    }

    constexpr static inline uint32_t majority(uint32_t a, uint32_t b, uint32_t c)
    {
        return (a & b) ^ (a & c) ^ (b & c);
    }

    constexpr static inline uint32_t sigma0(uint32_t x)
    {
        return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
    }

    constexpr static inline uint32_t sigma1(uint32_t x)
    {
        return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
    }

    constexpr static inline uint32_t gamma0(uint32_t x)
    {
        return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
    }

    constexpr static inline uint32_t gamma1(uint32_t x)
    {
        return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
    }

    std::string generateSHA256(const std::vector<uint8_t>& data)
    {
        uint64_t bitlen = data.size();
        std::vector<uint8_t> paddedData = data;

        paddedData.push_back(0x80);
        while ((paddedData.size() + 8) % 64 != 0)
            paddedData.push_back(0x00);

        for (int i = 7; i >= 0; i--)
            paddedData.push_back((bitlen >> (i * 8)) & 0xFF);

        uint32_t hash[8] = {
            0x6a09e667, 0xbb67ae85,
            0x3c6ef372, 0xa54ff53a,
            0x510e527f, 0x9b05688c,
            0x1f83d9ab, 0x5be0cd19
        };

        for (size_t chunkStart = 0; chunkStart < paddedData.size(); chunkStart += 64)
        {
            uint32_t messageSchedule[64] = {};

            for (size_t i = 0; i < 16; ++i)
            {
                messageSchedule[i] = (paddedData[chunkStart + i * 4] << 24) |
                    (paddedData[chunkStart + i * 4 + 1] << 16) |
                    (paddedData[chunkStart + i * 4 + 2] << 8) |
                    (paddedData[chunkStart + i * 4 + 3]);
            }

            for (int i = 16; i < 64; ++i)
            {
                messageSchedule[i] = gamma1(messageSchedule[i - 2]) +
                    messageSchedule[i - 7] +
                    gamma0(messageSchedule[i - 15]) +
                    messageSchedule[i - 16];
            }

            uint32_t A = hash[0];
            uint32_t B = hash[1];
            uint32_t C = hash[2];
            uint32_t D = hash[3];
            uint32_t E = hash[4];
            uint32_t F = hash[5];
            uint32_t G = hash[6];
            uint32_t H = hash[7];

            for (size_t i = 0; i < 64; i++)
            {
                uint32_t temp1 = H + sigma1(E) + choose(E, F, G) + c_SHA256Constants[i] + messageSchedule[i];
                uint32_t temp2 = sigma0(A) + majority(A, B, C);

                H = G;
                G = F;
                F = E;
                E = D + temp1;
                D = C;
                C = B;
                B = A;
                A = temp1 + temp2;
            }

            hash[0] += A;
            hash[1] += B;
            hash[2] += C;
            hash[3] += D;
            hash[4] += E;
            hash[5] += F;
            hash[6] += G;
            hash[7] += H;
        }

        std::stringstream ss;
        for (size_t i = 0; i < 8; i++)
            ss << std::hex << std::setw(8) << std::setfill('0') << hash[i];

        return ss.str();
    }

    std::string generateSHA256(std::string_view input)
    {
        std::vector<uint8_t> data(input.begin(), input.end());
        return generateSHA256(data);
    }

}
