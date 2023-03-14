# Introduction

## Git and Github

We recommend you to get familiar with the
[git](https://guides.github.com/introduction/git-handbook) and
[Github](https://guides.github.com) workflows before posting any changes.

### Set up in a nut shell

The following steps describe the process in a nut shell to provide you a basic
template:

* Create an account on [GitHub](https://github.com)
* Fork the [shadow repository](https://github.com/shadow-maint/shadow)
* Clone the shadow repository

```
git clone https://github.com/shadow-maint/shadow.git
```

* Add your fork as an extra remote

```
git remote add $ghusername git@github.com:$ghusername/shadow.git
```

* Setup your name contact e-mail that you want to use for the development

```
git config user.name "John Smith"
git config user.email "john.smith@home.com"
```

**Note**: this will setup the user information only for this repository. You
can also add `--global` switch to the `git config` command to setup these
options globally and thus making them available in every git repository.

* Create a working branch

```
git checkout -b my-changes
```

* Commit changes

```
vim change-what-you-need
git commit -s
```

Check
[the kernel patches guide](https://www.kernel.org/doc/html/v4.14/process/submitting-patches.html#describe-your-changes)
to get an idea on how to write a good commit message.

* Push your changes to your GitHub repository

```
git push $ghusername my-changes --force
```

* Open a Pull Request against shadow project by clicking on the link provided
in the output of the previous step

* Make sure that all Continuous Integration checks are green and wait review

## Internal guidelines

Additionally, you should also check the following internal guidelines to
understand the project's development model:

* [Build & install](build_install.md)
* [Coding style](coding_style.md)
* [Tests](tests.md)
* [Continuous Integration](CI.md)
* [Releases](releases.md)
* [License](license.md)
