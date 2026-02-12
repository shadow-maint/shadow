# Tests

Currently, shadow provides unit tests and system tests.
System tests are split in two:
Bash system tests and Python system tests.

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

System tests verify the behavior of shadow utilities
in a complete system environment.
There are two types of system tests available.

### Bash system tests

These tests are written in shell
and are tightly coupled to the Ubuntu distribution.
They can only be run on Ubuntu,
and if anything fails during execution,
the system can be left in an unstable state.
You shouldn't run this workflow on your host machine—we recommend using a disposable system
like a VM or a container instead.

You can execute Bash system tests by running:

```
cd tests && ./run_all
```

### Python system tests

The new system tests use Python,
[pytest](https://pytest.org/)
and [pytest-mh](https://pytest-mh.readthedocs.io/).
The Python framework provides several advantages over Bash tests:

• **Cross-distribution compatibility**: works on all distributions in CI
  (Alpine, Debian, Fedora and openSUSE)
  unlike Bash tests which only run on Ubuntu.
  
• **Proper environment management**: ensures clean setup and teardown
  with environment restoration even when tests fail.
  
• **Improved test maintainability**: provides a rich, high-level API
  that reduces complex test code
  and makes tests easier to understand.
  
• **Automated artifact collection**: automatically collects logs and artifacts
  when tests finish (even if they fail).
  
• **Flexible infrastructure**: supports both VMs and containers for testing
  with local execution capabilities.
  
• **Future extensibility**: enables potential filtering by ticket
  or importance and other advanced features.

[Read more about the benefits and rationale](https://github.com/shadow-maint/shadow/issues/835).

**For all new contributions,
we recommend using the Python system test framework.**

## Privilege levels

Python system tests are divided into two categories.

### Unprivileged tests (default)

These tests run with standard container permissions and do not require
elevated capabilities. They are suitable for CI and should be the
preferred choice for new test development.

### Privileged tests (opt-in)

These tests require elevated capabilities and may perform operations
such as:

-   Mounting filesystems
-   Creating loop devices
-   Working with BTRFS subvolumes
-   Modifying kernel-visible system state

These capabilities operate at the kernel boundary. If isolation is
misconfigured, they may affect the host system directly.

Concrete risks include:

-   Accessing and modifying any file on the host system through bind
    mounts
-   Interfering with host services by consuming loop devices
-   Filling host disk space by creating large filesystem images
-   Leaving orphaned mount points that prevent clean system shutdown

Privileged tests are useful when validating functionality that
fundamentally depends on those capabilities. However, they should be
introduced only when no reasonable unprivileged alternative exists.

When writing new tests:

-   Prefer unprivileged designs.
-   Consider whether the behavior can be validated without mounts or
    device manipulation.
-   Use privileged tests as a targeted tool, not the default approach.

Privileged tests should be executed **only** inside:

-   A disposable virtual machine, or
-   A dedicated privileged container created for testing

They are not intended to run directly on a host system.

#### Contribution guidance

The framework is under active development
and welcomes contributions for missing features.
When contributing:

1. **Framework improvements**: enhance the infrastructure in `tests/system/framework/`.
2. **Proposing new tests**: place them in `tests/system/tests/`.
   Review existing examples in `tests/system/tests/`
   to understand the established patterns and conventions.
3. **Missing features**: if you're comfortable implementing missing pytest-mh features,
   contributions are encouraged.

When adding new coverage, prioritize unprivileged test designs.
Privileged tests should be added only when the feature under test cannot
be meaningfully validated otherwise.

Review examples in `tests/system/tests/privileged` before introducing
new privileged scenarios.

#### Running tests

##### Environment setup

A testing environment is required to run the system test framework.
While the same system can be used,
a disposable environment (container or VM) is recommended.

The default configuration relies on a pre-built container named `builder`
that must be available in your container environment.
This container contains:

- A complete shadow-utils installation
- All required system dependencies

**Setup steps:**

1. **Build the container environment**: follow the instructions in [build_install.md](build_install.md)
   to create the required `builder` container.
2. **Verify container availability**: ensure the `builder` container is accessible via Docker.

The default `mhc.yaml` configuration expects this container
to be available as `builder`.

To ensure dependency isolation and system protection,
run the tests within a Python virtual environment:

```bash
cd tests/system
python -m venv .venv
source .venv/bin/activate
pip install -r ./requirements.txt
```

##### Configuration

The default configuration located in `tests/system` should be enough
if you are using containers.
If you run another environment you'll need to tune `tests/system/mhc.yaml`.
Instructions are available in the
[pytest-mh mhc.yaml documentation](https://pytest-mh.readthedocs.io/en/latest/articles/mhc-yaml.html).

##### Execution

Run all Python system tests (privileged tests excluded by default):

```bash
pytest --mh-config=mhc.yaml --mh-lazy-ssh -v
```

By default, privileged tests are excluded via pytest markers to prevent accidental execution on
non-disposable systems.

Run tests in a file:

```bash
pytest --mh-config=mhc.yaml --mh-lazy-ssh -v tests/test_useradd.py
```

Run a single test case:

```bash
pytest --mh-config=mhc.yaml --mh-lazy-ssh -v -k test_useradd__add_user
```

To run privileged tests, ensure you are **inside a disposable VM or a privileged
container**, then run:

```bash
pytest -m "privileged" --mh-config=mhc-privileged.yaml --mh-lazy-ssh -v
```

Run both unprivileged and privileged tests:

```bash
pytest -m "privileged or not privileged" --mh-config=mhc.yaml --mh-lazy-ssh -v
```

**Command options explained:**
- `--mh-config=mhc.yaml`: specifies the multihost configuration file
- `--mh-lazy-ssh`: enables lazy SSH connections for better performance
- `-v`: verbose output showing individual test results
- `-k`: filters tests by name pattern
- `-m`: filters tests by marker expression

For additional running options,
see the [pytest-mh running tests documentation](https://pytest-mh.readthedocs.io/en/latest/articles/running-tests.html).

#### Advanced testing features

The Python testing framework includes several advanced features
for comprehensive testing:

**Test markers:**
- `@pytest.mark.topology(KnownTopology.Shadow)`: specifies the test topology.
- `@pytest.mark.builtwith("gshadow")`: skips tests when specific features are unavailable.
- `@pytest.mark.parametrize()`: extensive parameterized testing
  for edge cases and different inputs.

**Rich command parsing:**
All shadow command outputs are parsed into structured Python objects
using the `jc` library:
- `PasswdEntry`, `ShadowEntry`, `GroupEntry`, `GShadowEntry` for file entries
- `ID` command parsing with group membership verification
- Structured error handling and validation

#### Test development patterns

When writing new Python tests,
follow these established patterns:

**Basic test structure:**
```python
@pytest.mark.topology(KnownTopology.Shadow)
def test_command__specific_behavior(shadow: Shadow):
    """
    :title: Descriptive test title
    :setup:
        1. Setup steps
    :steps:
        1. Test steps
    :expectedresults:
        1. Expected outcomes
    :customerscenario: False
    """
    # Arrange
    shadow.useradd("testuser")

    # Act
    result = shadow.tools.getent.passwd("testuser")

    # Assert
    assert result is not None, "User should exist"
    assert result.name == "testuser", "Username should match"
```

**Distribution-specific testing:**
```python
if "Debian" in shadow.host.distro_name:
    assert result.shell == "/bin/sh"
else:
    assert result.shell == "/bin/bash"
```

**Parameterized testing:**
```python
@pytest.mark.parametrize(
    "input_date, expected_epoch",
    [
        ("1970-01-01", 0),
        ("2025-01-01", 20089),
        ("0", 0),
    ],
)
def test_date_handling(shadow: Shadow, input_date: str, expected_epoch: int):
    # Test implementation
```

#### Debugging information

**Test artifacts:**
- **Location**: `tests/system/artifacts/` directory.
- **Content**: automatically collected logs and system state information.
- **Collection**: happens automatically when tests finish,
  regardless of success or failure.

**System state management:**
The framework automatically manages shadow system files:
- **Backup**: complete backup of `/etc/passwd`, `/etc/shadow`, `/etc/group`, `/etc/gshadow`, `/home`
  before tests.
- **Restore**: automatic restoration after each test.
- **Verification**: file mismatch detection
  to ensure unmodified files remain unchanged.

**Common debugging steps:**
1. **Check container status**: ensure the `builder` container is running and accessible.
2. **Verify artifacts**: examine collected logs in the `artifacts/` directory.
3. **Manual testing**: access the container directly
   to reproduce issues manually.
4. **File state**: check if unexpected file modifications caused test failures.

#### Troubleshooting & FAQs

**Common pitfalls:**
- **Missing virtual environment**: always activate the Python virtual environment
  before running tests.
- **Mismatched environment**: ensure your `mhc.yaml` configuration matches your testing environment.
- **Container vs host confusion**: remember that tests run inside containers by default;
  adjust expectations accordingly.

**Tips for disposable environments:**
- Use containers or VMs to avoid system state pollution.
- Ensure proper cleanup in the framework between test runs.
- Verify container/VM has necessary privileges for user/group operations.
- Running privileged tests on the host system, even for local development is a **bad idea**.

For additional information about the testing framework,
see the [pytest-mh documentation](https://pytest-mh.readthedocs.io).
