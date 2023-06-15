#include <cstdio>
#include <dirent.h>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <ctime>

void printPermissions(mode_t mode) {
    printf((S_ISDIR(mode)) ? "d" : "-");
    printf((mode & S_IRUSR) ? "r" : "-");
    printf((mode & S_IWUSR) ? "w" : "-");
    printf((mode & S_IXUSR) ? "x" : "-");
    printf((mode & S_IRGRP) ? "r" : "-");
    printf((mode & S_IWGRP) ? "w" : "-");
    printf((mode & S_IXGRP) ? "x" : "-");
    printf((mode & S_IROTH) ? "r" : "-");
    printf((mode & S_IWOTH) ? "w" : "-");
    printf((mode & S_IXOTH) ? "x" : "-");
}

void printFileSize(off_t size, int humanReadable) {
    if (humanReadable) {
        auto s = (double)size;
        const char* units[] = {"B", "K", "M", "G", "T"};
        int i = 0;
        while (s > 1024 && i < sizeof(units) / sizeof(units[0])) {
            s /= 1024;
            i++;
        }
        printf("\t%.1lf%s", s, units[i]);
    } else {
        printf("\t%lld", (long long)size);
    }
}

void printFileDetails(const char* filename, int showInode, int humanReadable, const char* name) {
    struct stat fileStat{};
    if (stat(filename, &fileStat) < 0) {
        fprintf(stderr, "Failed to stat file '%s': %s\n", filename, strerror(errno));
        return;
    }

    if (showInode) {
        printf("%ld\t", (long)fileStat.st_ino);
    }

    printPermissions(fileStat.st_mode);
    printf("\t%ld", (long)fileStat.st_nlink);

    struct passwd* pwd = getpwuid(fileStat.st_uid);
    if (pwd != nullptr) {
        printf("\t%s", pwd->pw_name);
    } else {
        printf("\t%d", fileStat.st_uid);
    }

    struct group* grp = getgrgid(fileStat.st_gid);
    if (grp != nullptr) {
        printf("\t%s", grp->gr_name);
    } else {
        printf("\t%d", fileStat.st_gid);
    }

    printFileSize(fileStat.st_size, humanReadable);

    char timeBuffer[20];
    strftime(timeBuffer, sizeof(timeBuffer), "%b %d %H:%M", localtime(&fileStat.st_mtime));
    printf("\t%s", timeBuffer);

    if (name != nullptr) {
        printf("\t%s\n", name);
    } else {
        printf("\t%s\n", filename);
    }
}

void lsRecursive(const char* dirName, int showHidden, int showInode, int humanReadable) {
    DIR *dir;
    struct dirent *entry;

    dir = opendir(dirName);
    if (dir == nullptr) {
        fprintf(stderr, "Failed to open directory '%s': %s\n", dirName, strerror(errno));
        return;
    }

    printf("%s:\n", dirName);

    while ((entry = readdir(dir)) != nullptr) {
        if (!showHidden && entry->d_name[0] == '.') {
            continue;
        }

        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", dirName, entry->d_name);
        printFileDetails(path, showInode, humanReadable, entry->d_name);

        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            lsRecursive(path, showHidden, showInode, humanReadable);
        }
    }

    closedir(dir);
}

int main(int argc, char *argv[]) {
    const char* directory = ".";
    int showDetails = 0;
    int showRecursive = 0;
    int showHidden = 0;
    int showInode = 0;
    int humanReadable = 0;

    int opt;
    while ((opt = getopt(argc, argv, "lRaith")) != -1) {
        switch (opt) {
            case 'l':
                showDetails = 1;
                break;
            case 'R':
                showRecursive = 1;
                break;
            case 'a':
                showHidden = 1;
                break;
            case 'i':
                showInode = 1;
                break;
            case 'h':
                humanReadable = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-l] [-R] [-a] [-i] [-h] [directory]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (optind < argc) {
        directory = argv[optind];
    }

    if (showRecursive) {
        lsRecursive(directory, showHidden, showInode, humanReadable);
    } else {
        DIR *dir;
        struct dirent *entry;

        dir = opendir(directory);
        if (dir == nullptr) {
            fprintf(stderr, "Failed to open directory '%s': %s\n", directory, strerror(errno));
            exit(EXIT_FAILURE);
        }

        while ((entry = readdir(dir)) != nullptr) {
            if (!showHidden && entry->d_name[0] == '.') {
                continue;
            }

            if (showDetails) {
                char path[1024];
                snprintf(path, sizeof(path), "%s/%s", directory, entry->d_name);
                printFileDetails(path, showInode, humanReadable, entry->d_name);
            } else {
                printf("%s\n", entry->d_name);
            }
        }

        closedir(dir);
    }

    return 0;
}
