# Generic functions to save, change, and restore configuration files

set -e

build_path=$(git rev-parse --show-toplevel)

# Save the configuration files in tmp.
save_config ()
{
	[ ! -d tmp ] && mkdir tmp
	find config -depth -path "*/.svn/*" -prune -o -type f -print | sed -e 's/config\///' |
	while read file
	do
		mkdir -p "tmp/$(dirname "$file")"
		[ -f "/$file" ] && cp -dp "/$file" "tmp/$file" || true
	done
}

# Copy the config files from config to the system
change_config ()
{
	find config -depth -path "*/.svn/*" -prune -o -type f -print | sed -e 's/config\///' |
	while read file
	do
		cp -f "config/$file" "/$file"
	done
}

# Restored the config files in the system.
# The config files must be saved before with save_config ().
restore_config ()
{
	find config -depth -path "*/.svn/*" -prune -o -type f -print | sed -e 's/config\///' |
	while read file
	do
		if [ -f "tmp/$file" ]; then
			cp -dp "tmp/$file" "/$file"
			rm "tmp/$file"
		else
			rm -f "/$file"
		fi
		d="$(dirname "tmp/$file")"
		while [ -n "$d" ] && [ "$d" != "." ]
		do
			rmdir "$d" 2>/dev/null || true
			d="$(dirname "$d")"
		done
	done

	rmdir tmp 2>/dev/null || true
}

prepare_chroot ()
{
	mkdir tmp/root
	cp -rfdp config_chroot/* tmp/root/
	find tmp/root/ -name .svn -type d -print0 | xargs -0 rm -rf

	lists=/root/tests/common/config_chroot.list
	[ -f config_chroot.list ] && lists="$lists config_chroot.list"
	cat $lists | grep -v "#" | while read f
	do
		# Create parent directory if needed
		d=$(dirname tmp/root/$f)
		[ -d $d ] || mkdir -p $d
		# Create hard link
		ln $f tmp/root/$f
	done

	# Copy existing gcda
	mkdir -p tmp/root$build_path/lib
	mkdir -p tmp/root$build_path/libmisc
	mkdir -p tmp/root$build_path/src
	find "$build_path" -name "*.gcda" | while read f
	do
		ln $f tmp/root/$f
	done
}

clean_chroot ()
{
	# Remove copied files
	lists=/root/tests/common/config_chroot.list
	[ -f config_chroot.list ] && lists="$lists config_chroot.list"
	cat $lists | grep -v "#" | while read f
	do
		rm -f tmp/root/$f
		# Remove parent directory if empty
		d=$(dirname tmp/root/$f)
		rmdir -p --ignore-fail-on-non-empty $d
	done

	find "$build_path" -name "*.gcda" | while read f
	do
		rm -f tmp/root/$f
	done
	find tmp/root -name "*.gcda" | while read f
	do
		g=${f#tmp/root}
		mv "$f" "$g"
	done
	rmdir tmp/root$build_path/lib
	rmdir tmp/root$build_path/libmisc
	rmdir tmp/root$build_path/src
	rmdir tmp/root$build_path
	rmdir tmp/root/root/build
	rmdir tmp/root/root

	find config_chroot -type f | while read f
	do
		f=${f#config_chroot/}
		rm -f tmp/root/$f
	done

	find config_chroot -depth -type d | while read d
	do
		d=${d#config_chroot}
		[ -d "tmp/root$d" ] && rmdir tmp/root$d
	done
}

