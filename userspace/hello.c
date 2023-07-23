#include <syscall.h>

int main(int argc, char** argv) {
    sys_print("Hello, userspace!\n");
    return 0;
}
