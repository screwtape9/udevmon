#include <cstdio>
#include <cstring>
#include <csignal>
#include <unistd.h>
#include <libudev.h>

static volatile sig_atomic_t run = 1;

static void print_sig_desc(int sig)
{
  // async-safe printing
  write(STDOUT_FILENO, "\nSIG", 4);
  write(STDOUT_FILENO, sigabbrev_np(sig), strlen(sigabbrev_np(sig)));
  write(STDOUT_FILENO, ": ", 2);
  write(STDOUT_FILENO, sigdescr_np(sig), strlen(sigdescr_np(sig)));
  write(STDOUT_FILENO, "\n", 1);
}

static void on_sig(int sig)
{
  print_sig_desc(sig);
  run = 0;
}

static int handle_sigs()
{
  // block all signals while we set up handlers
  sigset_t set;
  if (sigfillset(&set) || sigprocmask(SIG_SETMASK, &set, NULL))
    return -1;

  // set all signals to be blocked while in a handler
  struct sigaction act;
  memset(&act, 0, sizeof(act));
  if (sigfillset(&act.sa_mask))
    return -1;
  
  // ignore these signals
  act.sa_handler = SIG_IGN;
  if (sigaction(SIGHUP,  &act, NULL) ||
      sigaction(SIGQUIT, &act, NULL) ||
      sigaction(SIGPIPE, &act, NULL))
    return -1;
  
  // handle these signals
  act.sa_handler = on_sig;
  if (sigaction(SIGINT,  &act, NULL) ||
      sigaction(SIGTERM, &act, NULL) ||
      sigaction(SIGBUS,  &act, NULL) ||
      sigaction(SIGFPE,  &act, NULL) ||
      sigaction(SIGILL,  &act, NULL) ||
      sigaction(SIGSEGV, &act, NULL) ||
      sigaction(SIGSYS,  &act, NULL) ||
      sigaction(SIGXCPU, &act, NULL) ||
      sigaction(SIGXFSZ, &act, NULL))
    return -1;
  
  // unblock all signals
  if (sigemptyset(&set) || sigprocmask(SIG_SETMASK, &set, NULL))
    return -1;

  return 0;
}

int main(int argc, char *argv[])
{
  if (handle_sigs()) {
    printf("Failed to set up signal handling.\n");
    return 1;
  }

  struct udev *udev = udev_new();
  if (!udev) {
    perror("udev_new()");
    return 1;
  }
  
  struct udev_monitor *mon = udev_monitor_new_from_netlink(udev, "udev");
  if (!mon) {
    perror("udev_monitor_new_from_netlink()");
    udev_unref(udev);
    return 1;
  }
  udev_monitor_filter_add_match_subsystem_devtype(mon, "usb", NULL);
  udev_monitor_enable_receiving(mon);
  int fd = udev_monitor_get_fd(mon);

  printf("waiting...\n");
  for (;run;) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    int ret = select((fd + 1), &fds, NULL, NULL, NULL);

    if ((ret > 0) && FD_ISSET(fd, &fds)) {
      struct udev_device *dev = udev_monitor_receive_device(mon);
      if (dev) {
        const char *node = udev_device_get_devnode(dev);
        if (node) {
          printf("Got Device\n");
          printf("   Node: %s\n", node);
          printf("   Subsystem: %s\n", udev_device_get_subsystem(dev));
          printf("   Devtype: %s\n", udev_device_get_devtype(dev));
          printf("   Action: %s\n", udev_device_get_action(dev));
        }
        udev_device_unref(dev);
        dev = NULL;
      }
      else
        printf("No Device from receive_device(). An error occured.\n");
    }
  }

  udev_monitor_unref(mon);
  udev_unref(udev);

  // just to make valgrind report 0 leaks! :)
  close(STDERR_FILENO);
  close(STDOUT_FILENO);
  close(STDIN_FILENO);

  return 0;
}
