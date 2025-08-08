"""
Test newusers
"""

from __future__ import annotations

import pytest

from framework.misc import days_since_epoch
from framework.roles.shadow import Shadow
from framework.topology import KnownTopology


@pytest.mark.topology(KnownTopology.Shadow)
def test_newusers__create_users_from_stdin(shadow: Shadow):
    """
    :title: Create multiple users from stdin
    :setup:
        1. Prepare user data
        2. Create users using newusers from stdin
    :steps:
        1. Check first user passwd entry
        2. Check first user shadow entry
        3. Check first user group entry
        4. Check first user gshadow entry
        5. Check first user home folder
        6. Check second user passwd entry
        7. Check second user shadow entry
        8. Check second user group entry
        9. Check second user gshadow entry
        10. Check second user home folder
    :expectedresults:
        1. First user passwd entry exists with correct attributes
        2. First user shadow entry exists with correct attributes
        3. First user group entry exists with correct attributes
        4. First user gshadow entry exists with correct attributes
        5. First user home folder exists
        6. Second user passwd entry exists with correct attributes
        7. Second user shadow entry exists with correct attributes
        8. Second user group entry exists with correct attributes
        9. Second user gshadow entry exists with correct attributes
        10. Second user home folder exists
    :customerscenario: False
    """
    users_data = (
        "tuser1:Secret123:1001:1001:Test User One:/home/tuser1:/bin/bash\n"
        "tuser2:Secret123:1002:1002:Test User Two:/home/tuser2:/bin/bash"
    )

    shadow.newusers(users_data=users_data)

    result = shadow.tools.getent.passwd("tuser1")
    assert result is not None, "tuser1 user should be found in passwd"
    assert result.name == "tuser1", "Incorrect username"
    assert result.password == "x", "Incorrect password"
    assert result.uid == 1001, "Incorrect UID"
    assert result.gid == 1001, "Incorrect GID"
    assert result.gecos == "Test User One", "Incorrect GECOS"
    assert result.home == "/home/tuser1", "Incorrect home folder"
    assert result.shell == "/bin/bash", "Incorrect shell"

    result = shadow.tools.getent.shadow("tuser1")
    assert result is not None, "tuser1 user should be found in shadow"
    assert result.name == "tuser1", "Incorrect username"
    assert result.password is not None, "Incorrect password"
    assert result.last_changed == days_since_epoch(), "Incorrect last changed"
    assert result.min_days == 0, "Incorrect min days"
    assert result.max_days == 99999, "Incorrect max days"
    assert result.warn_days == 7, "Incorrect warn days"

    result = shadow.tools.getent.group("tuser1")
    assert result is not None, "tuser1 group should be found"
    assert result.name == "tuser1", "Incorrect group name"
    assert result.gid == 1001, "Incorrect GID"

    if shadow.host.features["gshadow"]:
        result = shadow.tools.getent.gshadow("tuser1")
        assert result is not None, "tuser1 group should be found"
        assert result.name == "tuser1", "Incorrect group name"
        assert result.password == "*", "Incorrect password"

    assert shadow.fs.exists("/home/tuser1"), "tuser1 home folder should exist"

    result = shadow.tools.getent.passwd("tuser2")
    assert result is not None, "tuser2 user should be found in passwd"
    assert result.name == "tuser2", "Incorrect username"
    assert result.password == "x", "Incorrect password"
    assert result.uid == 1002, "Incorrect UID"
    assert result.gid == 1002, "Incorrect GID"
    assert result.gecos == "Test User Two", "Incorrect GECOS"
    assert result.home == "/home/tuser2", "Incorrect home folder"
    assert result.shell == "/bin/bash", "Incorrect shell"

    result = shadow.tools.getent.shadow("tuser2")
    assert result is not None, "tuser2 user should be found in shadow"
    assert result.name == "tuser2", "Incorrect username"
    assert result.password is not None, "Incorrect password"
    assert result.last_changed == days_since_epoch(), "Incorrect last changed"
    assert result.min_days == 0, "Incorrect min days"
    assert result.max_days == 99999, "Incorrect max days"
    assert result.warn_days == 7, "Incorrect warn days"

    result = shadow.tools.getent.group("tuser2")
    assert result is not None, "tuser2 group should be found"
    assert result.name == "tuser2", "Incorrect group name"
    assert result.gid == 1002, "Incorrect GID"

    if shadow.host.features["gshadow"]:
        result = shadow.tools.getent.gshadow("tuser2")
        assert result is not None, "tuser2 group should be found"
        assert result.name == "tuser2", "Incorrect group name"
        assert result.password == "*", "Incorrect password"

    assert shadow.fs.exists("/home/tuser2"), "tuser2 home folder should exist"
