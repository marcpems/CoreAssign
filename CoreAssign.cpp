#include <iostream>
#include <iomanip>
#include <windows.h>
#include <psapi.h>
#include <regex>
#include "CoreAssign.h"

enum CommandType
{
    List,
    Attach,
    About,
    Help,
    Error
};

const char* LowLevel = "Background cores";
const char* MediumLevel = "Medium performance cores";
const char* HighLevel = "High performance cores";
const char* GenericLevel = "Power level %d";

CommandType cmd;
bool quiet = false; 
struct ProcDetails
{
    unsigned __int64 ProcessorMask;
    BYTE  Class;
};

struct EfficiencyDetails
{
    EfficiencyDetails() : ClassLevel(0), ProcessorMask(0), LevelName(NULL) {};
    BYTE ClassLevel;
    unsigned __int64 ProcessorMask;
    const char* LevelName;
};

int coreCount = 0;
ProcDetails* coreList = NULL;
int perfLevelCount = 0;
EfficiencyDetails* perfLevels = NULL;
unsigned __int64 MaxProcessorMask = 0;

int attachProcId = 0;
char* attachProcName = NULL;
bool setHigh = true;


void CleanUp()
{
    delete[] coreList;
    coreList = NULL;

    delete[] perfLevels;
    perfLevels = NULL;

    delete[] attachProcName;
    attachProcName = NULL;
}

bool CheckOptionParam(const char* lowerParam, const char* paramValue, const char* shortParam)
{
    bool retval = false;

    if (0 == strcmp(lowerParam, paramValue) || 0 == strcmp(lowerParam, shortParam))
    {
        retval = true;
    }

    return retval;
}

bool CheckSingleParam(const char* lowerParam, const char * paramValue, const char * shortParam, CommandType typeToSet)
{
    bool retval = CheckOptionParam(lowerParam, paramValue, shortParam);

    if (retval)
    {
        cmd = typeToSet;
    }

    return retval;
}

bool CheckValueParam(const char* lowerParam, const char* originalParam, const char* paramValue, const char* shortParam, CommandType typeToSet, char** outval)
{
    int retval = false;
    if (outval)
    {
        if (strlen(lowerParam) > strlen(paramValue))
        {
            if (0 == strncmp(lowerParam, paramValue, strlen(paramValue)))
            {
                // now extract the int at the end
                *outval = const_cast<char*>(originalParam + strlen(paramValue) + 1);
                retval = true;
            }
        }
        else if (strlen(lowerParam) > strlen(shortParam))
        {
            if (0 == strncmp(lowerParam, shortParam, strlen(shortParam)))
            {
                // now extract the int at the end
                *outval = const_cast<char*>(originalParam + strlen(shortParam) + 1);
                retval = true;
            }
        }
    }

    if (retval)
    {
        cmd = typeToSet;
    }

    return retval;
}

void GetCommand(char* argv[], int argc)
{
    cmd = CommandType::Error;

    if (argc > 1)
    {
        // find the commands
        for (int search = 1; search < argc; search++)
        {
            auto param = argv[search];

            auto length = strlen(param);
            char* lowerParam = new char[length + 1];
            for (size_t a = 0; a < length; a++)
            {
                lowerParam[a] = tolower(param[a]);
            }

            lowerParam[length] = '\0';

            // check the first character is a '/' or a '-' to ensure its a valid command
            if (param[0] == '-' || param[0] == '/')
            {
                if (param[1])
                {
                    auto lowerParamCmd = lowerParam + 1;
                    char* values = NULL;
                    if (CheckValueParam(lowerParamCmd, param, "attach:", "a:", CommandType::Attach, &values))
                    {
                        // is it numeric?
                        attachProcId = atoi(values);

                        // couldn't get a number, how about a name
                        if (attachProcId == 0 && values != NULL && *values != '\0')
                        {
                            // grab the value
                            attachProcName = new char[strlen(values) + 1]{ 0 };
                            strcpy_s(attachProcName, strlen(values) + 1, values);
                        }

                        if (attachProcId == 0 && attachProcName == NULL)
                        {
                            cmd = CommandType::Error;
                            return;
                        }
                    }
                    else
                    {
                        CheckSingleParam(lowerParamCmd, "list", "l", CommandType::List);
                        CheckSingleParam(lowerParamCmd, "about", "version", CommandType::About);
                        CheckSingleParam(lowerParamCmd, "help", "h", CommandType::Help);
                        CheckSingleParam(lowerParamCmd, "?", "?", CommandType::Help);
                    }

                    if (CheckOptionParam(lowerParamCmd, "quiet", "q"))
                    {
                        quiet = true;
                    }
                }
            }
            else
            {
                // look for high or low
                bool isHigh = CheckOptionParam(lowerParam, "high", "h");

                // look for high or low
                bool isLow = CheckOptionParam(lowerParam, "low", "l");

                if (isLow)
                {
                    setHigh = false;
                }

                if (!(isHigh ^ isLow))
                {
                    // ERROR!!
                    cmd = CommandType::Error;
                    return;
                }
            }

            delete[] lowerParam;
        }
    }
}

bool ReadLogicalProcessorMap()
{
    DWORD length = 0;
    bool retVal = false;

    if (!GetLogicalProcessorInformationEx(LOGICAL_PROCESSOR_RELATIONSHIP::RelationProcessorCore, NULL, &length) && ERROR_INSUFFICIENT_BUFFER == GetLastError())
    {
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX procinfolist = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)new char[length];
        if (GetLogicalProcessorInformationEx(LOGICAL_PROCESSOR_RELATIONSHIP::RelationProcessorCore, procinfolist, &length))
        {
            PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX procinfo = procinfolist;

            // count the cores
            while (procinfo < (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)((char*)procinfolist + length))
            {
                coreCount++;
                procinfo = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)((char*)procinfo + procinfo->Size);
            }

            // reset to re-iterate
            procinfo = procinfolist;
            coreList = new ProcDetails[coreCount];
            perfLevels = new EfficiencyDetails[coreCount];

            // grab the relevant details into a know struct. 
            for (int count = 0; count < coreCount; count++)
            { 
                coreList[count].ProcessorMask = procinfo->Processor.GroupMask->Mask;
                if (coreList[count].ProcessorMask > MaxProcessorMask)
                {
                    MaxProcessorMask = coreList[count].ProcessorMask;
                }

                coreList[count].Class = procinfo->Processor.EfficiencyClass;

                // Now see if this is a new efficiency level - add it to the list if so, or add the mask to the existing mask entry
                int countperflevel = 0;
                for (; countperflevel < perfLevelCount; countperflevel++)
                {
                    if (perfLevels[countperflevel].ClassLevel == coreList[count].Class)
                    {
                        // update the processor mask to include this core as well.
                        perfLevels[countperflevel].ProcessorMask |= coreList[count].ProcessorMask;
                        break;
                    }
                }

                if (countperflevel == perfLevelCount)
                {
                    // new efficiency level. Add a new entry.
                    perfLevels[countperflevel].ClassLevel = coreList[count].Class;
                    perfLevels[countperflevel].ProcessorMask = coreList[count].ProcessorMask;
                    perfLevelCount++;
                }

                procinfo = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)((char*)procinfo + procinfo->Size);
            }

            // sort into ascending order
            for (int count = 0; count < perfLevelCount -1; count++)
            {
                for (int innerCount = 0; innerCount < perfLevelCount - 1; innerCount++)
                {
                    if (perfLevels[innerCount].ClassLevel > perfLevels[innerCount + 1].ClassLevel)
                    {
                        auto savedItem = perfLevels[innerCount + 1];
                        perfLevels[innerCount + 1] = perfLevels[innerCount];
                        perfLevels[innerCount] = savedItem;
                    }
                }
            }

            if (perfLevelCount == 1)
            {
                // all processors are the same level.
                perfLevels[0].LevelName = "symetric performance";
            }
            else if (perfLevelCount > 1 && perfLevelCount <4)
            {
                perfLevels[0].LevelName = LowLevel;
                perfLevels[1].LevelName = MediumLevel;
                perfLevels[perfLevelCount - 1].LevelName = HighLevel;
            }
            else
            {
                auto len = strlen(GenericLevel) + 3;
                // add a display text to each mask
                for (int count = 0; count < perfLevelCount; count++)
                {
                    auto buff = new char[len];
                    sprintf_s(buff, len, GenericLevel, count);
                    perfLevels[count].LevelName = buff;
                }
            }

            // everything worked
            retVal = true;
        }

        delete[] procinfolist;
    }

    return retVal;
}

void ListProcs()
{
    std::cout << "Found " << coreCount << " cores grouped by " << perfLevelCount << " power levels.\n";

    if (perfLevelCount == 1)
    {
        // this is a symetric proccessor
        std::cout << "There are no capability differences for cores in this system.\n";
    }
    else
    {
        std::cout << "\n ";

        int width = coreCount < 9 ? 4 : 8;
        for (int count = 0; count < perfLevelCount; count++)
        {
            std::cout << "Core group: " << perfLevels[count].LevelName << "\n";
            std::cout << "  Mask: 0x";
            std::cout.fill('0');
            std::cout << std::hex << std::setw(width) << (int)perfLevels[count].ProcessorMask << std::dec << "   ";
            for (int bitCounter = __max(coreCount - 1, 7); bitCounter >= 0; bitCounter--)
            {
                std::cout << ((perfLevels[count].ProcessorMask >> bitCounter) & 1);
            }

            std::cout << "\n";
            std::cout << "\n ";
        }

        std::cout << "Full core list:\n" << std::hex;

        int bitCount = 0;
        auto countProcessorMask = MaxProcessorMask;
        while (countProcessorMask) 
        {
            countProcessorMask = countProcessorMask >> 1;
            bitCount++;
        }

        for (int count = 0; count < coreCount; count++)
        {
            std::cout << " Core Mask: 0x" << std::hex << std::setw(width) << (int)coreList[count].ProcessorMask << std::dec << "  ";
            for (int bitCounter = bitCount - 1; bitCounter >= 0; bitCounter--)
            {
                std::cout << ((coreList[count].ProcessorMask >> bitCounter) & 1);
            }

            std::cout << "  efficiency: " << (int)coreList[count].Class << "\n";
        }
    }
}

bool SetupProcessAffinity(DWORD procId, unsigned __int64 mask)
{
    bool retVal = false;

    auto hnd = OpenProcess(PROCESS_SET_INFORMATION, FALSE, procId);
    if (hnd != 0)
    {
        retVal = SetProcessAffinityMask(hnd, mask);
    }

    return retVal;
}

bool ProcessNameMatch(DWORD processID, wchar_t * name)
{
    bool retVal = false;

    wchar_t szProcessName[MAX_PATH] = { 0 };

    // Get a handle to the process.
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
        PROCESS_VM_READ,
        FALSE, processID);

    // Get the process name.
    if (NULL != hProcess)
    {
        HMODULE hMod;
        DWORD cbNeeded;

        if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded))
        {
            GetModuleBaseName(hProcess, hMod, szProcessName,
                sizeof(szProcessName) / sizeof(wchar_t));

            // Does the process name start with the provided regex name
            std::wstring target(szProcessName);
            std::wsmatch wideMatch;
            std::wregex wrx(name);

            retVal = regex_match(target.cbegin(), target.cend(), wideMatch, wrx);

            if (retVal && !quiet)
            {
                std::cout << " Found process ID:0x" << std::hex << processID << " with name '";
                std::wcout << szProcessName << L"'.";
            }
        }
        // Release the handle to the process.
        CloseHandle(hProcess);
    }

    return retVal;
}

void ShowAffinityError(DWORD procId)
{
    // got an error
    if (!quiet)
    {
        std::cout << "Error adjusting proc: " << procId << ". failed with error: " << GetLastError() << "\n";
    }
}

bool SetProcessAffinity()
{
    bool retVal = false;

    if (!quiet)
    {
        std::cout << "Setting affinity for ";
        if (attachProcId)
        {
            std::cout << "process id:" << attachProcId;
        }
        else
        {
            std::cout << "all processes matching the name: " << attachProcName;
        }

        std::cout << " to " << (setHigh ? "high" : "low") << " powered cores.\n";
    }

    auto mask = perfLevels[0].ProcessorMask;
    if (setHigh)
    {
        mask = perfLevels[perfLevelCount - 1].ProcessorMask;
    }

    if (attachProcId)
    {
        if (!SetupProcessAffinity(attachProcId, mask))
        {
            ShowAffinityError(attachProcId);
        }
        else
        {
            retVal = true;
        }
    }
    else
    {
        DWORD requested = 0;
        DWORD needed = 0;
        DWORD* processList = NULL; 
        auto hitError = false;
        int listLength = 0;

        do
        {
            requested += 128;
            delete[] processList;
            processList = (DWORD*) new BYTE[requested]{ 0 };
            listLength = requested / sizeof(DWORD);
            if (!EnumProcesses(processList, listLength, &needed))
            {
                hitError = true;
                break;
            }
        } while (needed == listLength);

        // go find a list of processes and do the same thing
        if (!hitError)
        {
            wchar_t* wideProcName = NULL;

            // convert the input param to unicode.
            int len = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, attachProcName, -1, NULL, 0);
            wideProcName = new wchar_t[len] { 0 };
            MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, attachProcName, -1, wideProcName, len);

            // look through each one
            for (unsigned int count = 0; count < needed; count++)
            {
                if (ProcessNameMatch(processList[count], wideProcName))
                {
                    if (!SetupProcessAffinity(processList[count], mask))
                    {
                        if (!quiet)
                        {
                            std::cout << " Set failed with 0x" << GetLastError() << ".\n";
                        }
                        hitError = true;
                        break;
                    }
                    else
                    {
                        if (!quiet)
                        {
                            std::cout << " Set succeeded.\n";
                        }
                    }
                }
            }

            retVal = !hitError;
            delete[] wideProcName;
        }

        delete[] processList;
    }

    return retVal;
}


int main(int argc, char* argv[])
{
    GetCommand(argv, argc);

    if (!quiet)
    {
        std::cout << "CoreAssign v1.0, Copyright Microsoft Ltd [2020]\n";
        std::cout << "\n";
    }

    bool error = false;

    switch (cmd)
    {
        case List:
            // go get the CPU details and print them out
            error = !ReadLogicalProcessorMap();
            if (!error)
            {
                ListProcs();
            }
            break;

        case Attach:
            error = !ReadLogicalProcessorMap();
            if (!error)
            {
                error = !SetProcessAffinity();
            }
            break;

        case About:
            std::cout << "Displays details of CPU core capability and sets process affinity to high or low powered cores.\n";
            std::cout << "\n";
            std::cout << "COREASSIGN [/LIST] [/L] [/ATTACH:<PROC-ID>|<REGX-PROCESS-DISPLAY-NAME> [HIGH|LOW]] [/HELP] [/H] [/?]\n";
            std::cout << "           [/QUIET] [/Q] \n";
            std::cout << "\n";
            break;

        case Help:
            std::cout << "Displays details of CPU core capability and sets process affinity to high or low powered cores.\n";
            std::cout << "\n";
            std::cout << "COREASSIGN [/LIST] [/L] [/ATTACH:<PROC-ID>|<REGX-PROCESS-DISPLAY-NAME> [HIGH|LOW]] [/HELP] [/H] [/?]\n";
            std::cout << "           [/QUIET] [/Q] \n";
            std::cout << "\n";
            std::cout << "  /LIST /L        List the cores available with detail of high / low power.\n";
            std::cout << "  /HELP /H /?     Display this help text.\n";
            std::cout << "  /ATTACH:<PROC-ID>|<REGX-PROCESS-DISPLAY-NAME> [HIGH|LOW]\n";
            std::cout << "                  Set the core affinity for the process identified by the PROC-ID or <REGX-PROCESS-DISPLAY-NAME>\n";
            std::cout << "                  to either a high powered core if HIGH is specified (also by default) or to low powered\n";
            std::cout << "                  core if LOW is specified.\n";
            std::cout << "                  PROC-ID is the decimal proccess ID for the target single process. \n";
            std::cout << "                  REGX-PROCESS-DISPLAY-NAME is a regular expression that will be used to search all running\n";
            std::cout << "                  processes, comparing to the display name for the process. Affinity will be set for all matches.\n";
            std::cout << "  /QUIET /Q       Suppress the tool banner and other output.\n";
            break;

        default:
        case Error:
            std::cout << "Error in parameters\n";
            std::cout << "\n";
            std::cout << "COREASSIGN [/LIST] [/L] [/ATTACH:<PROC-ID>|<REGX-PROCESS-DISPLAY-NAME> [HIGH|LOW]] [/HELP] [/H] [/?]\n";
            std::cout << "           [/QUIET] [/Q] \n";
            break;
    }

    if (error)
    {
        std::cout << "Error reading logical processor info:";
        std::cout << GetLastError();
        std::cout << "\n";
    }

    CleanUp();
 }