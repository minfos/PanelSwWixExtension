
#include "stdafx.h"
#include "..\CaCommon\WixString.h"
#include <fileutil.h>
#include <errno.h>
#include <objbase.h>
#include <msxml6.h>
#include <atlbase.h>
#pragma comment( lib, "msxml6.lib")

#define XmlSearchQuery L"SELECT `Id`, `Property_`, `FilePath`, `Expression`, `Language`, `Namespaces`, `Match`, `Condition` FROM `PSW_XmlSearch`"
enum eXmlSearchQueryQuery { Id = 1, Property_, FilePath, Expression, Language, Namespaces, Match, Condition };

enum eXmlMatch
{
	first,
	all,
	enforceSingle
};

static HRESULT QueryXml(LPCWSTR pFile, LPCWSTR pExpression, LPCWSTR Language, LPCWSTR Namespaces, eXmlMatch eMatch, LPCWSTR pProperty);

extern "C" UINT __stdcall XmlSearch(MSIHANDLE hInstall)
{
	HRESULT hr = S_OK;
	UINT er = ERROR_SUCCESS;
	PMSIHANDLE hView;
	PMSIHANDLE hRecord;

	hr = WcaInitialize(hInstall, __FUNCTION__);
	BreakExitOnFailure(hr, "Failed to initialize");
	WcaLog(LOGMSG_STANDARD, "Initialized from PanelSwCustomActions " FullVersion);

	hr = ::CoInitialize(nullptr);
	BreakExitOnFailure(hr, "Failed to CoInitialize");

	// Ensure table PSW_XmlSearch exists.
	hr = WcaTableExists(L"PSW_XmlSearch");
	BreakExitOnFailure(hr, "Table does not exist 'PSW_XmlSearch'. Have you authored 'PanelSw:XmlSearch' entries in WiX code?");

	// Execute view
	hr = WcaOpenExecuteView(XmlSearchQuery, &hView);
	BreakExitOnFailure(hr, "Failed to execute SQL query on 'PSW_XmlSearch'.");

	// Iterate records
	while ((hr = WcaFetchRecord(hView, &hRecord)) != E_NOMOREITEMS)
	{
		BreakExitOnFailure(hr, "Failed to fetch record.");

		// Get fields
		CWixString szId;
		CWixString szProperty;
		CWixString szFilePath;
		CWixString szExpression, szUnformattedExpression, szObfuscatedExpression;
		CWixString szLanguage;
		CWixString szNamespaces;
		CWixString szCondition;
		eXmlMatch eMatch = eXmlMatch::first;

		hr = WcaGetRecordString(hRecord, eXmlSearchQueryQuery::Id, (LPWSTR*)szId);
		BreakExitOnFailure(hr, "Failed to get Id.");
		hr = WcaGetRecordString(hRecord, eXmlSearchQueryQuery::Property_, (LPWSTR*)szProperty);
		BreakExitOnFailure(hr, "Failed to get Property_.");
		hr = WcaGetRecordFormattedString(hRecord, eXmlSearchQueryQuery::FilePath, (LPWSTR*)szFilePath);
		BreakExitOnFailure(hr, "Failed to get FilePath.");
		hr = WcaGetRecordString(hRecord, eXmlSearchQueryQuery::Expression, (LPWSTR*)szUnformattedExpression);
		BreakExitOnFailure(hr, "Failed to get Expression.");
		hr = WcaGetRecordString(hRecord, eXmlSearchQueryQuery::Language, (LPWSTR*)szLanguage);
		BreakExitOnFailure(hr, "Failed to get Language.");
		hr = WcaGetRecordString(hRecord, eXmlSearchQueryQuery::Namespaces, (LPWSTR*)szNamespaces);
		BreakExitOnFailure(hr, "Failed to get Namespaces.");
		hr = WcaGetRecordInteger(hRecord, eXmlSearchQueryQuery::Match, (int*)&eMatch);
		BreakExitOnFailure(hr, "Failed to get Match.");
		hr = WcaGetRecordString(hRecord, eXmlSearchQueryQuery::Condition, (LPWSTR*)szCondition);
		BreakExitOnFailure(hr, "Failed to get Condition.");

		// Test condition
		if (!szCondition.IsNullOrEmpty())
		{
			MSICONDITION condRes = ::MsiEvaluateConditionW(hInstall, szCondition);
			switch (condRes)
			{
			case MSICONDITION::MSICONDITION_NONE:
			case MSICONDITION::MSICONDITION_TRUE:
				WcaLog(LOGMSG_STANDARD, "Condition evaluated to true / none.");
				break;

			case MSICONDITION::MSICONDITION_FALSE:
				WcaLog(LOGMSG_STANDARD, "Skipping. Condition evaluated to false");
				continue;

			case MSICONDITION::MSICONDITION_ERROR:
				hr = E_FAIL;
				BreakExitOnFailure(hr, "Bad Condition field");
			}
		}

		hr = szExpression.MsiFormat(szUnformattedExpression, (LPWSTR*)szObfuscatedExpression);
		BreakExitOnFailure(hr, "Failed formatting query.");

		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "Running Expression '%ls' on '%ls'", (LPCWSTR)szObfuscatedExpression, (LPCWSTR)szFilePath);
		hr = QueryXml(szFilePath, szExpression, szLanguage, szNamespaces, eMatch, szProperty);
		BreakExitOnFailure(hr, "Failed to query XML.");
	}

	hr = ERROR_SUCCESS;

LExit:
	::CoUninitialize();

	er = SUCCEEDED(hr) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
	return WcaFinalize(er);
}

static HRESULT QueryXml(LPCWSTR pFile, LPCWSTR pExpression, LPCWSTR szLanguage, LPCWSTR szNamespaces, eXmlMatch eMatch, LPCWSTR pProperty)
{
	HRESULT hr = S_OK;
	CComPtr<IXMLDOMDocument2> pXmlDoc;
	CComPtr<IXMLDOMNodeList> pNodeList;
	CComVariant filePath;
	VARIANT_BOOL isXmlSuccess;
	LONG nodeCount = 0;
	LONG maxMatches = 0;
	CComBSTR result(L"");
	CComBSTR delimiter(L"[~]");

	BreakExitOnNull(pFile, hr, E_INVALIDARG, "pFile is null");
	BreakExitOnNull(pExpression, hr, E_INVALIDARG, "pExpression is null");
	BreakExitOnNull(pProperty, hr, E_INVALIDARG, "pProperty is null");

	if (!FileExistsEx(pFile, nullptr))
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "File doesn't exist: '%ls'", pFile);
		ExitFunction();
	}

	// Create XML doc.
	hr = ::CoCreateInstance(CLSID_DOMDocument, nullptr, CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument, (void**)&pXmlDoc);
	BreakExitOnFailure(hr, "Failed to CoCreateInstance CLSID_DOMDocument");

	// Load XML document
	filePath = pFile;
	hr = pXmlDoc->load(filePath, &isXmlSuccess);
	BreakExitOnFailure(hr, "Failed to load XML");
	if (!isXmlSuccess)
	{
		hr = E_FAIL;
		BreakExitOnFailure(hr, "Failed to load XML");
	}

	// Set language.
	if (szLanguage && *szLanguage)
	{
		static const CComBSTR SelectionLanguage(L"SelectionLanguage");
		hr = pXmlDoc->setProperty(SelectionLanguage, CComVariant(szLanguage));
		BreakExitOnFailure(hr, "Failed setting SelectionLanguage");

		CComVariant varTmp;
		hr = pXmlDoc->getProperty(SelectionLanguage, &varTmp);
		BreakExitOnFailure(hr, "Failed getting namespaces");

		WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "SelectionLanguage is %ls", varTmp.bstrVal);
	}

	// Set namespaces.
	if (szNamespaces && *szNamespaces)
	{
		static const CComBSTR SelectionNamespaces(L"SelectionNamespaces");
		hr = pXmlDoc->setProperty(SelectionNamespaces, CComVariant(szNamespaces));
		BreakExitOnFailure(hr, "Failed setting namespaces");

		CComVariant varTmp;
		hr = pXmlDoc->getProperty(SelectionNamespaces, &varTmp);
		BreakExitOnFailure(hr, "Failed getting namespaces");

		WcaLog(LOGLEVEL::LOGMSG_VERBOSE, "SelectionNamespaces is %ls", varTmp.bstrVal);
	}

	// Execute Expression
	hr = pXmlDoc->selectNodes(CComBSTR(pExpression), &pNodeList);
	BreakExitOnFailure(hr, "Failed to select XML nodes");
	BreakExitOnNull(pNodeList, hr, E_FAIL, "selectNodes returned NULL");

	// Get match-count
	hr = pNodeList->get_length(&nodeCount);
	BreakExitOnFailure(hr, "Failed to get node count");

	// Validate with request match parameter
	switch (eMatch)
	{
	case first:
		maxMatches = min(1, nodeCount);
		break;
	case all:
		maxMatches = nodeCount;
		break;
	case enforceSingle:
		if (nodeCount != 1)
		{
			hr = E_INVALIDARG;
			BreakExitOnFailure(hr, "XmlSreach %i matches. Expected exactly one match", nodeCount);
		}
		maxMatches = 1;
		break;
	default:
		hr = E_INVALIDARG;
		BreakExitOnFailure(hr, "Wrong match parameter for XmlSreach.");
		break;
	}

	if (maxMatches == 0)
	{
		WcaLog(LOGLEVEL::LOGMSG_STANDARD, "No matches found");
		ExitFunction();
	}

	for (LONG i = 0; i < maxMatches; ++i)
	{
		CComPtr<IXMLDOMNode> pNode;
		CComVariant nodeValue;

		hr = pNodeList->get_item(i, &pNode);
		BreakExitOnFailure(hr, "Failed to get node.");
		BreakExitOnNull(pNode.p, hr, E_FAIL, "Failed to get node.");

		hr = pNode->get_nodeValue(&nodeValue);
		BreakExitOnFailure(hr, "Failed to get node's value.");

		hr = nodeValue.ChangeType(VT_BSTR);
		BreakExitOnFailure(hr, "Failed to get node's value as string.");

		// Add result
		hr = result.AppendBSTR(nodeValue.bstrVal);
		BreakExitOnFailure(hr, "Failed to append result.");

		// Add delimiter (unless this is the last)
		if (i < maxMatches - 1)
		{
			hr = result.AppendBSTR(delimiter);
			BreakExitOnFailure(hr, "Failed to append delimiter.");
		}
	}

	// Put in property
	hr = WcaSetProperty(pProperty, (LPWSTR)result);

LExit:
	return hr;
}
