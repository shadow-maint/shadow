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

    group_entry = shadow.tools.getent.group("tgroup")
    assert group_entry is None, "Group should not be found"

    gshadow_entry = shadow.tools.getent.gshadow("tgroup")
    assert gshadow_entry is None, "Group should not be found"


@pytest.mark.topology(KnownTopology.Shadow)
def test_groupdel__delete_group_no_gshadow_entry(shadow: Shadow):
    """
    :title: Group deletion with no gshadow entry
    :setup:
        1. Create group
        2. Remove gshadow entry manually
    :steps:
        1. Check group entry
        2. Check gshadow entry
        3. Delete group
    :expectedresults:
        1. Group entry is found
        2. No gshadow entry is found
        3. Group is deleted
    :customerscenario: False
    """
    shadow.groupadd("tgroup")

    shadow.fs.sed("/^tgroup:/d", "/etc/gshadow")

    shadow.groupdel("tgroup")

    group_entry = shadow.tools.getent.group("tgroup")
    assert group_entry is None, "Group should not be found"

    if shadow.host.features["gshadow"]:
        gshadow_entry = shadow.tools.getent.gshadow("tgroup")
        assert gshadow_entry is None, "Group should not be found"
