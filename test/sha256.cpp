#include <climits>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <array>
#include <vector>

/* assure 1 byte consists of 8 bits */
static_assert(CHAR_BIT == 8);

using word = std::uint32_t;
using dword = std::uint64_t;

constexpr unsigned word_bytes = 4;
constexpr unsigned word_bits = word_bytes * CHAR_BIT;
constexpr unsigned block_bytes = 64;
constexpr unsigned block_bits = block_bytes * CHAR_BIT;
constexpr std::array<word, 8> H0 = {
    0x6a09e667,
    0xbb67ae85,
    0x3c6ef372,
    0xa54ff53a,
    0x510e527f,
    0x9b05688c,
    0x1f83d9ab,
    0x5be0cd19
};
constexpr std::array<word, 64> K = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

template<unsigned N>
word SHR(const word x) noexcept
{
    static_assert(N < word_bits);
    return x >> N;
}
template<unsigned N>
word ROTR(const word x) noexcept
{
    static_assert(N < word_bits);
    if constexpr (N == 0) {
        return x;
    } else {
        return (x >> N) | (x << (word_bits - N));
    }
}
word Ch(const word x, const word y, const word z) noexcept
{
    return (x & y) ^ (~x & z);
}
word Maj(const word x, const word y, const word z) noexcept
{
    return (x & y) ^ (x & z) ^ (y & z);
}
word Sigma0(const word x) noexcept
{
    return ROTR<2>(x) ^ ROTR<13>(x) ^ ROTR<22>(x);
}
word Sigma1(const word x) noexcept
{
    return ROTR<6>(x) ^ ROTR<11>(x) ^ ROTR<25>(x);
}
word sigma0(const word x) noexcept
{
    return ROTR<7>(x) ^ ROTR<18>(x) ^ SHR<3>(x);
}
word sigma1(const word x) noexcept
{
    return ROTR<17>(x) ^ ROTR<19>(x) ^ SHR<10>(x);
}

class message : private std::vector<unsigned char>
{
    using std::vector<unsigned char>::begin;
    using std::vector<unsigned char>::end;
public:
    template<class InputIterator>
    message(InputIterator first, InputIterator last) noexcept
        : std::vector<unsigned char>(first, last)
    {
        const auto mod_subtract = [mod = block_bits](const auto n, const auto m) {
            return ((n + mod) - m % mod) % mod;
        };
        const dword bytes = size();
        const dword l = bytes * CHAR_BIT;
        const dword k = mod_subtract(block_bits - 64, l + 1);
        resize((l + 1 + k + 64) / CHAR_BIT);
        data()[bytes] |= 0x80;
        for (unsigned i = 0; i < 8; ++i) {
            data()[size() - (8 - i)] |= (l >> ((7 - i) * CHAR_BIT)) & 0xff;
        }
    }
    dword N() const noexcept
    {
        return size() / block_bytes;
    }
    word get_word(const dword block_index, const unsigned word_index) const noexcept
    {
        const unsigned char*const word_ptr =
            data() + block_index * block_bytes + word_index * word_bytes;
        word ret = 0;
        for (unsigned i = 0; i < word_bytes; ++i) {
            ret |= (static_cast<word>(word_ptr[i]) << (word_bits - ((i + 1) * CHAR_BIT)));
        }
        return ret;
    }
};

int main(int argc, char* argv[])
{
    if (argc != 2) {
        std::cerr << "usage: sha-256.out FILEPATH" << std::endl;
        return EXIT_FAILURE;
    }
    std::ifstream ifs(argv[1], std::ios::binary);
    if (!ifs) {
        std::cerr << "COULD NOT OPEN FILE " << argv[1] << std::endl;
        return EXIT_FAILURE;
    }
    const auto M = message(
        std::istreambuf_iterator<char>(ifs),
        std::istreambuf_iterator<char>()
    );
    auto H = H0;
    __attribute__((lambdaize_loop))
    for (dword i = 0; i < M.N(); ++i) {
        word W[64];
        for (unsigned t = 0; t < 16; ++t) {
            W[t] = M.get_word(i, t);
        }
        for (unsigned t = 16; t < 64; ++t) {
            W[t] = sigma1(W[t - 2]) + W[t - 7] + sigma0(W[t - 15]) + W[t - 16];
        }
        word a = H[0], b = H[1], c = H[2], d = H[3], e = H[4], f = H[5], g = H[6], h = H[7];
        for (unsigned t = 0; t < 64; ++t) {
            const word T1 = h + Sigma1(e) + Ch(e, f, g) + K[t] + W[t];
            const word T2 = Sigma0(a) + Maj(a, b, c);
            h = g;
            g = f;
            f = e;
            e = d + T1;
            d = c;
            c = b;
            b = a;
            a = T1 + T2;
        }
        H[0] += a;
        H[1] += b;
        H[2] += c;
        H[3] += d;
        H[4] += e;
        H[5] += f;
        H[6] += g;
        H[7] += h;
    }
    for (word h : H) {
        std::cout << std::hex << std::setfill('0') << std::setw(word_bytes * 2) << h;
    }
    std::cout << std::endl;
    return EXIT_SUCCESS;
}
