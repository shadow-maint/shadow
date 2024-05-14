Role Name
=========

Build, install and test package.

Example Playbook
----------------

Usage example:

    - hosts: builder
      connection: podman
      gather_facts: false
      roles:
        - role: ci_run

License
-------

BSD

Author Information
------------------

Iker Pedrosa <ipedrosa@redhat.com>
