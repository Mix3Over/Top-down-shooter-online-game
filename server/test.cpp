#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>

struct Child {
    pid_t pid;
    std::string buf;  // niepełna linia
};

static int epfd;
static std::map<int, Child> children;  // klucz: fd odczytu

static void spawn() {
    int fds[2];
    if (pipe2(fds, O_CLOEXEC | O_NONBLOCK) != 0) { perror("pipe2"); return; }

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); close(fds[0]); close(fds[1]); return; }

    if (pid == 0) {
        dup2(fds[1], STDOUT_FILENO);  // dup2 zdejmuje CLOEXEC z nowego fd
        execl("./slave", "slave", (char*)nullptr);
        _exit(127);
    }

    close(fds[1]);
    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = fds[0];
    epoll_ctl(epfd, EPOLL_CTL_ADD, fds[0], &ev);
    children[fds[0]] = {pid, ""};
    std::cout << "[master] uruchomiono slave, pid=" << pid << std::endl;
}

static void onChildReadable(int fd) {
    Child& c = children[fd];
    char tmp[256];
    for (;;) {
        ssize_t n = read(fd, tmp, sizeof(tmp));
        if (n > 0) {
            c.buf.append(tmp, n);
            size_t pos;
            while ((pos = c.buf.find('\n')) != std::string::npos) {
                std::cout << "[slave " << c.pid << "] " << c.buf.substr(0, pos) << std::endl;
                c.buf.erase(0, pos + 1);
            }
        } else if (n == 0) {  // EOF - slave zamknął stdout (zakończył się)
            epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
            close(fd);
            waitpid(c.pid, nullptr, 0);
            std::cout << "[master] slave " << c.pid << " zakończony" << std::endl;
            children.erase(fd);
            return;
        } else {
            if (errno != EAGAIN && errno != EWOULDBLOCK) perror("read");
            return;  // brak więcej danych na teraz
        }
    }
}

int main() {
    epfd = epoll_create1(EPOLL_CLOEXEC);

    // timer co 5 s
    int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    itimerspec its{};
    its.it_value.tv_sec = 5;
    its.it_interval.tv_sec = 5;
    timerfd_settime(tfd, 0, &its, nullptr);

    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = tfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, tfd, &ev);

    spawn();  // pierwszy slave od razu, kolejne co 5 s

    epoll_event events[16];
    for (;;) {
        int n = epoll_wait(epfd, events, 16, -1);
        if (n < 0) { if (errno == EINTR) continue; perror("epoll_wait"); break; }

        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;
            if (fd == tfd) {
                uint64_t expirations;
                read(tfd, &expirations, sizeof(expirations));
                spawn();
            } else {
                onChildReadable(fd);
            }
        }
    }
    return 0;
}