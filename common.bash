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
    exit 1
}

function _PrintVars()
{
    cat << EOF || Fail

${BASH_SOURCE[0]} setup:

    scriptdir=$scriptdir
    name=$name
    topsrcdir=$topsrcdir
    prefix=$prefix

EOF
}


function Init()
{
    local i
    i=${#BASH_SOURCE[@]}
    let i=$i-1

    # now BASH_SOURCE[$i] the top bash script file
    scriptdir="$(dirname ${BASH_SOURCE[$i]})"
    cd "$scriptdir" || Fail "cd to scriptdir \"$scriptdir\" failed"
    scriptdir="$PWD" # now we have full path
    name="$(basename $scriptdir)" || Fail
    prefix="$root/encap/$name"
    cd .. || Fail
    topsrcdir="$PWD"
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



# Usage: MkBuildDir builddir
# makes an empty directory in the current directory
# and set $1 to the directory path
function MkBuildDir()
{
    for i in build_01 build_02 build_03 build_04 build_05 build_06 ; do
        d="${PWD}/$i"
        if [ ! -e "$d" ] ; then
            mkdir -p "$d" || Fail
            echo "Build directory set to $d" 1>&2
            eval "$1=\"$d\""
            return
        fi
    done
    Fail "So many builds directories already.  Why not remove some?"
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
