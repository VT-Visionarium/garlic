#!/usr/bin/perl
#   Copyright 1996 - Board of Trustees - University of Illinois
#   All rights reserved. 
#
#   encap.pl - Encapsulated Software Manager
#
#   Jason Wessel - jwessel@uiuc.edu
#   Computing & Communications Services Office
#   University of Illinois - Urbana
#
#   version 1.2 Jan 18, 1997

$0 =~ s/.*\///;                 # shorten program name

umask(18); # set the umask to 022 
$verbose = 1;       # verbose on unless -q
$cleanmode = 0;     # clean set to off unless -c 
$reslinks = 0;      # decides if we are resolving links
$excludeKill = 0;   # Walk the excluded areas and blast according links
$killAllLinks = 0;  # kill all the link when resolvings
$killAllFiles = 0;  # kill all the files when resolving
$encapexfile = "encap.exclude";
$|=1; # turn off buffering

# parse the command line

sub usage {
    print<<EOM;
Usage:
$0 [-q] [-r [-kl] [-kf]] [-e] <encap area> [target area] [link prefix]
$0 [-q] -c <directory> [encap area <link prefix>] 

In Mode1, $0 acts as the software mananger where all the files
       in the encap area get linked into the target area, by default
       the target area is one directory above the encap area.
       The linkprefix is location prepended to all the links in the
       target area, by default it is the same as the encap area.
 -q  Quiet mode, where all but errors are suppressed
 -r  Resolve conflicts between software in the encap and where it is to be 
     linked you will be prompted unless you use -kl and -kf
     -kl  Answer yes to all Re-link questions
     -kf  Answer yes to all Remove file, and replace with link questions
 -e  Remove links from link target area 

In Mode2, $0 will remove links that don't point to something. Encap
       area and link prefix can be specficed if you have a target area
       directory with resolve links that point to a hierarchy that doesn't
       exist on local machine

 -q  Quiet mode where all but errors are suppressed.
 -i  Ignore exclude files.
EOM
}

$encapdir = "";
$todir = "";
$linkprefix = "";
$ignore_ex = 0;
if ($#ARGV < 0) {
    &usage();
    exit(1);
}
while ($ARGV[0] =~ /./) {
    if ($ARGV[0] !~ /^\-/) {
	if ($encapdir eq "") {
	    $encapdir = $ARGV[0];
	    shift;
	} elsif ($todir eq "") {
	    $todir = $ARGV[0];
	    shift;
	} elsif ($linkprefix eq "") {
	    $linkprefix = $ARGV[0];
	    shift;
	} else {
	    print "$0: too many arguments: $ARGV[0]\n";
	    &usage();
	    exit(1);
	}
    } else {
	if ($ARGV[0] eq "-q") {
	    $verbose = 0;
	    shift;
	} elsif ($ARGV[0] eq "-i") {
	    $ignore_ex = 1;
	    shift;
	} elsif ($ARGV[0] eq "-c") {
	    $cleanmode = 1;
	    shift;
	} elsif ($ARGV[0] eq "-r") {
	    $reslinks = 1;
	    shift;
	} elsif ($ARGV[0] eq "-e") {
	    $excludeKill = 1;
	    shift;
	} elsif ($ARGV[0] eq "-kf") {
	    $killAllFiles = 1;
	    shift;
	} elsif ($ARGV[0] eq "-kl") {
	    $killAllLinks = 1;
	    shift;
	} else {
	    print "$0: unknown flag: $ARGV[0]";
	    &usage();
	    exit(1);
	}
    }
}

# Do checks on the data we read from the command line
if ($encapdir !~ /^\//) {
    die "\n$0: path to encap area must be absolute ie: /usr/local/encap\n\n";
}
if (! (-d "$encapdir")) {
    die "\n$0: $encapdir directory does not exist\n\n";
}

if ($cleanmode) {
    $dircheck = 0;
    if ($todir ne "" && ! (-d "$todir")) {
	die "\n$0: $todir directory does not exist\n\n";
    }
    if ($todir ne "") {
	$dircheck = 1;
    }
    &cleandir($encapdir);
    exit(0);
}

if ($todir eq "") {
    $todir = $encapdir;       #dir where all the links and directories of links go
    $todir =~ s/(.*\/.*)\/.*/$1/;
}
if (! (-d "$todir")) {
    die "\n$0: $todir directory does not exist\n\n";
}

if ($linkprefix eq "") {
    $linkprefix = $encapdir;  #the linkprefix is attatched to all the links
}
#------------------Dir cleaning program---------------------------
# Given a directory this program will check all the links in the directory 
# for validity.
# If they are invalid, they unlinked.

sub cleandir {
    local($dir) = @_;
    local($i,@dirlist,@tmplist,%excludelist);
#    print "cleaning $dir\n" if $verbose;
    opendir(RDIR,"$dir") || die "$0: couln't open $dir\n";
    @dirlist = readdir(RDIR);
    closedir(RDIR);
    if (-f "$dir/$encapexfile" && !$ignore_ex) {
	print "using $dir/$encapexfile\n" if $verbose;
	open(INFILE,"$encapdir/$encapexfile");
	while (<INFILE>) {
	    chop($_);
	    if ($_ eq "" || $_ =~ /^\#/) {
		next;
	    }
	    $excludelist{$_} = 1;
	}
	close(INFILE);
    }
    for ($i = 0; $i <= $#dirlist; $i++) {
	if ($dirlist[$i] eq "." || $dirlist[$i] eq ".." || 
	    $dirlist[$i] eq $encapexfile) {
	    next;
	}
	if ($excludelist{$dirlist[$i]} && !$ignore_ex) {
	    print "  excluding: $dir/$dirlist[$i]\n" if $verbose;
	    next;
	}
	if (-d "$dir/$dirlist[$i]" && !(-l "$dir/dirlist[$i]")) {
	    &cleandir("$dir/$dirlist[$i]");
	    opendir(TMPDIR,"$dir/$dirlist[$i]");
	    @tmplist = readdir(TMPDIR);
	    closedir(TMPDIR);
	    if ($#tmplist == 1) {
		print "  removing dir: $dir/$dirlist[$i]\n" if $verbose;
		rmdir("$dir/$dirlist[$i]");
	    }
	    next;
	}
	if ($dircheck) {
	    if ($tmplink = readlink("$dir/$dirlist[$i]")) {
		$tmplink =~ s/^$linkprefix/$todir/;
		if (!(-e "$tmplink")) {
		    print "   removing: $dir/$dirlist[$i]\n" if $verbose;
		    unlink("$dir/$dirlist[$i]");
		}
	    }
	} elsif (!(-e "$dir/$dirlist[$i]")) {
	    print "   removing: $dir/$dirlist[$i]\n" if $verbose;
	    unlink("$dir/$dirlist[$i]");
        }
    }
    if (!(-l $dir)) {
	opendir(TMPDIR,"$dir");
	@tmplist = readdir(TMPDIR);
	closedir(TMPDIR);
	if ($#tmplist == 1) {
	    print "  removing dir: $dir\n" if $verbose;
	    rmdir("$dir/$dirlist[$i]");
	}
    }
}

#------------------End dir cleaning program-----------------------
print "doing encap: $encapdir\ninto: $todir\n" if $verbose;
print "linking with: $linkprefix\n" if $verbose;

# iterate over all the directories in the encap area, these are our packages. 
# We call linkpackage for each directory that is found
#   Also we will support a .encap file in any directory which could contain a 
#   list of crotrols to make encap.pl do things like exclude useless 
#   directories from being linked in

# first do some system dependant magic to setup up input buffering correctly
if ($reslinks) {
  &do_stty();
}

opendir(LEVEL1,"$encapdir") || die "$0: could not open $encapdir\n";
@level1list = readdir(LEVEL1);
closedir(LEVEL1);
@level1list = sort @level1list;
if (-f "$encapdir/$encapexfile") {
    open(INFILE,"$encapdir/$encapexfile");
    while (<INFILE>) {
	chop($_);
	if ($_ eq "" || $_ =~ /^\#/) {
	    next;
	}
	$excludelist{$_} = 1;
    }
    close(INFILE);
}
foreach $level1file (@level1list) {
    if ($level1file eq "." || $level1file eq "..") {
	next;
    }
    if ($excludelist{$level1file}) {
	print " excluding package: $level1file\n" if $verbose;
	&exclude1killer($level1file) if $excludeKill;
    } else {
	if (-d "$encapdir/$level1file") {
	    &linkpackage($level1file);
	}
    }
}
# undo the magic input stuff if it actually worked in the first place
if ($reslinks) {
  &undo_stty();
}

#-----------------end of the main program------------------------
# we need this sub procedure to clean out an entire package with the
#   exclude killer routine
sub exclude1killer {
    local($packagedir) = @_;
    local($i,$j,@packagelist);
    opendir(APACKAGE,"$encapdir/$packagedir");
    @packagelist = readdir(APACKAGE);
    closedir(APACKAGE);
    for ($i = 0; $i <= $#packagelist; $i++) {
	if ($packagelist[$i] eq "." || $packagelist[$i] eq ".." ||
	    $packagelist[$i] eq $encapexfile) {
	    next;
	}
	&excludekiller($packagedir,"",$packagelist[$i]);
    }
}

sub do_stty {
    $sinput = `tty`;
    chop($sinput);
    if ($sinput !~ /not/) {
	system("stty -icanon eol  min 1");
    }
}

sub undo_stty {
    $sinput = `tty`;
    chop($sinput);
    if ($sinput !~ /not/) {
	system("stty -icanon");
    }
}
# this procedure will a process package directory in the encap and it 
# will build all the links
sub linkpackage {
    local($packagedir) = @_;
    local($i,$j,$localres,@packagelist,%excludelist);
    $localres = 0;
    print " package: $packagedir\n" if $verbose;
    opendir(APACKAGE,"$encapdir/$packagedir");
    @packagelist = readdir(APACKAGE);
    closedir(APACKAGE);
#    check for a encap control files to control how we do our linking
    if (-f "$encapdir/$packagedir/$encapexfile") {
	open(INFILE,"$encapdir/$packagedir/$encapexfile");
	while (<INFILE>) {
	    chop($_);
	    if ($_ eq "" || $_ =~ /^\#/) {
		next;
	    }
	    $excludelist{$_} = 1;
	}
	close(INFILE);
    }
#    go ahead and iterate over all the dirs and process
#    only the directories and links in this level
    for ($i = 0; $i <= $#packagelist; $i++) {
	if ($packagelist[$i] eq "." || $packagelist[$i] eq ".." ||
	    $packagelist[$i] eq $encapexfile || 
	    $excludelist{$packagelist[$i]}) {
	    if ($excludelist{$packagelist[$i]}) {
	        print "   excluding: $packagelist[$i]\n" if $verbose;
		if ($excludeKill) {
		    &excludekiller($packagedir,"",$packagelist[$i]);
		}
	    }
	    next;
	} 
	if ($tmplink = readlink("$encapdir/$packagedir/$packagelist[$i]")) {
	    $linkplace = readlink("$todir/$packagelist[$i]");
	    if (-e "$todir/$packagelist[$i]" || $linkplace) {
		if ($reslinks && $linkplace ne 
		    "$linkprefix/$packagedir/$packagelist[$i]") {
		    if ($linkplace) {
			print "   Old: $todir/$packagelist[$i] -> $linkplace\n" 
			    if $verbose;
			print "   New: -> $linkprefix/$packagedir/$packagelist[$i]\n"
			    if $verbose;
			$ans = 'y';
			while (1 && !$killAllLinks && !$localres) {
			    print "Re-link link? (y)es/(n)o/yes for rest of (p)ackages/(Y)es for all/(q)uit ";
			    $ans = getc(STDIN);
			    print "\n";
			    if ($ans eq "p") {
				$localres = 1;
				$ans = 'y';
			    }
			    if ($ans eq "Y") {
				$killAllLinks = 1;
				$ans = 'y';
			    }
			    if ($ans eq "y" || $ans eq "n" || $ans eq "q") {
				last;
			    }
			}
			if ($ans eq 'q') {
			    &undo_stty();
			    exit(0);
			}
			if ($ans eq 'y') {
			    unlink("$todir/$packagelist[$i]");
			    symlink("$linkprefix/$packagedir/$packagelist[$i]",
				    "$todir/$packagelist[$i]");
			} else {
			    next;
			}
		    } else {
			print "   Old: $todir/$packagelist[$i]\n" if $verbose; 
			print "   New: -> $linkprefix/$packagedir/$packagelist[$i]\n"
			    if $verbose;
			$ans = 'y';
			while (1 && !$killAllFiles && !$localres) {
			    print "Re-link link? (y)es/(n)o/yes for rest of (p)ackage/(Y)es for all/(q)uit ";
			    $ans = getc(STDIN);
			    print "\n";
			    if ($ans eq "p") {
				$localres = 1;
				$ans = 'y';
			    }
			    if ($ans eq "Y") {
				$killAllLinks = 1;
				$ans = 'y';
			    }
			    if ($ans eq "y" || $ans eq "n" || $ans eq "q") {
				last;
			    }
			}
			if ($ans eq 'q') {
			    &undo_stty();
			    exit(0);
			}
			if ($ans eq 'y') {
			    if (-d "$todir/$packagelist[$i]") {
				print "==rm -rf $todir/$packagelist[$i]\n" if $verbose;
				system("rm -rf $todir/$packagelist[$i]");
			    } else {
				unlink("$todir/$packagelist[$i]");
			    }
			    symlink("$linkprefix/$packagedir/$packagelist[$i]",
				    "$todir/$packagelist[$i]");
			} else {
			    next;
			}
		    }
		}
	    } else {
		print "     linking /$packagelist[$i]\n" if $verbose;
		symlink("$linkprefix/$packagedir/$packagelist[$i]",
			"$todir/$packagelist[$i]");
	    }
	} elsif (-d "$encapdir/$packagedir/$packagelist[$i]") {
	    print "   doing $packagelist[$i]\n" if $verbose;
	    $localres = &mkdirlinks($packagedir,"",$packagelist[$i],$localres);
	}
    }
}

# This procedure will remove links if something was linked in that should
# be excluded
sub excludekiller {
    local($packagedir,$subdir,$curdir) = @_;
    local($linkplace,$i,@subdirlist,@stats);
    if (! (-d "$encapdir/$packagedir$subdir/$curdir")) {
	$linkplace = readlink("$todir$subdir/$curdir");
	if ($linkplace eq 
	    "$encapdir/$packagedir$subdir/$curdir") {
	    print "      removing: $todir$subdir/$curdir\n" if $verbose;
	    unlink("$todir$subdir/$curdir");
	}
	return;
    }
    opendir(APACKAGE,"$encapdir/$packagedir$subdir/$curdir");
    @subdirlist = readdir(APACKAGE);
    closedir(APACKAGE);

    for ($i = 0; $i <= $#subdirlist; $i++) {
	if ($subdirlist[$i] eq "." || $subdirlist[$i] eq ".." || 
	    $subdirlist[$i] eq $encapexfile) {
	    next;
	}
	if (-d "$encapdir/$packagedir$subdir/$curdir/$subdirlist[$i]") {
	    &excludekiller($packagedir,"$subdir/$curdir",$subdirlist[$i]);
	    next;
	}
	$linkplace = readlink("$todir$subdir/$curdir/$subdirlist[$i]");
	if ($linkplace eq 
	    "$encapdir/$packagedir$subdir/$curdir/$subdirlist[$i]") {
	    print "      removing: $todir$subdir/$curdir/$subdirlist[$i]\n" 
		if $verbose;
	    unlink("$todir$subdir/$curdir/$subdirlist[$i]");
	}
    }
    if (-d "$todir$subdir/$curdir") {
	opendir(TMPDIR,"$todir$subdir/$curdir");
	@tmplist = readdir(TMPDIR);
	closedir(TMPDIR);
	if ($#tmplist == 1) {
	    print "  removing dir: $todir$subdir/$curdir\n" if $verbose;
	    rmdir("$todir$subdir/$curdir");
	}
    }
}

# this procedure will make the directories and the links if they don't 
# already exist in the destination (todir) area
sub mkdirlinks {
    local($packagedir,$subdir,$curdir,$localres) = @_;
    local($i,%excludelist,@subdirlist,@stats,$tmplink);
    if (-e "$todir$subdir/$curdir") {
	if (! (-d "$todir$subdir/$curdir")) {
	    die "\n$0: $todir$subdir/$curdir is not a directory\n";
	}
    } else {
	print "    making dir $todir$subdir/$curdir\n" if $verbose;
	mkdir("$todir$subdir/$curdir",0755);
    }
    opendir(APACKAGE,"$encapdir/$packagedir$subdir/$curdir");
    @subdirlist = readdir(APACKAGE);
    closedir(APACKAGE);
#    check for a encap control files to control how we do our linking
    if (-f "$encapdir/$packagedir$subdir/$curdir/$encapexfile") {
	open(INFILE,"$encapdir/$packagedir$subdir/$curdir/$encapexfile");
	while (<INFILE>) {
	    chop($_);
	    if ($_ eq "" || $_ =~ /^\#/) {
		next;
	    }
	    $excludelist{$_} = 1;
	}
	close(INFILE);
    }
    for ($i = 0; $i <= $#subdirlist; $i++) {
	if ($subdirlist[$i] eq "." || $subdirlist[$i] eq ".." ||
	    $subdirlist[$i] eq $encapexfile || 
	    $excludelist{$subdirlist[$i]}) {
	    if ($excludelist{$subdirlist[$i]}) {
	        print "   excluding: $subdirlist[$i]\n" if $verbose;
		if ($excludeKill) {
		    &excludekiller($packagedir,"$subdir/$curdir",$subdirlist[$i]);
		}
	    }
	    next;
	} 
	$tmplink = readlink("$encapdir/$packagedir$subdir/$curdir/$subdirlist[$i]");
	$linkplace = readlink("$todir$subdir/$curdir/$subdirlist[$i]");
	if ($tmplink) {
	    if (-e "$todir$subdir/$curdir/$subdirlist[$i]" || $linkplace) {
		if ($reslinks && $linkplace ne 
		    "$linkprefix/$packagedir$subdir/$curdir/$subdirlist[$i]") {
		    if ($linkplace) {
			print "   Old: $todir$subdir/$curdir/$subdirlist[$i] -> $linkplace\n" if $verbose;
			print "   New: -> $linkprefix/$packagedir$subdir/$curdir/$subdirlist[$i]\n" if $verbose;
			$ans = 'y';
			while (1 && !$killAllLinks && !$localres) {
			    print "Re-link link? (y)es/(n)o/yes for rest of (p)ackage/(Y)es for all/(q)uit ";
			    $ans = getc(STDIN);
			    print "\n";
			    if ($ans eq "p") {
				$localres = 1;
				$ans = 'y';
			    }
			    if ($ans eq "Y") {
				$killAllLinks = 1;
				$ans = 'y';
			    }
			    if ($ans eq "y" || $ans eq "n" || $ans eq "q") {
				last;
			    }
			}
			if ($ans eq 'q') {
			    &undo_stty();
			    exit(0);
			}
			if ($ans eq 'y') {
			    unlink("$todir$subdir/$curdir/$subdirlist[$i]");
			    symlink("$linkprefix/$packagedir$subdir/$curdir/$subdirlist[$i]",
				    "$todir$subdir/$curdir/$subdirlist[$i]");
			} else {
			    next;
			}
		    } else {
			print "   Old: $todir$subdir/$curdir/$subdirlist[$i]\n" if $verbose;
			print "   New: -> $linkprefix/$packagedir$subdir/$curdir/$subdirlist[$i]\n" if $verbose;
			$ans = 'y';
			while (1 && !$killAllFiles && !$localres) {
			    print "Re-link link? (y)es/(n)o/yes for rest of (p)ackage/(Y)es for all/(q)uit ";
			    $ans = getc(STDIN);
			    print "\n";
			    if ($ans eq "p") {
				$localres = 1;
				$ans = 'y';
			    }
			    if ($ans eq "Y") {
				$killAllFiles = 1;
				$ans = 'y';
			    }
			    if ($ans eq "y" || $ans eq "n" || $ans eq "q") {
				last;
			    }
			}
			if ($ans eq 'q') {
			    &undo_stty();
			    exit(0);
			}
			if ($ans eq 'y') {
			    if (-d "$todir$subdir/$curdir/$subdirlist[$i]") {
				print "==rm -rf $todir$subdir/$curdir/$subdirlist[$i]\n" if $verbose;
				system("rm -rf $todir$subdir/$curdir/$subdirlist[$i]");
			    } else {
				unlink("$todir$subdir/$curdir/$subdirlist[$i]");
			    }
			    symlink("$linkprefix/$packagedir$subdir/$curdir/$subdirlist[$i]",
				    "$todir$subdir/$curdir/$subdirlist[$i]");
			} else {
			    next;
			}
		    }
		}
	    } else {
		print "     linking $subdir/$curdir/$subdirlist[$i]\n" if $verbose;
		symlink("$linkprefix/$packagedir$subdir/$curdir/$subdirlist[$i]",
			"$todir$subdir/$curdir/$subdirlist[$i]");
	    }
	    next;
	}
	if (-d "$encapdir/$packagedir$subdir/$curdir/$subdirlist[$i]") {
	    $localres = &mkdirlinks($packagedir,"$subdir/$curdir",$subdirlist[$i],$localres);
	    next;
	}
	if ($reslinks) {
	    if ($linkplace) {
		if ($linkplace ne 
		    "$linkprefix/$packagedir$subdir/$curdir/$subdirlist[$i]") {
		    print "old: $todir$subdir/$curdir/$subdirlist[$i] -> $linkplace\n" if $verbose;
		    print "new: -> $linkprefix/$packagedir$subdir/$curdir/$subdirlist[$i]\n" 
			if $verbose;
		    $ans = 'y';
		    while (1 && !$killAllLinks && !$localres) {
			print "Re-link link? (y)es/(n)o/yes for rest of (p)ackage/(Y)es for all/(q)uit ";
			$ans = getc(STDIN);
			print "\n";
			if ($ans eq "p") {
			    $localres = 1;
			    $ans = 'y';
			}
			if ($ans eq "Y") {
			    $killAllLinks = 1;
			    $ans = 'y';
			}
			if ($ans eq "y" || $ans eq "n" || $ans eq "q") {
			    last;
			}
		    }
		    if ($ans eq 'q') {
			&undo_stty();
			exit(0);
		    }
		    if ($ans eq 'y') {
			if (-d "$todir$subdir/$curdir/$subdirlist[$i]") {
			    print "==rm -rf $todir$subdir/$curdir/$subdirlist[$i]\n" if $verbose;
			    system("rm -rf $todir$subdir/$curdir/$subdirlist[$i]");
			} else {
			    unlink("$todir$subdir/$curdir/$subdirlist[$i]");
			}
			symlink("$linkprefix/$packagedir$subdir/$curdir/$subdirlist[$i]",
				"$todir$subdir/$curdir/$subdirlist[$i]");
		    }
		}
	    } else {
		@stats = stat("$todir$subdir/$curdir/$subdirlist[$i]");
		if (@stats) {
		    print "old: ", `ls -l $todir$subdir/$curdir/$subdirlist[$i]` if $verbose;
		    print "new: $linkprefix/$packagedir$subdir/$curdir/$subdirlist[$i]\n" if $verbose;
		    $ans = 'y';
		    while (1 && !$killAllFiles && !$localres) {
			print "Overwrite file? Re-link link? (y)es/(n)o/yes for rest of (p)ackage/(Y)es for all/(q)uit ";
			$ans = getc(STDIN);
			print "\n";
			if ($ans eq "p") {
			    $localres = 1;
			    $ans = 'y';
			}
			if ($ans eq "Y") {
			    $killAllFiles = 1;
			    $ans = 'y';
			}
			if ($ans eq "y" || $ans eq "n" || $ans eq "q") {
			    last;
			}
		    }
		    if ($ans eq 'q') {
			&undo_stty();
			exit(0);
		    }
		    if ($ans eq 'y') {
			if (-d "$todir$subdir/$curdir/$subdirlist[$i]") {
			    print "==rm -rf $todir$subdir/$curdir/$subdirlist[$i]\n" if $verbose;
			    system("rm -rf $todir$subdir/$curdir/$subdirlist[$i]");
			} else {
			    unlink("$todir$subdir/$curdir/$subdirlist[$i]");
			}
			symlink("$linkprefix/$packagedir$subdir/$curdir/$subdirlist[$i]",
				"$todir$subdir/$curdir/$subdirlist[$i]");
		    }
		} else {
		    print "     linking $subdir/$curdir/$subdirlist[$i]\n" if $verbose;
		    symlink("$linkprefix/$packagedir$subdir/$curdir/$subdirlist[$i]",
			    "$todir$subdir/$curdir/$subdirlist[$i]");
		}
	    }
	} else {
	    @stats = lstat("$todir$subdir/$curdir/$subdirlist[$i]");
	    if (@stats) {
		next;
	    } else {
		print "     linking $subdir/$curdir/$subdirlist[$i]\n" if $verbose;
		symlink("$linkprefix/$packagedir$subdir/$curdir/$subdirlist[$i]",
			"$todir$subdir/$curdir/$subdirlist[$i]");
	    }
	}
    }
    return($localres);
}
