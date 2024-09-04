#include "stdafx.h"
#include <Windows.h>
#include <Shlobj.h>
#include <string>
#include <filesystem> // C++17 or later
#include <iostream>

namespace fs = std::filesystem;

// Constants for plugin installation paths and resources
const std::wstring kPluginFolderName = L"DaVinciResolveTweaks";
const std::wstring kPluginFileName = L"DaVinciResolveTweaks.dll";
const std::wstring kResolvePluginsPath = L"C:\\ProgramData\\Blackmagic Design\\DaVinci Resolve\\Support\\Plugins"; // Default Resolve plugins path (may vary)
const std::wstring kPluginResourcesPath = L"Resources"; // Path to plugin resources (e.g., LUTs, images)

// Function to check if DaVinci Resolve is installed (simplified check)
bool IsResolveInstalled()
{
    // Check if the Resolve plugins directory exists
    return fs::exists(kResolvePluginsPath);
}

// Function to get the DaVinci Resolve installation path (simplified)
std::wstring GetResolveInstallationPath()
{
    // (In a real installer, you would need a more robust method 
    // to find the Resolve installation path, e.g., using registry keys) 
    return L"C:\\Program Files\\Blackmagic Design\\DaVinci Resolve"; 
}


// Function to check if the plugin is already installed
bool IsPluginInstalled()
{
    std::wstring pluginPath = kResolvePluginsPath + L"\\" + kPluginFolderName + L"\\" + kPluginFileName;
    return fs::exists(pluginPath);
}


// Function to copy a file
bool CopyFileWithRetry(const std::wstring& source, const std::wstring& destination, bool overwrite)
{
    int retries = 5; // Number of retry attempts
    DWORD attributes = GetFileAttributesW(source.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        std::wcerr << L"Error: Could not get file attributes for " << source << std::endl;
        return false;
    }
    if (attributes & FILE_ATTRIBUTE_READONLY) {
        if (!SetFileAttributesW(source.c_str(), attributes & ~FILE_ATTRIBUTE_READONLY)) {
            std::wcerr << L"Error: Could not remove read-only attribute from " << source << std::endl;
            return false;
        }
    }

    while (retries > 0)
    {
        if (CopyFileW(source.c_str(), destination.c_str(), !overwrite))
        {
            return true;
        }

        DWORD error = GetLastError();
        if (error == ERROR_SHARING_VIOLATION || error == ERROR_LOCK_VIOLATION)
        {
            std::wcerr << L"Error copying file (sharing violation). Retrying..." << std::endl;
            Sleep(1000); // Wait 1 second before retrying
            retries--;
        }
        else
        {
            std::wcerr << L"Error copying file: " << error << std::endl;
            return false;
        }
    }

    std::wcerr << L"Error: Failed to copy file after multiple retries." << std::endl;
    return false;
}



// Function to create a directory recursively
bool CreateDirectoryRecursive(const std::wstring& path)
{
    if (fs::exists(path)) {
        return true; // Directory already exists
    }

    // Create parent directories if they don't exist
    std::wstring parentPath = fs::path(path).parent_path().wstring();
    if (!parentPath.empty() && !CreateDirectoryRecursive(parentPath)) {
        return false;
    }

    if (CreateDirectoryW(path.c_str(), nullptr)) {
        return true;
    } else {
        std::wcerr << L"Error creating directory: " << path << L", error code: " << GetLastError() << std::endl;
        return false;
    }
}


// Function to copy the plugin files to the DaVinci Resolve plugins directory
bool InstallPluginFiles()
{
    // Get the path to the plugin DLL in the current directory
    wchar_t currentDirectory[MAX_PATH];
    GetCurrentDirectoryW(MAX_PATH, currentDirectory);
    std::wstring pluginSourcePath = currentDirectory + L"\\" + kPluginFileName;


    // Create the plugin destination directory
    std::wstring pluginDestinationDir = kResolvePluginsPath + L"\\" + kPluginFolderName;
    if (!CreateDirectoryRecursive(pluginDestinationDir))
    {
        std::wcerr << L"Error: Could not create plugin directory." << std::endl;
        return false;
    }

    // Copy the plugin DLL 
    std::wstring pluginDestinationPath = pluginDestinationDir + L"\\" + kPluginFileName;
    if (!CopyFileWithRetry(pluginSourcePath, pluginDestinationPath, true))
    {
        std::wcerr << L"Error: Could not copy plugin DLL." << std::endl;
        return false;
    }

    // Copy plugin resources (e.g., LUTs, images)
    std::wstring resourcesSourcePath = currentDirectory + L"\\" + kPluginResourcesPath;
    std::wstring resourcesDestinationPath = pluginDestinationDir + L"\\" + kPluginResourcesPath;
    if (fs::exists(resourcesSourcePath))
    {
        if (!CreateDirectoryRecursive(resourcesDestinationPath))
        {
            std::wcerr << L"Error: Could not create resources directory." << std::endl;
            return false;
        }

        for (const auto& entry : fs::directory_iterator(resourcesSourcePath))
        {
            if (entry.is_regular_file()) 
            {
                std::wstring resourceSource = entry.path().wstring();
                std::wstring resourceDestination = resourcesDestinationPath + L"\\" + entry.path().filename().wstring();
                if (!CopyFileWithRetry(resourceSource, resourceDestination, true))
                {
                    std::wcerr << L"Error: Could not copy resource file: " << resourceSource << std::endl;
                    return false;
                }
            }
        }
    }

    return true; 
}


// Function to register the plugin with DaVinci Resolve (simulated)
bool RegisterPlugin()
{
    // (In a real installer, you would need to interact with the Resolve API 
    // to register the plugin properly, if required)

    // For this example, we'll just simulate registration with a message
    std::wcout << L"Plugin registered successfully (simulated)." << std::endl;
    return true;
}


// Function to display a message box
void ShowMessageBox(const std::wstring& title, const std::wstring& message)
{
    MessageBoxW(nullptr, message.c_str(), title.c_str(), MB_OK);
}


// Main installer function
int InstallPlugin()
{
    // Check if DaVinci Resolve is installed
    if (!IsResolveInstalled())
    {
        ShowMessageBox(L"Error", L"DaVinci Resolve does not appear to be installed. Please install DaVinci Resolve before proceeding.");
        return 1; // Error code
    }


    // Check if the plugin is already installed
    if (IsPluginInstalled())
    {
        int result = MessageBoxW(nullptr, L"The plugin is already installed. Do you want to reinstall it?", L"Plugin Already Installed", MB_YESNO);
        if (result != IDYES)
        {
            return 0; // User cancelled reinstallation
        }
    }

    // Copy plugin files 
    if (!InstallPluginFiles())
    {
        ShowMessageBox(L"Error", L"Failed to install plugin files.");
        return 1;
    }


    // Register plugin (simulated)
    if (!RegisterPlugin()) 
    {
        ShowMessageBox(L"Error", L"Failed to register plugin.");
        return 1;
    }


    // Installation successful
    ShowMessageBox(L"Success", L"DaVinci Resolve Tweaks plugin installed successfully! Please restart DaVinci Resolve.");
    return 0; 
}


// Entry point for the installer
int wmain(int argc, wchar_t* argv[])
{
    return InstallPlugin(); 
}
