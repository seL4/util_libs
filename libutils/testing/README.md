# Libutils Testing

This directory contains some basic infrastructure for testing the functionality of this library. It
currently only tests the raw JSON printing functions. Please feel free to extend it to cover more
functionality.

## Running Tests

```bash
cmake .
make
PATH=${PATH}:$(pwd)/../scripts ./test-json
```
