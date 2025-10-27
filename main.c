#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <windows.h>
#include <direct.h>
#include <shlobj.h>
#include <shlobj_core.h>

#define MAX_PATH_LEN 1024
#define MAX_NAME_LEN 256

/*
 * Function: get_config_path
 * -------------------------
 * Constructs the full path to the config file in AppData\Local\FolderManager\config.txt.
 *
 * Parameters:
 *   configPath - buffer to store the full path
 *   size       - size of the buffer
 *
 * Returns:
 *   1 on success, 0 on failure
 */
int get_config_path(char *configPath, size_t size) {
    const char *home = getenv("USERPROFILE");
    if (!home) {
        fprintf(stderr, "Error: Could not determine user profile directory.\n");
        return 0;
    }

    snprintf(configPath, size, "%s\\AppData\\Local\\FolderManager", home);
    _mkdir(configPath);  // Create config directory if needed
    strncat(configPath, "\\config.txt", size - strlen(configPath) - 1);
    return 1;
}

/*
 * Function: read_or_create_config
 * -------------------------------
 * Reads the base directory from config file or prompts the user to create one.
 * Suggests a default like %USERPROFILE%\Projects.
 *
 * Parameters:
 *   basePath - buffer to store the base directory
 *   size     - size of the buffer
 *
 * Returns:
 *   1 on success, 0 on failure
 */
int read_or_create_config(char *basePath, size_t size) {
    char configPath[MAX_PATH_LEN];
    if (!get_config_path(configPath, sizeof(configPath))) return 0;

    FILE *file = fopen(configPath, "r");
    if (file) {
        if (!fgets(basePath, (int)size, file)) {
            fclose(file);
            return 0;
        }
        basePath[strcspn(basePath, "\r\n")] = '\0';
        fclose(file);
        return 1;
    }

    const char *home = getenv("USERPROFILE");
    snprintf(basePath, size, "%s\\Projects", home);
    printf("Config file not found.\n");
    printf("Suggested default base directory: %s\n", basePath);
    printf("Use this as your base directory? (y/n): ");

    char response[10];
    fgets(response, sizeof(response), stdin);

    if (tolower(response[0]) != 'y') {
        printf("Enter your preferred base directory: ");
        fgets(basePath, (int)size, stdin);
        basePath[strcspn(basePath, "\r\n")] = '\0';
    }

    file = fopen(configPath, "w");
    if (!file) {
        fprintf(stderr, "Error: Could not write config file.\n");
        return 0;
    }
    fprintf(file, "%s\n", basePath);
    fclose(file);
    printf("Saved base directory to config file.\n");
    return 1;
}

/*
 * Function: sanitize_folder_name
 * ------------------------------
 * Removes invalid characters and replaces spaces with underscores.
 *
 * Parameters:
 *   input  - original folder name
 *   output - sanitized folder name
 */
void sanitize_folder_name(const char *input, char *output) {
    const char *invalid = "<>:\"/\\|?*";
    int j = 0;
    for (int i = 0; input[i] != '\0' && j < MAX_NAME_LEN - 1; ++i) {
        if (strchr(invalid, input[i]) || iscntrl(input[i])) continue;
        else if (isspace(input[i])) output[j++] = '_';
        else output[j++] = input[i];
    }
    output[j] = '\0';
}

/*
 * Function: prompt_user_for_folder_name
 * -------------------------------------
 * Sanitizes folder name and confirms with user.
 *
 * Parameters:
 *   finalName - buffer containing initial name; updated with confirmed name
 */
void prompt_user_for_folder_name(char *finalName) {
    char tempName[MAX_NAME_LEN];
    char response[10];

    while (1) {
        sanitize_folder_name(finalName, tempName);
        printf("Sanitized folder name: \"%s\"\n", tempName);
        printf("Do you want to use this name? (y/n): ");
        fgets(response, sizeof(response), stdin);

        if (tolower(response[0]) == 'y') {
            strcpy(finalName, tempName);
            break;
        } else {
            printf("Enter a new folder name: ");
            fgets(finalName, MAX_NAME_LEN, stdin);
            finalName[strcspn(finalName, "\r\n")] = '\0';
        }
    }
}

/*
 * Function: directory_exists
 * --------------------------
 * Checks if a directory exists.
 *
 * Parameters:
 *   path - full path to check
 *
 * Returns:
 *   1 if exists, 0 otherwise
 */
int directory_exists(const char *path) {
    DWORD attribs = GetFileAttributes(path);
    return (attribs != INVALID_FILE_ATTRIBUTES && (attribs & FILE_ATTRIBUTE_DIRECTORY));
}

/*
 * Function: open_in_file_explorer
 * -------------------------------
 * Opens the given path in Windows File Explorer.
 *
 * Parameters:
 *   path - directory to open
 */
void open_in_file_explorer(const char *path) {
    ShellExecute(NULL, "open", "explorer.exe", path, NULL, SW_SHOWNORMAL);
}

/*
 * Function: open_downloads_folder
 * -------------------------------
 * Opens the user's Downloads folder.
 */
void open_downloads_folder() {
    PWSTR downloadsPath = NULL;
    if (SUCCEEDED(SHGetKnownFolderPath(&FOLDERID_Downloads, 0, NULL, &downloadsPath))) {
        ShellExecuteW(NULL, L"open", L"explorer.exe", downloadsPath, NULL, SW_SHOWNORMAL);
        CoTaskMemFree(downloadsPath);
    } else {
        fprintf(stderr, "Error: Could not locate Downloads folder\n");
    }
}

/*
 * Function: main
 * --------------
 * Entry point. Accepts one argument, sanitizes it, ensures config and directory,
 * switches to it, and opens it and Downloads folder.
 */
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <folder_name>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Step 1: Get and sanitize folder name
    char folderName[MAX_NAME_LEN];
    strncpy(folderName, argv[1], MAX_NAME_LEN - 1);
    folderName[MAX_NAME_LEN - 1] = '\0';
    prompt_user_for_folder_name(folderName);

    // Step 2: Read or create config file to get base path
    char basePath[MAX_PATH_LEN];
    if (!read_or_create_config(basePath, sizeof(basePath))) {
        return EXIT_FAILURE;
    }

    // Step 3: Construct full path to target folder
    char fullPath[MAX_PATH_LEN];
    snprintf(fullPath, sizeof(fullPath), "%s\\%s", basePath, folderName);

    // Step 4: Create folder if it doesn't exist
    if (!directory_exists(fullPath)) {
        if (_mkdir(fullPath) != 0) {
            perror("Error creating directory");
            return EXIT_FAILURE;
        }
        printf("Directory created: %s\n", fullPath);
    } else {
        printf("Directory already exists: %s\n", fullPath);
    }

    // Step 5: Change current working directory
    if (_chdir(fullPath) != 0) {
        perror("Error changing directory");
        return EXIT_FAILURE;
    }

    // Step 6: Open folder and Downloads folder in File Explorer
    open_in_file_explorer(fullPath);
    open_downloads_folder();

    return EXIT_SUCCESS;
}
