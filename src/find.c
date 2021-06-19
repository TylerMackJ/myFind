#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

// Series of variables which make up criteria
char* name = (char*)-1;

int mmin = 0;
enum MminMode {NotFlagged, Equal, Less, More};
enum MminMode mminMode = NotFlagged;

ino_t inum = -1;

enum FindMode {None, Delete, Exec};
enum FindMode findMode = None;
char execCommand[255];

// Prototypes
void parseCriteria(int argc, char* argv[]);
int meetsCriteria(char* filePath);
void readSub(char* subDir);

// Entry
int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Error must run as %s where-to-look\n", argv[0]);
        return -1;
    }
    parseCriteria(argc, argv);
    readSub(argv[1]);

    //printf("%d\n", meetsCriteria(argv[1]));
}

void parseCriteria(int argc, char* argv[]) {
    int i;
    for(i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-name") == 0) {
            // Make sure there is something after -name
            if (i + 1 >= argc) {
                printf("Must provide a name after -name\n");
                exit(-1);
            }
            // Set name to the name
            name = argv[++i];
        } else if (strcmp(argv[i], "-mmin") == 0) {
            // Make sure there is something after -mmin
            if (i + 1 >= argc) {
                printf("Must provide a number of minutes after -mmin\n");
                exit(-1);
            }
            i++;
            // Handle - or + minutes
            if (argv[i][0] == '-') {
                mminMode = Less;
            } else if (argv[i][0] == '+') {
                mminMode = More;
            }
            // Parse the number of minutes with or without the +/-
            if (mminMode == Equal) {
                mmin = atoi(argv[i]);
            } else {
                mmin = atoi(argv[i] + 1);
            }
        } else if (strcmp(argv[i], "-inum") == 0) {
            // Make sure there is something after -inum
            if (i + 1 >= argc) {
                printf("Must provide an inode number after -inum\n");
                exit(-1);
            }
            // Save the inum
            inum = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-delete") == 0) {
            findMode = Delete;
        } else if (strcmp(argv[i], "-exec") == 0) {
            findMode = Exec;
            // Make sure there is something after -exec
            if (i + 1 >= argc) {
                printf("Must provide a command after -exec\n");
                exit(-1);
            }
            // Save the command
            int j;
            for (j = i + 1; j < argc; j++) {
                strcat(execCommand, argv[j]);
                strcat(execCommand, " ");
            }
            break;
        }
    }
}

int meetsCriteria(char* filePath) {
    // Get filename
    char* fileName = filePath;
    while (strchr(fileName, '/') != NULL) {
        fileName = strchr(fileName, '/') + 1;
    }

    // Check -name
    if (name != (char*)-1) {
        if (strcmp(fileName, name) != 0) {
            return 0;
        }
    }

    // Get filestatus
    struct stat fileStatus;
    if (stat(filePath, &fileStatus)) {
        printf("Error getting stat of %s\n", filePath);
        perror("stat");
        exit(1);
    }

    // Get current time
    time_t currentTime;
    time(&currentTime);

    // Check -mmin
    if (mminMode != None) {
        int minutes = (currentTime - fileStatus.st_mtime) / 60;
        switch (mminMode) {
            case Equal:
                if (minutes != mmin) {
                    return 0;
                }
                break;
            case Less:
                if (minutes >= mmin) {
                    return 0;
                }
                break;
            case More:
                if (minutes <= mmin) {
                    return 0;
                }
        }
    }

    // Check -inum
    if (inum != -1) {
        if (inum != fileStatus.st_ino) {
            return 0;
        }
    }

    // Return 1 if it passed everythiing else
    return 1;
}

void readSub(char* subDir) {
    DIR* subDP = opendir(subDir);
    struct dirent* subDirp;
    struct stat buf;
    char slash[] = "/";

    if (subDP != NULL) {
        while ((subDirp = readdir(subDP)) != NULL) {

            char* temp = subDirp->d_name;

            if (strcmp(temp, ".") != 0 && strcmp(temp, "..") != 0) {
                char* tempFullPath = malloc(sizeof(char) * 2000);
                tempFullPath = strcpy(tempFullPath, subDir);
                tempFullPath = strcat(tempFullPath, slash);
                tempFullPath = strcat(tempFullPath, temp);

                if(meetsCriteria(tempFullPath)) {
                    switch (findMode) {
                        case None:
                            printf("%s\n", tempFullPath);
                            break;
                        case Delete:
                            remove(tempFullPath);
                            break;
                        case Exec:
                            {
                                char* tempCommand = malloc(sizeof(char) * 2256);
                                int tempPosition = 0;
                                tempCommand[tempPosition] = '\0';
                                int execPosition;
                                for (execPosition = 0; execPosition < strlen(execCommand); execPosition++) {
                                    if (execCommand[execPosition] == '{' && execCommand[execPosition + 1] == '}') {
                                        int pathPosition;
                                        for (pathPosition = 0; pathPosition < strlen(tempFullPath); pathPosition++) {
                                            tempCommand[tempPosition++] = tempFullPath[pathPosition];
                                        }
                                        tempCommand[tempPosition] = '\0';
                                        execPosition++;
                                    } else {
                                        tempCommand[tempPosition++] = execCommand[execPosition];
                                        tempCommand[tempPosition] = '\0';
                                    }
                                }
                                system(tempCommand);
                            }
                            break;
                    }
                }

                DIR* subSubDP = opendir(tempFullPath);
                if (subSubDP != NULL) {
                    readSub(tempFullPath);
                }
            }
        }
        closedir(subDP);
    } else {
        printf("Cannot open directory\n");
        exit(-1);
    }
}
