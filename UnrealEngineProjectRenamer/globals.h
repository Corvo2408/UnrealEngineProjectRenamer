#pragma once
#include <windows.h>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

//Console Colours
const WORD COLOUR_WHITE = 7;
const WORD COLOUR_RED = 12;
const WORD COLOUR_GREEN = 10;
const WORD COLOUR_ORANGE = 6;

std::string projectRootDirectory;
std::string unrealEngineDirectory;
std::string userCPPSourceName;
std::string backupFolderName;
std::string newProjectName;

fs::path oldUprojectFilePath;
fs::path newUprojectFilePath;
fs::path sourceDirectory;
fs::path backupDirectory;
fs::path currentDateTimeBackupFolder;

bool isCPPProject;

