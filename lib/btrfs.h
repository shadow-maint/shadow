// SPDX-FileCopyrightText: 2026, Alejandro Colomar <alx@kernel.org>
// SPDX-License-Identifier: BSD-3-Clause


#ifndef SHADOW_INCLUDE_LIB_BTRFS_H_
#define SHADOW_INCLUDE_LIB_BTRFS_H_


#include "config.h"

#include <stdbool.h>
#include <sys/statfs.h>


#ifdef WITH_BTRFS
int btrfs_create_subvolume(const char *path);
int btrfs_remove_subvolume(const char *path);
int btrfs_is_subvolume(const char *path);
bool is_btrfs(const struct statfs *sfs);
#endif


#endif  // include guard
