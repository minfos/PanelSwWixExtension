#pragma once
#include "../CaCommon/DeferredActionBase.h"
#include "FileOperations.h"
#include <taskschd.h>
#include <atlbase.h>

class CTaskScheduler :
	public CDeferredActionBase
{
public:

	CTaskScheduler();
	virtual ~CTaskScheduler();

	HRESULT AddCreateTask(LPCWSTR szTaskName, LPCWSTR szTaskXml, LPCWSTR szUser, LPCWSTR szPassword);
	HRESULT AddRemoveTask(LPCWSTR szTaskName);
	HRESULT AddRollbackTask(LPCWSTR szTaskName, CTaskScheduler* pRollback, CFileOperations* pCommit);

protected:
	// Execute the command object
	HRESULT DeferredExecute(const ::std::string& command) override;

private:
	HRESULT AddBackupTask(LPCWSTR szTaskName, LPCWSTR szBackupFile);
	HRESULT AddRestoreTask(LPCWSTR szTaskName, LPCWSTR szBackupFile);

	HRESULT CreateTask(LPCWSTR szTaskName, LPCWSTR szTaskXml, LPCWSTR szUser, LPCWSTR szPassword);
	HRESULT RemoveTask(LPCWSTR szTaskName);
	HRESULT BackupTask(LPCWSTR szTaskName, LPCWSTR szBackupFile);
	HRESULT RestoreTask(LPCWSTR szTaskName, LPCWSTR szBackupFile);

	// Fields
	bool bComInit_;
	CComPtr<ITaskService> pService_;
	CComPtr<ITaskFolder> pRootFolder_;
};

