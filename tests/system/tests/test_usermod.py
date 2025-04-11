"""
Test usermod
"""

from __future__ import annotations

import pytest

from framework.roles.shadow import Shadow
from framework.topology import KnownTopology


@pytest.mark.topology(KnownTopology.Shadow)
def test_usermod__rename_user(shadow: Shadow):
    """
    :title: Rename user
    :setup:
        1. Create user
        2. Rename user
    :steps:
        1. Check passwd entry
        2. Check shadow entry
        3. Check group entry
        4. Check gshadow entry
        5. Check home folder
    :expectedresults:
        1. passwd entry for the user exists and the attributes are correct
        2. shadow entry for the user exists and the attributes are correct
        3. group entry for the user exists and the attributes are correct
        4. gshadow entry for the user exists and the attributes are correct
        5. Home folder exists
    :customerscenario: False
    """
    shadow.useradd("tuser1")
    shadow.usermod("-l tuser2 tuser1")

    result = shadow.tools.getent.passwd("tuser2")
    assert result is not None, "User should be found"
    assert result.name == "tuser2", "Incorrect username"
    assert result.uid == 1000, "Incorrect UID"

    result = shadow.tools.getent.shadow("tuser2")
    assert result is not None, "User should be found"
    assert result.name == "tuser2", "Incorrect username"

    result = shadow.tools.getent.group("tuser1")
    assert result is not None, "Group should be found"
    assert result.name == "tuser1", "Incorrect groupname"
    assert result.gid == 1000, "Incorrect GID"

    if shadow.host.features["gshadow"]:
        result = shadow.tools.getent.gshadow("tuser1")
        assert result is not None, "User should be found"
        assert result.name == "tuser1", "Incorrect username"

    assert shadow.fs.exists("/home/tuser1"), "Home folder should be found"
