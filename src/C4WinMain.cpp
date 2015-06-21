/* Copyright (C) 1998-2000  Matthes Bender  RedWolf Design */

/* Main program entry point */

#include "C4Include.h"
#include "game/C4Application.h"

#include "config/C4Config.h"
#include "C4Console.h"
#include "game/C4FullScreen.h"
#include "C4Log.h"

#ifdef WITH_DEVELOPER_MODE
#include <gtk/gtkmain.h>
#endif

C4Application Application;
C4Console Console;
C4FullScreen FullScreen;
C4Game Game;
C4Config Config;

#ifdef _WIN32

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR lpszCmdParam,
                   int nCmdShow) {
#if defined(_DEBUG) && defined(_MSC_VER)
  // enable debugheap!
  _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_ALLOC_MEM_DF |
                 _CRTDBG_LEAK_CHECK_DF);
#endif

  // Init application
  if (!Application.Init(hInst, nCmdShow, lpszCmdParam)) {
    Application.Clear();
    return C4XRV_Failure;
  }

  // Run it
  Application.Run();
  Application.Clear();

  // Return exit code
  if (!Game.GameOver)
    return C4XRV_Aborted;
  return C4XRV_Completed;
}

int main() {
  // Get command line, go over program name
  char *pCommandLine = GetCommandLine();
  if (*pCommandLine == '"') {
    pCommandLine++;
    while (*pCommandLine && *pCommandLine != '"')
      pCommandLine++;
    if (*pCommandLine == '"')
      pCommandLine++;
  } else
    while (*pCommandLine && *pCommandLine != ' ')
      pCommandLine++;
  while (*pCommandLine == ' ')
    pCommandLine++;
  // Call
  return WinMain(GetModuleHandle(NULL), 0, pCommandLine, 0);
}

#else
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_EXECINFO_H
#include <execinfo.h>

static void crash_handler(int signo) {
  int logfd = STDERR_FILENO;
  for (;;) {
    // Print out the signal
    write(logfd, C4VERSION ": Caught signal ",
          sizeof(C4VERSION ": Caught signal ") - 1);
    switch (signo) {
    case SIGBUS:
      write(logfd, "SIGBUS", sizeof("SIGBUS") - 1);
      break;
    case SIGILL:
      write(logfd, "SIGILL", sizeof("SIGILL") - 1);
      break;
    case SIGSEGV:
      write(logfd, "SIGSEGV", sizeof("SIGSEGV") - 1);
      break;
    case SIGABRT:
      write(logfd, "SIGABRT", sizeof("SIGABRT") - 1);
      break;
    case SIGINT:
      write(logfd, "SIGINT", sizeof("SIGINT") - 1);
      break;
    case SIGQUIT:
      write(logfd, "SIGQUIT", sizeof("SIGQUIT") - 1);
      break;
    case SIGFPE:
      write(logfd, "SIGFPE", sizeof("SIGFPE") - 1);
      break;
    case SIGTERM:
      write(logfd, "SIGTERM", sizeof("SIGTERM") - 1);
      break;
    }
    write(logfd, "\n", sizeof("\n") - 1);
    if (logfd == STDERR_FILENO)
      logfd = GetLogFD();
    else
      break;
    if (logfd < 0)
      break;
  }
  // Get the backtrace
  void *stack[100];
  int count = backtrace(stack, 100);
  // Print it out
  backtrace_symbols_fd(stack, count, STDERR_FILENO);
  // Also to the log file
  if (logfd >= 0)
    backtrace_symbols_fd(stack, count, logfd);
  // Bye.
  _exit(C4XRV_Failure);
}
#endif

#ifdef __APPLE__
void restart(char *[]); // MacUtility.mm
#else
static void restart(char *argv[]) {
  // Close all file descriptors except stdin, stdout, stderr
  int open_max = sysconf(_SC_OPEN_MAX);
  for (int fd = 4; fd < open_max; fd++)
    fcntl(fd, F_SETFD, FD_CLOEXEC);
  // Execute the new engine
  execlp(argv[0], argv[0], static_cast<char *>(0));
}
#endif

int main(int argc, char *argv[]) {
  if (!geteuid()) {
    printf("Do not run %s as root!\n", argc ? argv[0] : "this program");
    return C4XRV_Failure;
  }
#ifdef HAVE_EXECINFO_H
  // Set up debugging facilities
  signal(SIGBUS, crash_handler);
  signal(SIGILL, crash_handler);
  signal(SIGSEGV, crash_handler);
  signal(SIGABRT, crash_handler);
  signal(SIGINT, crash_handler);
  signal(SIGQUIT, crash_handler);
  signal(SIGFPE, crash_handler);
  signal(SIGTERM, crash_handler);
#endif

// FIXME: This should only be done in developer mode.
#ifdef WITH_DEVELOPER_MODE
  gtk_init(&argc, &argv);
#endif

  // Init application
  if (!Application.Init(argc, argv)) {
    Application.Clear();
    return C4XRV_Failure;
  }
  // Execute application
  Application.Run();
  // free app stuff
  Application.Clear();
  if (Application.restartAtEnd)
    restart(argv);
  // Return exit code
  if (!Game.GameOver)
    return C4XRV_Aborted;
  return C4XRV_Completed;
}
#endif
