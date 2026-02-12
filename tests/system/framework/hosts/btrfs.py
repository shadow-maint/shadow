from __future__ import annotations

import uuid
from contextlib import contextmanager
from typing import Dict, List, Optional

from pytest_mh import MultihostHost, MultihostUtility


class Btrfs(MultihostUtility[MultihostHost]):
    """
    Utility for managing Btrfs subvolumes and loopback‑mounted Btrfs filesystems.

    Each created resource is tracked and cleaned up immediately when its context
    manager exits. The teardown() method acts as a safety net, cleaning any
    leftover resources if the normal cleanup was skipped.
    """

    def __init__(self, host: MultihostHost) -> None:
        super().__init__(host)
        self._resources: List[Dict[str, Optional[str]]] = []

    def subvolumes(self, path: str) -> List[str]:
        result = self.host.conn.run(f"btrfs subvolume list {path}")
        if result.rc != 0:
            raise RuntimeError(f"Failed to list Btrfs subvolumes under {path}: {result.stderr}")
        names = []
        for line in result.stdout.strip().splitlines():
            if " path " in line:
                names.append(line.split(" path ", 1)[1].strip())
        return names

    def subvolume_exists(self, path: str, name: str) -> bool:
        return name in self.subvolumes(path)

    @contextmanager
    def loopback_mount(self, mountpoint: str, size: str = "128M"):
        image_path = f"/tmp/btrfs-{uuid.uuid4().hex}.img"
        resource: Dict[str, Optional[str]] = {
            "mountpoint": mountpoint,
            "image_path": image_path,
            "loop_dev": None,
        }

        setup_script = f"""
            set -e
            mountpoint -q {mountpoint} && umount {mountpoint} || true
            [ -e /dev/loop-control ] || mknod /dev/loop-control c 10 237
            for i in 0 1 2 3 4 5 6 7; do
                [ -b /dev/loop$i ] || mknod /dev/loop$i b 7 $i
            done
            truncate -s {size} {image_path}
            mkfs.btrfs -f {image_path}
            LOOP_DEV=$(losetup --find --show {image_path})
            echo $LOOP_DEV > {image_path}.loop
            mount -t btrfs $LOOP_DEV {mountpoint}
        """

        self.logger.info(f"Setting up loop‑backed Btrfs at {mountpoint}")
        result = self.host.conn.run(setup_script)
        if result.rc != 0:
            raise RuntimeError(f"Failed to set up loop‑backed Btrfs at {mountpoint}: {result.stderr}")

        result = self.host.conn.run(f"cat {image_path}.loop")
        if result.rc != 0:
            raise RuntimeError(f"Failed to read loop device from {image_path}.loop: {result.stderr}")
        resource["loop_dev"] = result.stdout.strip()

        # Register the resource for safety net
        self._resources.append(resource)

        try:
            yield image_path
        finally:
            # Clean up only this resource immediately
            self._cleanup_resource(resource)

    def _cleanup_resource(self, resource: Dict[str, Optional[str]]) -> None:
        """Clean up a single resource."""
        mountpoint = resource["mountpoint"]
        image_path = resource["image_path"]
        cleanup_script = f"""
            set +e
            mountpoint -q {mountpoint} && umount {mountpoint}
            if [ -f {image_path}.loop ]; then
                LOOP_DEV=$(cat {image_path}.loop)
                losetup -d "$LOOP_DEV" 2>/dev/null || true
                rm -f {image_path}.loop
            fi
            rm -f {image_path}
        """
        self.logger.info(f"Cleaning up loop‑backed Btrfs at {mountpoint}")
        cleanup_result = self.host.conn.run(cleanup_script)
        if cleanup_result.rc != 0:
            self.logger.error(f"Cleanup of Btrfs resource {mountpoint} failed: {cleanup_result.stderr}")
        # Remove from tracking list
        if resource in self._resources:
            self._resources.remove(resource)

    def teardown(self) -> None:
        """
        Safety net: cleans up any Btrfs resources that were not properly released.
        This runs during the utility teardown phase, after all tests.
        """
        if not self._resources:
            return
        self.logger.warning(f"Cleaning up {len(self._resources)} leftover Btrfs resources")
        for res in self._resources[:]:
            self._cleanup_resource(res)
