#include <unistd.h>
#include <cstring>

static void say(const char* msg) {
    write(STDOUT_FILENO, msg, strlen(msg));  // write() = bez buforowania
}

int main() {
    say("Start\n");
    sleep(3);
    say("End\n");
    return 0;
}