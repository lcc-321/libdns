// created by lcc 12/20/2021

#ifdef __linux__
  #include <sys/epoll.h>
#else
  #include <sys/event.h>
  #include <ctime>
#endif

int main() {
#ifdef __linux__
  int event_fd = epoll_create1(0);
  for (int i = 0; i < 99; i++) {
    epoll_wait(event_fd, {}, 1, 9);
  }
#else
  int event_fd = kqueue();
  timespec timespecOut { };
  timespecOut.tv_sec = 1;
  return kevent(event_fd, nullptr, 0, {}, 1, &timespecOut);
#endif
}
