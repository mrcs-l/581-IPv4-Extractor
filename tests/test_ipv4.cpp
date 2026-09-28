// Unit tests for extractIPv4. Build and run with: make test
#include <iostream>
#include <string>
#include "ipv4.hpp"

struct TestCase {
    std::string input;
    bool ok;
    unsigned long address;  // expected on success; must be 0 on failure
    int port;               // expected on success; must be -1 on failure/no port
};

static const TestCase kCases[] = {
    // --- Sample run from the assignment ---
    {"connecting to 192.168.1.1 now",       true,  3232235777UL, -1},
    {"server=10.0.0.255:8080end",           true,  167772415UL,  8080},
    {"192a168.1.1.1",                       true,  2818638081UL, -1},
    {"192.168.1.1.",                        false, 0, -1},
    {"Connection from 192.168.1.1 refused", true,  3232235777UL, -1},
    {"192.168.01.1",                        false, 0, -1},
    {"1.2.3.4:99999",                       false, 0, -1},
    {"12.34.56",                            false, 0, -1},
    {"no number here",                      false, 0, -1},

    // --- Octet range and boundaries ---
    {"0.0.0.0",             true,  0UL,          -1},  // valid, value collides with failure sentinel
    {"255.255.255.255",     true,  4294967295UL, -1},
    {"128.0.0.0",           true,  2147483648UL, -1},  // high bit set: catches signed-shift bugs
    {"127.0.0.1",           true,  2130706433UL, -1},
    {"256.1.1.1",           false, 0, -1},
    {"1.1.1.256",           false, 0, -1},
    {"999.1.1.1",           false, 0, -1},
    {"1000.1.1.1",          false, 0, -1},             // 4-digit octet
    {"99999999999999999999.1.1.1", false, 0, -1},      // would overflow if accumulated unchecked
    {"18446744073709551617.1.1.1", false, 0, -1},      // 2^64+1: wraps to 1 without a digit-count limit

    // --- Leading zeros ---
    {"01.1.1.1",            false, 0, -1},
    {"1.1.1.00",            false, 0, -1},
    {"1.1.1.000",           false, 0, -1},
    {"1.1.1.0",             true,  16843008UL,   -1},
    {"10.0.0.1",            true,  167772161UL,  -1},  // trailing zero is fine
    {"100.1.1.1",           true,  1677787393UL, -1},

    // --- Octet count / empty octets ---
    {"1.2.3",               false, 0, -1},
    {"1.2.3.4.5",           false, 0, -1},             // no truncating to 1.2.3.4
    {"1..2.3",              false, 0, -1},
    {"1.2..3.4",            false, 0, -1},
    {"1.2.3.",              false, 0, -1},
    {".1.2.3.4",            false, 0, -1},
    {"1234",                false, 0, -1},

    // --- Port rules ---
    {"1.2.3.4:0",           true,  16909060UL, 0},     // port 0 is valid, distinct from -1
    {"1.2.3.4:65535",       true,  16909060UL, 65535},
    {"1.2.3.4:80",          true,  16909060UL, 80},
    {"1.2.3.4:65536",       false, 0, -1},
    {"1.2.3.4:123456",      false, 0, -1},             // 6 digits
    {"1.2.3.4:99999999999999999999", false, 0, -1},
    {"1.2.3.4:18446744073709551696", false, 0, -1},    // 2^64+80: wraps to 80 without a digit-count limit
    {"1.2.3.4:",            false, 0, -1},             // colon with empty port
    {"1.2.3.4:080",         false, 0, -1},
    {"1.2.3.4:00",          false, 0, -1},

    // --- Colon / period placement ---
    {"1.2.3.4::80",         false, 0, -1},
    {"1.2.3.4:80:",         false, 0, -1},
    {"1.2.3.4:80:90",       false, 0, -1},
    {"1.2:3.4",             false, 0, -1},
    {":1.2.3.4",            false, 0, -1},
    {"1.2.3.4:80.",         false, 0, -1},
    {"1.2.3.4.:80",         false, 0, -1},
    {"1.2.3:4",             false, 0, -1},

    // --- Garbage around the token ---
    {"ip=1.2.3.4;",         true,  16909060UL, -1},
    {"-1.2.3.4-5",          true,  16909060UL, -1},    // '-' is garbage, not part of the token
    {"1.2.3.4 :80",         true,  16909060UL, -1},    // space splits; ":80" is its own (invalid) token
    {"[1.2.3.4]:80",        true,  16909060UL, -1},    // ']' splits; port not attached
    {"\t1.2.3.4\t",         true,  16909060UL, -1},

    // --- Multiple candidates: first valid token wins ---
    {"from 1.1.1.1 to 2.2.2.2",      true, 16843009UL,  -1},
    {"bad 1.2.3.256 good 5.6.7.8",   true, 84281096UL,  -1},
    {"12:30:45 host 10.1.2.3:22",    true, 167838211UL, 22},
    {"1.2.3.4:99999 then 5.6.7.8",   true, 84281096UL,  -1},  // failed port rejects first token entirely

    // --- Empty / nothing useful ---
    {"",                    false, 0, -1},
    {"...:::",              false, 0, -1},
    {"   ",                 false, 0, -1},

    // --- Non-ASCII bytes (negative as signed char) ---
    {"\xc3\xa9 1.2.3.4 \xc3\xbc", true, 16909060UL, -1},
    {"\xff" "1.2.3.4" "\xff",     true, 16909060UL, -1},
};

int main() {
    int failures = 0;
    int total = 0;
    for (const TestCase& tc : kCases) {
        ++total;
        // Pre-load junk so we can tell whether failure resets the outputs.
        unsigned long address = 12345;
        int port = 777;
        bool ok = extractIPv4(tc.input, address, port);
        if (ok != tc.ok || address != tc.address || port != tc.port) {
            ++failures;
            std::cout << "FAIL: \"" << tc.input << "\"\n"
                      << "  expected ok=" << tc.ok << " address=" << tc.address << " port=" << tc.port << "\n"
                      << "  actual   ok=" << ok << " address=" << address << " port=" << port << "\n";
        }
    }
    std::cout << (total - failures) << "/" << total << " tests passed\n";
    return failures == 0 ? 0 : 1;
}
