#include "pch.h"
#include "PanelSwBackendExtension.h"
#include "PanelSwZipContainer.h"
#include "PanelSwLzmaContainer.h"
#include <BextBaseBundleExtensionProc.h>
using namespace std;

CPanelSwBundleExtension::CPanelSwBundleExtension(IBundleExtensionEngine* pEngine)
	: CBextBaseBundleExtension(pEngine)
{
	XmlInitialize();
}

CPanelSwBundleExtension::~CPanelSwBundleExtension()
{
	Reset();
	XmlUninitialize();
}

HRESULT CPanelSwBundleExtension::Reset()
{
	ContainerIterator endIt = _containers.end();
	for (ContainerIterator it = _containers.begin(); it != endIt; ++it)
	{
		if (*it)
		{
			delete* it;
		}
	}
	_containers.clear();
	return S_OK;
}

STDMETHODIMP CPanelSwBundleExtension::ContainerOpen(LPCWSTR wzContainerId, LPCWSTR wzFilePath, LPVOID* pContext)
{
	HRESULT hr = S_OK;
	CComPtr<IXMLDOMDocument> pixdManifest;
	CComPtr<IXMLDOMNode> pixnBundleExtension;
	CComPtr<IXMLDOMNode> pixnCompression;
	CComVariant compression;
	LPWSTR szXPath = nullptr;
	IPanelSwContainer* pContainer = nullptr;

	hr = XmlLoadDocumentFromFile(m_sczBundleExtensionDataPath, &pixdManifest);
	BextExitOnFailure(hr, "Failed to load bundle extension manifest from path: %ls", m_sczBundleExtensionDataPath);

	hr = BextGetBundleExtensionDataNode(pixdManifest, PANELSW_BACKEND_EXTENSION_ID, &pixnBundleExtension);
	BextExitOnFailure(hr, "Failed to get BundleExtension '%ls'", PANELSW_BACKEND_EXTENSION_ID);

	hr = StrAllocFormatted(&szXPath, L"PSW_ContainerExtensionData[@ContainerId='%ls']/@Compression", wzContainerId);
	BextExitOnFailure(hr, "Failed to allocate XPath string");

	hr = XmlSelectSingleNode(pixnBundleExtension, szXPath, &pixnCompression);
	BextExitOnFailure(hr, "Failed to get container extension data");

	hr = pixnCompression->get_nodeValue(&compression);
	BextExitOnFailure(hr, "Failed to get container compression");

	if (::wcsicmp(compression.bstrVal, L"Zip") == 0)
	{
		pContainer = new CPanelSwZipContainer();
		BextExitOnNull(pContainer, hr, E_FAIL, "Failed to get allocate zip container");
	}
	else if (::wcsicmp(compression.bstrVal, L"SevenZip") == 0)
	{
		pContainer = new CPanelSwLzmaContainer();
		BextExitOnNull(pContainer, hr, E_FAIL, "Failed to get allocate 7z container");
	}
	else
	{
		hr = E_INVALIDDATA;
		BextExitOnFailure(hr, "Unsupported container compression '%ls'", compression.bstrVal);
	}

	hr = pContainer->ContainerOpen(wzContainerId, wzFilePath);
	BextExitOnFailure(hr, "Failed to open container");

	_containers.push_back(pContainer);
	*pContext = pContainer;
	pContainer = nullptr;

LExit:
	ReleaseStr(szXPath);
	if (pContainer)
	{
		delete pContainer;
	}

	return hr;
}

STDMETHODIMP CPanelSwBundleExtension::ContainerNextStream(LPVOID pContext, BSTR* psczStreamName)
{
	HRESULT hr = S_OK;
	IPanelSwContainer* pContainer = nullptr;

	hr = GetContainer(pContext, &pContainer);
	BextExitOnFailure(hr, "Failed to get container");
	
	hr = pContainer->ContainerNextStream(psczStreamName);

LExit:
	return hr;
}

STDMETHODIMP CPanelSwBundleExtension::ContainerStreamToFile(LPVOID pContext, LPCWSTR wzFileName)
{
	HRESULT hr = S_OK;
	IPanelSwContainer* pContainer = nullptr;

	hr = GetContainer(pContext, &pContainer);
	BextExitOnFailure(hr, "Failed to get container");
	
	hr = pContainer->ContainerStreamToFile(wzFileName);

LExit:
	return hr;
}

STDMETHODIMP CPanelSwBundleExtension::ContainerStreamToBuffer(LPVOID pContext, BYTE** ppbBuffer, SIZE_T* pcbBuffer)
{
	HRESULT hr = S_OK;
	IPanelSwContainer* pContainer = nullptr;

	hr = GetContainer(pContext, &pContainer);
	BextExitOnFailure(hr, "Failed to get container");
	
	hr = pContainer->ContainerStreamToBuffer(ppbBuffer, pcbBuffer);

LExit:
	return hr;
}

STDMETHODIMP CPanelSwBundleExtension::ContainerSkipStream(LPVOID pContext)
{
	HRESULT hr = S_OK;
	IPanelSwContainer* pContainer = nullptr;

	hr = GetContainer(pContext, &pContainer);
	BextExitOnFailure(hr, "Failed to get container");
	
	hr = pContainer->ContainerSkipStream();

LExit:
	return hr;
}

STDMETHODIMP CPanelSwBundleExtension::ContainerClose(LPVOID pContext)
{
	HRESULT hr = S_OK;
	IPanelSwContainer* pContainer = nullptr;

	hr = GetContainer(pContext, &pContainer);
	BextExitOnFailure(hr, "Failed to get container");
	
	hr = pContainer->ContainerClose();
	BextExitOnFailure(hr, "Failed to close container");

	ReleaseContainer(pContainer);

LExit:
	return hr;
}

HRESULT CPanelSwBundleExtension::GetContainer(LPVOID pContext, IPanelSwContainer** ppContainer)
{
	HRESULT hr = E_NOTFOUND;

	ContainerIterator endIt = _containers.end();
	for (ContainerIterator it = _containers.begin(); it != endIt; ++it)
	{
		if (*it == (IPanelSwContainer*)pContext)
		{
			hr = S_OK;
			*ppContainer = (IPanelSwContainer*)pContext;
			break;
		}
	}

	return hr;
}

HRESULT CPanelSwBundleExtension::ReleaseContainer(IPanelSwContainer* pContainer)
{
	HRESULT hr = E_NOTFOUND;

	ContainerIterator endIt = _containers.end();
	for (ContainerIterator it = _containers.begin(); it != endIt; ++it)
	{
		if (*it == pContainer)
		{
			hr = S_OK;
			_containers.remove(pContainer);
			delete pContainer;
			break;
		}
	}
	
LExit:
	return hr;
}

extern "C" HRESULT WINAPI BundleExtensionCreate(
	__in const BUNDLE_EXTENSION_CREATE_ARGS * pArgs,
	__inout BUNDLE_EXTENSION_CREATE_RESULTS * pResults
)
{
	HRESULT hr = S_OK;
	IBundleExtensionEngine* pEngine = nullptr;
	CPanelSwBundleExtension* pExtension = nullptr;

	hr = BextInitializeFromCreateArgs(pArgs, &pEngine);
	ExitOnFailure(hr, "Failed to initialize bext");

	pExtension = new CPanelSwBundleExtension(pEngine);
	BextExitOnNull(pExtension, hr, E_OUTOFMEMORY, "Failed to create new CPanelSwBundleExtension.");

	hr = pExtension->Initialize(pArgs);
	BextExitOnFailure(hr, "CPanelSwBundleExtension initialization failed.");

	pResults->pfnBundleExtensionProc = BextBaseBundleExtensionProc;
	pResults->pvBundleExtensionProcContext = pExtension;

LExit:
	ReleaseObject(pEngine);

	return hr;
}

extern "C" void WINAPI BundleExtensionDestroy()
{
	BextUninitialize();
}
