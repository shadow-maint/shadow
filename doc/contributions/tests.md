# Tests

Currently, shadow provides unit and system tests.

## Unit tests

Unit testing is provided by the [cmocka](https://cmocka.org/) framework. It's
recommended to read the
[basics](https://cmocka.org/talks/cmocka_unit_testing_and_mocking.pdf) and
[API](https://api.cmocka.org/) before writing any test case.

In addition, you can check [test_logind.c](../../tests/unit/test_logind.c) to
get a general idea on how to implement unit tests for shadow using cmocka.

You can execute unit tests by running:

```
make check
```

## System tests

These type of tests are written in shell. Unfortunately, the testing framework
is tightly coupled to the Ubuntu distribution and it can only be run in this
distribution. Besides, if anything fails during the execution the system can
be left in an unstable state. Taking that into account you shouldn't run this
workflow in your host machine, we recommend to use a disposable system like a
VM or a container instead.

You can execute system tests by running:

```
cd tests && ./run_all`.
```
