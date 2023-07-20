#include <syscall.h>

int main() {
    sys_print("Hello, userspace!\n");
    return 0;
}
