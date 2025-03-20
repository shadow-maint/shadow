"""
Test userdel
"""

from __future__ import annotations

import pytest

from framework.roles.shadow import Shadow
from framework.topology import KnownTopology


@pytest.mark.topology(KnownTopology.Shadow)
def test_userdel__homedir_removed(shadow: Shadow):
    """
    :title: Delete user and homedir
    :setup:
        1. Create user
        2. Delete the user and the homedir
    :steps:
        1. Check passwd entry
        2. Check shadow entry
        3. Check group entry
        4. Check gshadow entry
        5. Check home folder
    :expectedresults:
        1. passwd entry for the user doesn't exist
        2. shadow entry for the user doesn't exist
        3. group entry for the user doesn't exist
        4. gshadow entry for the user doesn't exist
        5. Home folder doesn't exist
    :customerscenario: False
    """
    shadow.useradd("tuser")
    shadow.userdel("-r tuser")

    result = shadow.tools.getent.passwd("tuser")
    assert result is None, "User should not be found"

    result = shadow.tools.getent.shadow("tuser")
    assert result is None, "User should not be found"

    result = shadow.tools.getent.group("tuser")
    assert result is None, "Group should not be found"

    result = shadow.tools.getent.gshadow("tuser1")
    assert result is None, "User should not be found"

    assert not shadow.fs.exists("/home/tuser"), "Home folder should not exist"
