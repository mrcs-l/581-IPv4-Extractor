# IPv4 Extractor (EECS 581)

Reads lines of text and extracts the first valid IPv4 address, with an optional `:port`, embedded in each line. All parsing is done by hand: no `stoi`/`strtol`/`sscanf`, no regex, and no `inet_*`.

## Build and run

```sh
make            # builds ./ipv4
./ipv4          # type lines; END to quit
make test       # unit tests + sample-run output check
```

If `make` isn't available: `g++ -std=c++17 -o ipv4 src/main.cpp src/ipv4.cpp`

## Layout

| Path | Contents |
|---|---|
| `src/ipv4.hpp`, `src/ipv4.cpp` | `extractIPv4` and its helpers |
| `src/main.cpp` | Input loop and output formatting |
| `tests/test_ipv4.cpp` | Table-driven unit tests for `extractIPv4` |
| `tests/sample_input.txt`, `tests/sample_expected.txt` | The assignment's sample run, used for exact output comparison |

## Interpretation notes

- A candidate token is a maximal run of digits, `.` and `:`. Each token must match `octet.octet.octet.octet[:port]` in full, or it is rejected; there are no partial matches.
- If a line contains several valid addresses, the first one is reported.
- End of input (Ctrl-D) is treated the same as `END`. A trailing `\r` (Windows line ending) is ignored.
