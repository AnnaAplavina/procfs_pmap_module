#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define PROCFS_PATH "/proc/pmap_info"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Использование: %s <pid>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int pid = atoi(argv[1]);
    char procfs_file_path[256];

    snprintf(procfs_file_path, sizeof(procfs_file_path), PROCFS_PATH, pid);

    int fd = open(procfs_file_path, O_WRONLY);
    if (fd == -1) {
        perror("Ошибка открытия файла /proc/pmap_info");
        exit(EXIT_FAILURE);
    }

    if (write(fd, &pid, sizeof(pid)) == -1) {
        perror("Ошибка записи в файл /proc/pmap_info");
        close(fd);
        exit(EXIT_FAILURE);
    }

    close(fd);

    fd = open(procfs_file_path, O_RDONLY);
    if (fd == -1) {
        perror("Ошибка открытия файла /proc/pmap_info для чтения результата");
        exit(EXIT_FAILURE);
    }

    char buffer[1024*1024];
    ssize_t bytesRead = read(fd, buffer, sizeof(buffer) - 1);
    if (bytesRead == -1) {
        perror("Ошибка чтения из файла /proc/pmap_info");
        close(fd);
        exit(EXIT_FAILURE);
    }

    buffer[bytesRead] = '\0'; 
    printf("Результат от ядра:\n%s", buffer);

    close(fd);

    return 0;
}
