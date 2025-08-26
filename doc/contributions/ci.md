# Continuous Integration (CI)

Shadow runs a CI workflow every time a pull-request (PR) is updated. This
workflow contains several checks to assure the quality of the project, and
only pull-requests with green results are merged.

## Build & install

The project is built & installed on various distributions. The workflow can
be triggered locally by following the instructions specified in the
[Build & install](build_install.md#containers) page.

## System tests

The project runs system tests to verify functionality
across different environments
using two complementary approaches:

### Bash system tests

Legacy Bash system tests run on Ubuntu in a VM environment.
These provide coverage for Ubuntu-specific scenarios and legacy test cases.
You can run this step locally by following the instructions provided
in the [Tests](tests.md#bash-system-tests) page.

### Python system tests

The new Python system tests use pytest and pytest-mh,
running across multiple distributions (Fedora, Debian, Alpine, openSUSE)
in containerized environments.
These tests provide cross-distribution compatibility
and improved environment management compared to the Bash tests.

For local execution of Python system tests,
follow the instructions in the [Tests](tests.md#python-system-tests) page.

## Static code analysis

C and shell static code analysis is also executed. For that purpose
[CodeQL](https://codeql.github.com/) and
[Differential ShellCheck](https://github.com/marketplace/actions/differential-shellcheck)
are used.
