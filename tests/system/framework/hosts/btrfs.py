from __future__ import annotations

import uuid
from contextlib import contextmanager


class Btrfs:
    def __init__(self, host):
        self.host = host

    def subvolumes(self, path: str) -> list[str]:
        """
        Returns a list of Btrfs Subvolumes under the specified path.
        """
        result = self.host.conn.run(f"btrfs subvolume list {path}").stdout

        names = []
        for line in result.strip().splitlines():
            if " path " in line:
                names.append(line.split(" path ", 1)[1].strip())

        return names

    def subvolume_exists(self, path: str, name: str) -> bool:
        """
        Returns true if the Btrfs Subvolume exists under the specified path.
        """
        return name in self.subvolumes(path)

    @contextmanager
    def loopback_mount(self, mountpoint: str, size: str = "128M"):
        """
        Mount a temporary loop-backed Btrfs filesystem on the specified path.

        - Creates loop devices manually (for containers without udev)
        - Uses a random image path under /tmp
        - Fully cleans up on exit
        """

        image_path = f"/tmp/btrfs-{uuid.uuid4().hex}.img"

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

        self.host.logger.info(f"Setting up loop-backed Btrfs {mountpoint}")
        self.host.conn.run(setup_script)

        try:
            yield image_path
        finally:
            self.host.logger.info(f"Removing loop-backed Btrfs {mountpoint}")
            self.host.conn.run(cleanup_script)
