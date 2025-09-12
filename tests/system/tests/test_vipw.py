"""
Test vipw
"""

from __future__ import annotations

import pytest

from framework.roles.shadow import Shadow
from framework.topology import KnownTopology


@pytest.mark.topology(KnownTopology.Shadow)
def test_vipw__add_user_to_passwd(shadow: Shadow):
    """
    :title: Run vipw on passwd file and add a user
    :setup:
    :steps:
        1. Run vipw and add a user entry in passwd
        2. Check the user's passwd entry
    :expectedresults:
        1. vipw runs successfully
        2. User's passwd entry is correct
    :customerscenario: False
    """
    editor_script = "Go\ntuser:x:1000:1000::/home/tuser:/bin/bash\n:wq"
    shadow.vipw(editor_script=editor_script)

    passwd_entry = shadow.tools.getent.passwd("tuser")
    assert passwd_entry is not None, "User should be found"
    assert passwd_entry.name == "tuser", "Incorrect username"
    assert passwd_entry.password == "x", "Incorrect password"
    assert passwd_entry.uid == 1000, "Incorrect UID"
    assert passwd_entry.gid == 1000, "Incorrect GID"
    assert passwd_entry.home == "/home/tuser", "Incorrect home"
    assert passwd_entry.shell == "/bin/bash", "Incorrect shell"


@pytest.mark.topology(KnownTopology.Shadow)
def test_vipw__add_user_to_shadow(shadow: Shadow):
    """
    :title: Run vipw on shadow file and add a user
    :setup:
    :steps:
        1. Run vipw and add a user entry in shadow
        2. Check the user's shadow entry
    :expectedresults:
        1. vipw runs successfully
        2. User's shadow entry is correct
    :customerscenario: False
    """
    editor_script = "Go\ntuser:!:20342:0:99999:7:::\n:wq!"
    shadow.vipw("-s", editor_script=editor_script)

    shadow_entry = shadow.tools.getent.shadow("tuser")
    assert shadow_entry is not None, "User should be found"
    assert shadow_entry.name == "tuser", "Incorrect username"
    assert shadow_entry.password is not None, "Password should not be None"
    assert shadow_entry.password == "!", "Incorrect password"
    assert shadow_entry.last_changed == 20342, "Incorrect last changed"
    assert shadow_entry.min_days == 0, "Incorrect min days"
    assert shadow_entry.max_days == 99999, "Incorrect max days"
    assert shadow_entry.warn_days == 7, "Incorrect warn days"


@pytest.mark.topology(KnownTopology.Shadow)
def test_vipw__add_group_to_group(shadow: Shadow):
    """
    :title: Run vipw on group file and add a group
    :setup:
    :steps:
        1. Run vipw and add a group entry in group
        2. Check the group's group entry
    :expectedresults:
        1. vipw runs successfully
        2. Group's shadow group is correct
    :customerscenario: False
    """
    editor_script = "Go\ntgroup:x:1000:\n:wq"
    shadow.vipw("-g", editor_script=editor_script)

    group_entry = shadow.tools.getent.group("tgroup")
    assert group_entry is not None, "Group should be found"
    assert group_entry.name == "tgroup", "Incorrect groupname"
    assert group_entry.gid == 1000, "Incorrect GID"


@pytest.mark.topology(KnownTopology.Shadow)
@pytest.mark.builtwith(shadow="gshadow")
def test_vipw__add_group_to_gshadow(shadow: Shadow):
    """
    :title: Run vipw on gshadow file and add a group
    :setup:
    :steps:
        1. Run vipw and add a gshadow entry in group
        2. Check the group's gshadow entry
    :expectedresults:
        1. vipw runs successfully
        2. Group's shadow gshadow is correct
    :customerscenario: False
    """
    editor_script = "Go\ntgroup:!::\n:wq!"
    shadow.vipw("-gs", editor_script=editor_script)

    gshadow_entry = shadow.tools.getent.gshadow("tgroup")
    assert gshadow_entry is not None, "Group should be found"
    assert gshadow_entry.name == "tgroup", "Incorrect groupname"
    assert gshadow_entry.password == "!", "Incorrect password"
