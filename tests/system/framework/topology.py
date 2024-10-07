"""Predefined well-known topologies."""

from __future__ import annotations

from enum import unique
from typing import final

from pytest_mh import KnownTopologyBase, KnownTopologyGroupBase, Topology, TopologyDomain, TopologyMark

__all__ = [
    "KnownTopology",
    "KnownTopologyGroup",
]


@final
@unique
class KnownTopology(KnownTopologyBase):
    """
    Well-known topologies that can be given to ``pytest.mark.topology``
    directly. It is expected to use these values in favor of providing
    custom marker values.

    .. code-block:: python
        :caption: Example usage

        @pytest.mark.topology(KnownTopology.Shadow)
        def test_ldap(shadow: Shadow):
            assert True
    """

    Shadow = TopologyMark(
        name="shadow",
        topology=Topology(TopologyDomain("shadow", shadow=1)),
        fixtures=dict(shadow="shadow.shadow[0]"),
    )


class KnownTopologyGroup(KnownTopologyGroupBase):
    """
    Groups of well-known topologies that can be given to ``pytest.mark.topology``
    directly. It is expected to use these values in favor of providing
    custom marker values.

    The test is parametrized and runs multiple times, once per each topology.

    .. code-block:: python
        :caption: Example usage (runs on Shadow topology)

        @pytest.mark.topology(KnownTopologyGroup.AnyProvider)
        def test_ldap(shadow: Shadow):
            assert True
    """

    AnyProvider = [KnownTopology.Shadow]
