# .bashrc

if [ -f /etc/bashrc ]; then
        . /etc/bashrc
fi

alias b='cd ..;pwd'
alias egrep='egrep --color=auto'
alias fgrep='fgrep --color=auto'
alias gittagbydate='git log --date-order --tags --simplify-by-decoration --pretty="format:%ai %d"'
alias grep='grep --color=auto'
alias l='ls -CF --color=auto'
alias la='ls -aCF --color=auto'
alias ll='ls -thlF --color=auto'
alias lla='ls -athlF --color=auto'
