""" Automation of Shadow Utils tests """

from __future__ import print_function
import subprocess
import os
import pytest


class TestShadowUtilsErrors():
    """
    Automation of Shadow Utils tests
    """
    def test_error_2006_0032(self):
        """
        :title: Tests if shadow-utils are
         immune against bugs in 2006:0032
        :id: TC#46675
        :steps:
          1. Creating tmp directory
          2. Added two patches to shadow-utils for
          additional usermod flag
          3. The user foo must be included in both
          foo-group and foo-group2 groups
          4. usermod -p does not update sp_lstchg
        :expectedresults:
          1. Should succeed
          2. Should succeed
          3. Should succeed
          4. Should succeed
        """
        # Creating tmp directory
        mktmp = "mktemp -d"
        process = subprocess.Popen(mktmp.split(),
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
        stdout, stderr = process.communicate()
        assert stderr.decode() == ''
        # Added two patches to shadow-utils for
        # additional usermod flag
        foo_info = ["useradd foo",
                    "groupadd foo-group",
                    "usermod -G foo-group foo",
                    "groupadd foo-group2",
                    "usermod -G foo-group2 -a foo"]
        for i in foo_info:
            process = subprocess.Popen(i.split(),
                                       stdout=subprocess.PIPE,
                                       stderr=subprocess.PIPE)
            stdout, stderr = process.communicate()
            assert stderr.decode() == ''
        # The user foo must be included in both
        # foo-group and foo-group2 groups
        cmd_id = "id foo"
        process = subprocess.Popen(cmd_id.split(),
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
        stdout, stderr = process.communicate()
        assert stderr.decode() == ''
        for grp in ['foo-group', 'foo-group2']:
            assert grp in stdout.decode()
        del_info = ["groupdel foo-group", "groupdel foo-group2"]
        for i in del_info:
            process = subprocess.Popen(i.split(),
                                       stdout=subprocess.PIPE,
                                       stderr=subprocess.PIPE)
            stdout, stderr = process.communicate()
            assert stderr.decode() == ''
        # usermod -p does not update sp_lstchg
        pass_change = "chage -l foo"
        process = subprocess.Popen(pass_change.split(),
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
        stdout, stderr = process.communicate()
        first_pass = stdout.decode().split('\n')
        process = subprocess.Popen(["date", "-s", "-2 day"],
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
        stdout, stderr = process.communicate()
        assert stderr.decode() == ''
        process = subprocess.Popen("usermod -p foopass foo".split(),
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
        stdout, stderr = process.communicate()
        assert stderr.decode() == ''
        process = subprocess.Popen(["date", "-s", "+2 day"],
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
        stdout, stderr = process.communicate()
        assert stderr.decode() == ''
        pass_change = "chage -l foo"
        process = subprocess.Popen(pass_change.split(),
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
        stdout, stderr = process.communicate()
        second_pass = stdout.decode().split('\n')
        assert first_pass != second_pass
        del_user = "userdel -r foo"
        process = subprocess.Popen(del_user.split(),
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE)
        stdout, stderr = process.communicate()
        assert stderr.decode() == ''
