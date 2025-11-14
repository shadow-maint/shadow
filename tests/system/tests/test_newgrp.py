"""
Test newgrp
"""

from __future__ import annotations

import pytest

from framework.roles.shadow import Shadow
from framework.topology import KnownTopology


@pytest.mark.topology(KnownTopology.Shadow)
def test_newgrp__change_to_new_group(shadow: Shadow):
    """
    :title: Change to a new group using newgrp
    :setup:
        1. Create test user and group
        2. Add user to the group
    :steps:
        1. Use newgrp to change to the new group
        2. Check that the group change worked
    :expectedresults:
        1. newgrp command succeeds
        2. Current group ID matches the new group
    :customerscenario: False
    """
    shadow.useradd("tuser")
    shadow.groupadd("tgroup")
    shadow.usermod("-a -G tgroup tuser")

    cmd, gid = shadow.newgrp("tgroup", run_as="tuser")
    assert cmd.rc == 0, "newgrp command should succeed"
    assert gid == 1001, f"Current GID should be {1001}, got {gid}"
