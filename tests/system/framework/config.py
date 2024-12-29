from __future__ import annotations

from typing import Type

from pytest_mh import MultihostConfig, MultihostDomain, MultihostHost, MultihostRole

__all__ = [
    "ShadowMultihostConfig",
    "ShadowMultihostDomain",
]


class ShadowMultihostConfig(MultihostConfig):
    @property
    def id_to_domain_class(self) -> dict[str, Type[MultihostDomain]]:
        """
        All domains are mapped to :class:`ShadowMultihostDomain`.

        :rtype: Class name.
        """
        return {"*": ShadowMultihostDomain}


class ShadowMultihostDomain(MultihostDomain[ShadowMultihostConfig]):
    @property
    def role_to_host_class(self) -> dict[str, Type[MultihostHost]]:
        """
        Map roles to classes:

        * shadow to ShadowHost

        :rtype: Class name.
        """
        from .hosts.shadow import ShadowHost

        return {
            "shadow": ShadowHost,
        }

    @property
    def role_to_role_class(self) -> dict[str, Type[MultihostRole]]:
        """
        Map roles to classes:

        * shadow to Shadow

        :rtype: Class name.
        """
        from .roles.shadow import Shadow

        return {
            "shadow": Shadow,
        }
