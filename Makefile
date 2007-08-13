# Generated automatically from Makefile.in by configure.
# DIST: This is the distribution Makefile for XEmacs.  configure can
# DIST: make most of the changes to this file you might want, so try
# DIST: that first.

# This file is part of XEmacs.

# XEmacs is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2, or (at your option) any
# later version.

# XEmacs is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.

# You should have received a copy of the GNU General Public License
# along with XEmacs; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.

# make all	to compile and build XEmacs.
# make install	to install it.
# make TAGS	to update tags tables.
#
# make clean  or  make mostlyclean
#      Delete all files from the current directory that are normally
#      created by building the program.	 Don't delete the files that
#      record the configuration.  Also preserve files that could be made
#      by building, but normally aren't because the distribution comes
#      with them.
#
#      Delete `.dvi' files here if they are not part of the distribution.
# 
# make distclean
#      Delete all files from the current directory that are created by
#      configuring or building the program.  If you have unpacked the
#      source and built the program without creating any other files,
#      `make distclean' should leave only the files that were in the
#      distribution.
# 
# make realclean
#      Delete everything from the current directory that can be
#      reconstructed with this Makefile.  This typically includes
#      everything deleted by distclean, plus more: C source files
#      produced by Bison, tags tables, info files, and so on.
#
# make extraclean
#      Still more severe - delete backup and autosave files, too.

SHELL = /bin/sh


# ==================== Things `configure' Might Edit ====================

CC=gcc
CPP=gcc -E -I/usr/X11R6/include
C_SWITCH_SYSTEM= -DFUNCPROTO=11 -DNARROWPROTO -D_BSD_SOURCE 
LN_S=ln -s
CFLAGS=-m486 -g -O4 -malign-loops=2 -malign-jumps=2 -malign-functions=2
C_SWITCH_X_SITE=-I/usr/X11R6/include
LD_SWITCH_X_SITE=-L/usr/X11R6/lib
YACC=bison -y

### These help us choose version- and architecture-specific directories
### to install files in.

### This should be the number of the XEmacs version we're building,
### like `19.12' or `19.13'.
version=20.0-b32

### This should be the name of the configuration we're building XEmacs
### for, like `mips-dec-ultrix' or `sparc-sun-sunos'.
configuration=i586-unknown-linux2.0.27

### Libraries which should be edited into lib-src/Makefile.
libsrc_libs= -lgcc -lc -lgcc /usr/lib/crtn.o 

# ==================== Where To Install Things ====================

# The default location for installation.  Everything is placed in
# subdirectories of this directory.  The default values for many of
# the variables below are expressed in terms of this one, so you may
# not need to change them.  This defaults to /usr/local.
prefix=/usr/local

# Like `prefix', but used for architecture-specific files.
exec_prefix=${prefix}

# Where to install XEmacs and other binaries that people will want to
# run directly (like etags).
bindir=${exec_prefix}/bin

# Where to install architecture-independent data files.	 ${lispdir}
# and ${etcdir} are subdirectories of this.
datadir=${prefix}/lib

# Where to install and expect the files that XEmacs modifies as it
# runs.	 These files are all architecture-independent. Right now, the
# only such data is the locking directory; ${lockdir} is a
# subdirectory of this.
statedir=${prefix}/lib

# Where to install and expect executable files to be run by XEmacs
# rather than directly by users, and other architecture-dependent
# data.	 ${archlibdir} is a subdirectory of this.
libdir=${exec_prefix}/lib

# Where to install XEmacs's man pages, and what extension they should have.
mandir=${prefix}/man/man1
manext=.1

# Where to install and expect the info files describing XEmacs.	In the
# past, this defaulted to a subdirectory of ${prefix}/lib/xemacs, but
# since there are now many packages documented with the texinfo
# system, it is inappropriate to imply that it is part of XEmacs.
infodir=${prefix}/lib/xemacs-${version}/info

# This is set to 'yes' if the user specified the --infodir flag at
# configuration time.
infodir_user_defined=no

# Where to find the source code.  The source code for XEmacs's C kernel is
# expected to be in ${srcdir}/src, and the source code for XEmacs's
# utility programs is expected to be in ${srcdir}/lib-src.  This is
# set by the configure script's `--srcdir' option.
srcdir=/usr/local/xemacs/xemacs-20.0-b32

# Tell make where to find source files; this is needed for the makefiles.
VPATH=/usr/local/xemacs/xemacs-20.0-b32

# ==================== XEmacs-specific directories ====================

# These variables hold the values XEmacs will actually use.  They are
# based on the values of the standard Make variables above.

# Where to install the lisp files distributed with
# XEmacs.  This includes the XEmacs version, so that the
# lisp files for different versions of XEmacs will install
# themselves in separate directories.
lispdir=${datadir}/xemacs-${version}/lisp

# This is set to 'yes' if the user specified the --lispdir or
# --datadir flag at configuration time.
lispdir_user_defined=no

# Directories XEmacs should search for lisp files specific
# to this site (i.e. customizations), before consulting
# ${lispdir}.  This should be a colon-separated list of
# directories.
sitelispdir=${datadir}/xemacs/site-lisp

# Where XEmacs will search for its lisp files while
# building.  This is only used during the process of
# compiling XEmacs, to help XEmacs find its lisp files
# before they've been installed in their final location.
# It's usually identical to lispdir, except that the
# entry for the directory containing the installed lisp
# files has been replaced with ../lisp.  This should be a
# colon-separated list of directories.
buildlispdir=${srcdir}/lisp

# Where to install the other architecture-independent
# data files distributed with XEmacs (like the tutorial,
# the cookie recipes and the Zippy database). This path
# usually contains the XEmacs version number, so the data
# files for multiple versions of XEmacs may be installed
# at once.
etcdir=${datadir}/xemacs-${version}/etc

# This is set to 'yes' if the user specified the --etcdir or
# --datadir flag at configuration time.
etcdir_user_defined=no

# Where to create and expect the locking directory, where
# the XEmacs locking code keeps track of which files are
# currently being edited.
lockdir=${statedir}/xemacs/lock

# This is set to 'yes' if the user specified the --lockdir or
# --statedir flag at configuration time.
lockdir_user_defined=no

# Where to put executables to be run by XEmacs rather than
# the user.  This path usually includes the XEmacs version
# and configuration name, so that multiple configurations
# for multiple versions of XEmacs may be installed at
# once.
archlibdir=${libdir}/xemacs-${version}/${configuration}

# This is set to 'yes' if the user specified any of --exec-prefix,
# --libdir or --archlibdir at configuration time.
archlibdir_user_defined=no

# ==================== Utility Programs for the Build ====================

# Allow the user to specify the install program.
INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644

# ============================= Targets ==============================

# Subdirectories to make recursively.  `lisp' is not included
# because the compiled lisp files are part of the distribution
# and you cannot remake them without installing XEmacs first.
MAKE_SUBDIR = lib-src lwlib dynodump src

# Subdirectories that can be made recursively.
SUBDIR = ${MAKE_SUBDIR} man

# The makefiles of the directories in $SUBDIR.
SUBDIR_MAKEFILES = lib-src/Makefile lwlib/Makefile dynodump/Makefile src/Makefile

# Subdirectories to install, and where they'll go.
# lib-src's makefile knows how to install it, so we don't do that here.
# When installing the info files, we need to do special things to
# avoid nuking an existing dir file, so we don't do that here;
# instead, we have written out explicit code in the `install' targets.
COPYDIR = ${srcdir}/etc ${srcdir}/lisp
COPYDESTS = ${etcdir} ${lispdir}

.NO_PARALLEL:	src/paths.h src/Emacs.ad.h ${MAKE_SUBDIR} dump-elcs

all:	src/paths.h src/Emacs.ad.h ${MAKE_SUBDIR}

# Convenience target for XEmacs beta testers
beta:	clean all-elc

# Build XEmacs and recompile out-of-date and missing .elc files along
# the way.
all-elc all-elcs:  src/paths.h src/Emacs.ad.h lib-src lwlib dump-elcs src
	sh ${srcdir}/lib-src/update-elc.sh

# Sub-target for all-elc.
dump-elc dump-elcs:  FRC.dump-elcs
	cd src; $(MAKE) dump-elcs $(MFLAGS) \
		CC='${CC}' CFLAGS='${CFLAGS}' MAKE='${MAKE}'

autoloads:	src
	sh ${srcdir}/lib-src/update-autoloads.sh

# We force the rebuilding of src/paths.h because the user might give
# different values for the various directories.  Since we use
# move-if-change, src/paths.h only actually changes if the user did
# something notable, so the only unnecessary work we do is in building
# src/paths.h.tmp, which isn't much.  Note that sed is not in /bin on
# 386bsd.
src/paths.h: Makefile ${srcdir}/src/paths.h.in FRC.src.paths.h
	@echo "Producing \`src/paths.h' from \`src/paths.h.in'."
	-rm -f src/paths.h.tmp
	@cp ${srcdir}/src/paths.h.in src/paths.h.tmp
	-chmod 0644 src/paths.h.tmp
	@echo '#define PATH_PREFIX "${prefix}"' >> src/paths.h.tmp
	@if [ x"${lispdir_user_defined}" = x"yes" ]; then \
	  echo '#define PATH_LOADSEARCH "${lispdir}"' >> src/paths.h.tmp;\
	else \
	  echo '/* #define PATH_LOADSEARCH "${lispdir}" */' >>src/paths.h.tmp;\
	fi
	@if [ x"${archlibdir_user_defined}" = x"yes" ]; then \
	  echo '#define PATH_EXEC "${archlibdir}"' >> src/paths.h.tmp ;\
	else \
	  echo '/* #define PATH_EXEC "${archlibdir}" */' >> src/paths.h.tmp ;\
	fi
	@if [ x"${etcdir_user_defined}" = x"yes" ]; then \
	  echo '#define PATH_DATA "${etcdir}"' >> src/paths.h.tmp ;\
	else \
	  echo '/* #define PATH_DATA "${etcdir}" */' >> src/paths.h.tmp ;\
	fi
	@if [ x"${lockdir_user_defined}" = x"yes" ]; then \
	  echo '#define PATH_LOCK "${lockdir}"' >> src/paths.h.tmp ;\
	else \
	  echo '/* #define PATH_LOCK "${lockdir}" */' >> src/paths.h.tmp ;\
	fi
	@if [ x"${infodir_user_defined}" = x"yes" ]; then \
	  echo '#define PATH_INFO "${infodir}"' >> src/paths.h.tmp ;\
	else \
	  echo '/* #define PATH_INFO "${infodir}" */' >> src/paths.h.tmp ;\
	fi
	@sh ${srcdir}/move-if-change src/paths.h.tmp src/paths.h

# We have to force the building of Emacs.ad.h as well in order to get it
# updated correctly when VPATH is being used.  Since we use move-if-change,
# it will only actually change if the user modified ${etcdir}/Emacs.ad.
src/Emacs.ad.h: ${srcdir}/etc/Emacs.ad
	@echo "Producing \`src/Emacs.ad.h' from \`etc/Emacs.ad'."
	-rm -f src/Emacs.ad.h
	@(echo "/*	Do not edit this file!" ; \
	  echo "  	Automatically generated from ${srcdir}/etc/Emacs.ad" ; \
	  echo " */" ; \
	  /bin/sh ${srcdir}/lib-src/ad2c ${srcdir}/etc/Emacs.ad ) > \
	  src/Emacs.ad.h

src:	lib-src lwlib dynodump FRC.src
lib-src: FRC.lib-src
lwlib:	FRC.lwlib
dynodump: FRC.dynodump

.RECURSIVE: ${SUBDIR}

${SUBDIR}: ${SUBDIR_MAKEFILES} src/config.h FRC
	cd $@; $(MAKE) all $(MFLAGS) \
		CC='${CC}' CFLAGS='${CFLAGS}' MAKE='${MAKE}'

Makefile: ${srcdir}/Makefile.in config.status
	./config.status

src/Makefile: ${srcdir}/src/Makefile.in.in config.status
	./config.status

lib-src/Makefile: ${srcdir}/lib-src/Makefile.in.in config.status
	./config.status

lwlib/Makefile: ${srcdir}/lwlib/Makefile.in.in config.status
	./config.status

dynodump/Makefile: ${srcdir}/dynodump/Makefile.in.in config.status
	./config.status

src/config.h: ${srcdir}/src/config.h.in
	./config.status

# ==================== Installation ====================

## If we let lib-src do its own installation, that means we
## don't have to duplicate the list of utilities to install in
## this Makefile as well.

## On AIX, use tar xBf.
## On Xenix, use tar xpf.

.PHONY: install install-arch-dep install-arch-indep mkdir

## We delete each directory in ${COPYDESTS} before we copy into it;
## that way, we can reinstall over directories that have been put in
## place with their files read-only (perhaps because they are checked
## into RCS).  In order to make this safe, we make sure that the
## source exists and is distinct from the destination.

## FSF doesn't depend on `all', but rather on ${MAKE_SUBDIR}, so that
## they "won't ever modify src/paths.h".  But that means you can't do
## 'make install' right off the bat because src/paths.h won't exist.
## And, in XEmacs case, src/Emacs.ad.h won't exist either.  I also
## don't see the point in avoiding modifying paths.h.  It creates an
## inconsistency in the build process.  So we go ahead and depend on
## all.  --cet

install: all install-arch-dep install-arch-indep;

install-arch-dep: mkdir
	(cd lib-src && \
	  $(MAKE) install $(MFLAGS) prefix=${prefix} \
	    exec_prefix=${exec_prefix} bindir=${bindir} libdir=${libdir} \
	    archlibdir=${archlibdir})
	if [ `(cd ${archlibdir}; /bin/pwd)` != `(cd ./lib-src; /bin/pwd)` ]; \
	then \
	   ${INSTALL_DATA} lib-src/DOC ${archlibdir}/DOC ; \
	   for subdir in `find ${archlibdir} -type d ! -name RCS ! -name SCCS ! -name CVS -print` ; do \
	     rm -rf $${subdir}/RCS $${subdir}/CVS $${subdir}/SCCS ; \
	     rm -f  $${subdir}/\#* $${subdir}/*~ ; \
	   done ; \
	else true; fi
	${INSTALL_PROGRAM} src/xemacs ${bindir}/xemacs-${version}
	-chmod 0755 ${bindir}/xemacs-${version}
	rm -f ${bindir}/xemacs
	(cd ${bindir} ; ${LN_S} xemacs-${version} ./xemacs)

install-arch-indep: mkdir
	-set ${COPYDESTS} ; \
	 for dir in ${COPYDIR} ; do \
	   if [ `(cd $$1 && pwd)` != `(cd $${dir} && pwd)` ] ; then \
	     echo "rm -rf $$1" ; \
	   fi ; \
	   shift ; \
	 done
	-set ${COPYDESTS} ; \
	 mkdir ${COPYDESTS} ; \
	 for dir in ${COPYDIR} ; do \
	   dest=$$1 ; shift ; \
	   [ -d $${dir} ] \
	   && [ `(cd $${dir} && /bin/pwd)` != `(cd $${dest} && /bin/pwd)` ] \
	   && (echo "Copying $${dir}..." ; \
	       (cd $${dir}; tar -cf - . )|(cd $${dest};umask 022; tar -xf - );\
	       chmod 0755 $${dest}; \
	       for subdir in `find $${dest} -type d ! -name RCS ! -name SCCS ! -name CVS -print` ; do \
		 rm -rf $${subdir}/RCS $${subdir}/CVS $${subdir}/SCCS ; \
		 rm -f  $${subdir}/\#* $${subdir}/*~ ; \
	       done) ; \
	 done
	if [ `(cd ${srcdir}/info && /bin/pwd)` != `(cd ${infodir} && /bin/pwd)` ]; \
	then \
	  (cd ${srcdir}/info ; \
	   if [ ! -f ${infodir}/dir ] && [ -f dir ]; then \
	     ${INSTALL_DATA} ${srcdir}/info/dir ${infodir}/dir ; \
	   fi ; \
	   for f in ange-ftp* cc-mode* cl* dired* ediff* external-widget* \
		    forms* gnus* hyperbole* ilisp* info* internals* \
		    ispell* lispref* mailcrypt* message* mh-e* \
                    new-users-guide* oo-browser* pcl-cvs* psgml* rmail* \
                    standards* supercite* term.* termcap* texinfo* viper* \
                    vm* w3* xemacs* ; do \
	     ${INSTALL_DATA} ${srcdir}/info/$$f ${infodir}/$$f ; \
	     chmod 0644 ${infodir}/$$f; \
	     gzip -9 ${infodir}/$$f; \
	   done); \
	else true; fi
	cd ${srcdir}/etc; for page in xemacs etags ctags gnuserv \
				      gnuclient gnuattach gnudoit ; do \
	  ${INSTALL_DATA} ${srcdir}/etc/$${page}.1 ${mandir}/$${page}${manext} ; \
	  chmod 0644 ${mandir}/$${page}${manext} ; \
	done

MAKEPATH=./lib-src/make-path
### Build all the directories we're going to install XEmacs in.	Since
### we may be creating several layers of directories (for example,
### /usr/local/lib/xemacs-19.13/mips-dec-ultrix4.2), we use make-path
### instead of mkdir.  Not all systems' mkdirs have the `-p' flag.
mkdir: FRC.mkdir
	${MAKEPATH} ${COPYDESTS} ${lockdir} ${infodir} ${mandir} \
	  ${bindir} ${datadir} ${libdir} ${sitelispdir}
	-chmod 0777 ${lockdir}

### Delete all the installed files that the `install' target would
### create (but not the noninstalled files such as `make all' would
### create).
###
### Don't delete the lisp and etc directories if they're in the source tree.
#### This target has not been updated in sometime and until it is it
#### would be extremely dangerous for anyone to use it.
#uninstall:
#	(cd lib-src; 					\
#	 $(MAKE) $(MFLAGS) uninstall			\
#	    prefix=${prefix} exec_prefix=${exec_prefix}	\
#	    bindir=${bindir} libdir=${libdir} archlibdir=${archlibdir})
#	for dir in ${lispdir} ${etcdir} ; do 		\
#	  case `(cd $${dir} ; pwd)` in			\
#	    `(cd ${srcdir} ; pwd)`* ) ;;		\
#	    * ) rm -rf $${dir} ;;			\
#	  esac ;					\
#	  case $${dir} in				\
#	    ${datadir}/xemacs/${version}/* )		\
#	      rm -rf ${datadir}/xemacs/${version}	\
#	    ;;						\
#	  esac ;					\
#	done
#	(cd ${infodir} && rm -f cl* xemacs* forms* info* vip*)
#	(cd ${mandir} && rm -f xemacs.1 etags.1 ctags.1 gnuserv.1)
#	(cd ${bindir} && rm -f xemacs-${version} xemacs)


### Some makes seem to remember that they've built something called FRC,
### so you can only use a given FRC once per makefile.
FRC FRC.src.paths.h FRC.src FRC.lib-src FRC.lwlib FRC.mkdir FRC.dump-elcs:
FRC.dynodump:
FRC.mostlyclean FRC.clean FRC.distclean FRC.realclean:

# ==================== Cleaning up and miscellanea ====================

.PHONY: mostlyclean clean distclean realclean extraclean

### `mostlyclean'
###      Like `clean', but may refrain from deleting a few files that people
###      normally don't want to recompile.  For example, the `mostlyclean'
###      target for GCC does not delete `libgcc.a', because recompiling it
###      is rarely necessary and takes a lot of time.
mostlyclean: FRC.mostlyclean
	(cd src      && $(MAKE) $(MFLAGS) mostlyclean)
	(cd lib-src  && $(MAKE) $(MFLAGS) mostlyclean)
	(cd lwlib    && $(MAKE) $(MFLAGS) mostlyclean)
	(cd dynodump && $(MAKE) $(MFLAGS) mostlyclean)
	-(cd man     && $(MAKE) $(MFLAGS) mostlyclean)

### `clean'
###      Delete all files from the current directory that are normally
###      created by building the program.  Don't delete the files that
###      record the configuration.  Also preserve files that could be made
###      by building, but normally aren't because the distribution comes
###      with them.
### 
###      Delete `.dvi' files here if they are not part of the distribution.
clean: FRC.clean
	(cd src      && $(MAKE) $(MFLAGS) clean)
	(cd lib-src  && $(MAKE) $(MFLAGS) clean)
	(cd lwlib    && $(MAKE) $(MFLAGS) clean)
	(cd dynodump && $(MAKE) $(MFLAGS) clean)
	-(cd man     && $(MAKE) $(MFLAGS) clean)

### `distclean'
###      Delete all files from the current directory that are created by
###      configuring or building the program.  If you have unpacked the
###      source and built the program without creating any other files,
###      `make distclean' should leave only the files that were in the
###      distribution.
top_distclean=\
	rm -f config.status config-tmp-* build-install ; \
	rm -f Makefile ${SUBDIR_MAKEFILES}; \
	(cd lock && rm -f *)

distclean: FRC.distclean
	(cd src      && $(MAKE) $(MFLAGS) distclean)
	(cd lib-src  && $(MAKE) $(MFLAGS) distclean)
	(cd lwlib    && $(MAKE) $(MFLAGS) distclean)
	(cd dynodump && $(MAKE) $(MFLAGS) distclean)
	-(cd man     && $(MAKE) $(MFLAGS) distclean)
	-${top_distclean}

### `realclean'
###      Delete everything from the current directory that can be
###      reconstructed with this Makefile.  This typically includes
###      everything deleted by distclean, plus more: C source files
###      produced by Bison, tags tables, info files, and so on.
### 
###      One exception, however: `make realclean' should not delete
###      `configure' even if `configure' can be remade using a rule in the
###      Makefile.  More generally, `make realclean' should not delete
###      anything that needs to exist in order to run `configure' and then
###      begin to build the program.
realclean: FRC.realclean
	(cd src      && $(MAKE) $(MFLAGS) realclean)
	(cd lib-src  && $(MAKE) $(MFLAGS) realclean)
	(cd lwlib    && $(MAKE) $(MFLAGS) realclean)
	(cd dynodump && $(MAKE) $(MFLAGS) realclean)
	-(cd man     && $(MAKE) $(MFLAGS) realclean)
	-${top_distclean}

### This doesn't actually appear in the coding standards, but Karl
### says GCC supports it, and that's where the configuration part of
### the coding standards seem to come from.  It's like distclean, but
### it deletes backup and autosave files too.
extraclean:
	(cd src      && $(MAKE) $(MFLAGS) extraclean)
	(cd lib-src  && $(MAKE) $(MFLAGS) extraclean)
	(cd lwlib    && $(MAKE) $(MFLAGS) extraclean)
	(cd dynodump && $(MAKE) $(MFLAGS) extraclean)
	-(cd man     && $(MAKE) $(MFLAGS) extraclean)
	-rm -f *~ \#*
	-${top_distclean}

### Unlocking and relocking.  The idea of these productions is to reduce
### hassles when installing an incremental tar of XEmacs.  Do `make unlock'
### before unlocking the file to take the write locks off all sources so
### that tar xvof will overwrite them without fuss.  Then do `make relock'
### afterward so that VC mode will know which files should be checked in
### if you want to mung them.
###
### Note: it's no disaster if these productions miss a file or two; tar
### and VC will swiftly let you know if this happens, and it is easily
### corrected.
SOURCES = ChangeLog GETTING.GNU.SOFTWARE INSTALL Makefile.in PROBLEMS \
	README build-install.in configure make-dist move-if-change

.PHONY: unlock relock

unlock:
	chmod u+w $(SOURCES) cpp/*
	-(cd elisp && chmod u+w Makefile README *.texi)
	(cd etc     && $(MAKE) $(MFLAGS) unlock)
	(cd lib-src && $(MAKE) $(MFLAGS) unlock)
	(cd lisp    && $(MAKE) $(MFLAGS) unlock)
	(cd lisp/term && chmod u+w README *.el)
	(cd man && chmod u+w *texi* ChangeLog split-man)
	(cd lwlib && chmod u+w *.[ch] Makefile.in.in)
	(cd src && $(MAKE) $(MFLAGS) unlock)

relock:
	chmod u-w $(SOURCES) cpp/*
	-(cd elisp && chmod u-w Makefile README *.texi)
	(cd etc     && $(MAKE) $(MFLAGS) relock)
	(cd lib-src && $(MAKE) $(MFLAGS) relock)
	(cd lisp    && $(MAKE) $(MFLAGS) relock)
	(cd lisp/term && chmod u+w README *.el)
	(cd man && chmod u+w *texi* ChangeLog split-man)
	(cd lwlib && chmod u+w *.[ch] Makefile.in.in)
	(cd src && $(MAKE) $(MFLAGS) relock)

.PHONY: TAGS tags check dist

TAGS tags:
	@echo "If you don't have a copy of etags around, then do 'make lib-src' first."
	@PATH=`pwd`/lib-src:$$PATH HOME=/-=-; export PATH HOME; \
	  echo "Using etags from `which etags`."
	PATH=`pwd`/lib-src:$$PATH ; export PATH; cd ${srcdir} ; \
	etags --regex='/[ 	]*DEF\(VAR\|INE\)_[A-Z_]+[ 	]*([ 	]*"\([^"]+\)"/\2/' src/*.[ch] ; \
	for d in `find lisp -name SCCS -prune -o -name RCS -prune -o -type d -print` ; do \
	  (cd $$d ; if [ "`echo *.el`" != "*.el" ] ; then etags -a -o ${srcdir}/TAGS *.el ; fi ) ; \
	done ; \
	etags -a lwlib/*.[ch]

check:
	@echo "We don't have any automated tests for XEmacs yet."

dist:
	cd ${srcdir} && make-dist

.PHONY: info dvi
force-info:
info: force-info
	cd ${srcdir}/man && $(MAKE) $(MFLAGS) $@
dvi:
	cd ${srcdir}/man && $(MAKE) $(MFLAGS) $@

# Fix up version information in executables (Solaris-only)
mcs:
	date=`LANG=C LC_ALL=C date -u '+%e %b %Y'`; \
	ident="@(#)RELEASE VERSION XEmacs ${version} $${date}"; \
	for f in `file lib-src/* src/xemacs | grep ELF | sed -e 's/:.*//'`; do \
	  mcs -da "$${ident} `echo $${f} | sed 's/.*\///'`" $${f}; \
	done
