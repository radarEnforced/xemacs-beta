/* Utility routines for XEmacs on Windows 9x, NT and Cygwin.
   Copyright (C) 2000 Ben Wing.

This file is part of XEmacs.

XEmacs is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2, or (at your option) any
later version.

XEmacs is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with XEmacs; see the file COPYING.  If not, write to the Free
Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA. */

#include <config.h>
#include "lisp.h"

#include "buffer.h"

#include "syssignal.h"
#include "systime.h"
#include "syswindows.h"

typedef BOOL (WINAPI *pfSwitchToThread_t) (VOID);
pfSwitchToThread_t xSwitchToThread;

typedef HKL (WINAPI *pfGetKeyboardLayout_t) (DWORD);
pfGetKeyboardLayout_t xGetKeyboardLayout;
typedef BOOL (WINAPI *pfSetMenuDefaultItem_t) (HMENU, UINT, UINT);
pfSetMenuDefaultItem_t xSetMenuDefaultItem;
typedef BOOL (WINAPI *pfInsertMenuItemA_t) 
     (HMENU, UINT, BOOL, LPCMENUITEMINFOA);
pfInsertMenuItemA_t xInsertMenuItemA;
typedef BOOL (WINAPI *pfInsertMenuItemW_t) 
     (HMENU, UINT, BOOL, LPCMENUITEMINFOW);
pfInsertMenuItemW_t xInsertMenuItemW;
typedef HANDLE (WINAPI *pfLoadImageA_t) 
     (HINSTANCE, LPCSTR, UINT, int, int, UINT);
pfLoadImageA_t xLoadImageA;
typedef HANDLE (WINAPI *pfLoadImageW_t)
     (HINSTANCE, LPCWSTR, UINT, int, int, UINT);
pfLoadImageW_t xLoadImageW;
typedef ATOM (WINAPI *pfRegisterClassExA_t) (CONST WNDCLASSEXA *);
pfRegisterClassExA_t xRegisterClassExA;
typedef ATOM (WINAPI *pfRegisterClassExW_t) (CONST WNDCLASSEXW *);
pfRegisterClassExW_t xRegisterClassExW;

typedef int (WINAPI *pfEnumFontFamiliesExA_t) 
     (HDC, LPLOGFONTA, FONTENUMPROCA, LPARAM, DWORD);
pfEnumFontFamiliesExA_t xEnumFontFamiliesExA;
typedef int (WINAPI *pfEnumFontFamiliesExW_t) 
     (HDC, LPLOGFONTW, FONTENUMPROCW, LPARAM, DWORD);
pfEnumFontFamiliesExW_t xEnumFontFamiliesExW;

typedef DWORD (WINAPI *pfSHGetFileInfoA_t) 
     (LPCSTR, DWORD, SHFILEINFOA FAR *, UINT, UINT);
pfSHGetFileInfoA_t xSHGetFileInfoA;
typedef DWORD (WINAPI *pfSHGetFileInfoW_t) 
     (LPCWSTR, DWORD, SHFILEINFOW FAR *, UINT, UINT);
pfSHGetFileInfoW_t xSHGetFileInfoW;

typedef NET_API_STATUS (NET_API_FUNCTION *pfNetUserEnum_t)
     (
      IN  LPCWSTR     servername OPTIONAL,
      IN  DWORD      level,
      IN  DWORD      filter,
      OUT LPBYTE     *bufptr,
      IN  DWORD      prefmaxlen,
      OUT LPDWORD    entriesread,
      OUT LPDWORD    totalentries,
      IN OUT LPDWORD resume_handle OPTIONAL
      );
pfNetUserEnum_t xNetUserEnum;

typedef NET_API_STATUS (NET_API_FUNCTION *pfNetApiBufferFree_t)
     (
      IN LPVOID Buffer
      );
pfNetApiBufferFree_t xNetApiBufferFree;

Lisp_Object
tstr_to_local_file_format (Extbyte *pathout)
{
  Intbyte *ttlff;
  Lisp_Object in;

  EXTERNAL_TO_C_STRING (pathout, ttlff, Qmswindows_tstr);
  WIN32_TO_LOCAL_FILE_FORMAT (ttlff, in);

  return in;
}

static void
init_potentially_nonexistent_functions (void)
{
  HMODULE h_kernel = GetModuleHandle ("kernel32");
  HMODULE h_user = GetModuleHandle ("user32");
  HMODULE h_gdi = GetModuleHandle ("gdi32");
  HMODULE h_shell = GetModuleHandle ("shell32");
  /* the following does not seem to get mapped in automatically */
  HMODULE h_netapi = LoadLibrary ("netapi32.dll");

  if (h_kernel)
    {
      xSwitchToThread =
	(pfSwitchToThread_t) GetProcAddress (h_kernel, "SwitchToThread");
    }

  if (h_user)
    {
      xGetKeyboardLayout =
	(pfGetKeyboardLayout_t) GetProcAddress (h_user, "GetKeyboardLayout");
      xSetMenuDefaultItem =
	(pfSetMenuDefaultItem_t) GetProcAddress (h_user, "SetMenuDefaultItem");
      xInsertMenuItemA =
	(pfInsertMenuItemA_t) GetProcAddress (h_user, "InsertMenuItemA");
      xInsertMenuItemW =
	(pfInsertMenuItemW_t) GetProcAddress (h_user, "InsertMenuItemW");
      xLoadImageA =
	(pfLoadImageA_t) GetProcAddress (h_user, "LoadImageA");
      xLoadImageW =
	(pfLoadImageW_t) GetProcAddress (h_user, "LoadImageW");
      xRegisterClassExA =
	(pfRegisterClassExA_t) GetProcAddress (h_user, "RegisterClassExA");
      xRegisterClassExW =
	(pfRegisterClassExW_t) GetProcAddress (h_user, "RegisterClassExW");
    }

  if (h_gdi)
    {
      xEnumFontFamiliesExA =
	(pfEnumFontFamiliesExA_t) GetProcAddress (h_gdi, "EnumFontFamiliesExA");
      xEnumFontFamiliesExW =
	(pfEnumFontFamiliesExW_t) GetProcAddress (h_gdi, "EnumFontFamiliesExW");
    }

  if (h_shell)
    {
      xSHGetFileInfoA =
	(pfSHGetFileInfoA_t) GetProcAddress (h_shell, "SHGetFileInfoA");
      xSHGetFileInfoW =
	(pfSHGetFileInfoW_t) GetProcAddress (h_shell, "SHGetFileInfoW");
    }

  if (h_netapi)
    {
      xNetUserEnum =
	(pfNetUserEnum_t) GetProcAddress (h_netapi, "NetUserEnum");
      xNetApiBufferFree =
	(pfNetApiBufferFree_t) GetProcAddress (h_netapi, "NetApiBufferFree");
    }
}

DEFUN ("mswindows-shell-execute", Fmswindows_shell_execute, 2, 4, 0, /*
Get Windows to perform OPERATION on DOCUMENT.
This is a wrapper around the ShellExecute system function, which
invokes the application registered to handle OPERATION for DOCUMENT.
OPERATION is typically \"open\", \"print\" or \"explore\" (but can be
nil for the default action), and DOCUMENT is typically the name of a
document file or URL, but can also be a program executable to run or
a directory to open in the Windows Explorer.

If DOCUMENT is a program executable, PARAMETERS can be a string
containing command line parameters, but otherwise should be nil.

SHOW-FLAG can be used to control whether the invoked application is hidden
or minimized.  If SHOW-FLAG is nil, the application is displayed normally,
otherwise it is an integer representing a ShowWindow flag:

  0 - start hidden
  1 - start normally
  3 - start maximized
  6 - start minimized
*/
       (operation, document, parameters, show_flag))
{
  /* Encode filename and current directory.  */
  Lisp_Object current_dir = Ffile_name_directory (document);
  char* path = NULL;
#ifdef CYGWIN
  char* fname1, *fname2;
  int pos, sz;
#endif
  char* doc = NULL;
  int ret;
  struct gcpro gcpro1, gcpro2;

  CHECK_STRING (document);

  if (NILP (current_dir))
    current_dir = current_buffer->directory;

  GCPRO2 (current_dir, document);

  /* Use mule and cygwin-safe APIs top get at file data. */
  if (STRINGP (current_dir))
    {
      LOCAL_TO_WIN32_FILE_FORMAT (current_dir, path);
    }

  if (STRINGP (document))
    {
      doc = XSTRING_DATA (document);
#ifdef CYGWIN
      if ((fname1 = strchr (doc, ':')) != NULL 
	  && *++fname1 == '/' && *++fname1 == '/')
	{
	  fname1++;
	  pos = fname1 - doc;
	  if (!(isalpha (fname1[0]) && (IS_DEVICE_SEP (fname1[1]))))
	    {
	      sz = cygwin_posix_to_win32_path_list_buf_size (fname1);
	      fname2 = alloca (sz + pos);
	      strncpy (fname2, doc, pos);
	      doc = fname2;
	      fname2 += pos;
	      cygwin_posix_to_win32_path_list (fname1, fname2);
	    }
	}
      else {
	LOCAL_TO_WIN32_FILE_FORMAT (document, doc);
      }
#endif
    }

  UNGCPRO;

  ret = (int) ShellExecute (NULL,
			    (STRINGP (operation) ?
			     /* !!#### more mule bogosity */
			     (char *) XSTRING_DATA (operation) : NULL),
			    doc, 
			    (STRINGP (parameters) ?
			     /* !!#### more mule bogosity */
			     (char *) XSTRING_DATA (parameters) : NULL),
			    path,
			    (INTP (show_flag) ?
			     XINT (show_flag) : SW_SHOWDEFAULT));

  if (ret > 32)
    return Qt;
  
  if (ret == ERROR_FILE_NOT_FOUND)
    signal_error (Qfile_error, "file not found", document);
  else if (ret == ERROR_PATH_NOT_FOUND)
    signal_error (Qfile_error, "path not found", current_dir);
  else if (ret == ERROR_BAD_FORMAT)
    signal_error (Qfile_error, "bad executable format", document);
  else
    signal_error (Qinternal_error, "internal error", Qunbound);

  return Qnil;
}

#ifdef CYGWIN
DEFUN ("mswindows-cygwin-to-win32-path", Fmswindows_cygwin_to_win32_path, 1, 1, 0, /*
Get the cygwin environment to convert the Unix PATH to win32 format.
No expansion is performed, all conversion is done by the cygwin runtime.
*/
       (path))
{
  Extbyte* f;
  Intbyte* p;
  CHECK_STRING (path);

  /* There appears to be a bug in the cygwin conversion routines in
     that they are not idempotent. */
  p = XSTRING_DATA (path);
  if (isalpha (p[0]) && (IS_DEVICE_SEP (p[1])))
    return path;

  /* Use mule and cygwin-safe APIs top get at file data. */
  LOCAL_TO_WIN32_FILE_FORMAT (path, f);
  return build_ext_string (f, Qnative);
}
#endif

#if defined (WIN32_NATIVE) || defined (CYGWIN_BROKEN_SIGNALS)

/* setitimer() does not exist on native MS Windows, and appears broken
   on Cygwin (random lockups when BROKEN_SIGIO is defined), so we
   emulate in both cases by using multimedia timers.  Furthermore,
   the lockups still occur on Cygwin even when we do nothing but
   use the standard signalling mechanism -- so we have to emulate
   that, too. (But only for timeouts -- we have to use the standard
   mechanism for SIGCHLD.  Yuck.)
 */


/*--------------------------------------------------------------------*/
/*                             Signal support                         */
/*--------------------------------------------------------------------*/

#define sigmask(nsig) (1U << nsig)

/* We can support as many signals as fit into word */
#define SIG_MAX 32

/* Signal handlers. Initial value = 0 = SIG_DFL */
static mswindows_sighandler signal_handlers[SIG_MAX] = {0};

/* Signal block mask: bit set to 1 means blocked */
unsigned signal_block_mask = 0;

/* Signal pending mask: bit set to 1 means sig is pending */
unsigned signal_pending_mask = 0;

mswindows_sighandler
mswindows_sigset (int nsig, mswindows_sighandler handler)
{
  /* We delegate some signals to the system function */
  if (nsig == SIGFPE || nsig == SIGABRT || nsig == SIGINT)
    return signal (nsig, handler);

  if (nsig < 0 || nsig > SIG_MAX)
    {
      errno = EINVAL;
      return NULL;
    }

  /* Store handler ptr */
  {
    mswindows_sighandler old_handler = signal_handlers[nsig];
    signal_handlers[nsig] = handler;
    return old_handler;
  }
}

int
mswindows_sighold (int nsig)
{
  if (nsig < 0 || nsig > SIG_MAX)
    return errno = EINVAL;

  signal_block_mask |= sigmask (nsig);
  return 0;
}

int
mswindows_sigrelse (int nsig)
{
  if (nsig < 0 || nsig > SIG_MAX)
    return errno = EINVAL;

  signal_block_mask &= ~sigmask (nsig);

  if (signal_pending_mask & sigmask (nsig))
    mswindows_raise (nsig);

  return 0;
}

int
mswindows_sigpause (int nsig)
{
  /* This is currently not called, because the only call to sigpause
     inside XEmacs is with SIGCHLD parameter. Just in case, we put an
     assert here, so anyone adds a call to sigpause will be surprised
     (or surprise someone else...) */
  assert (0);
  return 0;
}

int
mswindows_raise (int nsig)
{
  /* We delegate some raises to the system routine */
  if (nsig == SIGFPE || nsig == SIGABRT || nsig == SIGINT)
    return raise (nsig);

  if (nsig < 0 || nsig > SIG_MAX)
    return errno = EINVAL;

  /* If the signal is blocked, remember to issue later */
  if (signal_block_mask & sigmask (nsig))
    {
      signal_pending_mask |= sigmask (nsig);
      return 0;
    }

  if (signal_handlers[nsig] == SIG_IGN)
    return 0;

  if (signal_handlers[nsig] != SIG_DFL)
    {
      (*signal_handlers[nsig]) (nsig);
      return 0;
    }

  /* Default signal actions */
  if (nsig == SIGALRM || nsig == SIGPROF)
    exit (3);

  /* Other signals are ignored by default */
  return 0;
}


/*--------------------------------------------------------------------*/
/*                               Async timers                         */
/*--------------------------------------------------------------------*/

/* We emulate two timers, one for SIGALRM, another for SIGPROF.

   itimerproc() function has an implementation limitation: it does
   not allow to set *both* interval and period. If an attempt is
   made to set both, and then they are unequal, the function
   asserts.

   Minimum timer resolution on Win32 systems varies, and is greater
   than or equal than 1 ms. The resolution is always wrapped not to
   attempt to get below the system defined limit.
   */

/* Timer precision, denominator of one fraction: for 100 ms
   interval, request 10 ms precision
   */
const int setitimer_helper_timer_prec = 10;

/* Last itimervals, as set by calls to setitimer */
static struct itimerval it_alarm;
static struct itimerval it_prof;

/* Timer IDs as returned by MM */
MMRESULT tid_alarm = 0;
MMRESULT tid_prof = 0;

static void CALLBACK
setitimer_helper_proc (UINT uID, UINT uMsg, DWORD dwUser,
		       DWORD dw1, DWORD dw2)
{
  /* Just raise the signal indicated by the dwUser parameter */
  mswindows_raise (dwUser);
}

/* Divide time in ms specified by IT by DENOM. Return 1 ms
   if division results in zero */
static UINT
setitimer_helper_period (const struct itimerval* it, UINT denom)
{
  static TIMECAPS time_caps;

  UINT res;
  const struct timeval* tv = 
    (it->it_value.tv_sec == 0 && it->it_value.tv_usec == 0)
    ? &it->it_interval : &it->it_value;
  
  /* Zero means stop timer */
  if (tv->tv_sec == 0 && tv->tv_usec == 0)
    return 0;
  
  /* Convert to ms and divide by denom */
  res = (tv->tv_sec * 1000 + (tv->tv_usec + 500) / 1000) / denom;
  
  /* Converge to minimum timer resolution */
  if (time_caps.wPeriodMin == 0)
      timeGetDevCaps (&time_caps, sizeof(time_caps));

  if (res < time_caps.wPeriodMin)
    res = time_caps.wPeriodMin;

  return res;
}

static int
setitimer_helper (const struct itimerval* itnew,
		  struct itimerval* itold, struct itimerval* itcurrent,
		  MMRESULT* tid, DWORD sigkind)
{
  UINT delay, resolution, event_type;

  /* First stop the old timer */
  if (*tid)
    {
      timeKillEvent (*tid);
      timeEndPeriod (setitimer_helper_period (itcurrent,
					      setitimer_helper_timer_prec));
      *tid = 0;
    }

  /* Return old itimerval if requested */
  if (itold)
    *itold = *itcurrent;

  *itcurrent = *itnew;

  /* Determine if to start new timer */
  delay = setitimer_helper_period (itnew, 1);
  if (delay)
    {
      resolution = setitimer_helper_period (itnew,
					    setitimer_helper_timer_prec);
      event_type = (itnew->it_value.tv_sec == 0 &&
		    itnew->it_value.tv_usec == 0)
	? TIME_ONESHOT : TIME_PERIODIC;
      timeBeginPeriod (resolution);
      *tid = timeSetEvent (delay, resolution, setitimer_helper_proc, sigkind,
			   event_type);
    }

  return !delay || *tid;
}
 
int
mswindows_setitimer (int kind, const struct itimerval *itnew,
		     struct itimerval *itold)
{
  /* In this version, both interval and value are allowed
     only if they are equal. */
  assert ((itnew->it_value.tv_sec == 0 && itnew->it_value.tv_usec == 0)
	  || (itnew->it_interval.tv_sec == 0 &&
	      itnew->it_interval.tv_usec == 0)
	  || (itnew->it_value.tv_sec == itnew->it_interval.tv_sec &&
	      itnew->it_value.tv_usec == itnew->it_interval.tv_usec));

  if (kind == ITIMER_REAL)
    return setitimer_helper (itnew, itold, &it_alarm, &tid_alarm, SIGALRM);
  else if (kind == ITIMER_PROF)
    return setitimer_helper (itnew, itold, &it_prof, &tid_prof, SIGPROF);
  else
    return errno = EINVAL;
}

#endif /* defined (WIN32_NATIVE) || defined (CYGWIN_BROKEN_SIGNALS) */


void
syms_of_win32 (void)
{
  DEFSUBR (Fmswindows_shell_execute);
#ifdef CYGWIN
  DEFSUBR (Fmswindows_cygwin_to_win32_path);
#endif
}

void
init_win32 (void)
{
  init_potentially_nonexistent_functions ();
}
