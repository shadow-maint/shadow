# Build & install

The following page explains how to build and install the shadow project.
Additional information on how to do this in a container environment is provided
at the end of the page.

## Local

### Dependency installation

This projects depends on other software packages that need to be installed
before building it. We recommend using the dependency installation commands
provided by the distributions to install them. Some examples below.

Debian:
```
apt-get build-dep shadow
```

Fedora:
```
dnf builddep shadow-utils
```

An alternative would be to take a look at the CI workflow [file](../../.github/workflows/runner.yml)
and get the package names from there. This has the advantage that it
also includes new dependencies needed for the development version
which might have not been present in the last release.

### Configure

The first step is to configure it. You can use the
`autogen.sh` script provided by the project. Example:

```
./autogen.sh --without-selinux --enable-man --with-yescrypt
```

### Build

The next step is to build the project:

```
make -j4
```

### Install

The last step is to install it. We recommend avoiding this step and using a
disposable system like a VM or a container instead.

```
make install
```

## Containers

Alternatively, you can use any of the preconfigured container images builders
to build and install shadow.

You can either generate a single image by running the following command from
the root folder of the project (i.e. Alpine):

```
ansible-playbook share/ansible/playbook.yml -i share/ansible/inventory.ini -e 'distribution=alpine'
```

**Note**: you'll need to install ansible to run this automation.

Or generate all of the images with the `container-build.sh` script, as if you
were running some of the CI checks locally:

```
share/container-build.sh
```
