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
    j="  "
    let i=$i-1
    printStackLine $i
    let i=$i-1
    j="$j  "
    while [ "$i" != "0" ] ; do
        printStackLine $i
        let i=$i-1
        j="$j  "
    done
    exit 1
}

# Print Call Stack
# TODO remove this unused function
function StackTrace ()
{
  local i=0
  local FRAMES=${#BASH_LINENO[@]}
  local j=""
  echo -e "\nStack Trace\n"
  for ((i=FRAMES-2; i>=1; i--)); do
    echo ${j}${BASH_SOURCE[i+1]}:${BASH_LINENO[i]} in ${FUNCNAME[i+1]}
    # Grab the source code of the line
    echo
    sed -n "${BASH_LINENO[i]}{s/^/    /;p}" "${BASH_SOURCE[i+1]}"
    echo
    j="$j  "
  done
}

# Usage: PrintLine FILE LINE_NO
function PrintLine()
{
    echo -n " >> "
    if [[ "$1" =~ ^/ ]] ; then
        sed -n "${2},${2}p" "$1" 1>&2
    else
        sed -n "${2},${2}p" "$scriptdir/$1" 1>&2
    fi
}

indent="  "

# Usage: printStackLine NO
function printStackLine()
{
    echo -n "${indent}${BASH_SOURCE[(($1+1))]}:${BASH_LINENO[(($1))]}\
  ${FUNCNAME[(($1+1))]}()" 1>&2
    PrintLine "${BASH_SOURCE[(($1+1))]}" ${BASH_LINENO[(($1))]}
    #indent="$indent  "
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
    cat << EOF
${BASH_SOURCE[0]} setup:

    scriptdir=$scriptdir
    name=$name
    topsrcdir=$topsrcdir
    prefix=$prefix

EOF

    [ -e "$root/encap" ] || Fail "encap root $root/encap does not exist"
    [ ! -e "$prefix" ] || Fail "prefix \"$prefix\" exists already"
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
    if [ -d "$prefix" ] ; then
        echo -e "You may want to run: sudo encap\n"
    fi
    echo
    exit
}
