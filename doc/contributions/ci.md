# Continuous Integration (CI)

Shadow runs a CI workflow every time a pull-request (PR) is updated. This
workflow contains several checks to assure the quality of the project, and
only pull-requests with green results are merged.

## Build & install

The project is built & installed on Ubuntu, Alpine, Debian and Fedora. The last
three distributions are built & installed on containers, and the workflow can
be triggered locally by following the instructions specified in the
[Build & install](build_install.md#containers) page.

## System tests

The project is tested on Ubuntu. For that purpose it is built & installed in
this distribution in a VM. You can run this step locally by following the
instructions provided in the [Tests](tests.md#system-tests) page.

## Static code analysis

C and shell static code analysis is also executed. For that purpose
[CodeQL](https://codeql.github.com/) and
[Differential ShellCheck](https://github.com/marketplace/actions/differential-shellcheck)
are used.
