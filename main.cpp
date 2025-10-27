#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <windows.h>
#include <shlobj.h>  // For SHGetKnownFolderPath

namespace fs = std::filesystem;

const std::string CONFIG_DIR = "\\AppData\\Local\\FolderManager";
const std::string CONFIG_FILE = "config.txt";
const std::string DEFAULT_SUBDIR = "Projects";

/*
 * Function: getUserProfile
 * ------------------------
 * Retrieves the user's home directory from the USERPROFILE environment variable.
 */
std::string getUserProfile() {
    char* userProfile = getenv("USERPROFILE");
    if (!userProfile) {
        std::cerr << "Error: USERPROFILE not found.\n";
        exit(EXIT_FAILURE);
    }
    return std::string(userProfile);
}

/*
 * Function: getConfigFilePath
 * ---------------------------
 * Constructs the full path to the config file.
 */
fs::path getConfigFilePath() {
    fs::path configPath = getUserProfile() + CONFIG_DIR;
    fs::create_directories(configPath);  // Ensure directory exists
    return configPath / CONFIG_FILE;
}

/*
 * Function: readOrCreateBasePath
 * ------------------------------
 * Reads the base path from config file or prompts the user to create one.
 */
std::string readOrCreateBasePath() {
    fs::path configFile = getConfigFilePath();
    std::string basePath;

    if (fs::exists(configFile)) {
        std::ifstream in(configFile);
        std::getline(in, basePath);
        return basePath;
    }

    std::string suggested = getUserProfile() + "\\" + DEFAULT_SUBDIR;
    std::cout << "Config file not found.\n";
    std::cout << "Suggested default base directory: " << suggested << "\n";
    std::cout << "Use this as your base directory? (y/n): ";

    std::string response;
    std::getline(std::cin, response);

    if (tolower(response[0]) == 'y') {
        basePath = suggested;
    } else {
        std::cout << "Enter your preferred base directory: ";
        std::getline(std::cin, basePath);
    }

    std::ofstream out(configFile);
    out << basePath << "\n";
    std::cout << "Saved base directory to config file.\n";
    return basePath;
}

/*
 * Function: sanitizeFolderName
 * ----------------------------
 * Removes invalid characters and replaces spaces with underscores.
 */
std::string sanitizeFolderName(const std::string& input) {
    const std::string invalid = "<>:\"/\\|?*";
    std::string output;
    for (char ch : input) {
        if (iscntrl(ch) || invalid.find(ch) != std::string::npos) continue;
        output += isspace(ch) ? '_' : ch;
    }
    return output;
}

/*
 * Function: promptForFolderName
 * -----------------------------
 * Prompts user to confirm or re-enter folder name after sanitization.
 */
std::string promptForFolderName(const std::string& original) {
    std::string name = original;
    while (true) {
        std::string sanitized = sanitizeFolderName(name);
        std::cout << "Sanitized folder name: \"" << sanitized << "\"\n";
        std::cout << "Do you want to use this name? (y/n): ";

        std::string response;
        std::getline(std::cin, response);

        if (tolower(response[0]) == 'y') {
        return sanitized;
        } else {
            std::cout << "Enter a new folder name: ";
            std::getline(std::cin, name);
        }
    }
}

/*
 * Function: openInExplorer
 * ------------------------
 * Opens the given path in Windows File Explorer.
 */
void openInExplorer(const std::string& path) {
    ShellExecuteA(NULL, "open", "explorer.exe", path.c_str(), NULL, SW_SHOWNORMAL);
}

/*
 * Function: openDownloadsFolder
 * -----------------------------
 * Opens the user's Downloads folder.
 */
void openDownloadsFolder() {
    PWSTR downloadsPath = NULL;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Downloads, 0, NULL, &downloadsPath))) {
        ShellExecuteW(NULL, L"open", L"explorer.exe", downloadsPath, NULL, SW_SHOWNORMAL);
        CoTaskMemFree(downloadsPath);
    } else {
        std::cerr << "Error: Could not locate Downloads folder.\n";
    }
}

/*
 * Function: main
 * --------------
 * Entry point. Accepts one argument, sanitizes it, ensures config and directory,
 * switches to it, and opens it and Downloads folder.
 */
int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <folder_name>\n";
        return EXIT_FAILURE;
    }

    std::string folderName = promptForFolderName(argv[1]);
    std::string basePath = readOrCreateBasePath();
    fs::path fullPath = fs::path(basePath) / folderName;

    if (!fs::exists(fullPath)) {
        if (!fs::create_directory(fullPath)) {
            std::cerr << "Error creating directory: " << fullPath << "\n";
            return EXIT_FAILURE;
        }
        std::cout << "Directory created: " << fullPath << "\n";
    } else {
        std::cout << "Directory already exists: " << fullPath << "\n";
    }

    if (_chdir(fullPath.string().c_str()) != 0) {
        perror("Error changing directory");
        return EXIT_FAILURE;
    }

    openInExplorer(fullPath.string());
    openDownloadsFolder();

    return EXIT_SUCCESS;
}
