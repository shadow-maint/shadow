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

An alternative would be to take a look at the CI workflow
[file](../../.github/workflows/runner.yml)
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

Alternatively, you can use the preconfigured container builders
to build, install, and test shadow in an isolated environment.

The container workflow supports two execution modes:

- Unprivileged (default)
- Privileged (opt-in, required for BTRFS, mounts, and similar tests)

### Single distribution build

```
ansible-playbook share/ansible/playbook-unprivileged.yml \
  -i share/ansible/inventory.ini \
  -e 'distribution=alpine'
```

**Note**: you'll need to install ansible to run this automation.

Or generate all of the images with the `container-build.sh` script, as if you
were running some of the CI checks locally:
### Multiple distributions

```
share/container-build.sh
```

## Privileged builds and tests

Most development and CI scenarios should use the default (unprivileged)
mode. It is sufficient for validating core functionality and avoids
unnecessary exposure to elevated capabilities.

Privileged mode is available for tests that require kernel-level
operations such as BTRFS subvolumes, loop devices, or filesystem mounts.

To run privileged builds across all distributions:

``` bash
sudo share/container-build.sh --privileged
```

Or for a single distribution:

``` bash
sudo ansible-playbook share/ansible/playbook-privileged.yml \
  -i share/ansible/inventory.ini \
  -e "distribution=fedora"
```

Privileged containers should be used in disposable environments (for
example, a VM or a temporary development system). While they are useful
for certain scenarios, most development workflows do not require them.

### Container troubleshooting

When working with containers for testing or development,
you may encounter issues.
Here are common troubleshooting steps:

**Post-test inspection:**
- **Container persistence**: after tests complete, containers are left running
  to allow inspection of the test environment and debugging of any failures.
  This enables you to examine logs, file states, and system configuration
  that existed when tests ran.

**Container management:**
If using a privileged container, you will need to be root to execute these commands.
- **List containers**: `docker ps -a` to see all containers and their status.
- **Access container**: `docker exec -it <container-name> bash` to get shell access.
- **Container logs**: `docker logs <container-name>` to view container output.
- **Remove containers**: `docker rm <container-name>` to clean up stopped containers.

**Common issues:**
- **Container not found**: ensure you've run the Ansible playbook
  to create the required containers.
- **Permission issues**: verify the container has proper privileges
  for user/group operations.
- **Network connectivity**: check that containers can communicate
  if tests involve network operations.
- **Resource constraints**: ensure sufficient disk space and memory
  for container operations.
