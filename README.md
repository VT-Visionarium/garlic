# garlic

A very crude meta software package management system.

A repo to track the building of /usr/local on many computers.  These
computers as used by Dr. Nicholas Polys at Virginia Tech.

Notice: If you see this file in /usr/local/src/ this could be working
clone of a git repository, you should see a .git directory here too; run
'cat .get/config' to see the source origin.

## About
This is only intended to be useful on GNU/Linux or UNIX-like systems.
This package is intended to make local software installations in
/usr/local more uniform, and better documented.  It is not meant to make
the /usr/local directory tree on all systems that use this package be
exactly the same.  Clearly that would be easily done by replicating
/usr/local between the machines involved via rsync or network file mount.
We anticipate some differences between /usr/local/ on computer systems
that use this.  Hence this project exists to keep common files in
/usr/local/src/ on the computers involved.

To get this source code:
```
git clone https://github.com/VT-Visionarium/garlic.git
```

## garlic
It makes installed software easier to manage using simple UNIX shell
commands.

This project includes, as a sub component, a software that predates this
package, a package, by the same name, encap.

This encap package does not appear to be maintained or distributed
anywhere (but here) but there is some related web pages at URL
http://www.ks.uiuc.edu/Development/Computers/docs/sysadmin/Build/encap.html
from 08 Aug 2005.

## installing
Each directory in this top source directory contains files, scripts and
whatnot, used to install a particular software package.  The name of the
directory should be the name of the software package.

To start:

   ```
   cd encap/
   ```
and read on.

To install other packages
  ```
  cd PACKAGE_NAME/
  ```
and read the README or README.md. 


We do not commit the source of the packages that are installing, except
for packages that are native to this package, like encap.   We do not
commit any generated files from the building process from the building of
said packages.  We just keep a runnable script record of where to get the
source to the packages (wget, git clone, ftp, and whatever), and we keep
the source of the packages in /usr/local/ on all the systems that we
install the packages on.  We encourage the use of a networked git repo
as the source, keeping local clones, and pulling particular tagged
versions to install.  We do not add any changes to the original sources,
and we copy them or use other methods to keep package sources from being
contaminated, so that we can track exactly how things are built and
installed.  We can install N different versions of a package using one git
clone of the packages source, using git tags.  For those software packages
still in the stone ages, not using git repos, (or svn, CVS, and like) we
keep copies of the different tar-like files for each version that we
install, without checking them in to this project repo.


## More details
In this directory is the source code or binary package archives of all
things installed in /usr/local/.

We use the encap package management system
(http://www.ks.uiuc.edu/Development/Computers/docs/sysadmin/Build/encap.html)
to maintain sym-links for most of the packages installed. We can and do
install multiple versions of some packages.

Directory names in this level are general package names.  There may be
more than one version of packages within these directories.  Unaltered
(original) source files (tarball, git repo, svn co, .deb and etc.) are
kept in an encapsulated form for all packages installed.  All packages are
installed with a one script, and there is a different script for each
package version installed.

The built and un-installed files are left in place after being installed,
so that debugging the /usr/local software stack is easier and people that
are not involved at the time of the installation of the software be better
equipped at maintaining this /usr/local thingy.  i.e. I prefer doing a
good job over job security, unlike most sys-admins.

Some files in this directory:

   common.bash contains some scripts used by many package installation scripts
   that are in /usr/local/src/.


   /usr/local/encap/ is where all packages get installed.  For example
   /usr/local/encap/OpenSceneGraph-3.5.1 (PREFIX) contains the installed
   files for version 3.5.1 of Open Scene Graph, the original source files
   are in all in /usr/local/openscenegraph/git and
   /usr/local/src/openscenegraph/OpenSceneGraph-Data-3.4.0.zip.  The
   build/installation script that installed it is in
   /usr/local/src/openscenegraph/OpenSceneGraph-3.5.1/install.bash.
   Running install.bash will do all the work of installing this version of
   this package in /usr/local/encap/OpenSceneGraph-3.5.1, without root
   access.  /usr/local/sbin/encap is run by root to sym-link this packages
   files into /usr/local/bin/, /usr/local/lib64/, /usr/local/include/, and
   so on.  If there was another version of Open Scene Graph if would be
   in, for example, /usr/local/encap/opengcenegraph-5.9 (PREFIX).
   Assuming that it has some of the same file names as
   OpenSceneGraph-3.5.1, there would be a choice as to which of the two
   Open Scene Graph installations to be sym-linked into /usr/local/bin/,
   /usr/local/lib64/, and so on; encap manages that choice with the file
   /usr/local/encap/encap.exclude.

   Not all packages come in the same source form, some are just tarballs,
   some are git repos, some are .deb binary packages, some are svn repos,
   and does any one use CVS any more.  All are stored on the web.

   The source files to packages are always kept in an original form
   with no local changes or files added.  The install scripts must
   be able to work from the original form archived files be they
   tarballs, git repo clone, svn checkouts, zip files, and so on.
   The general source directory file name schemes are for example:

   /usr/local/src/PACKAGENAME/PACKAGENAME_VERSION/git/
   a git repo clone that may have local edits if all the installed
   versions are tagged and the install scripts pull the tagged
   version from the repo.  This paradigm uses the power of git
   so that we may have all versions of the source code in one
   place, and just have the different build/install versions
   point to that one source.

   /usr/local/src/PACKAGENAME/PACKAGENAME_VERSION.tgz a tarball
   is always the distributed original.  The install script must
   apply (and keep) any needed patches for the local installations.
   
   /usr/local/src/PACKAGENAME/PACKAGENAME_VERSION.tar.bz2  a tarball
   
   /usr/local/src/PACKAGENAME/PACKAGENAME_VERSION.zip  a zip file

   /usr/local/src/PACKAGENAME/svn/  a svn checkout. This management system
   shows svn to be lacking compared to git.  We need to have different
   checked out trees to keep different versions of the source locally.

   /usr/local/src/PACKAGENAME/PACKAGENAME_VERSIONTAG/ is
   a build/script tree, the package will install in prefix
   /usr/local/encap/PACKAGENAME_VERSIONTAG/
   
   /usr/local/src/PACKAGENAME/PACKAGENAME_VERSIONTAG/install.bash
   build script

   /usr/local/src/PACKAGENAME/PACKAGENAME_VERSIONTAG/build_01
   a build prefix 1st try

   /usr/local/src/PACKAGENAME/PACKAGENAME_VERSIONTAG/build_02
   a build prefix 2nd try

