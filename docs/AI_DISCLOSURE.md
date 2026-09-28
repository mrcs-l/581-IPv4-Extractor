# AI Usage Disclosure — IPv4 Extractor (EECS 581)

## 1. General Disclosure

| Item | Detail |
|---|---|
| Tool | Claude Code (Anthropic's agentic CLI coding assistant) |
| Model | Claude Opus 5.5 (`claude-opus-5-5`) |
| Date(s) consulted | 2026-09-27 |
| How it was used | Prompted from the terminal inside this repository. The tool created and edited files directly; every change was reviewed before it was accepted. |

## 2. Design Decisions and Assumptions

These are places where the assignment is ambiguous, with the interpretation we chose and why.

| # | Question | Decision | Rationale |
|---|---|---|---|
| A1 | What counts as a "candidate token"? | A maximal run of consecutive characters from `{0-9 . :}`. Any other character ends the run. | The spec says only digits, `.`, and `:` can be part of a token and everything else is garbage. The sample `192a168.1.1.1` → `168.1.1.1` confirms that `a` splits the tokens. |
| A2 | A line contains more than one valid address. | Return the **first** token that validates, scanning left to right. | "Exactly one valid address may be extracted per line" is read as a limit on output, not a requirement that the line contain only one address. The sample above shows the scanner continues past a failed token. |

## 3. Prompt Log

### Prompt 1 — Project setup (2026-09-27)

**Prompt:**

> I want to build an IPv4 Extractor. Ask me any setup questions you need, such as initializing the repo. Then we'll discuss design decisions for your review.

**AI questions and my answers:**
- Repo location → a new subfolder, `581-IPv4-Extractor/`, rather than the whole course folder.
- Language → C++.
- Multiple valid addresses on one line → Recommended "first valid token wins."
- Disclosure logging → the AI keeps this prompt log up to date after each round, and I review it.

**What the AI did:** It ran `git init` (branch `main`), created the public GitHub repo `mrcs-l/581-IPv4-Extractor` with `gh` (after I logged in myself with `gh auth login`) and set it as `origin`, and created `.gitignore`, `docs/ASSIGNMENT.md`, and this file. No program code was generated in this step.

### Prompt 2 — Design planning, before any code (2026-09-27)

**Prompt:**

> Before writing any code, walk me through the design decisions and tradeoffs the requirements call for.

**What the AI produced:** A design proposal with tradeoffs covering tokenization, the validation strategy, overflow safety, the failure sentinel colliding with `0.0.0.0`, the `isdigit` pitfall with signed `char`, how `main` rebuilds the dotted form, input/EOF handling, file layout, and the testing strategy. It also listed the decisions that were still open, and suggested prompts for the next steps (see Prompt 3).

### Prompt 3 — Approve the design and build (2026-09-27)

**Prompt:**

> The design looks good given the recommendations. Keep it simple and avoid over-engineering for the scale of this project. Go ahead and build it using the workflow you suggested.

**Decisions:**
- EOF behaves like `END`.
- One trailing `\r` is stripped.
- Multi-file layout with a Makefile.
- The split-then-validate rules as proposed.

**Workflow prompts I adopted:**

- **3a.** "Before writing code, list every rejection rule from the spec and map each to the specific check in your design that enforces it."
- **3b.** "Write the test table first, from the spec only, before any implementation."
- **3c.** "Now implement `parseNumber` and `extractIPv4` per the agreed design. Constraints: no stoi/strtol/sscanf/regex/inet_*, digits accumulated by hand, and cast to unsigned char before any `<cctype>` call."
- **3d.** "Review your own code against the test table and the pitfalls from your design proposal; list anything you'd flag."

#### 3a — Rule → check traceability

| Spec rule | Enforced by |
|---|---|
| Only digits, `.`, and `:` can be in a token; everything else is garbage | `isTokenChar`: the tokenizer splits on any other character |
| No partial matches or truncation | Each maximal token is validated whole; it passes or fails, with no sub-search |
| Wrong octet count | `parseToken`: `octets != 4` after the address part is scanned means reject |
| Empty octet (`1..2.3.4`, `.1.2.3.4`, `1.2.3.4.`) | `parseNumber`: a length-0 piece is rejected |
| Octet 1–3 digits, value 0–255 | `parseNumber(..., 3, 255, ...)` |
| No leading zero unless the value is exactly 0 | `parseNumber`: `len > 1 && first == '0'` means reject |
| Port 1–5 digits, value 0–65535, same leading-zero rule | `parseNumber(..., 5, 65535, ...)` |
| If a colon is present, the port must be valid or the whole match fails | The port check failing makes `parseToken` return false; there is no fallback to address-only |
| A second colon | Colon scan: a second `:` means reject |
| A colon not directly after the 4th octet | The address part (before the colon) must have exactly 4 octets |
| A stray `.` or `:` next to an otherwise valid address | It is part of the same maximal token, so the token fails as a whole |
| Hand accumulation, no forbidden functions | `value = value * 10 + (c - '0')` in `parseNumber`; no `<cctype>` either (a plain `'0'..'9'` comparison, which also avoids the signed-`char` problem) |
| On failure, address = 0 and port = -1 | Set at the top of `extractIPv4`; overwritten only on a full match |

#### 3b — Tests written first

`tests/test_ipv4.cpp` was written from the spec only, before any implementation existed: 62 cases in 11 groups. The expected decimal values were computed by hand and then cross-checked against Python's `ipaddress` module, which is independent of our code; all 13 matched. `tests/sample_input.txt` and `tests/sample_expected.txt` were copied from the assignment's sample run, for an exact comparison of the program's output.

#### 3c — Implementation

`src/ipv4.cpp`, `src/ipv4.hpp`, `src/main.cpp`, and the `Makefile` were generated in a single pass following the agreed design. **Result: 62/62 unit tests passed and the sample-run output matched on the first attempt.**

#### 3d — Self-review

The assignment warns to be suspicious of code that works first time, so the AI planted deliberate bugs in a scratch copy to check whether the tests could catch them:

| Planted bug | Caught? |
|---|---|
| Leading-zero check removed | Yes (6 tests failed) |
| Range check removed | Yes (7) |
| Final octet-count check removed | Yes (4) |
| Port failure ignored (address kept) | Yes (9) |
| Outputs not reset on failure | Yes (37) |
| `int` shift instead of `unsigned long` | Yes (5, including 128.0.0.0) |
| **Digit-count limit removed** | **No: tests passed** |
| 5th-octet early guard removed | No: redundant code |
| Second-colon check removed | No: redundant code |
| Real `std::isdigit(char)` used | No: undetectable on macOS |

Two notes on this table:
- The AI's first attempt at the `isdigit` mutant was written wrongly: it was logically identical to the original, so its "pass" meant nothing. The AI noticed this and redid it with a real `std::isdigit`.
- The last four rows were investigated as described under Findings below.

**Findings:**
1. **Test gap (the important one).** The original overflow test used `99999999999999999999`, which wraps modulo 2⁶⁴ to a value that is still far above 255, so it was rejected by accident. A probe showed that without the digit-count limit, `18446744073709551617.1.1.1` (2⁶⁴+1) wraps to `1` and is **accepted as 1.1.1.1**. `1.2.3.4:18446744073709551696` is likewise accepted with port 80. The implementation already had the limit, so it was correct, but the tests could not have caught the bug. **Fix:** both wraparound inputs were added as tests (now 64). The same mutant then failed 2 tests.
2. **Redundant check.** The `octets == 4 ||` guard duplicated the final `octets != 4` check. It was removed, following my "avoid over-engineering" constraint.
3. **The second-colon check is also redundant**, because a leftover `:` fails the digit check anyway. It was **kept on purpose**: it is one line and maps directly to a rule in the spec.
4. **`isdigit` on a negative `char` can't be tested here.** Apple's libc tolerates it, but the C++ standard makes it undefined behaviour. The code avoids it by design (plain `'0'..'9'` comparisons), and this is recorded as a limitation of the tests.

**Manual I/O checks on `main`:**
- `0.0.0.0` → prints decimal 0 as a success.
- `1.2.3.4:0\r` → port 0 with the `\r` stripped.
- `end` and ` END` → Invalid (END must match exactly).
- `END\r` → terminates.
- EOF without END → "Program terminated."

All behaved as designed.

## 4. Code Attribution

| File / Section | Origin | Modifications and why |
|---|---|---|
| `src/ipv4.cpp`: `isDigit`, `isTokenChar`, `parseNumber`, `parseToken`, `extractIPv4` | AI-generated (Claude), following a design I reviewed and approved | Redundant `octets == 4` guard removed after mutation testing (3d, finding 2) |
| `src/ipv4.hpp` | AI-generated; the prototype and comment are copied from the assignment | None |
| `src/main.cpp` | AI-generated | None |
| `tests/test_ipv4.cpp` | AI-generated before the implementation (3b) | Two 2⁶⁴-wraparound cases added (3d, finding 1) |
| `tests/sample_*.txt` | Copied from the assignment's sample run | Prompt echo removed, since input is piped |
| `Makefile`, `README.md` | AI-generated | None |

## 5. Bugs Found in AI Output

- **Test-suite gap (3d, finding 1):** the overflow test couldn't detect a missing digit-count limit because of how the value wraps. Found by mutation testing and fixed by adding two tests.
- **Invalid mutant (3d):** the AI's first `isdigit` mutant was logically identical to the original code. The AI caught this itself and redid it.
