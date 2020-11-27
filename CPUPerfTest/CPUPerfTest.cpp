// CPUPerfTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <iomanip>
#include <windows.h>

int threadCount = 1;

bool CheckValueParam(const char* lowerParam, const char* paramValue, const char* shortParam, int* outval)
{
	int retval = false;
	if (outval)
	{
		if (strlen(lowerParam) > strlen(paramValue))
		{
			if (0 == strncmp(lowerParam, paramValue, strlen(paramValue)))
			{
				// now extract the int at the end
				*outval = atoi(lowerParam + strlen(paramValue));
				retval = true;
			}
		} else if (strlen(lowerParam) > strlen(shortParam))
		{
			if (0 == strncmp(lowerParam, shortParam, strlen(shortParam)))
			{
				// now extract the int at the end
				*outval = atoi(lowerParam + strlen(shortParam));
				retval = true;
			}
		}
	}

	return retval;
}


void GetCommand(char* argv[], int argc)
{
	if (argc > 1)
	{
		// find the commands
		for (int search = 1; search < argc; search++)
		{
			auto param = argv[search];

			// check the first character is a '/' or a '-' to ensure its a valid command
			if (param[0] == '-' || param[0] == '/')
			{
				// got a parameter
				// check which one it is

				if (param[1])
				{
					auto length = strlen(param);
					char* lowerParam = new char[length];
					for (size_t a = 1; a < length; a++)
					{
						lowerParam[a - 1] = tolower(param[a]);
					}

					lowerParam[length - 1] = '\0';
					int tempThreads = 0;
					if (CheckValueParam(lowerParam, "threads:", "t:", &tempThreads))
					{
						threadCount = tempThreads;
					}

					delete[] lowerParam;
				}
			}
		}
	}
}

void RunCPUTest(DWORD thread)
{
	const wchar_t* message = L"Lorem Ipsum is simply dummy text of the printing and typesetting industry. Lorem Ipsum has been the industry's standard dummy text ever since the 1500s, when an unknown printer took a galley of type and scrambled it to make a type specimen book. It has survived not only five centuries, but also the leap into electronic typesetting, remaining essentially unchanged. It was popularised in the 1960s with the release of Letraset sheets containing Lorem Ipsum passages, and more recently with desktop publishing software like Aldus PageMaker including versions of Lorem Ipsum.";

	// Get a start time
	DWORD start = GetTickCount();
	int hashsum = 0;
	// do a big operation
	for (int i = 0; i < 2000000; i++)
	{
		wchar_t* countChars = const_cast<wchar_t *>(message);
		int count = 0;
		while (*countChars)
		{
			countChars += sizeof(wchar_t);
			count++;
		}

		wchar_t* newBuffer = new wchar_t[count];

		count = 0;
		countChars = const_cast<wchar_t*>(message);
		while (*countChars)
		{
			newBuffer[count] = (*countChars * 2) << 1;
			countChars += sizeof(wchar_t);
			count++;
		}

		countChars = newBuffer;
		while (*countChars)
		{
			hashsum += (int)*countChars;
			countChars++;
		}

		delete[] newBuffer;
	}

	// get an end time
	DWORD end = GetTickCount();
	int duration = end - start;
	std::cout.fill('0');
	std::cout << "[" << thread << "] ";
	std::cout << "Time taken: 0x" << std::hex << std::setw(4) << duration << "\n";

}

DWORD WINAPI ThreadProc(_In_ LPVOID lpParameter)
{
	while (1)
	{
		RunCPUTest((DWORD)lpParameter);
	}
}

// principle is to run a tight loop and print a message every X itterations with the tick count it took to execute that loop
int main(int argc, char* argv[])
{
	GetCommand(argv, argc);

	std::cout << "Starting CPU load test... \n";
	for (int count = 0; count < threadCount; count++)
	{
		CreateThread(NULL, 0, ThreadProc, (LPVOID)count, 0, 0);
	}

	int x;
	std::cin >> x;
}

