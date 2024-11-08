# Configuration file for multihost tests.

from __future__ import annotations

from pytest_mh import MultihostPlugin

from framework.config import ShadowMultihostConfig

# Load additional plugins
pytest_plugins = (
    "pytest_mh",
    "pytest_ticket",
)


def pytest_plugin_registered(plugin) -> None:
    if isinstance(plugin, MultihostPlugin):
        plugin.config_class = ShadowMultihostConfig
