# Do not this source more than once
[ -n "$__usr_local_src_common_bash__" ] && return
__usr_local_src_common_bash__=nonzero

######################################################################
#               CONFIGURATION
######################################################################

# The prefix directory (tree) that has all this stuff installed.
# If you have a DIR/bin than root=DIR
root=/usr/local



######################################################################
######################################################################

# If you edit this do not make any changes that will change the existing
# functionality, because if you do you may brake existing install
# scripts.  Adding functions should be no problem.

# This is sourced in bash scripts from somewhere in $root/src/

function _PrintVars()
{
    [ -z "$ncores" ] && return

    cat << EOF || Fail

${BASH_SOURCE[0]} setup:

    scriptdir=$scriptdir
    name=$name
    topsrcdir=$topsrcdir
    prefix=$prefix
    ncores=$ncores
    gitdir=$gitdir

EOF
}


function Fail()
{
    set +x
    echo -e "  \x1b[41;1m ----- BASH SCRIPT\
 \x1b[32m${0}\x1b[37m FAILED  ----- \x1b[0m"

    if [ -n "$1" ] ; then
        echo -e "  \x1b[44;1m  $*  \x1b[0m" 1>&2
        echo 1>&2
    fi
    i=0
    # print call stack

    
    while [ -n "${FUNCNAME[$i]}" ] ; do
        let i=$i+1
    done

    echo "Bash Call Stack:" 1>&2

    local frame=0
    while caller $frame; do
        ((frame++));
    done

    _PrintVars

    exit 1
}


function Init()
{
    local i
    local script
    i=${#BASH_SOURCE[@]}
    let i=$i-1

    # now BASH_SOURCE[$i] the top bash script file
    # scriptdir is the directory where the first script is located
    if [ -z "$scriptdir" ] ; then
        # A previous script has not set the scriptdir variable
        scriptdir="$(dirname ${BASH_SOURCE[$i]})"
        cd "$scriptdir" || Fail "cd to scriptdir \"$scriptdir\" failed"
        scriptdir="$PWD" # now we have full path
    else
        # A previous script has set the scriptdir variable
        script="$(basename ${BASH_SOURCE[$i]})"
        # scriptdir is set, lets assume some things about it
        [ -x "$scriptdir/$script" ] || Fail "scriptdir=$scriptdir was set wrong"
    fi

    name="$(basename $scriptdir)" || Fail
    prefix="$root/encap/$name"
    cd .. || Fail
    topsrcdir="$PWD"
    gitdir="$topsrcdir/git"
    ncores="$(nproc)" || Fail
    cd "$scriptdir" || Fail
    [ -e "$root/encap" ] || Fail "encap root $root/encap does not exist"
    [ ! -e "$prefix" ] || Fail "prefix \"$prefix\" exists already"
    if [ -f "$topsrcdir/common.bash" ] ; then
        echo "Sourcing $topsrcdir/common.bash"
        source "$topsrcdir/common.bash" || Fail
    fi
    _PrintVars
}

Init

# Usage: GitCreateClone URL
function GitCreateClone()
{
    [ -z "$1" ] && Fail "Usage: ${FUNCNAME[0]} URL"

    if [ ! -f "$gitdir/.git/config" ] ; then
        set -x
        git clone --recursive "$1" "$gitdir" || Fail
        set +x
    else
        local url
        local cwd
        cwd="$PWD"
        cd "$gitdir" || Fail
        url="$(git config --get remote.origin.url)"
        [ "$url" = "$1" ] || \
            Fail "git cloned repo is not from $1 it's from $url"

        echo -e "\ngit clone of \"$1\" \"$gitdir\" was found.\n"
        echo -e "pulling latest changes from $url\n"
        set -x
        git pull --recurse-submodules || Fail
        cd "$cwd" || Fail
        set +x
    fi
}

# Usage: MkBuildDir builddir
# makes an empty directory in the current directory
# and sets $1 to the directory path
function MkBuildDir()
{
    [ -n "$1" ] || Fail
    for i in build_01 build_02 build_03 build_04 build_05 build_06 ; do
        d="$scriptdir/$i"
        if [ ! -e "$d" ] ; then
            mkdir -p "$d" || Fail
            echo "Build directory set to $d" 1>&2
            eval "$1=\"$d\""
            return
        fi
    done
    Fail "There are so many builds directories ($scriptdir/build_0?)\
 already.\n  Why not remove some?"
}

# List submodules with 'cat .git/config'
# you must be in the build dir (or build_0?/src/)
# to run this.
function GitTarSubMod()
{
    [ -n "$2" ] || Fail "Usage: ${FUNCNAME[0]} DIR TAG"
    cwd="$PWD"
    cd "$1" || Fail
    to="$PWD"
    set -x
    cd "$gitdir/$1" || Fail
    git archive  --format=tar "$2" | $(cd "$to" && tar -xf -) || Fail
    cd "$cwd" || Fail
}

#Exmaple: GitToBuildDir 435.8
#Usage: GitToBuildDir [TAG [--separate-src-build]]
#
#  Option: --separate-src-build puts source in src/
#                   and then cd to build/ in the new
#                   build0?/ dir.  So with this option
#                   you make build0?/src/ with the source
#                   and build0?/build/ to build in using
#                   build0?/src/
# 
function GitToBuildDir()
{
    local tag
    local bdir
    local sdir
    if [ -n "$1" ] ; then
        tag="$1"
    else
        tag="$name"
    fi

    cd "$gitdir" || Fail
    if ! git rev-parse $tag >/dev/null 2>&1 ; then
        Fail "git tag \"$tag\" not found"
    fi

    sdir=
    MkBuildDir sdir

    if [ "$2" = "--separate-src-build" ] ; then
        # We make more file structure for stupid packages
        # like paraview that cannot build in the same dir
        # as the source.
        bdir="$sdir/build"
        sdir="$sdir/src"
        mkdir "$bdir" "$sdir" || Fail
    else
        # build dir is the source dir
        bdir="$sdir"
    fi

    set -x
    # dump the source tree of a given version with git tag $tag
    git archive  --format=tar "$tag" | $(cd "$sdir" && tar -xf -) || Fail
    cd "$bdir" || Fail
    set +x
}

# prints message to stderr
function PrintSuccess()
{
    set +x
    echo -ne "\x1b[44;1m Successfully installed: \x1b[42;34;1m\
$name\x1b[44;37;1m in \x1b[42;34;1m$prefix\x1b[44;37;1m "

    echo -e "\x1b[0m\n"
    if [ -n "$*" ] ; then
        echo -e "\n \x1b[45;1m $* \x1b[0m\n"
    fi
    _PrintVars
    if [ -d "$prefix" ] ; then
        echo -e "You may want to run: sudo encap\n"
    fi
    echo
    exit
}
