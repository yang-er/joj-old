// JudgeW32.cpp: 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"

#include <jobapi2.h>
#include <Psapi.h>
#include <WerApi.h>
#pragma comment(lib, "wer.lib")

SIZE_T __stdcall PeakProcessMemoryInfo(HANDLE hProcess)
{
	PROCESS_MEMORY_COUNTERS pmc;
	memset(&pmc, 0, sizeof(pmc));
	pmc.cb = sizeof(pmc);
	if (K32GetProcessMemoryInfo(hProcess, &pmc, pmc.cb) == TRUE)
		return pmc.PeakWorkingSetSize;
	else
		return 0;
}

DWORD __stdcall CreateJudgeProcess(PJUDGE_INFORMATION pInfo)
{
	STARTUPINFOW si;
	memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	si.hStdError = pInfo->hStdErr;
	si.hStdInput = pInfo->hStdIn;
	si.hStdOutput = pInfo->hStdOut;
	si.dwFlags = STARTF_USESTDHANDLES;
	
	DWORD crFlag = CREATE_NO_WINDOW | CREATE_UNICODE_ENVIRONMENT
		| CREATE_SUSPENDED | CREATE_BREAKAWAY_FROM_JOB; // Sandbox

	PROCESS_INFORMATION pi;
	memset(&pi, 0, sizeof(pi));

	size_t len = wcslen(pInfo->pwzCmd) + wcslen(pInfo->pwzExe) + 2;
	PWSTR pwCmdLine = new WCHAR[len];
	pwCmdLine[0] = L'\0';
	wcscat_s(pwCmdLine, len, pInfo->pwzExe);
	wcscat_s(pwCmdLine, len, L" ");
	wcscat_s(pwCmdLine, len, pInfo->pwzCmd);

	WerAddExcludedApplication(pInfo->pwzExe, false);
	CreateProcessW(NULL, pwCmdLine, NULL, NULL, TRUE,
		crFlag, pInfo->pEnv, pInfo->pwzDir, &si, &pi);
	AssignProcessToJobObject(pInfo->hJob, pi.hProcess);
	ResumeThread(pi.hThread);
	return pi.dwProcessId;
}

HANDLE __stdcall SetupSandbox(DWORD dwMemoryLimit, DWORD dwCPUTime)
{
	HANDLE hJob = CreateJobObjectW(NULL, L"JOJ");
	BOOL result;

	JOBOBJECT_BASIC_LIMIT_INFORMATION Job_Limit;
	memset(&Job_Limit, 0, sizeof(Job_Limit));
	Job_Limit.LimitFlags = 
		JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE |
		JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION |
		JOB_OBJECT_LIMIT_PROCESS_TIME |
		JOB_OBJECT_LIMIT_WORKINGSET |
		JOB_OBJECT_LIMIT_ACTIVE_PROCESS;

	Job_Limit.PerProcessUserTimeLimit.QuadPart = 10000 * dwCPUTime;
	Job_Limit.MinimumWorkingSetSize = 1;
	Job_Limit.MaximumWorkingSetSize = 1024 * 1024 * dwMemoryLimit;
	Job_Limit.ActiveProcessLimit = 1;
	result = SetInformationJobObject(hJob, JobObjectBasicLimitInformation, &Job_Limit, sizeof(Job_Limit));
	if (result == FALSE) OutputDebugStringW(L"JobObjectBasicLimitInformation failed to set up.");

	JOBOBJECT_BASIC_UI_RESTRICTIONS Job_UI_Limit;
	Job_UI_Limit.UIRestrictionsClass = JOB_OBJECT_UILIMIT_EXITWINDOWS |
		JOB_OBJECT_UILIMIT_READCLIPBOARD |
		JOB_OBJECT_UILIMIT_WRITECLIPBOARD |
		JOB_OBJECT_UILIMIT_SYSTEMPARAMETERS |
		JOB_OBJECT_UILIMIT_DISPLAYSETTINGS |
		JOB_OBJECT_UILIMIT_GLOBALATOMS |
		JOB_OBJECT_UILIMIT_DESKTOP |
		JOB_OBJECT_UILIMIT_HANDLES;
	SetInformationJobObject(hJob, JobObjectBasicUIRestrictions, &Job_UI_Limit, sizeof(Job_UI_Limit));
	if (result == FALSE) OutputDebugStringW(L"JobObjectBasicUIRestrictions failed to set up.");

	return hJob;
}

void __stdcall UnsetSandbox(HANDLE hJob)
{
	TerminateJobObject(hJob, ~0);
	CloseHandle(hJob);
}
