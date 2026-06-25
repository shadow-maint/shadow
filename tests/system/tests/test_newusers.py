"""
Test newusers
"""

from __future__ import annotations

import pytest

from framework.roles.shadow import Shadow
from framework.topology import KnownTopology


@pytest.mark.topology(KnownTopology.Shadow)
def test_newusers__create_users_from_stdin(shadow: Shadow):
    """
    :title: Create multiple users from stdin
    :setup:
        1. Prepare and create user data from stdin
    :steps:
        1. Check the first user's passwd, shadow, group, gshadow, and home folder
        2. Check second user's passwd, shadow, group, gshadow, and home folder
    :expectedresults:
        1. First user's passwd, shadow, group, gshadow, and home folders are correct
        2. Second user's passwd, shadow, group, gshadow, and home folders are correct
    :customerscenario: False
    """
    users_data = (
        "tuser1:Secret123:1001:1001:Test User One:/home/tuser1:/bin/bash\n"
        "tuser2:Secret123:1002:1002:Test User Two:/home/tuser2:/bin/bash"
    )

    shadow.newusers(users_data=users_data)

    passwd_entry = shadow.tools.getent.passwd("tuser1")
    assert passwd_entry is not None, "tuser1 user should be found in passwd"
    assert passwd_entry.name == "tuser1", "Incorrect username"
    assert passwd_entry.password == "x", "Incorrect password"
    assert passwd_entry.uid == 1001, "Incorrect UID"
    assert passwd_entry.gid == 1001, "Incorrect GID"
    assert passwd_entry.gecos == "Test User One", "Incorrect GECOS"
    assert passwd_entry.home == "/home/tuser1", "Incorrect home folder"
    assert passwd_entry.shell == "/bin/bash", "Incorrect shell"

    shadow_entry = shadow.tools.getent.shadow("tuser1")
    assert shadow_entry is not None, "tuser1 user should be found in shadow"
    assert shadow_entry.name == "tuser1", "Incorrect username"
    assert shadow_entry.password is not None, "Incorrect password"
    assert shadow_entry.min_days is None, "Incorrect min days"
    assert shadow_entry.max_days is None, "Incorrect max days"
    assert shadow_entry.warn_days is None, "Incorrect warn days"

    group_entry = shadow.tools.getent.group("tuser1")
    assert group_entry is not None, "tuser1 group should be found"
    assert group_entry.name == "tuser1", "Incorrect group name"
    assert group_entry.gid == 1001, "Incorrect GID"

    if shadow.host.features["gshadow"]:
        gshadow_entry = shadow.tools.getent.gshadow("tuser1")
        assert gshadow_entry is not None, "tuser1 group should be found"
        assert gshadow_entry.name == "tuser1", "Incorrect group name"
        assert gshadow_entry.password == "*", "Incorrect password"

    assert shadow.fs.exists("/home/tuser1"), "tuser1 home folder should exist"

    passwd_entry = shadow.tools.getent.passwd("tuser2")
    assert passwd_entry is not None, "tuser2 user should be found in passwd"
    assert passwd_entry.name == "tuser2", "Incorrect username"
    assert passwd_entry.password == "x", "Incorrect password"
    assert passwd_entry.uid == 1002, "Incorrect UID"
    assert passwd_entry.gid == 1002, "Incorrect GID"
    assert passwd_entry.gecos == "Test User Two", "Incorrect GECOS"
    assert passwd_entry.home == "/home/tuser2", "Incorrect home folder"
    assert passwd_entry.shell == "/bin/bash", "Incorrect shell"

    shadow_entry = shadow.tools.getent.shadow("tuser2")
    assert shadow_entry is not None, "tuser2 user should be found in shadow"
    assert shadow_entry.name == "tuser2", "Incorrect username"
    assert shadow_entry.password is not None, "Incorrect password"
    assert shadow_entry.min_days is None, "Incorrect min days"
    assert shadow_entry.max_days is None, "Incorrect max days"
    assert shadow_entry.warn_days is None, "Incorrect warn days"

    group_entry = shadow.tools.getent.group("tuser2")
    assert group_entry is not None, "tuser2 group should be found"
    assert group_entry.name == "tuser2", "Incorrect group name"
    assert group_entry.gid == 1002, "Incorrect GID"

    if shadow.host.features["gshadow"]:
        gshadow_entry = shadow.tools.getent.gshadow("tuser2")
        assert gshadow_entry is not None, "tuser2 group should be found"
        assert gshadow_entry.name == "tuser2", "Incorrect group name"
        assert gshadow_entry.password == "*", "Incorrect password"

    assert shadow.fs.exists("/home/tuser2"), "tuser2 home folder should exist"


@pytest.mark.topology(KnownTopology.Shadow)
def test_newusers__create_users_from_file(shadow: Shadow):
    """
    :title: Create multiple users using file input
    :setup:
        1. Prepare and create user data from input file
    :steps:
        1. Check the first user's passwd, shadow, group, gshadow, and home folder
        2. Check second user's passwd, shadow, group, gshadow, and home folder
    :expectedresults:
        1. First user's passwd, shadow, group, gshadow, and home folders are correct
        2. Second user's passwd, shadow, group, gshadow, and home folders are correct
    :customerscenario: False
    """
    temp_file = "/tmp/test_newusers_data.txt"
    users_data = (
        "tuser1:Secret123:1001:1001:Test User One:/home/tuser1:/bin/bash\n"
        "tuser2:Secret123:1002:1002:Test User Two:/home/tuser2:/bin/bash"
    )
    shadow.fs.write(temp_file, users_data)

    shadow.newusers(temp_file)

    passwd_entry = shadow.tools.getent.passwd("tuser1")
    assert passwd_entry is not None, "tuser1 user should be found in passwd"
    assert passwd_entry.name == "tuser1", "Incorrect username"
    assert passwd_entry.password == "x", "Incorrect password"
    assert passwd_entry.uid == 1001, "Incorrect UID"
    assert passwd_entry.gid == 1001, "Incorrect GID"
    assert passwd_entry.gecos == "Test User One", "Incorrect GECOS"
    assert passwd_entry.home == "/home/tuser1", "Incorrect home folder"
    assert passwd_entry.shell == "/bin/bash", "Incorrect shell"

    shadow_entry = shadow.tools.getent.shadow("tuser1")
    assert shadow_entry is not None, "tuser1 user should be found in shadow"
    assert shadow_entry.name == "tuser1", "Incorrect username"
    assert shadow_entry.password is not None, "Incorrect password"
    assert shadow_entry.min_days is None, "Incorrect min days"
    assert shadow_entry.max_days is None, "Incorrect max days"
    assert shadow_entry.warn_days is None, "Incorrect warn days"

    group_entry = shadow.tools.getent.group("tuser1")
    assert group_entry is not None, "tuser1 group should be found"
    assert group_entry.name == "tuser1", "Incorrect group name"
    assert group_entry.gid == 1001, "Incorrect GID"

    if shadow.host.features["gshadow"]:
        gshadow_entry = shadow.tools.getent.gshadow("tuser1")
        assert gshadow_entry is not None, "tuser1 group should be found"
        assert gshadow_entry.name == "tuser1", "Incorrect group name"
        assert gshadow_entry.password == "*", "Incorrect password"

    assert shadow.fs.exists("/home/tuser1"), "tuser1 home folder should exist"

    passwd_entry = shadow.tools.getent.passwd("tuser2")
    assert passwd_entry is not None, "tuser2 user should be found in passwd"
    assert passwd_entry.name == "tuser2", "Incorrect username"
    assert passwd_entry.password == "x", "Incorrect password"
    assert passwd_entry.uid == 1002, "Incorrect UID"
    assert passwd_entry.gid == 1002, "Incorrect GID"
    assert passwd_entry.gecos == "Test User Two", "Incorrect GECOS"
    assert passwd_entry.home == "/home/tuser2", "Incorrect home folder"
    assert passwd_entry.shell == "/bin/bash", "Incorrect shell"

    shadow_entry = shadow.tools.getent.shadow("tuser2")
    assert shadow_entry is not None, "tuser2 user should be found in shadow"
    assert shadow_entry.name == "tuser2", "Incorrect username"
    assert shadow_entry.password is not None, "Incorrect password"
    assert shadow_entry.min_days is None, "Incorrect min days"
    assert shadow_entry.max_days is None, "Incorrect max days"
    assert shadow_entry.warn_days is None, "Incorrect warn days"

    group_entry = shadow.tools.getent.group("tuser2")
    assert group_entry is not None, "tuser2 group should be found"
    assert group_entry.name == "tuser2", "Incorrect group name"
    assert group_entry.gid == 1002, "Incorrect GID"

    if shadow.host.features["gshadow"]:
        gshadow_entry = shadow.tools.getent.gshadow("tuser2")
        assert gshadow_entry is not None, "tuser2 group should be found"
        assert gshadow_entry.name == "tuser2", "Incorrect group name"
        assert gshadow_entry.password == "*", "Incorrect password"

    assert shadow.fs.exists("/home/tuser2"), "tuser2 home folder should exist"
