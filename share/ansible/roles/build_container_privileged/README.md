Role Name
=========

Build privileged container images.

Role Variables
--------------

* `image[distribution]` defines the container image URL.

Example Playbook
----------------

Usage example:

    - hosts: localhost
      become: true
      roles:
        - role: build_container_privileged

License
-------

BSD

Author Information
------------------

Iker Pedrosa <ipedrosa@redhat.com>
Hadi Chokr <hadichokr@icloud.com>
