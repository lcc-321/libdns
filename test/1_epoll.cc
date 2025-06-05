// created by lcc 12/20/2021

#ifdef __linux__
  #include <sys/epoll.h>
  #include <sys/inotify.h>  // For inotify functions
#else
  #include <sys/event.h>
#endif
#include <cassert>
#include <ctime>
#include <iostream>
#include <fcntl.h>  // For open()
#include <fstream>
#include <unistd.h>  // For close()

int main() {
  // Create a test file
  const std::string filename = "/tmp/test-" + std::to_string(std::time(nullptr));
  std::ofstream ofs(filename);
  if (!ofs.is_open()) {
    std::cerr << "Failed to open file " << filename << std::endl;
  }
  ofs << "This is an initial line." << std::endl;
  ofs.close();

  // Process file event with epoll or kqueue
#ifdef __linux__
  // Watch inotify
  int inotify_fd = inotify_init1(IN_NONBLOCK);
  if (inotify_fd == -1) {
    std::cerr << "Failed to inotify_init1" << std::endl;
    return 1;
  }

  int file_watch_descriptor = inotify_add_watch(inotify_fd, filename.c_str(), IN_DELETE_SELF | IN_ATTRIB | IN_IGNORED);
  if (file_watch_descriptor == -1) {
    close(inotify_fd);
    std::cerr << "Failed to inotify_add_watch " << filename << std::endl;
  }

  // Create an epoll
  int event_fd = epoll_create1(0);

  // Define the epoll_event structure for monitoring the file
  struct epoll_event event { };
  event.events = EPOLLIN | EPOLLET;
  event.data.fd = inotify_fd;

  // Register the epoll_event to epoll
  if (epoll_ctl(event_fd, EPOLL_CTL_ADD, inotify_fd, &event) == -1) {
    close(inotify_fd);
    close(event_fd);
    std::cerr << "Error adding event_fd to epoll" << std::endl;
    return 1;
  }

  // Trigger the event
  if (std::remove(filename.c_str())) {
    std::cerr << "Error removing file " << filename << std::endl;
  }

  // Wait for events
  struct epoll_event events[1] { };
  if (epoll_wait(event_fd, events, 1, 9) > 0) {
    std::cout << "event.data.fd: " << events[0].data.fd << ", fd: " << inotify_fd << std::endl;
    assert(events[0].data.fd == inotify_fd);
    return 0;
  }
#else
  // Get file descriptor
  const int fd = open(filename.c_str(), O_RDONLY);
  if (fd == -1) {
    std::cerr << "Error opening " << filename << std::endl;
    return 1;
  }

  // Create a kqueue
  const int event_fd = kqueue();
  if (event_fd == -1) {
    std::cerr << "kqueue failed" << std::endl;
    close(fd);
    return 1;
  }

  // Define the kevent structure for monitoring the file
  struct kevent event { };
  event.ident = fd;
  EV_SET(&event, fd, EVFILT_VNODE, EV_ADD | EV_ENABLE | EV_CLEAR, NOTE_DELETE, 0, nullptr);

  // Register the kevent with kqueue
  if (kevent(event_fd, &event, 1, nullptr, 0, nullptr) == -1) {
    std::cerr << "kevent failed" << std::endl;
  }

  // Trigger the event
  if (std::remove(filename.c_str())) {
    std::cerr << "Error removing file " << filename << std::endl;
  }

  // Wait for events
  struct kevent events[1];
  timespec timeout { };
  timeout.tv_sec = 0;
  timeout.tv_nsec = 9999999;
  if (kevent(event_fd, nullptr, 0, events, 1, &timeout) > 0) {
    assert(events[0].ident == fd);
    assert(events[0].filter == EVFILT_VNODE);
    assert(events[0].fflags & NOTE_DELETE);
    std::cout << "event.ident: " << events[0].ident << ", fd: " << fd << std::endl;
    return 0;
  }

  return 1;
#endif
}
