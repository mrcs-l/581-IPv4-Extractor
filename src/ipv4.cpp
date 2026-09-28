#include "ipv4.hpp"

namespace {

// Plain comparisons instead of <cctype>: isdigit() on a negative char
// (any non-ASCII byte) is undefined behavior.
bool isDigit(char c) { return c >= '0' && c <= '9'; }

bool isTokenChar(char c) { return isDigit(c) || c == '.' || c == ':'; }

// Parses s[begin, end) as a decimal number: 1..maxDigits digits, no leading
// zero unless the number is exactly "0", and value <= maxValue.
// The length check happens before accumulating, so value cannot overflow.
bool parseNumber(const std::string& s, std::size_t begin, std::size_t end,
                 std::size_t maxDigits, unsigned long maxValue, unsigned long& value) {
    std::size_t len = end - begin;
    if (len == 0 || len > maxDigits) return false;
    if (len > 1 && s[begin] == '0') return false;

    value = 0;
    for (std::size_t i = begin; i < end; ++i) {
        if (!isDigit(s[i])) return false;
        value = value * 10 + static_cast<unsigned long>(s[i] - '0');
    }
    return value <= maxValue;
}

// Validates one whole token s[begin, end) against octet.octet.octet.octet[:port].
// No partial matches: the entire token must fit the grammar.
bool parseToken(const std::string& s, std::size_t begin, std::size_t end,
                unsigned long& address, int& port) {
    // Find the colon, if any; more than one is invalid.
    std::size_t colon = end;
    for (std::size_t i = begin; i < end; ++i) {
        if (s[i] == ':') {
            if (colon != end) return false;
            colon = i;
        }
    }

    // Everything before the colon must be exactly four octets.
    address = 0;
    int octets = 0;
    std::size_t pieceStart = begin;
    for (std::size_t i = begin; i <= colon; ++i) {
        if (i == colon || s[i] == '.') {
            unsigned long octet;
            if (!parseNumber(s, pieceStart, i, 3, 255, octet)) return false;
            address = (address << 8) | octet;
            ++octets;
            pieceStart = i + 1;
        }
    }
    if (octets != 4) return false;

    // If a colon is present, the port must be valid or the whole token fails.
    port = -1;
    if (colon != end) {
        unsigned long p;
        if (!parseNumber(s, colon + 1, end, 5, 65535, p)) return false;
        port = static_cast<int>(p);
    }
    return true;
}

}  // namespace

bool extractIPv4(const std::string& str, unsigned long& outAddress, int& outPort) {
    outAddress = 0;
    outPort = -1;

    // Scan maximal runs of digits/'.'/':'; the first run that validates wins.
    std::size_t i = 0;
    while (i < str.size()) {
        if (!isTokenChar(str[i])) {
            ++i;
            continue;
        }
        std::size_t start = i;
        while (i < str.size() && isTokenChar(str[i])) ++i;

        unsigned long address;
        int port;
        if (parseToken(str, start, i, address, port)) {
            outAddress = address;
            outPort = port;
            return true;
        }
    }
    return false;
}
