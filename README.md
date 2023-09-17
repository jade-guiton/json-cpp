# A simple C++20 JSON parser and serializer

Not necessarily fast, but fairly simple and spec-compliant.

Requires a C++20-ready compiler and header files (notably the \<format\> header). You may have to tweak the Makefile to get it compiled.

## Makefile targets

- `make test` uses the test files from https://github.com/nst/JSONTestSuite to check spec compliance.

- `make out/validator` builds the `out/validator` binary. It takes a JSON file path as a command line argument, tries to parse the file, and returns a 0/2 status code depending on the result. Any parsing or encoding error is printed to stderr. This is used by `test.py` to run the test suite. (Status code 1 is skipped to allow distinguishing parser errors from crashes.)

- `make out/formatter` builds the `out/formatter` binary. It takes a JSON file path as a command line argument, parses the file, then serializes it back to standard output. This can be used to manually check round trips.

## Implementation choices

**Parser:**
- Has a maximum nesting depth (see `MAX_DEPTH` in `json.tpp`). This could be avoided at the cost of code complexity.
- Numbers are parsed into a `double` if they contain a period or exponent, and into an `int64_t` otherwise. Out-of-range values are rejected by the parser.
- Only input encoded with UTF-8 without BOM is supported. No attempt at encoding detection is done.
- Strings that describe invalid Unicode (a lone UTF-16 surrogate, for example) are rejected by the parser.

**Serializer:**
- No superfluous whitespace is added.
- Characters outside of the printable ASCII range in strings are always turned into a Unicode escape sequence, even when they could theoretically be output as-is. This means the output of the serializer is always valid ASCII.
- Doubles are currently serialized with 17 digits of precision systematically. This is always enough to round trip, but may not be very human-readable. For example, `0.1` becomes `0.10000000000000001`.
- Doubles that have a simple integer value are serialized with `.0` appended to them. This avoids them being parsed back as an integer.

The part of the test suite dealing with "implementation-defined" behavior can be run by setting the `CHECK_IMPL_DEFINED` constant to `True` in `test.py`. All such tests currently result in a parse error, except for `tests/i_structure_500_nested_arrays.json`, since the nesting limit is higher than 500.
