"""
Test groupdel
"""

from __future__ import annotations

import pytest

from framework.roles.shadow import Shadow
from framework.topology import KnownTopology


@pytest.mark.topology(KnownTopology.Shadow)
def test_groupdel__delete_group(shadow: Shadow):
    """
    :title: Basic group deletion
    :setup:
        1. Create group
        2. Delete group
    :steps:
        1. Check group entry
        2. Check gshadow entry
    :expectedresults:
        1. group entry for the user doesn't exist
        2. gshadow entry for the user doesn't exist
    :customerscenario: False
    """
    shadow.groupadd("tgroup")
    shadow.groupdel("tgroup")

    result = shadow.tools.getent.group("tgroup")
    assert result is None, "Group should not be found"

    result = shadow.tools.getent.gshadow("tgroup")
    assert result is None, "Group should not be found"
