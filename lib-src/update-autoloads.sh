#!/bin/sh
### update-autoloads.sh --- update auto-autoloads.el as necessary

set -eu

# This means we're running in a Sun workspace
test -d ../era-specific && cd ../editor

# get to the right directory
test ! -d ./lisp -a -d ../lisp && cd ..
if test ! -d ./lisp ; then
  echo $0: neither ./lisp/ nor ../lisp/ exist
  exit 1
fi

EMACS="./src/xemacs"
echo " (using $EMACS)"

export EMACS

REAL=`cd \`dirname $EMACS\` ; pwd | sed 's|^/tmp_mnt||'`/`basename $EMACS`

echo "Recompiling in `pwd|sed 's|^/tmp_mnt||'`"
echo "          with $REAL..."

dirs=
for dir in lisp/*; do
  if test -d $dir \
   -a $dir != lisp/CVS \
   -a $dir != lisp/SCCS \
   -a $dir != lisp/egg \
   -a $dir != lisp/its \
   -a $dir != lisp/language \
   -a $dir != lisp/leim; then
    dirs="$dirs $dir"
  fi
done
# cat > lisp/prim/auto-autoloads.el << EOF
# ;;; Do NOT edit this file!
# ;;; It is automatically generated using "make autoloads"
# ;;; See update-autoloads.sh and autoload.el for more details.
# 
# EOF
set -x
for dir in $dirs; do
	$EMACS -batch -q -l autoload -f batch-update-directory $dir
done
