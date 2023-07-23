#include "process.h"

#include <cpu/gdt.h>
#include <sys/kernel_mem.h>
#include <sys/mount.h>
#include <sys/heap.h>
#include <sys/panic.h>
#include <sys/lock.h>
#include <lib/string.h>
#include <lib/terminal.h>
#include <lib/kprintf.h>

struct process_queue_list {
    struct process_queue* first;
    struct process_queue* last;
};

tree_t* process_tree = 0;
struct process_queue_list process_queue = {.first = 0, .last = 0};
struct process_queue_list reap_queue = {.first = 0, .last = 0};
volatile process_t* current_process = 0;

static volatile uint8_t reap_lock;
static volatile uint8_t tree_lock;

static pid_t current_pid = 0;

void init_process(uint32_t esp) {
    asm("cli");

    current_process = spawn_init(esp);
    set_process_page_directory((process_t*) current_process, current_page_directory);
    enable_paging(current_process->thread.page_directory);

    asm("sti");
}

process_t* spawn_process(volatile process_t* parent) {
    process_t* process = malloc(sizeof(process_t));
    process->id = ++current_pid;
    process->name = strdup("unnamed process");
    process->thread.esp = 0;
    process->thread.ebp = 0;
    process->thread.eip = 0;
    process->image.entry = parent->image.entry;
    process->image.heap = parent->image.heap;
    process->image.heap_aligned = parent->image.heap_aligned;
    process->image.stack = (uintptr_t) pfa_request_pages(&pfa, 8) + 0x8000;
    process->image.user_stack = parent->image.user_stack;
    process->process_tree = tree_create();
    process->fds = parent->fds ? malloc(sizeof(file_descriptor_t)) : 0;
    file_descriptor_t* current_fd = process->fds;
    for (file_descriptor_t* fd = parent->fds; fd; fd = fd->link) {
        memcpy(current_fd, fd, sizeof(file_descriptor_t));
        if (fd->link) {
            current_fd->link = malloc(sizeof(file_descriptor_t));
            current_fd = current_fd->link;
        }
        if (process->current_fd < fd->fd) {
            process->current_fd = fd->fd;
        }
    }
    process->current_fd = parent->current_fd;
    process->working_dir_entry = parent->working_dir_entry;
    process->working_dir_path = parent->working_dir_path;
    process->status = 0;
    process->finished = 0;
    process->started = 0;
    process->syscall_regs = 0;
    process->queue_node.next = 0;
    process->queue_node.prev = 0;
    process->queue_node.process = process;
    process->queue_node.queued = 0;
    process->reap_node.next = 0;
    process->reap_node.prev = 0;
    process->reap_node.process = process;
    process->reap_node.queued = 0;

    spin_lock(&tree_lock);
    tree_insert_value(parent->process_tree, process);
    spin_unlock(&tree_lock);

    return process;
}

process_t* spawn_init(uint32_t esp) {
    process_t* init = malloc(sizeof(process_t));
    process_tree = tree_create();
    process_tree->value = init;
    init->process_tree = process_tree;
    init->id = 0;
    init->name = strdup("init");
    init->status = 0;
    init->fds = 0;
    init->current_fd = 0;
    init->working_dir_entry = get_root_dir();
    init->working_dir_path = strdup("/");
    init->image.entry = 0;
    init->image.heap = 0;
    init->image.heap_aligned = 0;
    init->image.stack = esp + 1;
    init->image.user_stack = 0;
    init->image.size = 0;
    init->finished = 0;
    init->started = 1;
    init->syscall_regs = 0;
    init->queue_node.next = 0;
    init->queue_node.prev = 0;
    init->queue_node.process = init;
    init->queue_node.queued = 0;
    init->reap_node.next = 0;
    init->reap_node.prev = 0;
    init->reap_node.process = init;
    init->reap_node.queued = 0;

    return init;
}

void set_process_page_directory(process_t* process, page_directory_t* page_dir) {
    if (!process || !page_dir) {
        return;
    }

    uintptr_t stack = process->image.stack - 0x8000;
    for (uint32_t i = 0; i < 8; i++) {
        void* ptr = (void*) (stack + i * 0x1000);
        pde_map_user_memory(&page_directory, &pfa, ptr, pde_get_phys_addr(current_page_directory, ptr));
    }

    process->thread.page_directory = page_dir;
}

void make_process_ready(process_t* process) {
    if (!process) {
        return;
    }

    if (!process_queue.last) {
        process_queue.first = &process->queue_node;
        process_queue.last = process_queue.first;
    } else {
        process->queue_node.prev = process_queue.last;
        process_queue.last->next = &process->queue_node;
        process_queue.last = &process->queue_node;
    }

    process->queue_node.queued = 1;
}

void make_process_reapable(process_t* process) {
    if (!process) {
        return;
    }

    spin_lock(&reap_lock);
    if (!reap_queue.last) {
        reap_queue.first = &process->reap_node;
        reap_queue.last = reap_queue.first;
    } else {
        process->reap_node.prev = reap_queue.last;
        reap_queue.last->next = &process->reap_node;
        reap_queue.last = &process->reap_node;
    }
    spin_unlock(&reap_lock);
}

uint8_t process_available() {
    return process_queue.first != 0;
}

process_t* next_ready_process() {
    if (!process_queue.first) {
        return 0;
    }

    process_t* process = process_queue.first->process;
    process_queue.first = process_queue.first->next;
    if (process_queue.first) {
        process_queue.first->prev = 0;
    } else {
        process_queue.last = 0;
    }

    process->queue_node.next = 0;
    process->queue_node.prev = 0;
    process->queue_node.queued = 0;

    return process;
}

uint8_t should_reap() {
    return reap_queue.first != 0;
}

process_t* next_reapable_process() {
    spin_lock(&reap_lock);
    if (!reap_queue.first) {
        return 0;
    }

    process_t* reap_process = reap_queue.first->process;
    reap_queue.first = reap_queue.first->next;
    if (reap_queue.first) {
        reap_queue.first->prev = 0;
    } else {
        reap_queue.last = 0;
    }

    reap_process->reap_node.next = 0;
    reap_process->reap_node.prev = 0;

    spin_unlock(&reap_lock);
    return reap_process;
}

void reap_process(process_t* process) {
    free(process->working_dir_path);
    free(process->name);
    for (file_descriptor_t* fd = process->fds; fd;) {
        file_descriptor_t* next = fd->link;
        fd->close(fd);
        free(fd);
        fd = next;
    }
    pfa_free_pages(&pfa, (void*) (process->image.stack - 0x8000), 8);
    if (process->image.heap) {
        uintptr_t heap = current_process->image.entry + current_process->image.size;
        heap = (heap & ~0xFFF) + ((heap & 0xFFF) ? 0x1000 : 0);

        for (; heap < process->image.heap_aligned; heap += 0x1000) {
            void* physical_page = pde_get_phys_addr(process->thread.page_directory, (void*) heap);
            pfa_free_page(&pfa, physical_page);
        }
    }
    pde_free(process->thread.page_directory, &pfa);
    delete_process(process);
}

uint32_t process_add_fd(process_t* process, file_descriptor_t* file) {
    if (!process || !file) {
        return -1;
    }

    file->link = process->fds;
    process->fds = file;
    file->fd = ++process->current_fd;
    return file->fd;
}

uint8_t process_pid_comparator(void* process, void* pid) {
    if (!process) {
        return 0;
    }

    return ((process_t*) process)->id == *(pid_t*) pid;
}

process_t* get_process(pid_t pid) {
    spin_lock(&tree_lock);
    tree_t* node = tree_find(process_tree, &pid, process_pid_comparator);
    spin_unlock(&tree_lock);
    return node ? (process_t*) node->value : 0;
}

void delete_process(process_t* process) {
    if (!process) {
        return;
    }

    if (process->process_tree == process_tree) {
        puts("[Error] Attempt to kill init.\n");
        return;
    }

    spin_lock(&tree_lock);
    tree_remove_and_merge(process->process_tree);
    spin_unlock(&tree_lock);
    free(process);
}

file_descriptor_t* process_get_fd(process_t* process, uint32_t fd) {
    for (file_descriptor_t* file = process->fds; file; file = file->link) {
        if (file->fd == fd) {
            return file;
        }
    }

    return 0;
}

uint32_t process_clone_fd(process_t* process, int from, int to) {
    file_descriptor_t* fd = process_get_fd(process, from);
    if (!fd) {
        return -1;
    }

    file_descriptor_t* newfd = malloc(sizeof(file_descriptor_t));
    memcpy(newfd, fd, sizeof(file_descriptor_t));
    newfd->link = process->fds;
    process->fds = newfd;
    newfd->fd = to;

    if (process->current_fd < (uint32_t) to) {
        process->current_fd = to;
    }

    return to;
}

int process_is_ready(process_t* process) {
    return process->queue_node.queued;
}

uintptr_t read_eip();

pid_t fork() {
    asm("cli");

    uint32_t magic = 0xF3F5;
    uintptr_t esp;
    uintptr_t ebp;
    uintptr_t eip;

    process_t* parent = (process_t*) current_process;
    page_directory_t* page_dir = pde_clone(current_page_directory, &pfa);

    process_t* new_process = spawn_process(current_process);
    set_process_page_directory(new_process, page_dir);
    eip = read_eip();

    if (current_process == parent) {
        if (magic != 0xF3F5) {
            panic("[Error] Bad fork() magic (parent).");
        }

        asm volatile("mov %%esp, %0" : "=r"(esp));
        asm volatile("mov %%ebp, %0" : "=r"(ebp));
        if (current_process->image.stack > new_process->image.stack) {
            new_process->thread.esp = esp - (current_process->image.stack - new_process->image.stack);
            new_process->thread.ebp = ebp - (current_process->image.stack - new_process->image.stack);
        } else {
            new_process->thread.esp = esp + (new_process->image.stack - current_process->image.stack);
            new_process->thread.ebp = ebp - (current_process->image.stack - new_process->image.stack);
        }
        memcpy((void*) (new_process->image.stack - 0x8000), (void*) (current_process->image.stack - 0x8000), 0x8000);
        uintptr_t o_stack = ((uintptr_t) current_process->image.stack - 0x8000);
        uintptr_t n_stack = ((uintptr_t) new_process->image.stack - 0x8000);
        uintptr_t offset = ((uintptr_t) current_process->syscall_regs - o_stack);
        new_process->syscall_regs = (struct syscall_regs*)(n_stack + offset);
        new_process->thread.eip = eip;
        make_process_ready(new_process);
        asm("sti");
        return new_process->id;
    } else {
        if (magic != 0xF3F5) {
            panic("[Error] Bad fork() magic (child).");
        }

        return 0;
    }
}

pid_t clone(uintptr_t new_stack, uintptr_t thread_func, uintptr_t arg) {
    uint32_t magic = 0xF3F5;
    uintptr_t esp;
    uintptr_t ebp;
    uintptr_t eip;
    asm("cli");
    process_t* parent = (process_t*) current_process;
    page_directory_t* page_dir = current_page_directory;
    process_t* new_process = spawn_process(current_process);
    set_process_page_directory(new_process, page_dir);
    eip = read_eip();

    if (current_process == parent) {
        if (magic != 0xF3F5) {
            panic("[Error] Bad fork() magic (parent clone).");
        }

        asm volatile("mov %%esp, %0" : "=r"(esp));
        asm volatile("mov %%ebp, %0" : "=r"(ebp));
        if (current_process->image.stack > new_process->image.stack) {
            new_process->thread.esp = esp - (current_process->image.stack - new_process->image.stack);
            new_process->thread.ebp = ebp - (current_process->image.stack - new_process->image.stack);
        } else {
            new_process->thread.esp = esp + (new_process->image.stack - current_process->image.stack);
            new_process->thread.ebp = ebp - (current_process->image.stack - new_process->image.stack);
        }
        memcpy((void*) (new_process->image.stack - 0x8000), (void*) (current_process->image.stack - 0x8000), 0x8000);
        uintptr_t o_stack = ((uintptr_t) current_process->image.stack - 0x8000);
        uintptr_t n_stack = ((uintptr_t) new_process->image.stack - 0x8000);
        uintptr_t offset = ((uintptr_t) current_process->syscall_regs - o_stack);
        new_process->syscall_regs = (struct syscall_regs*)(n_stack + offset);
        new_process->syscall_regs->ebp = new_stack;
        new_process->syscall_regs->eip = thread_func;
        new_stack -= sizeof(uintptr_t);
        *((uintptr_t*) new_stack) = arg;
        new_stack -= sizeof(uintptr_t);
        *((uintptr_t*) new_stack) = 0xFFFFB00F;
        new_process->syscall_regs->esp = new_stack;
        new_process->syscall_regs->useresp = new_stack;
        for (file_descriptor_t* fd = new_process->fds; fd;) {
            file_descriptor_t* next = fd->link;
            free(fd);
            fd = next;
        }
        new_process->fds = current_process->fds;
        new_process->thread.eip = eip;
        make_process_ready(new_process);
        asm("sti");
        return new_process->id;
    } else {
        if (magic != 0xF3F5) {
            panic("[Error] Bad fork() magic (child clone).");
        }

        return 0;
    }
}

pid_t getpid() {
    return current_process->id;
}

void switch_task(uint8_t reschedule) {
    if (!current_process) {
        return;
    }

    if (!current_process->started) {
        switch_next();
    }

    if (!process_available()) {
        return;
    }

    uintptr_t esp;
    uintptr_t ebp;
    uintptr_t eip;
    asm volatile("mov %%esp, %0" : "=r"(esp));
    asm volatile("mov %%ebp, %0" : "=r"(ebp));
    eip = read_eip();

    if (eip == 0xFA705) {
        while (should_reap()) {
            process_t* process = next_reapable_process();
            reap_process(process);
        }

        return;
    }

    current_process->thread.eip = eip;
    current_process->thread.esp = esp;
    current_process->thread.ebp = ebp;
    current_process->started = 0;

    if (reschedule) {
        make_process_ready((process_t*) current_process);
    }

    switch_next();
}

void switch_next() {
    uintptr_t esp;
    uintptr_t ebp;
    uintptr_t eip;

    if (!process_available()) {
        return;
    }

    current_process = next_ready_process();
    eip = current_process->thread.eip;
    esp = current_process->thread.esp;
    ebp = current_process->thread.ebp;

    if (current_process->finished) {
        switch_next();
    }

    current_page_directory = current_process->thread.page_directory;
    enable_paging(current_page_directory);
    set_kernel_stack(current_process->image.stack);

    current_process->started = 1;

    asm volatile("mov %0, %%ebx\n"
                 "mov %1, %%esp\n"
                 "mov %2, %%ebp\n"
                 "mov %3, %%cr3\n"
                 "mov $0xFA705, %%eax\n"
                 "sti\n"
                 "jmp *%%ebx"
                 : : "r"(eip), "r"(esp), "r"(ebp), "r"(current_page_directory->physical_address));
}

void enter_userspace(uintptr_t entry, uintptr_t stack, int argc, const char** argv) {
    asm("cli");

    set_kernel_stack(current_process->image.stack);

    stack -= sizeof(uintptr_t);
    *((uintptr_t*) stack) = (uintptr_t) argv;
    stack -= sizeof(int);
    *((int*) stack) = argc;

    asm volatile("mov %1, %%esp\n"
                 "pushl $0x00\n"
                 "mov $0x23, %%ax\n"
                 "mov %%ax, %%ds\n"
                 "mov %%ax, %%es\n"
                 "mov %%ax, %%fs\n"
                 "mov %%ax, %%gs\n"
                 "mov %%esp, %%eax\n"
                 "pushl $0x23\n"
                 "pushl %%eax\n"
                 "pushl $0x202\n"
                 "pushl $0x1B\n"
                 "pushl %0\n"
                 "iret\n"
                 : : "m"(entry), "r"(stack)
                 : "%ax", "%esp", "%eax");
}

void task_exit(int r) {
    asm("cli");
    if (__builtin_expect(current_process->id == 0,0)) {
        switch_next();
        asm("sti");
        return;
    }

    kprintf("Process [%ld] %s terminated with code %d.\n", current_process->id, current_process->name, r);

    current_process->status = r;
    current_process->finished = 1;
    make_process_reapable((process_t*) current_process);
    switch_next();
    asm("sti");
}