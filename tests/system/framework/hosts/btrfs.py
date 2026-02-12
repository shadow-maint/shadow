from __future__ import annotations

import uuid
from contextlib import contextmanager
from typing import Dict, List, Optional

from pytest_mh import MultihostHost, MultihostReentrantUtility


class Btrfs(MultihostReentrantUtility[MultihostHost]):
    """
    Utility for managing Btrfs subvolumes and loopback‑mounted Btrfs filesystems.

    All created resources (loop devices, mount points, image files) are tracked
    and automatically cleaned up during test teardown.
    """

    def __init__(self, host: MultihostHost) -> None:
        super().__init__(host)
        # Each entry is a dict with keys: mountpoint, image_path, loop_dev
        # loop_dev may be None initially, then becomes a string after setup.
        self._resources: List[Dict[str, Optional[str]]] = []

    def __enter__(self) -> Btrfs:
        """Enter the runtime context for this utility."""
        return self

    def __exit__(self, exc_type, exc_val, exc_tb) -> None:
        """Exit the runtime context and clean up any leftover resources."""
        self.teardown()

    def subvolumes(self, path: str) -> List[str]:
        """
        Returns a list of Btrfs subvolume paths (relative to the mount point)
        under the given path.

        Args:
            path: Mount point of the Btrfs filesystem.

        Returns:
            List of subvolume paths (e.g. ['alice', 'bob/data']).

        Raises:
            RuntimeError: If the `btrfs subvolume list` command fails.
        """
        result = self.host.conn.run(f"btrfs subvolume list {path}")
        if result.rc != 0:
            raise RuntimeError(f"Failed to list Btrfs subvolumes under {path}: {result.stderr}")

        names = []
        for line in result.stdout.strip().splitlines():
            if " path " in line:
                # The line format: "ID 257 gen 8 top level 5 path home/alice"
                # We extract the part after " path "
                names.append(line.split(" path ", 1)[1].strip())
        return names

    def subvolume_exists(self, path: str, name: str) -> bool:
        """
        Check if a Btrfs subvolume with the given name exists under the mount point.

        The name is matched against the **relative path** as returned by
        ``btrfs subvolume list``. For a subvolume directly under the mount point,
        the name should be the last component only (e.g. "alice").

        Args:
            path: Mount point of the Btrfs filesystem.
            name: Subvolume name (relative path) to look for.

        Returns:
            True if the subvolume exists, False otherwise.
        """
        return name in self.subvolumes(path)

    @contextmanager
    def loopback_mount(self, mountpoint: str, size: str = "128M"):
        """
        Context manager that mounts a temporary loop‑backed Btrfs filesystem.

        The filesystem is created on a temporary image file under /tmp.
        On exit, the filesystem is unmounted, the loop device is detached,
        and the image file is removed.

        Args:
            mountpoint: Directory where the filesystem will be mounted.
            size:       Size passed to ``truncate`` (e.g. "256M", "1G").

        Yields:
            The path to the temporary image file (useful for debugging).

        Raises:
            RuntimeError: If any step of the setup fails.
        """
        image_path = f"/tmp/btrfs-{uuid.uuid4().hex}.img"
        resource: Dict[str, Optional[str]] = {
            "mountpoint": mountpoint,
            "image_path": image_path,
            "loop_dev": None,
        }

        setup_script = f"""
            set -e

            # Unmount if something is already there (idempotent)
            mountpoint -q {mountpoint} && umount {mountpoint} || true

            # Create loop control device if missing (common in containers)
            [ -e /dev/loop-control ] || mknod /dev/loop-control c 10 237
            for i in 0 1 2 3 4 5 6 7; do
                [ -b /dev/loop$i ] || mknod /dev/loop$i b 7 $i
            done

            # Create and format the backing file
            truncate -s {size} {image_path}
            mkfs.btrfs -f {image_path}

            # Find a free loop device and store its name
            LOOP_DEV=$(losetup --find --show {image_path})
            echo $LOOP_DEV > {image_path}.loop

            mount -t btrfs $LOOP_DEV {mountpoint}
        """

        cleanup_script = f"""
            set +e

            # Unmount if still mounted
            mountpoint -q {mountpoint} && umount {mountpoint}

            # Detach loop device if we recorded it
            if [ -f {image_path}.loop ]; then
                LOOP_DEV=$(cat {image_path}.loop)
                losetup -d "$LOOP_DEV" 2>/dev/null || true
                rm -f {image_path}.loop
            fi

            # Remove the image file
            rm -f {image_path}
        """

        try:
            # Run setup
            self.logger.info(f"Setting up loop‑backed Btrfs at {mountpoint}")
            result = self.host.conn.run(setup_script)
            if result.rc != 0:
                raise RuntimeError(f"Failed to set up loop‑backed Btrfs at {mountpoint}: {result.stderr}")

            # Read back the loop device that was actually used
            result = self.host.conn.run(f"cat {image_path}.loop")
            if result.rc != 0:
                raise RuntimeError(f"Failed to read loop device from {image_path}.loop: {result.stderr}")
            loop_dev = result.stdout.strip()
            resource["loop_dev"] = loop_dev

            # Track this resource
            self._resources.append(resource)

            yield image_path

        finally:
            # Always attempt cleanup
            self.logger.info(f"Cleaning up loop‑backed Btrfs at {mountpoint}")
            self.host.conn.run(cleanup_script)  # ignore errors

            # Remove from tracking list if it was added
            if resource in self._resources:
                self._resources.remove(resource)

    def teardown(self) -> None:
        """
        Cleans up any Btrfs resources that were not properly released.
        """
        if not self._resources:
            return

        self.logger.warning(f"Cleaning up {len(self._resources)} leftover Btrfs resources")
        for res in self._resources[:]:
            mountpoint = res["mountpoint"]
            image_path = res["image_path"]

            cleanup_script = f"""
                mountpoint -q {mountpoint} && umount {mountpoint}
                if [ -f {image_path}.loop ]; then
                    LOOP_DEV=$(cat {image_path}.loop)
                    losetup -d "$LOOP_DEV" 2>/dev/null || true
                    rm -f {image_path}.loop
                fi
                rm -f {image_path}
            """
            self.host.conn.run(cleanup_script)
            self._resources.remove(res)
