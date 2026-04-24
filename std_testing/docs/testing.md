# Standard Library Testing

## Using the test runner

Configure the test runner by including the `EXT_TESTS` option when configuring
the project using CMake.

```
cmake [...] -DEXT_TESTS=ON
```

## Running tests

Test files can be run using the compiled test runner, compiled as the `test`
executable. Test files can then be passed in using the `-f` flag.

```
test -ftest1 --file=test2
```

Test files are compiled C binaries that are written using the `testing.h`
library.

## Writing tests

A standard test file should be comprised of:

The include directive for this testing header file, which is found under the
`include/extension_tests` directory.

```
#include "std/testing.h"
```

A set of test functions. Test functions are functions that are defined using the
test macros in `testing.h`, and are _registered_ in the test files main function.
Test functions can be added with the TEST macro:

```
TEST(test_name) { ... }
```

`test_name` must be a valid function name.

The body of a testing function is written using verification macros provided by
`testing.h`.
A simple test function might look like this:

```
TEST(testAddFunction) {
 int act = add(3, 2);
 IS_TRUE(act == 5);
}
```

The main function initializes the test data using `INIT()`, registers the defined
tests using `RUN(test)` or `SKIP(test)`, then sends the test data back to the runner using
`CONCLUDE()`. A main function should generally look like this:

```
int main() {
 INIT();

 RUN(test1);
 RUN(test2);
 RUN(test3);

 CONCLUDE();
}
```
