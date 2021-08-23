#[macro_use]
extern crate anyhow;
extern crate clap;
extern crate nix;

use std::env;
use std::fs;
use std::path::Path;
use std::process::exit;

use clap::{Clap, ErrorKind, IntoApp};
use nix::unistd::{chroot, Uid};

const E_SUCCESS: i32 = 0; /* success */
const E_NOPERM: i32 = 1; /* permission denied */
const E_USAGE: i32 = 2; /* invalid combination of options */
const E_FAILURE: i32 = 3; /* unexpected failure, nothing done */
const E_BAD_ARG: i32 = 6; /* invalid argument to option */

#[derive(Clap, Debug)]
#[clap(version = "0.0.0-rust")]
struct Opts {
    // TODO: find a better way to aggregate all other args?
    #[clap(short, long, requires = "status", conflicts_with_all = &["delete", "expire"])]
    all: bool,

    #[clap(short, long)]
    delete: bool,

    #[clap(short, long)]
    expire: bool,

    #[clap(short, long)]
    help: bool,

    #[clap(short, long)]
    keep_tokens: bool,

    #[clap(short, long)]
    inactive: Option<u64>,

    #[clap(short, long)]
    lock: bool,

    #[clap(short = 'n', long)]
    mindays: Option<u64>,

    #[clap(short, long)]
    quiet: bool,

    #[clap(short, long)]
    repository: Option<String>,

    #[clap(short = 'R', long)]
    root: Option<String>,

    #[clap(short = 'S', long)]
    status: bool,

    login: Option<String>,
}

struct Shadow_entry {
    sp_namp: String, // Login name
    sp_pwdp: String, // Hashed passphrase
    sp_lstchg: i64,  // Date of last change
    sp_min: i64,     // Minimum number of days between changes
    sp_max: i64,     // Maximum number of days between changes
    sp_warn: i64,    // Number of days to warn user to change the password
    sp_inact: i64,   // Number of days the account may be inactive
    sp_expire: i64,  // Number of days since 1970-01-01 until account expires
    sp_flag: u64,    // Reserved
}

fn do_chroot(newroot: &Path) -> anyhow::Result<()> {
    if !newroot.is_absolute() {
        bail!("{:?} is not an absolute path", newroot)
    }

    if let Err(e) = fs::symlink_metadata(newroot) {
        bail!("cannot access {:?}: {}", newroot, e)
    }

    env::set_current_dir(newroot)?;
    Ok(chroot(newroot)?)
}

fn main() {
    let opts = Opts::try_parse().unwrap_or_else(|e| {
        match e.kind {
            ErrorKind::DisplayHelp => {
                Opts::into_app().print_long_help().unwrap_or_else(|e| {
                    eprintln!("failed to render help: {}", e);
                    exit(E_FAILURE)
                });
                exit(E_SUCCESS)
            },
            ErrorKind::UnknownArgument => exit(E_BAD_ARG),
            ErrorKind::InvalidValue => exit(E_BAD_ARG),
            _ => exit(E_USAGE),
        }
    });

    if let Some(newroot) = opts.root {
        do_chroot(Path::new(&newroot)).unwrap_or_else(|e| {
            eprintln!("failed to chroot: {}", e);
            exit(E_FAILURE)
        })
    }

    // TODO: openlog()
    // sanitize environment

    let amroot = Uid::effective().is_root();

    if opts.all {
        if !amroot {
            exit(E_NOPERM)
        }

        todo!()
    }

    let _name = opts.login.unwrap_or_else(|| {
        // need to implement get_my_pwent(); or really just libc::getlogin(), but it sucks that
        // it's unsafe...
        todo!()
    });

    exit(E_SUCCESS)
}
