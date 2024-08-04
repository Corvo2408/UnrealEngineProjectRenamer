#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <cstdlib>
#include "globals.h"

// Set console text colour
void SetConsoleColour(WORD colour)
{
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, colour);
}


// Function to replace all instances of a substring with another substring
void ReplaceAll(std::string& str, const std::string& from, const std::string& to)
{
    if (from.empty()) return;
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}


// Backup / issue warning
void DisplayWarningAndGetConfirmation()
{
    std::string userInput;

    while (true)
    {
        SetConsoleColour(COLOUR_ORANGE);
        std::cout << "INFO: There is no going back. This can break things if errors occur or done incorrectly." << std::endl;
        std::cout << "To declare you have made a backup, and that you accept the consequences, type 'YES': ";
        std::getline(std::cin, userInput);

        if (userInput == "YES")
        {
            SetConsoleColour(COLOUR_GREEN);
            std::cout << "Successfully accepted. Proceeding with the operation..." << std::endl;
            SetConsoleColour(COLOUR_WHITE);
            break;
        }
        else
        {
            SetConsoleColour(COLOUR_RED);
            std::cout << "ERROR: Invalid input. Please type 'YES' to confirm you have made a backup and accept the consequences." << std::endl;
            SetConsoleColour(COLOUR_WHITE);
        }
    }
}


// Set new project name
void SetNewProjectName()
{
    const std::string invalidChars = "\\/:*?\"<>|";

    while (true)
    {
        // Set new project name
        SetConsoleColour(COLOUR_WHITE);
        std::cout << "\nPlease enter the new desired project name: ";
        std::getline(std::cin, newProjectName);

        if (newProjectName.empty())
        {
            SetConsoleColour(COLOUR_RED);
            std::cout << "ERROR: Project name cannot be empty. Please try again." << std::endl;
            SetConsoleColour(COLOUR_WHITE);
            continue;
        }

        if (newProjectName.length() > 255)
        {
            SetConsoleColour(COLOUR_RED);
            std::cout << "ERROR: Project name is too long. Please use a name with 255 characters or fewer." << std::endl;
            SetConsoleColour(COLOUR_WHITE);
            continue;
        }

        bool isValid = true;
        for (char c : newProjectName)
        {
            if (invalidChars.find(c) != std::string::npos)
            {
                SetConsoleColour(COLOUR_RED);
                std::cout << "ERROR: Project name contains invalid characters. Avoid using: \\ / : * ? \" < > |" << std::endl;
                SetConsoleColour(COLOUR_WHITE);
                isValid = false;
                break;
            }
        }

        if (isValid)
        {
            break;
        }
    }
}


// Set Uproject Module Name
void SetUprojectModuleName()
{
    // Read the content of the .uproject file
    std::ifstream inputFile(oldUprojectFilePath);
    if (!inputFile)
    {
        SetConsoleColour(COLOUR_RED);
        std::cerr << "\nERROR: Failed to open the .uproject file." << std::endl;
        SetConsoleColour(COLOUR_WHITE);
    }

    std::stringstream buffer;
    buffer << inputFile.rdbuf();
    std::string fileContent = buffer.str();
    inputFile.close();

    // Find the old project name in the "Modules" section
    std::regex nameRegex(R"("Modules":\s*\[\s*\{\s*"Name":\s*"([^"]+))");
    std::smatch matches;
    std::string oldProjectName;

    if (std::regex_search(fileContent, matches, nameRegex) && matches.size() > 1)
    {
        oldProjectName = matches[1].str();
    }
    else
    {
        SetConsoleColour(COLOUR_RED);
        std::cerr << "\nERROR: Could not find the module name in the .uproject file." << std::endl;
        SetConsoleColour(COLOUR_WHITE);
    }

    ReplaceAll(fileContent, oldProjectName, newProjectName);

    // Write the modified content back to the .uproject file
    std::ofstream outputFile(oldUprojectFilePath);
    if (!outputFile)
    {
        SetConsoleColour(COLOUR_RED);
        std::cerr << "\nERROR: Error writing to the .uproject file." << std::endl;
        SetConsoleColour(COLOUR_WHITE);
    }
    outputFile << fileContent;
    outputFile.close();
    SetConsoleColour(COLOUR_GREEN);
    std::cout << "\nSuccessfully modified .uproject file module name to : " << newProjectName << std::endl;
    SetConsoleColour(COLOUR_WHITE);
}


// Rename .uproject file
void RenameUprojectFile()
{
    newUprojectFilePath = oldUprojectFilePath.parent_path() / (newProjectName + ".uproject");

    try
    {
        fs::rename(oldUprojectFilePath, newUprojectFilePath);
        SetConsoleColour(COLOUR_GREEN);
        std::cout << "\nSuccessfully renamed project file to: " << newUprojectFilePath << std::endl;
        SetConsoleColour(COLOUR_WHITE);
    }
    catch (const fs::filesystem_error& e)
    {
        SetConsoleColour(COLOUR_RED);
        std::cerr << "\nERROR: Error renaming project file: " << e.what() << std::endl;
        SetConsoleColour(COLOUR_WHITE);
    }
}


// Locate target files
void EditTargetFiles()
{
    sourceDirectory = fs::path(projectRootDirectory) / "Source";

    if (!fs::exists(sourceDirectory) || !fs::is_directory(sourceDirectory))
    {
        SetConsoleColour(COLOUR_RED);
        std::cerr << "\nERROR: Source directory does not exist." << std::endl;
        SetConsoleColour(COLOUR_WHITE);
        return;
    }

    bool targetFileFound = false;
    bool editorTargetFileFound = false;

    for (const auto& entry : fs::directory_iterator(sourceDirectory))
    {
        if (entry.is_regular_file())
        {
            std::string filename = entry.path().filename().string();
            std::string oldName = filename.substr(0, filename.find('.'));
            fs::path newFilePath;

            if (filename.size() > 16 && filename.substr(filename.size() - 16) == "Editor.Target.cs")
            {
                editorTargetFileFound = true;
                oldName = filename.substr(0, filename.size() - 16);
                newFilePath = sourceDirectory / (newProjectName + "Editor.Target.cs");

                // Rename the file
                fs::rename(entry.path(), newFilePath);

                // Replace contents within the file
                std::ifstream editorTargetFile(newFilePath);
                std::stringstream buffer;
                buffer << editorTargetFile.rdbuf();
                std::string fileContent = buffer.str();
                editorTargetFile.close();

                ReplaceAll(fileContent, oldName, newProjectName);

                std::ofstream outFile(newFilePath);
                outFile << fileContent;
                outFile.close();

                SetConsoleColour(COLOUR_GREEN);
                std::cout << "\nSuccessfully renamed and updated Editor Target file to: " << newFilePath << std::endl;
                SetConsoleColour(COLOUR_WHITE);
                continue;
            }

            if (filename.size() > 10 && filename.substr(filename.size() - 10) == ".Target.cs")
            {
                targetFileFound = true;
                oldName = filename.substr(0, filename.size() - 10);
                newFilePath = sourceDirectory / (newProjectName + ".Target.cs");

                // Rename the file
                fs::rename(entry.path(), newFilePath);

                // Replace references within the file
                std::ifstream targetFile(newFilePath);
                std::stringstream buffer;
                buffer << targetFile.rdbuf();
                std::string fileContent = buffer.str();
                targetFile.close();

                ReplaceAll(fileContent, oldName, newProjectName);

                std::ofstream outFile(newFilePath);
                outFile << fileContent;
                outFile.close();

                SetConsoleColour(COLOUR_GREEN);
                std::cout << "\nSuccessfully renamed and updated Target file to: " << newFilePath << std::endl;
                SetConsoleColour(COLOUR_WHITE);
            }
        }
    }

    if (!targetFileFound)
    {
        SetConsoleColour(COLOUR_RED);
        std::cerr << "\nERROR: No Target file found in the Source directory." << std::endl;
        SetConsoleColour(COLOUR_WHITE);
    }

    if (!editorTargetFileFound)
    {
        SetConsoleColour(COLOUR_RED);
        std::cerr << "\nERROR: No Editor Target file found in the Source directory." << std::endl;
        SetConsoleColour(COLOUR_WHITE);
    }
}

void EditSourceFolder()
{
    ShellExecute(NULL, L"open", sourceDirectory.c_str(), NULL, NULL, SW_SHOWNORMAL);

    while (true)
    {
        SetConsoleColour(COLOUR_WHITE);
        std::cout << "\nPlease enter the EXACT name of the folder currently containing your C++ source files: ";
        std::getline(std::cin, userCPPSourceName);

        fs::path folderPath = sourceDirectory / userCPPSourceName;

        if (fs::exists(folderPath) && fs::is_directory(folderPath))
        {
            // Process all .h files recursively
            for (const auto& entry : fs::recursive_directory_iterator(folderPath))
            {
                if (entry.is_regular_file() && entry.path().extension() == ".h")
                {
                    std::string filename = entry.path().filename().string();

                    // Read and modify the .h file
                    std::ifstream headerFile(entry.path());
                    std::stringstream buffer;
                    buffer << headerFile.rdbuf();
                    std::string fileContent = buffer.str();
                    headerFile.close();

                    // Replace the API macro name
                    std::string oldAPIName = userCPPSourceName + "_API";
                    std::string newAPIName = newProjectName + "_API";

                    // Convert newAPINames to uppercase
                    std::transform(oldAPIName.begin(), oldAPIName.end(), oldAPIName.begin(),
                        [](unsigned char c)
                        {
                            return std::toupper(c);
                        });

                    
                    std::transform(newAPIName.begin(), newAPIName.end(), newAPIName.begin(),
                        [](unsigned char c)
                        {
                            return std::toupper(c);
                        });

                    ReplaceAll(fileContent, oldAPIName, newAPIName);

                    // Write the modified content back to the file
                    std::ofstream outFile(entry.path());
                    outFile << fileContent;
                    outFile.close();

                    SetConsoleColour(COLOUR_GREEN);
                    std::cout << "\nSuccessfully updated .h file: " << entry.path() << std::endl;
                    SetConsoleColour(COLOUR_WHITE);
                }
            }

            fs::path newFolderPath = sourceDirectory / newProjectName;
            //Find and edit .Build.cs
            bool buildFileFound = false;
            for (const auto& entry : fs::directory_iterator(folderPath))
            {
                if (entry.is_regular_file())
                {
                    std::string filename = entry.path().filename().string();
                    if (filename.size() > 9 && filename.substr(filename.size() - 9) == ".Build.cs")
                    {
                        buildFileFound = true;

                        // Read and modify the .Build.cs file
                        std::ifstream buildFile(entry.path());
                        std::stringstream buffer;
                        buffer << buildFile.rdbuf();
                        std::string fileContent = buffer.str();
                        buildFile.close();

                        ReplaceAll(fileContent, userCPPSourceName, newProjectName);

                        std::ofstream outFile(entry.path());
                        outFile << fileContent;
                        outFile.close();

                        // Rename the .Build.cs file
                        fs::path newBuildFilePath = folderPath / (newProjectName + ".Build.cs");
                        fs::rename(entry.path(), newBuildFilePath);

                        SetConsoleColour(COLOUR_GREEN);
                        std::cout << "\nSuccessfully renamed and updated .Build.cs file to: " << newBuildFilePath << std::endl;
                        SetConsoleColour(COLOUR_WHITE);
                        break;
                    }
                }
            }

            if (!buildFileFound)
            {
                SetConsoleColour(COLOUR_RED);
                std::cerr << "\nERROR: No .Build.cs file found in the specified folder. Please try again." << std::endl;
                SetConsoleColour(COLOUR_WHITE);
                continue;  // Continue the loop to prompt the user again
            }

            // Find and edit .cpp and .h files
            bool cppFileFound = false;
            bool headerFileFound = false;

            for (const auto& entry : fs::directory_iterator(folderPath))
            {
                if (entry.is_regular_file())
                {
                    std::string filename = entry.path().filename().string();

                    if (filename == userCPPSourceName + ".cpp")
                    {
                        cppFileFound = true;

                        // Read and modify the .cpp file
                        std::ifstream cppFile(entry.path());
                        std::stringstream buffer;
                        buffer << cppFile.rdbuf();
                        std::string fileContent = buffer.str();
                        cppFile.close();

                        ReplaceAll(fileContent, "#include \"" + userCPPSourceName + ".h\"", "#include \"" + newProjectName + ".h\"");
                        ReplaceAll(fileContent, "IMPLEMENT_PRIMARY_GAME_MODULE( FDefaultGameModuleImpl, " + userCPPSourceName + ", \"" + userCPPSourceName + "\" );", "IMPLEMENT_PRIMARY_GAME_MODULE( FDefaultGameModuleImpl, " + newProjectName + ", \"" + newProjectName + "\" );");

                        std::ofstream outFile(entry.path());
                        outFile << fileContent;
                        outFile.close();

                        // Rename the .cpp file
                        fs::path newCppFilePath = folderPath / (newProjectName + ".cpp");
                        fs::rename(entry.path(), newCppFilePath);

                        SetConsoleColour(COLOUR_GREEN);
                        std::cout << "\nSuccessfully renamed and updated .cpp file to: " << newCppFilePath << std::endl;
                        SetConsoleColour(COLOUR_WHITE);
                    }

                    if (filename == userCPPSourceName + ".h")
                    {
                        headerFileFound = true;

                        // Rename the .h file
                        fs::path newHeaderFilePath = folderPath / (newProjectName + ".h");
                        fs::rename(entry.path(), newHeaderFilePath);

                        SetConsoleColour(COLOUR_GREEN);
                        std::cout << "\nSuccessfully renamed .h file to: " << newHeaderFilePath << std::endl;
                        SetConsoleColour(COLOUR_WHITE);
                    }
                }
            }

            if (!cppFileFound || !headerFileFound)
            {
                SetConsoleColour(COLOUR_RED);
                std::cerr << "\nERROR: Could not find .cpp and/or .h file in the specified folder. Please try again." << std::endl;
                SetConsoleColour(COLOUR_WHITE);
                continue;  // Continue the loop to prompt the user again
            }

            // Rename source file folder
            try
            {
                fs::rename(folderPath, newFolderPath);
                SetConsoleColour(COLOUR_GREEN);
                std::cout << "\nSuccessfully renamed C++ source folder to: " << newFolderPath << std::endl;
                SetConsoleColour(COLOUR_WHITE);
                break;  // Exit the loop if successful
            }
            catch (const fs::filesystem_error& e)
            {
                SetConsoleColour(COLOUR_RED);
                std::cerr << "\nError: Error renaming C++ source folder: " << e.what() << std::endl;
                SetConsoleColour(COLOUR_WHITE);
            }
        }
        else
        {
            SetConsoleColour(COLOUR_RED);
            std::cerr << "\nERROR: The specified folder does not exist in the Source directory. Please try again." << std::endl;
            SetConsoleColour(COLOUR_WHITE);
        }
    }
}

void EditConfigFiles()
{
    fs::path configDirectory = fs::path(projectRootDirectory) / "Config";
    fs::path defaultEngineIniPath = configDirectory / "DefaultEngine.ini";

    if (!fs::exists(configDirectory) || !fs::is_directory(configDirectory))
    {
        SetConsoleColour(COLOUR_RED);
        std::cerr << "\nERROR: Config directory does not exist." << std::endl;
        SetConsoleColour(COLOUR_WHITE);
        return;
    }

    // Update all config files
    for (const auto& entry : fs::directory_iterator(configDirectory))
    {
        if (entry.is_regular_file())
        {
            std::string filename = entry.path().filename().string();

            // Read and modify the config file
            std::ifstream configFile(entry.path());
            std::stringstream buffer;
            buffer << configFile.rdbuf();
            std::string fileContent = buffer.str();
            configFile.close();

            ReplaceAll(fileContent, userCPPSourceName, newProjectName);

            std::ofstream outFile(entry.path());
            outFile << fileContent;
            outFile.close();

            SetConsoleColour(COLOUR_GREEN);
            std::cout << "\nSuccessfully updated config file: " << entry.path() << std::endl;
            SetConsoleColour(COLOUR_WHITE);
        }
    }

    // Specific logic for DefaultEngine.ini
    if (fs::exists(defaultEngineIniPath))
    {
        std::ifstream configFile(defaultEngineIniPath);
        std::stringstream buffer;
        buffer << configFile.rdbuf();
        std::string fileContent = buffer.str();
        configFile.close();

        // Check for [URL] section
        std::size_t urlPos = fileContent.find("[URL]");
        if (urlPos == std::string::npos)
        {
            // Add [URL] section if it doesn't exist
            fileContent += "\n[URL]\nGameName=" + newProjectName + "\n";
        }
        else
        {
            // Check for GameName= under [URL] section
            std::size_t gameNamePos = fileContent.find("GameName=", urlPos);
            std::size_t nextSectionPos = fileContent.find('[', urlPos + 1);
            if (gameNamePos == std::string::npos || (nextSectionPos != std::string::npos && gameNamePos > nextSectionPos))
            {
                // Add GameName= under [URL] section if it doesn't exist
                fileContent.insert(urlPos + 5, "\nGameName=" + newProjectName + "\n");
            }
            else
            {
                // Update GameName= value if it exists
                std::size_t endOfLinePos = fileContent.find('\n', gameNamePos);
                fileContent.replace(gameNamePos, endOfLinePos - gameNamePos, "GameName=" + newProjectName);
            }
        }

        // Check for [/Script/Engine.Engine] section
        std::size_t enginePos = fileContent.find("[/Script/Engine.Engine]");
        if (enginePos == std::string::npos)
        {
            // Add [/Script/Engine.Engine] section if it doesn't exist
            fileContent += "\n[/Script/Engine.Engine]\n+ActiveGameNameRedirects=(OldGameName=\"/Script/" + userCPPSourceName + "\", NewGameName=\"/Script/" + newProjectName + "\")\n";
        }
        else
        {
            // Check for +ActiveGameNameRedirects under [/Script/Engine.Engine]
            std::size_t redirectsPos = fileContent.find("+ActiveGameNameRedirects=", enginePos);
            std::size_t nextSectionPos = fileContent.find('[', enginePos + 1);

            bool redirectFound = false;
            std::size_t redirectsEndPos;
            while (redirectsPos != std::string::npos && (nextSectionPos == std::string::npos || redirectsPos < nextSectionPos))
            {
                redirectsEndPos = fileContent.find('\n', redirectsPos);
                std::size_t oldGameNamePos = fileContent.find("NewGameName=\"/Script/", redirectsPos);
                if (oldGameNamePos != std::string::npos && oldGameNamePos < redirectsEndPos)
                {
                    std::size_t oldGameNameEndPos = fileContent.find('\"', oldGameNamePos + 21);
                    fileContent.replace(oldGameNamePos + 21, oldGameNameEndPos - (oldGameNamePos + 21), newProjectName);
                    redirectFound = true;
                }
                redirectsPos = fileContent.find("+ActiveGameNameRedirects=", redirectsEndPos);
            }

            // Ensure there is at least one +ActiveGameNameRedirects entry
            if (!redirectFound)
            {
                fileContent.insert(enginePos + 23, "\n+ActiveGameNameRedirects=(OldGameName=\"/Script/" + userCPPSourceName + "\", NewGameName=\"/Script/" + newProjectName + "\")\n");
            }
        }

        std::ofstream outFile(defaultEngineIniPath);
        outFile << fileContent;
        outFile.close();

        SetConsoleColour(COLOUR_GREEN);
        std::cout << "\nSuccessfully updated DefaultEngine.ini file: " << defaultEngineIniPath << std::endl;
        SetConsoleColour(COLOUR_WHITE);
    }
    else
    {
        SetConsoleColour(COLOUR_RED);
        std::cerr << "\nERROR: DefaultEngine.ini file does not exist." << std::endl;
        SetConsoleColour(COLOUR_WHITE);
    }
}

void DeleteCachedProjectDirectories()
{
    std::vector<std::string> directories = {"Saved", "Intermediate", "Binaries"};
    for (const auto& dir : directories)
    {
        fs::path dirPath = fs::path(projectRootDirectory) / dir;
        if (fs::exists(dirPath) && fs::is_directory(dirPath))
        {
            try
            {
                fs::remove_all(dirPath);
                SetConsoleColour(COLOUR_GREEN);
                std::cout << "\nSuccessfully deleted directory: " << dirPath << std::endl;
                SetConsoleColour(COLOUR_WHITE);
            }
            catch (const fs::filesystem_error& e)
            {
                SetConsoleColour(COLOUR_RED);
                std::cerr << "\nERROR: Error deleting directory " << dirPath << ": " << e.what() << std::endl;
                SetConsoleColour(COLOUR_WHITE);
            }
        }
    }
}


void DeleteSlnFiles()
{
    for (const auto& entry : fs::directory_iterator(projectRootDirectory))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".sln")
        {
            try
            {
                fs::remove(entry.path());
                SetConsoleColour(COLOUR_GREEN);
                std::cout << "\nSuccessfully deleted .sln file: " << entry.path() << std::endl;
                SetConsoleColour(COLOUR_WHITE);
            }
            catch (const fs::filesystem_error& e)
            {
                SetConsoleColour(COLOUR_RED);
                std::cerr << "\nERROR: Error deleting .sln file " << entry.path() << ": " << e.what() << std::endl;
                SetConsoleColour(COLOUR_WHITE);
            }
        }
    }
}


// Function to generate Visual Studio project files using UnrealBuildTool
void GenerateVisualStudioProjectFiles()
{
    std::string unrealBuildToolPath;
    std::string engineVersion;

    while (true)
    {
        std::cout << "\nPlease enter the Major Unreal Engine version you are using (4 or 5): ";
        std::getline(std::cin, engineVersion);

        if (engineVersion == "4" || engineVersion == "5")
        {
            break; // Exit the loop if the input is valid
        }
        else
        {
            std::cout << "\nInvalid input. Please enter either 4 or 5.";
        }
    }

    while (true)
    {
        SetConsoleColour(COLOUR_WHITE);

        std::cout << "\nPlease enter the Unreal Engine directory (usually found inside C:\\Program Files\\Epic Games\\): ";
        std::getline(std::cin, unrealEngineDirectory);

        if (fs::exists(unrealEngineDirectory) && fs::is_directory(unrealEngineDirectory))
        {
            // Path to the UnrealBuildTool.exe - Use the provided Unreal Engine directory

            if (engineVersion == "4")
            {
                unrealBuildToolPath = unrealEngineDirectory + "\\Engine\\Binaries\\DotNET\\UnrealBuildTool.exe";
            }
            else
            {
                unrealBuildToolPath = unrealEngineDirectory + "\\Engine\\Binaries\\DotNET\\UnrealBuildTool\\UnrealBuildTool.exe";
            }
            if (fs::exists(unrealBuildToolPath))
            {
                break;
            }
            else
            {
                SetConsoleColour(COLOUR_RED);
                std::cout << "\nCannot find UnrealBuildTool.exe at " << unrealBuildToolPath << ". Please try again." << std::endl;
                SetConsoleColour(COLOUR_WHITE);
            }
        }
        else
        {
            SetConsoleColour(COLOUR_RED);
            std::cout << "\nThe provided path is not a valid directory. Please try again." << std::endl;
            SetConsoleColour(COLOUR_WHITE);
        }
    }


    // Command to generate Visual Studio project files
    std::string command = "\"\"" + unrealBuildToolPath + "\" -projectfiles -project=\"" + newUprojectFilePath.string() + "\" -game -engine\"";


    // Execute the command
    int result = std::system(command.c_str());
    if (result == 0)
    {
        SetConsoleColour(COLOUR_GREEN);
        std::cout << "\nVisual Studio project files generated successfully." << std::endl;
    }
    else
    {
        SetConsoleColour(COLOUR_RED);
        std::cerr << "\nError generating Visual Studio project files." << std::endl;
    }
    SetConsoleColour(COLOUR_WHITE);
}


int main()
{
    DisplayWarningAndGetConfirmation();
    
    bool isValidDirectory = false;
    bool foundUprojectFile = false;

    while (!isValidDirectory || !foundUprojectFile)
    {
        SetConsoleColour(COLOUR_WHITE);
        std::cout << "\nPlease enter the project root directory: ";
        std::getline(std::cin, projectRootDirectory);

        // Check if the directory exists
        if (fs::exists(projectRootDirectory) && fs::is_directory(projectRootDirectory))
        {
            isValidDirectory = true;
            SetConsoleColour(COLOUR_GREEN);
            std::cout << "\nSuccessfully located directory";
            SetConsoleColour(COLOUR_WHITE);

            // Look for a .uproject file in the directory
            foundUprojectFile = false;
            for (const auto& entry : fs::directory_iterator(projectRootDirectory))
            {
                if (entry.is_regular_file() && entry.path().extension() == ".uproject")
                {
                    oldUprojectFilePath = entry.path();
                    foundUprojectFile = true;
                    SetConsoleColour(COLOUR_GREEN);
                    std::cout << "\nSuccessfully validated directory is a Unreal engine project";
                    SetConsoleColour(COLOUR_WHITE);
                    break;
                }
            }

            if (!foundUprojectFile)
            {
                SetConsoleColour(COLOUR_RED);
                std::cout << "\nERROR: No .uproject file found in the directory. Please try again." << std::endl;
                SetConsoleColour(COLOUR_WHITE);
            }
        }
        else
        {
            SetConsoleColour(COLOUR_RED);
            std::cout << "\nERROR: The provided path is not a valid directory. Please try again." << std::endl;
            SetConsoleColour(COLOUR_WHITE);
            isValidDirectory = false;
        }
    }

    
    SetNewProjectName();
    SetUprojectModuleName();
    RenameUprojectFile();
    EditTargetFiles();
    EditSourceFolder();
    EditConfigFiles();
    DeleteCachedProjectDirectories();
    DeleteSlnFiles();
    GenerateVisualStudioProjectFiles();

    SetConsoleColour(COLOUR_GREEN);
    std::cout << "\nSuccessfully completed Unreal Engine project rename" << std::endl;
    SetConsoleColour(COLOUR_ORANGE);
    std::cout << "\nINFO: When opening your Unreal Engine project for the first time after renaming, it will prompt you to rebuild missing or out of date modules. Select 'Yes'" << std::endl;
    SetConsoleColour(COLOUR_WHITE);
}