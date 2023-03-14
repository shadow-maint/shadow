# Tests

Currently, shadow only provides system tests.

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
