#pragma once

#include <wininet.h>
#include <string>
#include <unordered_map>
#if !defined(_UNICODE) && !defined(UNICODE)
#define TSTRING std::string
#else
#define TSTRING std::wstring
#endif

class CAsyncClient
{
protected:
    struct SContext
    {
        CAsyncClient* pObj;
        DWORD dwContext;
    } m_context;

    HANDLE m_hConnectedEvent;
    HANDLE m_hRequestOpenedEvent;
    HANDLE m_hRequestCompleteEvent;

    HINTERNET m_hInstance;
    HINTERNET m_hConnect;
    HINTERNET m_hRequest;

    const std::unordered_map<TSTRING, TSTRING> m_cHeaders = {
           {TEXT("Accept:"),TEXT("*,*/*")},
           {TEXT("Accept-Language:"),TEXT("zh-cn")},
           {TEXT("User-Agent:"),TEXT("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/89.0.4389.72 Safari/537.36 Edg/89.0.774.45")},
           {TEXT("Content-Type:"),TEXT("application/x-www-form-urlencoded")},
           {TEXT("Accept-Encoding:"),TEXT("deflate")},
    };
public:

    CAsyncClient(void) {
        m_hConnectedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        m_hRequestOpenedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        m_hRequestCompleteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

        m_hInstance = NULL;
        m_hConnect = NULL;
        m_hRequest = NULL;
    }

    ~CAsyncClient(void) {
        if (m_hConnectedEvent)
        {
            CloseHandle(m_hConnectedEvent);
        }
        if (m_hRequestOpenedEvent)
        {
            CloseHandle(m_hRequestOpenedEvent);
        }
        if (m_hRequestCompleteEvent)
        {
            CloseHandle(m_hRequestCompleteEvent);
        }
        Close();
    }

public:

    typedef enum ContextHttpType
    {
        CONTEXT_CONNECT=0,
        CONTEXT_REQUESTHANDLE,
    }ContextHttpType;

    BOOL Connect(LPCTSTR lpszAddr,
        USHORT uPort = INTERNET_DEFAULT_HTTP_PORT,
        LPCTSTR lpszAgent = _T("AsyncClient"),
        DWORD dwTimeOut = 20000) {
        Close();

        ResetEvent(m_hConnectedEvent);
        ResetEvent(m_hRequestOpenedEvent);
        ResetEvent(m_hRequestCompleteEvent);

        if (!(m_hInstance = InternetOpen(lpszAgent,
            INTERNET_OPEN_TYPE_PRECONFIG,
            NULL,
            NULL,
            INTERNET_FLAG_ASYNC)))
        {
            return FALSE;
        }

        if (InternetSetStatusCallback(m_hInstance, (INTERNET_STATUS_CALLBACK)&Callback) == INTERNET_INVALID_STATUS_CALLBACK)
        {
            return FALSE;
        }

        m_context.dwContext = CONTEXT_CONNECT;
        m_context.pObj = this;

        m_hConnect = InternetConnect(m_hInstance,
            lpszAddr,
            uPort,
            NULL,
            NULL,
            INTERNET_SERVICE_HTTP,
            INTERNET_FLAG_KEEP_CONNECTION | INTERNET_FLAG_NO_CACHE_WRITE,
            (DWORD)&m_context);

        if (m_hConnect == NULL)
        {
            if (GetLastError() != ERROR_IO_PENDING)
            {
                return FALSE;
            }
            if (WaitForSingleObject(m_hConnectedEvent, dwTimeOut) == WAIT_TIMEOUT)
            {
                return FALSE;
            }
        }

        if (m_hConnect == NULL)
        {
            return FALSE;
        }

        return TRUE;
    }

    BOOL SendRequest(LPCTSTR szURI, LPCTSTR lpszReferrer, DWORD dwTimeOut = 20000, const std::unordered_map<TSTRING, TSTRING>& vHeaders = {}) {

        LPCTSTR szAcceptType = _T("*/*");


        m_context.dwContext = CONTEXT_REQUESTHANDLE;
        m_context.pObj = this;

        m_hRequest = HttpOpenRequest(m_hConnect,
            _T("GET"),
            szURI,
            NULL,
            lpszReferrer,
            NULL,
            INTERNET_FLAG_RELOAD | INTERNET_FLAG_KEEP_CONNECTION
            | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_FORMS_SUBMIT
            | INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS
            | INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP,
            (DWORD)&m_context);

        if (m_hRequest == NULL)
        {
            if (GetLastError() != ERROR_IO_PENDING)
            {
                return FALSE;
            }
            if (WaitForSingleObject(m_hRequestOpenedEvent, dwTimeOut) == WAIT_TIMEOUT)
            {
                return FALSE;
            }
        }


        if (m_hRequest == NULL)
            return FALSE;

        for (auto it : m_cHeaders)
        {
            auto f = vHeaders.find(it.first.c_str());
            if (f == vHeaders.end())
            {
                HttpAddRequestHeaders(m_hRequest, (it.first + it.second).data(), (it.first + it.second).size(), HTTP_ADDREQ_FLAG_ADD);
            }
            else
            {
                HttpAddRequestHeaders(m_hRequest, (f->first + f->second).data(), (f->first + f->second).size(), HTTP_ADDREQ_FLAG_ADD);
            }
        }
        for (auto it : vHeaders)
        {
            auto f = m_cHeaders.find(it.first.c_str());
            if (f == m_cHeaders.end())
            {
                HttpAddRequestHeaders(m_hRequest, (it.first + it.second).data(), (it.first + it.second).size(), HTTP_ADDREQ_FLAG_ADD);
            }
        }
        if (!HttpSendRequest(m_hRequest, NULL, 0, NULL, 0))
        {
            if (GetLastError() != ERROR_IO_PENDING)
            {
                return FALSE;
            }
        }

        if (WaitForSingleObject(m_hRequestCompleteEvent, dwTimeOut) == WAIT_TIMEOUT)
        {
            Close();
            return FALSE;
        }

        return TRUE;
    }

    DWORD Read(PBYTE pBuffer, DWORD dwSize, DWORD dwTimeOut = 20000) {
        INTERNET_BUFFERS InetBuff;

        FillMemory(&InetBuff, sizeof(InetBuff), 0);

        InetBuff.dwStructSize = sizeof(InetBuff);
        InetBuff.lpvBuffer = pBuffer;
        InetBuff.dwBufferLength = dwSize - 1;


        m_context.dwContext = CONTEXT_REQUESTHANDLE;
        m_context.pObj = this;

        if (!InternetReadFileEx(m_hRequest,
            &InetBuff,
            0,
            (DWORD)&m_context))
        {
            if (GetLastError() == ERROR_IO_PENDING)
            {
                if (WaitForSingleObject(m_hRequestCompleteEvent, dwTimeOut) == WAIT_TIMEOUT)
                {
                    return FALSE;
                }
            }
            else
            {
                return FALSE;
            }
        }

        return InetBuff.dwBufferLength;
    }
    DWORD Write(PBYTE pBuffer, DWORD dwSize, DWORD dwTimeOut = 20000) {
        INTERNET_BUFFERS InetBuff;

        FillMemory(&InetBuff, sizeof(InetBuff), 0);

        InetBuff.dwStructSize = sizeof(InetBuff);
        InetBuff.lpvBuffer = pBuffer;
        InetBuff.dwBufferLength = dwSize - 1;


        m_context.dwContext = CONTEXT_REQUESTHANDLE;
        m_context.pObj = this;

        if (!InternetReadFileEx(m_hRequest,
            &InetBuff,
            0,
            (DWORD)&m_context))
        {
            if (GetLastError() == ERROR_IO_PENDING)
            {
                if (WaitForSingleObject(m_hRequestCompleteEvent, dwTimeOut) == WAIT_TIMEOUT)
                {
                    return FALSE;
                }
            }
            else
            {
                return FALSE;
            }
        }

        return InetBuff.dwBufferLength;
    }
    void Close() {
        if (m_hInstance)
        {
            InternetCloseHandle(m_hInstance);
            m_hInstance = NULL;
        }

        if (m_hConnect)
        {
            InternetCloseHandle(m_hConnect);
            m_hConnect = NULL;
        }

        if (m_hRequest)
        {
            InternetCloseHandle(m_hRequest);
            m_hRequest = NULL;
        }
    }

    static void WINAPI Callback(HINTERNET hInternet,
        DWORD dwContext,
        DWORD dwInternetStatus,
        LPVOID lpStatusInfo,
        DWORD dwStatusInfoLen) {
        SContext* pContext = (SContext*)dwContext;

        switch (pContext->dwContext)
        {
        case CONTEXT_CONNECT:
            if (dwInternetStatus == INTERNET_STATUS_HANDLE_CREATED)
            {
                INTERNET_ASYNC_RESULT* pRes = (INTERNET_ASYNC_RESULT*)lpStatusInfo;
                pContext->pObj->m_hConnect = (HINTERNET)pRes->dwResult;
                SetEvent(pContext->pObj->m_hConnectedEvent);
            }
            break;

        case CONTEXT_REQUESTHANDLE: // Request handle
        {
            switch (dwInternetStatus)
            {
            case INTERNET_STATUS_HANDLE_CREATED:
            {
                INTERNET_ASYNC_RESULT* pRes = (INTERNET_ASYNC_RESULT*)lpStatusInfo;
                pContext->pObj->m_hRequest = (HINTERNET)pRes->dwResult;
                SetEvent(pContext->pObj->m_hRequestOpenedEvent);
            }
            break;

            case INTERNET_STATUS_REQUEST_SENT:
            {
                DWORD* lpBytesSent = (DWORD*)lpStatusInfo;
            }
            break;

            case INTERNET_STATUS_REQUEST_COMPLETE:
            {
                INTERNET_ASYNC_RESULT* pAsyncRes = (INTERNET_ASYNC_RESULT*)lpStatusInfo;
                SetEvent(pContext->pObj->m_hRequestCompleteEvent);
            }
            break;

            case INTERNET_STATUS_REDIRECT:
                //string strRealAddr = (LPSTR) lpStatusInfo;
                break;

            case INTERNET_STATUS_RECEIVING_RESPONSE:
                break;

            case INTERNET_STATUS_RESPONSE_RECEIVED:
            {
                DWORD* dwBytesReceived = (DWORD*)lpStatusInfo;
                //if (*dwBytesReceived == 0)
                //    bAllDone = TRUE;
            }
            }
        }
        }
    }
public:
    static int HttpGet(const TSTRING& tsHost, const TSTRING& tsPath)
    {
        CAsyncClient asyncClient = {};
        if (asyncClient.Connect(tsHost.c_str()))
        {
            if (asyncClient.SendRequest(tsPath.c_str(), _T("")))
            {
                std::string tsData = ("");
                CHAR szBuff[1024] = {};
                int nLen = 0;
                while ((nLen = asyncClient.Read((PBYTE)szBuff, 1024)) > 0)
                {
                    tsData.append(szBuff, nLen);
                }
                return (0);
            }
            else
            {
                return (-1);
            }
        }
        return (-1);
    }
};

#include <afxinet.h>
#define  IE_AGENT  TEXT("Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; .NET CLR 2.0.50727)")
#define  REQ_CODE_UTF8  TEXT("UTF-8")
#define  REQ_CODE_GBK   TEXT("GBK")

// 操作成功
#define SUCCESS        0
// 操作失败
#define FAILURE        1
// 操作超时
#define OUTTIME        2

#define  BUFFER_SIZE       1024

#define  NORMAL_CONNECT             INTERNET_FLAG_KEEP_CONNECTION
#define  SECURE_CONNECT             NORMAL_CONNECT | INTERNET_FLAG_SECURE
#define  NORMAL_REQUEST             INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE
#define  SECURE_REQUEST             NORMAL_REQUEST | INTERNET_FLAG_SECURE | \
                                    INTERNET_FLAG_IGNORE_CERT_CN_INVALID| \
                                    INTERNET_FLAG_IGNORE_CERT_DATE_INVALID 

class CHttpToolBak
{
public:
    CHttpToolBak(const CString& szReqCode = REQ_CODE_UTF8, LPCTSTR strAgent = IE_AGENT) {
        m_pSession = new CInternetSession(strAgent);
        m_szReqCode = szReqCode;
        m_pConnection = NULL;
        m_pFile = NULL;
    }
    virtual ~CHttpToolBak(void) {
        Clear();
        if (NULL != m_pSession)
        {
            m_pSession->Close();
            delete m_pSession;
            m_pSession = NULL;
        }
    }

    int HttpGet(LPCTSTR strUrl, CString& strResponse, const std::unordered_map<TSTRING, TSTRING>& vHeaders = {}) {
        return ExecuteRequest(TEXT("GET"), strUrl, NULL, strResponse, vHeaders, NULL);
    }
    int HttpGetFile(LPCTSTR strUrl, LPCTSTR lpFilePath, const std::unordered_map<TSTRING, TSTRING>& vHeaders = {}) {
        CString str = TEXT("");
        return ExecuteRequest(TEXT("GET"), strUrl, NULL, str, vHeaders, lpFilePath);
    }
    int HttpGetFileAsync(LPCTSTR strUrl, LPCTSTR lpFilePath, const std::unordered_map<TSTRING, TSTRING>& vHeaders = {}) {
        CString str = TEXT("");
        return ExecuteRequestAsync(TEXT("GET"), strUrl, NULL, str, vHeaders, lpFilePath);
    }
    int HttpPost(LPCTSTR strUrl, CString& strResponse, LPCTSTR strPostData, const std::unordered_map<TSTRING, TSTRING>& vHeaders = {}) {
        return ExecuteRequest(TEXT("POST"), strUrl, strPostData, strResponse, vHeaders, NULL);
    }

private:
    int ExecuteRequest(LPCTSTR strMethod, LPCTSTR strUrl, LPCTSTR strPostData, CString& strResponse, const std::unordered_map<TSTRING, TSTRING>& vHeaders = {}, LPCTSTR lpFilePath = NULL) {
        DWORD dwFlags = 0;
        CString strServer = TEXT("");
        CString strObject = TEXT("");
        DWORD dwServiceType = 0;
        INTERNET_PORT nPort = 0;
       
        strResponse = TEXT("");

        AfxParseURL(strUrl, dwServiceType, strServer, strObject, nPort);

        if (AFX_INET_SERVICE_HTTP != dwServiceType && AFX_INET_SERVICE_HTTPS != dwServiceType)
        {
            return FAILURE;
        }
        try
        {
            dwFlags = ((dwServiceType == AFX_INET_SERVICE_HTTP) ? NORMAL_CONNECT : SECURE_CONNECT);
            m_pConnection = m_pSession->GetHttpConnection(strServer, dwFlags, nPort);
            dwFlags = ((dwServiceType == AFX_INET_SERVICE_HTTP) ? NORMAL_REQUEST : SECURE_REQUEST);
            m_pFile = m_pConnection->OpenRequest(strMethod, strObject, NULL, 1, NULL, NULL, dwFlags | INTERNET_FLAG_RAW_DATA | INTERNET_FLAG_NO_AUTO_REDIRECT);
            if (AFX_INET_SERVICE_HTTPS == dwServiceType)
            {
                m_pFile->QueryOption(INTERNET_OPTION_SECURITY_FLAGS, dwFlags);
                dwFlags |= SECURITY_FLAG_IGNORE_UNKNOWN_CA;
                m_pFile->SetOption(INTERNET_OPTION_SECURITY_FLAGS, dwFlags);
            }
            for (auto it : m_cHeaders)
            {
                auto f = vHeaders.find(it.first.c_str());
                if (f == vHeaders.end())
                {
                    m_pFile->AddRequestHeaders((it.first + it.second).c_str());
                }
                else
                {
                    m_pFile->AddRequestHeaders((f->first + f->second).c_str());
                }
            }
            for (auto it : vHeaders)
            {
                auto f = m_cHeaders.find(it.first.c_str());
                if (f == m_cHeaders.end())
                {
                    m_pFile->AddRequestHeaders((it.first + it.second).c_str());
                }
            }

            if (strPostData != NULL) {
                CStringA strData = GetReqData(strPostData);
                m_pFile->SendRequest(NULL, 0, (LPVOID)(LPCSTR)strData, strlen(strData));
            }
            else {
                m_pFile->SendRequest();
            }
            DWORD dwStatus = 0;
            if (m_pFile->QueryInfoStatusCode(dwStatus))
            {
                switch (dwStatus)
                {
                case HTTP_STATUS_MOVED:
                case HTTP_STATUS_REDIRECT:
                {
                    int nLastPos = 0;
                    int nNextPos = 0;
                    CString strRedirectUrl = TEXT("");
                    CString strStartKey = TEXT("Location:");
                    CString strRedirectUrlKeyL0 = TEXT("https:");
                    CString strRedirectUrlKeyL1 = TEXT("http:");
                    CString strRedirectUrlKeyR = TEXT("\r\n");
                    if (m_pFile->QueryInfo(HTTP_QUERY_RAW_HEADERS_CRLF, strRedirectUrl, 0))
                    {
                        nNextPos = strRedirectUrl.Find(strStartKey);
                        if (nNextPos >= 0)
                        {
                            nLastPos = strRedirectUrl.Find(strRedirectUrlKeyL0, nNextPos);
                            if (nLastPos < 0)
                            {
                                nLastPos = strRedirectUrl.Find(strRedirectUrlKeyL1, nNextPos);
                            }
                            if (nLastPos >= 0)
                            {
                                nNextPos = strRedirectUrl.Find(strRedirectUrlKeyR, nLastPos);
                                if (nNextPos >= 0)
                                {
                                    strResponse = strRedirectUrl.Mid(nLastPos, nNextPos - nLastPos);
                                }
                            }
                        }
                    }
                }
                break;
                case HTTP_STATUS_OK:
                {
                    CHAR szChars[BUFFER_SIZE + 1] = { 0 };
                    CStringA strRawResponse = ("");
                    UINT nReaded = 0;

                    if (lpFilePath == NULL) {
                        while ((nReaded = m_pFile->Read((void*)szChars, BUFFER_SIZE)) > 0)
                        {
                            szChars[nReaded] = ('\0');
                            strRawResponse += szChars;
                            memset(szChars, 0, BUFFER_SIZE + 1);
                        }
                        strResponse = GetResData(strRawResponse);
                    }
                    else {
                        CFile file = {};
                        file.Open(lpFilePath, CFile::modeCreate | CFile::modeWrite | CFile::typeBinary);
                        while ((nReaded = m_pFile->Read((void*)szChars, BUFFER_SIZE)) > 0)
                        {
                            szChars[nReaded] = ('\0');
                            file.Write(szChars, BUFFER_SIZE);
                            memset(szChars, 0, BUFFER_SIZE + 1);
                        }
                        file.Close();
                    }
                }
                break;
                default:
                {
                }
                    break;
                }                
            }
            
            Clear();
        }
        catch (CInternetException* e)
        {
            Clear();
            e->Delete();
            switch (e->m_dwError)
            {
            case ERROR_INTERNET_TIMEOUT:
                return OUTTIME;
            default:
                return FAILURE;
            }
        }
        return SUCCESS;
    }

    int ExecuteRequestAsync(LPCTSTR strMethod, LPCTSTR strUrl, LPCTSTR strPostData, CString& strResponse, const std::unordered_map<TSTRING, TSTRING>& vHeaders = {}, LPCTSTR lpFilePath = NULL) {
        DWORD dwFlags = 0;
        CString strServer = TEXT("");
        CString strObject = TEXT("");
        DWORD dwServiceType = 0;
        INTERNET_PORT nPort = 0;
        CHttpFile* pAsyncHttpFile = NULL;
        CHttpConnection* pAsyncHttpConnection = NULL;
        CInternetSession asyncInternetSession(NULL, 1, 0, NULL, NULL, INTERNET_FLAG_ASYNC);
        strResponse = TEXT("");

        AfxParseURL(strUrl, dwServiceType, strServer, strObject, nPort);

        if (AFX_INET_SERVICE_HTTP != dwServiceType && AFX_INET_SERVICE_HTTPS != dwServiceType)
        {
            return FAILURE;
        }
        try
        {
            asyncInternetSession.EnableStatusCallback();
            dwFlags = ((dwServiceType == AFX_INET_SERVICE_HTTP) ? NORMAL_CONNECT : SECURE_CONNECT);
            pAsyncHttpConnection = asyncInternetSession.GetHttpConnection(strServer, dwFlags, nPort);
            dwFlags = ((dwServiceType == AFX_INET_SERVICE_HTTP) ? NORMAL_REQUEST : SECURE_REQUEST);
            pAsyncHttpFile = pAsyncHttpConnection->OpenRequest(strMethod, strObject, NULL, 1, NULL, NULL, dwFlags | INTERNET_FLAG_RAW_DATA | INTERNET_FLAG_NO_AUTO_REDIRECT);
            if (AFX_INET_SERVICE_HTTPS == dwServiceType)
            {
                pAsyncHttpFile->QueryOption(INTERNET_OPTION_SECURITY_FLAGS, dwFlags);
                dwFlags |= SECURITY_FLAG_IGNORE_UNKNOWN_CA;
                pAsyncHttpFile->SetOption(INTERNET_OPTION_SECURITY_FLAGS, dwFlags);
            }
            for (auto it : m_cHeaders)
            {
                auto f = vHeaders.find(it.first.c_str());
                if (f == vHeaders.end())
                {
                    pAsyncHttpFile->AddRequestHeaders((it.first + it.second).c_str());
                }
                else
                {
                    pAsyncHttpFile->AddRequestHeaders((f->first + f->second).c_str());
                }
            }
            for (auto it : vHeaders)
            {
                auto f = m_cHeaders.find(it.first.c_str());
                if (f == m_cHeaders.end())
                {
                    pAsyncHttpFile->AddRequestHeaders((it.first + it.second).c_str());
                }
            }

            if (strPostData != NULL) {
                CStringA strData = GetReqData(strPostData);
                pAsyncHttpFile->SendRequest(NULL, 0, (LPVOID)(LPCSTR)strData, strlen(strData));
            }
            else {
                m_pFile->SendRequest();
            }
            DWORD dwStatus = 0;
            if (pAsyncHttpFile->QueryInfoStatusCode(dwStatus))
            {
                switch (dwStatus)
                {
                case HTTP_STATUS_MOVED:
                case HTTP_STATUS_REDIRECT:
                {
                    int nLastPos = 0;
                    int nNextPos = 0;
                    CString strRedirectUrl = TEXT("");
                    CString strStartKey = TEXT("Location:");
                    CString strRedirectUrlKeyL0 = TEXT("https:");
                    CString strRedirectUrlKeyL1 = TEXT("http:");
                    CString strRedirectUrlKeyR = TEXT("\r\n");
                    if (pAsyncHttpFile->QueryInfo(HTTP_QUERY_RAW_HEADERS_CRLF, strRedirectUrl, 0))
                    {
                        nNextPos = strRedirectUrl.Find(strStartKey);
                        if (nNextPos >= 0)
                        {
                            nLastPos = strRedirectUrl.Find(strRedirectUrlKeyL0, nNextPos);
                            if (nLastPos < 0)
                            {
                                nLastPos = strRedirectUrl.Find(strRedirectUrlKeyL1, nNextPos);
                            }
                            if (nLastPos >= 0)
                            {
                                nNextPos = strRedirectUrl.Find(strRedirectUrlKeyR, nLastPos);
                                if (nNextPos >= 0)
                                {
                                    strResponse = strRedirectUrl.Mid(nLastPos, nNextPos - nLastPos);
                                }
                            }
                        }
                    }
                }
                break;
                case HTTP_STATUS_OK:
                {
                    CHAR szChars[BUFFER_SIZE + 1] = { 0 };
                    CStringA strRawResponse = ("");
                    UINT nReaded = 0;

                    if (lpFilePath == NULL) {
                        while ((nReaded = pAsyncHttpFile->Read((void*)szChars, BUFFER_SIZE)) > 0)
                        {
                            szChars[nReaded] = ('\0');
                            strRawResponse += szChars;
                            memset(szChars, 0, BUFFER_SIZE + 1);
                        }
                        strResponse = GetResData(strRawResponse);
                    }
                    else {
                        CFile file = {};
                        file.Open(lpFilePath, CFile::modeCreate | CFile::modeWrite | CFile::typeBinary);
                        while ((nReaded = pAsyncHttpFile->Read((void*)szChars, BUFFER_SIZE)) > 0)
                        {
                            szChars[nReaded] = ('\0');
                            file.Write(szChars, BUFFER_SIZE);
                            memset(szChars, 0, BUFFER_SIZE + 1);
                        }
                        file.Close();
                    }
                }
                break;
                default:
                {
                }
                break;
                }
            }

            Clear();
        }
        catch (CInternetException* e)
        {
            Clear();
            e->Delete();
            switch (e->m_dwError)
            {
            case ERROR_INTERNET_TIMEOUT:
                return OUTTIME;
            default:
                return FAILURE;
            }
        }
        return SUCCESS;
    }

    void Clear() {
        if (NULL != m_pFile)
        {
            m_pFile->Close();
            delete m_pFile;
            m_pFile = NULL;
        }

        if (NULL != m_pConnection)
        {
            m_pConnection->Close();
            delete m_pConnection;
            m_pConnection = NULL;
        }
    }
    CStringA GetReqData(const CString& szReqData) {
        // 预算-缓冲区中多字节的长度    
        int ansiiLen = WideCharToMultiByte(CP_UTF8, 0, szReqData, -1, NULL, 0, NULL, NULL);
        char* pAssii = (char*)malloc(sizeof(char) * ansiiLen);
        // 开始向缓冲区转换字节    
        WideCharToMultiByte(CP_UTF8, 0, szReqData, -1, pAssii, ansiiLen, NULL, NULL);
        CStringA szRet(pAssii);
        free(pAssii);
        return szRet;
    }
    CString  GetResData(const CStringA& szResData) {
        if (m_szReqCode == REQ_CODE_GBK)
            return CString(szResData);
        // 预算-缓冲区中宽字节的长度    
        int unicodeLen = MultiByteToWideChar(CP_UTF8, 0, szResData, -1, NULL, 0);
        wchar_t* pUnicode = (wchar_t*)malloc(sizeof(wchar_t) * unicodeLen);
        // 开始向缓冲区转换字节    
        MultiByteToWideChar(CP_UTF8, 0, szResData, -1, pUnicode, unicodeLen);
        CString szRet(pUnicode);
        free(pUnicode);
        return szRet;
    }
private:
    CHttpFile* m_pFile = NULL;
    CString m_szReqCode = TEXT("");
    CInternetSession* m_pSession = NULL;
    CHttpConnection* m_pConnection = NULL;
    const std::unordered_map<TSTRING, TSTRING> m_cHeaders = {
           {TEXT("Accept:"),TEXT("*,*/*")},
           {TEXT("Accept-Language:"),TEXT("zh-cn")},
           {TEXT("User-Agent:"),TEXT("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/89.0.4389.72 Safari/537.36 Edg/89.0.774.45")},
           {TEXT("Content-Type:"),TEXT("application/x-www-form-urlencoded")},
           {TEXT("Accept-Encoding:"),TEXT("deflate")},
    };
};

/*++
 Copyright (C) Microsoft.  All Rights Reserved.

--*/

#include <windows.h>
#include <wininet.h>
#include <stdio.h>

#define BUFFER_LEN  4096
#define ERR_MSG_LEN 512

typedef enum METHOD_TYPE {
    METHOD_NONE = 0,
    METHOD_GET,
    METHOD_POST,
}METHOD_TYPE;

typedef enum REQ_STATE_TYPE {
    REQ_STATE_SEND_REQ = 0,
    REQ_STATE_SEND_REQ_WITH_BODY,
    REQ_STATE_POST_GET_DATA,
    REQ_STATE_POST_SEND_DATA,
    REQ_STATE_POST_COMPLETE,
    REQ_STATE_RESPONSE_RECV_DATA,
    REQ_STATE_RESPONSE_WRITE_DATA,
    REQ_STATE_COMPLETE,
}REQ_STATE_TYPE;

#define DEFAULT_TIMEOUT 2 * 60 * 1000 // Two minutes

#define DEFAULT_HOSTNAME L"www.microsoft.com"
#define DEFAULT_RESOURCE L"/"

#define DEFAULT_OUTPUT_FILE_NAME L"response.htm"
#define SPIN_COUNT 4000

#pragma comment(lib, "wininet.lib")

#include <string>
#if !defined(_UNICODE) && !defined(UNICODE)
#define TSTRING std::string
#else
#define TSTRING std::wstring
#endif

#pragma warning(disable:4127) // Conditional expression is constant
//
// Structure to store configuration in that was gathered from
// passed in arguments
//
#define STRING_PTR(X) (X.empty() ? NULL : X.c_str())
class Configuration
{
public:
    DWORD AccessType = INTERNET_OPEN_TYPE_PRECONFIG; // Preconfig or Proxy (Use pre-configured options as default)
    METHOD_TYPE Method = METHOD_GET;           // Method, GET or POST
    std::wstring UserAgent = (L"WinInetHTTPAsync");              // User Agent to use
    INTERNET_PORT ServerPort = INTERNET_DEFAULT_HTTP_PORT;  // Port to connect to
    std::wstring HostName = (L"www.microsoft.com");              // Host to connect to
    std::wstring ResourceOnServer = (L"/");      // Resource to get from the server
    std::wstring InputFileName = (L"");         // File containing data to post
    std::wstring OutputFileName = (L"response.htm");        // File to write the data received from the server
    BOOL UseProxy = FALSE;                // Flag to indicate the use of a proxy
    std::wstring ProxyName = (L"");             // Name of the proxy to use
    std::wstring ProxyBypassName = (L"");             // Name of the proxy bypass to use
    BOOL IsSecureConnection = FALSE;      // Flag to indicate the use of SSL
    DWORD UserTimeout = 2 * 60 * 1000;            // Timeout for the async operations
    std::string PostField = ("");
    std::unordered_map<TSTRING, TSTRING> Headers = {};// Headers
};

//
// Structure used for storing the context for the asynchronous calls
//

class RequestContext {
public:
    LPVOID Instance = NULL;
    HINTERNET RequestHandle = NULL;
    HINTERNET ConnectHandle = NULL;
    HANDLE CompletionEvent = NULL;
    HANDLE CleanUpEvent = NULL;
    CHAR OutputBuffer[BUFFER_LEN] = { 0 };
    DWORD DownloadedBytes = 0;
    DWORD WrittenBytes = 0;
    DWORD ReadBytes = 0;
    HANDLE UploadFile = INVALID_HANDLE_VALUE;
    DWORD FileSize = 0;
    DWORD LeftSize = 0;
    HANDLE DownloadFile = INVALID_HANDLE_VALUE;
    METHOD_TYPE Method = METHOD_NONE;
    DWORD State = 0;

    CRITICAL_SECTION CriticalSection = { 0 };
    BOOL CritSecInitialized = FALSE;

    //
    // Synchronized by CriticalSection
    //

    DWORD HandleUsageCount = 0; // Request object is in use(not safe to close handle)
    BOOL Closing = FALSE;           // Request is closing(don't use handle)

    std::string RespData = ("");
};
typedef enum Encode { ANSI = 1, UTF16_LE, UTF16_BE, UTF8_BOM, UTF8 }Encode;
__inline static
Encode IsUtf8Data(const uint8_t* data, size_t size)
{
    bool bAnsi = true;
    uint8_t ch = 0x00;
    int32_t nBytes = 0;
    for (auto i = 0; i < size; i++)
    {
        ch = *(data + i);
        if ((ch & 0x80) != 0x00)
        {
            bAnsi = false;
        }
        if (nBytes == 0)
        {
            if (ch >= 0x80)
            {
                if (ch >= 0xFC && ch <= 0xFD)
                {
                    nBytes = 6;
                }
                else if (ch >= 0xF8)
                {
                    nBytes = 5;
                }
                else if (ch >= 0xF0)
                {
                    nBytes = 4;
                }
                else if (ch >= 0xE0)
                {
                    nBytes = 3;
                }
                else if (ch >= 0xC0)
                {
                    nBytes = 2;
                }
                else
                {
                    return Encode::ANSI;
                }
                nBytes--;
            }
        }
        else
        {
            if ((ch & 0xC0) != 0x80)
            {
                return Encode::ANSI;
            }
            nBytes--;
        }
    }
    if (nBytes > 0 || bAnsi)
    {
        return Encode::ANSI;
    }
    return Encode::UTF8;
}
__inline static
Encode DetectEncode(const uint8_t* data, size_t size)
{
    if (size > 2 && data[0] == 0xFF && data[1] == 0xFE)
    {
        return Encode::UTF16_LE;
    }
    else if (size > 2 && data[0] == 0xFE && data[1] == 0xFF)
    {
        return Encode::UTF16_BE;
    }
    else if (size > 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF)
    {
        return Encode::UTF8_BOM;
    }
    else
    {
        return IsUtf8Data(data, size);
    }
}
__inline static
const std::string& string_replace_all_after_pos(std::string& strData, const std::string& strDst, const std::string& strSrc, std::string::size_type stPos = 0)
{
    while ((stPos = strData.find(strSrc, stPos)) != std::string::npos)
    {
        strData.replace(stPos, strSrc.length(), strDst);
        stPos += strDst.length();
    }
    return strData;
}
__inline static
const std::string& string_replace_all(std::string& strData, const std::string& strDst, const std::string& strSrc)
{
    size_t pos = 0;
    while ((pos = strData.find(strSrc)) != std::string::npos)
    {
        strData.replace(pos, strSrc.length(), strDst);
    }
    return strData;
}
//通用版将wstring转化为string
__inline std::string WToA(const std::wstring& wstr, unsigned int codepage = CP_ACP)
{
    int nwstrlen = WideCharToMultiByte(codepage, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    if (nwstrlen > 0)
    {
        std::string str(nwstrlen, '\0');
        WideCharToMultiByte(codepage, 0, wstr.c_str(), -1, (LPSTR)str.c_str(), nwstrlen, NULL, NULL);
        return str;
    }
    return ("");
}
//通用版将string转化为wstring
__inline std::wstring AToW(const std::string& str, unsigned int codepage = CP_ACP)
{
    int nstrlen = MultiByteToWideChar(codepage, 0, str.c_str(), -1, NULL, 0);
    if (nstrlen > 0)
    {
        std::wstring wstr(nstrlen, L'\0');
        MultiByteToWideChar(codepage, 0, str.c_str(), -1, (LPWSTR)wstr.c_str(), nstrlen);
        return wstr;
    }
    return (L"");
}
__inline static
#if !defined(UNICODE) && !defined(_UNICODE)
std::string
#else
std::wstring
#endif
AToT(const std::string& str)
{
#if !defined(UNICODE) && !defined(_UNICODE)
    return str;
#else
    return AToW(str);
#endif
}
__inline static
#if !defined(UNICODE) && !defined(_UNICODE)
std::string
#else
std::wstring
#endif
WToT(const std::wstring& wstr)
{
#if !defined(UNICODE) && !defined(_UNICODE)
    return WToA(wstr);
#else
    return wstr;
#endif
}
__inline static std::string TToA(
    const
#if !defined(UNICODE) && !defined(_UNICODE)
    std::string
#else
    std::wstring
#endif
    & tsT)
{
#if !defined(UNICODE) && !defined(_UNICODE)
    return tsT;
#else
    return WToA(tsT);
#endif
}
__inline static std::wstring TToW(
    const
#if !defined(UNICODE) && !defined(_UNICODE)
    std::string
#else
    std::wstring
#endif
    & tsT)
{
#if !defined(UNICODE) && !defined(_UNICODE)
    return AToW(tsT);
#else
    return tsT;
#endif
}
#define WToUTF8(X) WToA(X, CP_UTF8)
#define UTF8ToW(X) AToW(X, CP_UTF8)
#define AToUTF8(X) WToUTF8(AToW(X))
#define UTF8ToA(X) WToA(UTF8ToW(X))
class CHttpTool {
public:
    int http_main(
        __in int argc,
        __in_ecount(argc) LPWSTR* argv
    )
    {
        DWORD Error = ERROR_SUCCESS;

        // Callback function
        INTERNET_STATUS_CALLBACK CallbackPointer = NULL;

        // Parse the command line arguments
        Error = ParseArguments(argc, argv);
        if (Error != ERROR_SUCCESS)
        {
            ShowUsage();
            goto Exit;
        }

        if (m_Configuration.UseProxy)
        {
            m_Configuration.AccessType = INTERNET_OPEN_TYPE_PROXY;
        }

        // Create Session handle and specify async Mode
        m_SessionHandle = InternetOpenW(STRING_PTR(m_Configuration.UserAgent),  // User Agent
            m_Configuration.AccessType,                      // Preconfig or Proxy
            STRING_PTR(m_Configuration.ProxyName),       // Proxy name
            STRING_PTR(m_Configuration.ProxyBypassName),                          // Proxy bypass, do not bypass any address
            INTERNET_FLAG_ASYNC);          // 0 for Synchronous

        if (m_SessionHandle == NULL)
        {
            LogInetError(GetLastError(), L"InternetOpen");
            goto Exit;
        }


        // Set the status callback for the handle to the Callback function
        CallbackPointer = InternetSetStatusCallbackW(m_SessionHandle, (INTERNET_STATUS_CALLBACK)InternetCallback);

        if (CallbackPointer == INTERNET_INVALID_STATUS_CALLBACK)
        {
            fprintf(stderr, "InternetSetStatusCallback failed with INTERNET_INVALID_STATUS_CALLBACK\n");
            goto Exit;
        }

        // Initialize the ReqContext to be used in the asynchronous calls
        Error = AllocateAndInitializeRequestContext();
        if (Error != ERROR_SUCCESS)
        {
            fprintf(stderr, "AllocateAndInitializeRequestContext failed with error %d\n", Error);
            goto Exit;
        }

        //
        // Send out request and receive response
        //

        ProcessRequest(ERROR_SUCCESS);

        //
        // Wait for request completion or timeout
        //

        WaitForRequestCompletion();

    Exit:

        // Clean up the allocated resources
        CleanUpRequestContext();

        CleanUpSessionHandle();

        return (Error != ERROR_SUCCESS) ? 1 : 0;
    }

    int http_get_file(const std::string& file, const std::string& url)
    {
        DWORD Error = ERROR_SUCCESS;

        // Callback function
        INTERNET_STATUS_CALLBACK CallbackPointer = NULL;

        {
            std::wstring& wUrl = AToW(url);
            int nNextPos = 0;
            int nLastPos = 0;
            std::wstring wstrHttp = L"http://";
            std::wstring wstrHttps = L"https://";
            if (_wcsnicmp(wUrl.c_str(), wstrHttps.c_str(), wstrHttps.length()) == 0)
            {
                m_Configuration.IsSecureConnection = TRUE;
                m_Configuration.ServerPort = INTERNET_DEFAULT_HTTPS_PORT;
                nNextPos = wstrHttps.length();
            }
            else if (_wcsnicmp(wUrl.c_str(), wstrHttp.c_str(), wstrHttp.length()) == 0)
            {
                m_Configuration.IsSecureConnection = FALSE;
                m_Configuration.ServerPort = INTERNET_DEFAULT_HTTP_PORT;
                nNextPos = wstrHttp.length();
            }
            else
            {
                // not support
            }
            std::wstring colon = L":";
            std::wstring sprit = L"/";
            nLastPos = wUrl.find(colon, nNextPos);
            if (nLastPos == std::string::npos)
            {
                nLastPos = wUrl.find(sprit, nNextPos);
                if (nLastPos == std::string::npos)
                {
                    m_Configuration.HostName = wUrl.substr(nNextPos);
                    m_Configuration.ResourceOnServer = sprit;
                }
                else
                {
                    m_Configuration.HostName = wUrl.substr(nNextPos, nLastPos - nNextPos);
                    m_Configuration.ResourceOnServer = wUrl.substr(nLastPos);
                }
            }
            else
            {
                m_Configuration.HostName = wUrl.substr(nNextPos, nLastPos - nNextPos);
                nNextPos = nLastPos + colon.length();
                nLastPos = wUrl.find(sprit, nNextPos);
                if (nLastPos == std::string::npos)
                {
                    m_Configuration.ServerPort = std::stoi(wUrl.substr(nNextPos));
                    m_Configuration.ResourceOnServer = sprit;
                }
                else
                {
                    m_Configuration.ServerPort = std::stoi(wUrl.substr(nNextPos, nLastPos - nNextPos));
                    m_Configuration.ResourceOnServer = wUrl.substr(nLastPos);
                }
            }
        }

        m_Configuration.OutputFileName = AToW(file);

        if (m_Configuration.UseProxy)
        {
            m_Configuration.AccessType = INTERNET_OPEN_TYPE_PROXY;
        }

        // Create Session handle and specify async Mode
        m_SessionHandle = InternetOpenW(STRING_PTR(m_Configuration.UserAgent),  // User Agent
            m_Configuration.AccessType,                      // Preconfig or Proxy
            STRING_PTR(m_Configuration.ProxyName),       // Proxy name
            STRING_PTR(m_Configuration.ProxyBypassName),                          // Proxy bypass, do not bypass any address
            INTERNET_FLAG_ASYNC);          // 0 for Synchronous

        if (m_SessionHandle == NULL)
        {
            LogInetError(GetLastError(), L"InternetOpen");
            goto Exit;
        }

        // Set the status callback for the handle to the Callback function
        CallbackPointer = InternetSetStatusCallbackW(m_SessionHandle, (INTERNET_STATUS_CALLBACK)InternetCallback);

        if (CallbackPointer == INTERNET_INVALID_STATUS_CALLBACK)
        {
            fprintf(stderr, "InternetSetStatusCallback failed with INTERNET_INVALID_STATUS_CALLBACK\n");
            goto Exit;
        }

        // Initialize the ReqContext to be used in the asynchronous calls
        Error = AllocateAndInitializeRequestContext();
        if (Error != ERROR_SUCCESS)
        {
            fprintf(stderr, "AllocateAndInitializeRequestContext failed with error %d\n", Error);
            goto Exit;
        }

        //
        // Send out request and receive response
        //

        ProcessRequest(ERROR_SUCCESS);

        //
        // Wait for request completion or timeout
        //

        WaitForRequestCompletion();

    Exit:

        // Clean up the allocated resources
        CleanUpRequestContext();

        CleanUpSessionHandle();

        return (Error != ERROR_SUCCESS) ? 1 : 0;
    }

    int http_get(std::string& resp, const std::string& url)
    {
        DWORD Error = ERROR_SUCCESS;

        // Callback function
        INTERNET_STATUS_CALLBACK CallbackPointer = NULL;

        {
            std::wstring& wUrl = AToW(url);
            int nNextPos = 0;
            int nLastPos = 0;
            std::wstring wstrHttp = L"http://";
            std::wstring wstrHttps = L"https://";
            if (_wcsnicmp(wUrl.c_str(), wstrHttps.c_str(), wstrHttps.length()) == 0)
            {
                m_Configuration.IsSecureConnection = TRUE;
                m_Configuration.ServerPort = INTERNET_DEFAULT_HTTPS_PORT;
                nNextPos = wstrHttps.length();
            }
            else if (_wcsnicmp(wUrl.c_str(), wstrHttp.c_str(), wstrHttp.length()) == 0)
            {
                m_Configuration.IsSecureConnection = FALSE;
                m_Configuration.ServerPort = INTERNET_DEFAULT_HTTP_PORT;
                nNextPos = wstrHttp.length();
            }
            else
            {
                // not support
            }
            std::wstring colon = L":";
            std::wstring sprit = L"/";
            nLastPos = wUrl.find(colon, nNextPos);
            if (nLastPos == std::string::npos)
            {
                nLastPos = wUrl.find(sprit, nNextPos);
                if (nLastPos == std::string::npos)
                {
                    m_Configuration.HostName = wUrl.substr(nNextPos);
                    m_Configuration.ResourceOnServer = sprit;
                }
                else
                {
                    m_Configuration.HostName = wUrl.substr(nNextPos, nLastPos - nNextPos);
                    m_Configuration.ResourceOnServer = wUrl.substr(nLastPos);
                }
            }
            else
            {
                m_Configuration.HostName = wUrl.substr(nNextPos, nLastPos - nNextPos);
                nNextPos = nLastPos + colon.length();
                nLastPos = wUrl.find(sprit, nNextPos);
                if (nLastPos == std::string::npos)
                {
                    m_Configuration.ServerPort = std::stoi(wUrl.substr(nNextPos));
                    m_Configuration.ResourceOnServer = sprit;
                }
                else
                {
                    m_Configuration.ServerPort = std::stoi(wUrl.substr(nNextPos, nLastPos - nNextPos));
                    m_Configuration.ResourceOnServer = wUrl.substr(nLastPos);
                }
            }
        }

        m_Configuration.OutputFileName = (L"");

        if (m_Configuration.UseProxy)
        {
            m_Configuration.AccessType = INTERNET_OPEN_TYPE_PROXY;
        }

        // Create Session handle and specify async Mode
        m_SessionHandle = InternetOpenW(STRING_PTR(m_Configuration.UserAgent),  // User Agent
            m_Configuration.AccessType,                      // Preconfig or Proxy
            STRING_PTR(m_Configuration.ProxyName),       // Proxy name
            STRING_PTR(m_Configuration.ProxyBypassName),                          // Proxy bypass, do not bypass any address
            INTERNET_FLAG_ASYNC);          // 0 for Synchronous

        if (m_SessionHandle == NULL)
        {
            LogInetError(GetLastError(), L"InternetOpen");
            goto Exit;
        }

        // Set the status callback for the handle to the Callback function
        CallbackPointer = InternetSetStatusCallbackW(m_SessionHandle, (INTERNET_STATUS_CALLBACK)InternetCallback);

        if (CallbackPointer == INTERNET_INVALID_STATUS_CALLBACK)
        {
            fprintf(stderr, "InternetSetStatusCallback failed with INTERNET_INVALID_STATUS_CALLBACK\n");
            goto Exit;
        }

        // Initialize the ReqContext to be used in the asynchronous calls
        Error = AllocateAndInitializeRequestContext();
        if (Error != ERROR_SUCCESS)
        {
            fprintf(stderr, "AllocateAndInitializeRequestContext failed with error %d\n", Error);
            goto Exit;
        }

        //
        // Send out request and receive response
        //

        ProcessRequest(ERROR_SUCCESS);

        //
        // Wait for request completion or timeout
        //

        WaitForRequestCompletion();

        if (!m_ReqContext.RespData.empty())
        {
            resp.assign(m_ReqContext.RespData.data(), m_ReqContext.RespData.size());
            switch (DetectEncode((const uint8_t*)resp.data(), resp.size()))
            {
            case ANSI:
                break;
            case UTF16_LE:
            case UTF16_BE:
                resp.erase(resp.begin());
                resp.erase(resp.begin());
                resp = WToA(std::wstring((const wchar_t*)resp.data(), resp.length() / sizeof(wchar_t)));
                break;
            case UTF8_BOM:
                resp.erase(resp.begin());
                resp.erase(resp.begin());
                resp.erase(resp.begin());
            case UTF8:
                resp = WToA(UTF8ToW(resp));
                break;
            default:
                break;
            }
            string_replace_all(resp, "", "\x20");
            string_replace_all(resp, "", "\x09");
            string_replace_all(resp, "", "\x0D\x0A");
        }
    Exit:

        // Clean up the allocated resources
        CleanUpRequestContext();

        CleanUpSessionHandle();

        return (Error != ERROR_SUCCESS) ? 1 : 0;
    }

    int http_post(std::string& resp, const std::string& url, const std::string& postfield, const std::unordered_map<TSTRING, TSTRING>& headers)
    {
        DWORD Error = ERROR_SUCCESS;

        // Callback function
        INTERNET_STATUS_CALLBACK CallbackPointer = NULL;

        {
            std::wstring& wUrl = AToW(url);
            int nNextPos = 0;
            int nLastPos = 0;
            std::wstring wstrHttp = L"http://";
            std::wstring wstrHttps = L"https://";
            if (_wcsnicmp(wUrl.c_str(), wstrHttps.c_str(), wstrHttps.length()) == 0)
            {
                m_Configuration.IsSecureConnection = TRUE;
                m_Configuration.ServerPort = INTERNET_DEFAULT_HTTPS_PORT;
                nNextPos = wstrHttps.length();
            }
            else if (_wcsnicmp(wUrl.c_str(), wstrHttp.c_str(), wstrHttp.length()) == 0)
            {
                m_Configuration.IsSecureConnection = FALSE;
                m_Configuration.ServerPort = INTERNET_DEFAULT_HTTP_PORT;
                nNextPos = wstrHttp.length();
            }
            else
            {
                // not support
            }
            std::wstring colon = L":";
            std::wstring sprit = L"/";
            nLastPos = wUrl.find(colon, nNextPos);
            if (nLastPos == std::string::npos)
            {
                nLastPos = wUrl.find(sprit, nNextPos);
                if (nLastPos == std::string::npos)
                {
                    m_Configuration.HostName = wUrl.substr(nNextPos);
                    m_Configuration.ResourceOnServer = sprit;
                }
                else
                {
                    m_Configuration.HostName = wUrl.substr(nNextPos, nLastPos - nNextPos);
                    m_Configuration.ResourceOnServer = wUrl.substr(nLastPos);
                }
            }
            else
            {
                m_Configuration.HostName = wUrl.substr(nNextPos, nLastPos - nNextPos);
                nNextPos = nLastPos + colon.length();
                nLastPos = wUrl.find(sprit, nNextPos);
                if (nLastPos == std::string::npos)
                {
                    m_Configuration.ServerPort = std::stoi(wUrl.substr(nNextPos));
                    m_Configuration.ResourceOnServer = sprit;
                }
                else
                {
                    m_Configuration.ServerPort = std::stoi(wUrl.substr(nNextPos, nLastPos - nNextPos));
                    m_Configuration.ResourceOnServer = wUrl.substr(nLastPos);
                }
            }
        }
        m_Configuration.Method = METHOD_POST;
        m_Configuration.PostField = postfield;
        m_Configuration.OutputFileName = (L"");
        m_Configuration.Headers.insert(headers.begin(), headers.end());

        if (m_Configuration.UseProxy)
        {
            m_Configuration.AccessType = INTERNET_OPEN_TYPE_PROXY;
        }

        // Create Session handle and specify async Mode
        m_SessionHandle = InternetOpenW(STRING_PTR(m_Configuration.UserAgent),  // User Agent
            m_Configuration.AccessType,                      // Preconfig or Proxy
            STRING_PTR(m_Configuration.ProxyName),       // Proxy name
            STRING_PTR(m_Configuration.ProxyBypassName),                          // Proxy bypass, do not bypass any address
            INTERNET_FLAG_ASYNC);          // 0 for Synchronous

        if (m_SessionHandle == NULL)
        {
            LogInetError(GetLastError(), L"InternetOpen");
            goto Exit;
        }

        // Set the status callback for the handle to the Callback function
        CallbackPointer = InternetSetStatusCallbackW(m_SessionHandle, (INTERNET_STATUS_CALLBACK)InternetCallback);

        if (CallbackPointer == INTERNET_INVALID_STATUS_CALLBACK)
        {
            fprintf(stderr, "InternetSetStatusCallback failed with INTERNET_INVALID_STATUS_CALLBACK\n");
            goto Exit;
        }

        // Initialize the ReqContext to be used in the asynchronous calls
        Error = AllocateAndInitializeRequestContext();
        if (Error != ERROR_SUCCESS)
        {
            fprintf(stderr, "AllocateAndInitializeRequestContext failed with error %d\n", Error);
            goto Exit;
        }

        //
        // Send out request and receive response
        //

        ProcessRequest(ERROR_SUCCESS);

        //
        // Wait for request completion or timeout
        //

        WaitForRequestCompletion();

        if (!m_ReqContext.RespData.empty())
        {
            resp.assign(m_ReqContext.RespData.data(), m_ReqContext.RespData.size());
            switch (DetectEncode((const uint8_t*)resp.data(), resp.size()))
            {
            case ANSI:
                break;
            case UTF16_LE:
            case UTF16_BE:
                resp.erase(resp.begin());
                resp.erase(resp.begin());
                resp = WToA(std::wstring((const wchar_t*)resp.data(), resp.length() / sizeof(wchar_t)));
                break;
            case UTF8_BOM:
                resp.erase(resp.begin());
                resp.erase(resp.begin());
                resp.erase(resp.begin());
            case UTF8:
                resp = WToA(UTF8ToW(resp));
                break;
            default:
                break;
            }
            string_replace_all(resp, "", "\x20");
            string_replace_all(resp, "", "\x09");
            string_replace_all(resp, "", "\x0D\x0A");
        }
    Exit:

        // Clean up the allocated resources
        CleanUpRequestContext();

        CleanUpSessionHandle();

        return (Error != ERROR_SUCCESS) ? 1 : 0;
    }

private:
    DWORD ParseArguments(
        __in int argc,
        __in_ecount(argc) LPWSTR* argv
    )
    /*++

    Routine Description:
         This routine is used to Parse command line arguments. Flags are
         case sensitive.

    Arguments:
         argc - Number of arguments
         argv - Pointer to the argument vector
         Configuration - pointer to configuration struct to write configuration

    Return Value:
        Error Code for the operation.

    --*/
    {
        int i;
        DWORD Error = ERROR_SUCCESS;

        for (i = 1; i < argc; ++i)
        {
            if (wcsncmp(argv[i], L"-", 1))
            {
                printf("Invalid switch %ws\n", argv[i]);
                i++;
                continue;
            }

            switch (argv[i][1])
            {
            case L'p':

                m_Configuration.UseProxy = 1;
                if (i < argc - 1)
                {
                    m_Configuration.ProxyName = argv[++i];
                }
                break;

            case L'h':

                if (i < argc - 1)
                {
                    m_Configuration.HostName = argv[++i];
                }

                break;

            case L'o':

                if (i < argc - 1)
                {
                    m_Configuration.ResourceOnServer = argv[++i];
                }

                break;

            case L'r':

                if (i < argc - 1)
                {
                    m_Configuration.InputFileName = argv[++i];
                }

                break;

            case L'w':

                if (i < argc - 1)
                {
                    m_Configuration.OutputFileName = argv[++i];
                }

                break;

            case L'm':

                if (i < argc - 1)
                {
                    if (!_wcsnicmp(argv[i + 1], L"get", 3))
                    {
                        m_Configuration.Method = METHOD_GET;
                    }
                    else if (!_wcsnicmp(argv[i + 1], L"post", 4))
                    {
                        m_Configuration.Method = METHOD_POST;
                    }
                }
                ++i;
                break;

            case L's':
                m_Configuration.IsSecureConnection = TRUE;
                break;

            case L't':
                if (i < argc - 1)
                {
                    m_Configuration.UserTimeout = _wtoi(argv[++i]);
                }
                break;

            default:
                Error = ERROR_INVALID_PARAMETER;
                break;
            }
        }

        if (Error == ERROR_SUCCESS)
        {
            if (m_Configuration.UseProxy && m_Configuration.ProxyName.empty())
            {
                printf("No proxy server name provided!\n\n");
                Error = ERROR_INVALID_PARAMETER;
                goto Exit;
            }

            if (m_Configuration.HostName.empty())
            {
                printf("Defaulting hostname to: %ws\n", DEFAULT_HOSTNAME);
                m_Configuration.HostName = DEFAULT_HOSTNAME;
            }

            if (m_Configuration.Method == METHOD_NONE)
            {
                printf("Defaulting method to: GET\n");
                m_Configuration.Method = METHOD_GET;
            }

            if (m_Configuration.ResourceOnServer.empty())
            {
                printf("Defaulting resource to: %ws\n", DEFAULT_RESOURCE);
                m_Configuration.ResourceOnServer = DEFAULT_RESOURCE;
            }

            if (m_Configuration.UserTimeout == 0)
            {
                printf("Defaulting timeout to: %d\n", DEFAULT_TIMEOUT);
                m_Configuration.UserTimeout = DEFAULT_TIMEOUT;
            }

            if (m_Configuration.InputFileName.empty() && m_Configuration.Method == METHOD_POST)
            {
                printf("Error: File to post not specified\n");
                Error = ERROR_INVALID_PARAMETER;
                goto Exit;
            }

            if (m_Configuration.OutputFileName.empty())
            {
                printf("Defaulting output file to: %ws\n", DEFAULT_OUTPUT_FILE_NAME);

                m_Configuration.OutputFileName = DEFAULT_OUTPUT_FILE_NAME;
            }

        }

    Exit:
        return Error;
    }


    VOID
    ShowUsage(
        VOID
    )
    /*++

    Routine Description:
          Shows the usage of the application.

    Arguments:
          None.

    Return Value:
          None.

    --*/
    {
        printf("Usage: async [-m {GET|POST}] [-h <hostname>] [-o <resourcename>] [-s] ");
        printf("[-p <proxyname>] [-w <output filename>] [-r <file to post>] [-t <userTimeout>]\n");
        printf("Option Semantics: \n");
        printf("-m : Specify method (Default: \"GET\")\n");
        printf("-h : Specify hostname (Default: \"%ws\"\n", DEFAULT_HOSTNAME);
        printf("-o : Specify resource name on the server (Default: \"%ws\")\n", DEFAULT_RESOURCE);
        printf("-s : Use secure connection - https\n");
        printf("-p : Specify proxy\n");
        printf("-w : Specify file to write output to (Default: \"%ws\")\n", DEFAULT_OUTPUT_FILE_NAME);
        printf("-r : Specify file to post data from\n");
        printf("-t : Specify time to wait in ms for operation completion (Default: %d)\n", DEFAULT_TIMEOUT);

        return;
    }

    VOID
    LogInetError(
        __in DWORD Err,
        __in LPCWSTR Str
    )
    /*++

    Routine Description:
         This routine is used to log WinInet errors in human readable form.

    Arguments:
         Err - Error number obtained from GetLastError()
         Str - String pointer holding caller-context information

    Return Value:
        None.

    --*/
    {
        DWORD Result;
        PWSTR MsgBuffer = NULL;

        Result = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_HMODULE,
            GetModuleHandleW(L"wininet.dll"),
            Err,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPWSTR)&MsgBuffer,
            ERR_MSG_LEN,
            NULL);

        if (Result)
        {
            fprintf(stderr, "%ws: %ws\n", Str, MsgBuffer);
            LocalFree(MsgBuffer);
        }
        else
        {
            fprintf(stderr,
                "Error %d while formatting message for %d in %ws\n",
                GetLastError(),
                Err,
                Str);
        }

        return;
    }

    VOID
    LogSysError(
        __in DWORD Err,
        __in LPCWSTR Str
    )
    /*++

    Routine Description:
         This routine is used to log System Errors in human readable form.

    Arguments:
         Err - Error number obtained from GetLastError()
         Str - String pointer holding caller-context information

    Return Value:
        None.

    --*/
    {
        DWORD Result;
        PWSTR MsgBuffer = NULL;

        Result = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            Err,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPWSTR)&MsgBuffer,
            ERR_MSG_LEN,
            NULL);

        if (Result)
        {
            fprintf(stderr,
                "%ws: %ws\n",
                Str,
                MsgBuffer);
            LocalFree(MsgBuffer);
        }
        else
        {
            fprintf(stderr,
                "Error %d while formatting message for %d in %ws\n",
                GetLastError(),
                Err,
                Str);
        }

        return;
    }

    BOOL
    AcquireRequestHandle()
    /*++

    Routine Description:
        Acquire use of the request handle to make a wininet call
    Arguments:
        ReqContext - Pointer to Request context structure
    Return Value:
        TRUE - Success
        FALSE - Failure
    --*/
    {
        BOOL Success = TRUE;

        EnterCriticalSection(&m_ReqContext.CriticalSection);

        if (m_ReqContext.Closing == TRUE)
        {
            Success = FALSE;
        }
        else
        {
            m_ReqContext.HandleUsageCount++;
        }

        LeaveCriticalSection(&m_ReqContext.CriticalSection);

        return Success;
    }


    VOID
    ReleaseRequestHandle()
    /*++

    Routine Description:
        release use of the request handle
    Arguments:
        ReqContext - Pointer to Request context structure
    Return Value:
        None.

    --*/
    {
        BOOL Close = FALSE;

        EnterCriticalSection(&m_ReqContext.CriticalSection);

        m_ReqContext.HandleUsageCount--;

        if (m_ReqContext.Closing == TRUE && m_ReqContext.HandleUsageCount == 0)
        {
            Close = TRUE;

        }

        LeaveCriticalSection(&m_ReqContext.CriticalSection);


        if (Close)
        {
            //
            // At this point there must be the guarantee that all calls
            // to wininet with this handle have returned with some value
            // including ERROR_IO_PENDING, and none will be made after
            // InternetCloseHandle.
            //        
            (VOID)InternetCloseHandle(m_ReqContext.RequestHandle);
        }

        return;
    }

    DWORD
    CreateWininetHandles(
        /*__in const std::wstring& HostName,
        __in const std::wstring& Resource,
        __in BOOL IsSecureConnection*/
    )
    /*++

    Routine Description:
        Create connect and request handles

    Arguments:
        ReqContext - Pointer to Request context structure
        SessionHandle - Wininet session handle used to create
                        connect handle
        HostName - Hostname to connect
        Resource - Resource to get/post
        IsSecureConnection - SSL?

    Return Value:
        Error Code for the operation.

    --*/
    {
        DWORD Error = ERROR_SUCCESS;
        DWORD RequestFlags = 0;
        LPWSTR Verb;


        //
        // Set the correct server port if using SSL
        // Also set the flag for HttpOpenRequest 
        //

        if (m_Configuration.IsSecureConnection)
        {
            RequestFlags = INTERNET_FLAG_SECURE;
        }

        // Create Connection handle and provide context for async operations
        m_ReqContext.ConnectHandle = InternetConnectW(m_SessionHandle,
            STRING_PTR(m_Configuration.HostName),                  // Name of the server to connect to
            m_Configuration.ServerPort,                // HTTP (80) or HTTPS (443)
            NULL,                      // Do not provide a user name for the server
            NULL,                      // Do not provide a password for the server
            INTERNET_SERVICE_HTTP,
            0,                         // Do not provide any special flag
            (DWORD_PTR)&m_ReqContext);    // Provide the context to be
                                       // used during the callbacks
    //                                                                        
    // For HTTP InternetConnect returns synchronously because it does not
    // actually make the connection.
    //
    // For FTP InternetConnect connects the control channel, and therefore
    // can be completed asynchronously.  This sample would have to be
    // changed, so that the InternetConnect's asynchronous completion
    // is handled correctly to support FTP.
    //

        if (m_ReqContext.ConnectHandle == NULL)
        {
            Error = GetLastError();
            LogInetError(Error, L"InternetConnect");
            goto Exit;
        }


        // Set the Verb depending on the operation to perform
        if (m_ReqContext.Method == METHOD_GET)
        {
            Verb = L"GET";
        }
        else
        {
            Verb = L"POST";
        }

        //
        // We're overriding WinInet's default behavior.
        // Setting these flags, we make sure we get the response from the server and not the cache.
        // Also ask WinInet not to store the response in the cache.
        //
        // These flags are NOT performant and are only used to show case WinInet's Async I/O.
        // A real WinInet application would not want to use this flags.
        //

        RequestFlags |= INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE;

        // Create a Request handle
        m_ReqContext.RequestHandle = HttpOpenRequestW(m_ReqContext.ConnectHandle,
            Verb,                     // GET or POST
            STRING_PTR(m_Configuration.ResourceOnServer),                 // root "/" by default
            NULL,                     // Use default HTTP/1.1 as the version
            NULL,                     // Do not provide any referrer
            NULL,                     // Do not provide Accept types
            RequestFlags,
            (DWORD_PTR)&m_ReqContext);
        if (m_ReqContext.RequestHandle == NULL)
        {
            Error = GetLastError();
            LogInetError(Error, L"HttpOpenRequest");

            goto Exit;
        }


        for (auto it : m_cHeaders)
        {
            std::wstring wstr = (L"");
            auto f = m_Configuration.Headers.find(it.first.c_str());
            if (f == m_Configuration.Headers.end())
            {
                wstr = TToW(it.first + it.second);
            }
            else
            {
                wstr = TToW(f->first + f->second);
            }
            HttpAddRequestHeadersW(m_ReqContext.RequestHandle, wstr.data(), wstr.size(), HTTP_ADDREQ_FLAG_ADD);
        }
        for (auto it : m_Configuration.Headers)
        {
            std::wstring wstr = (L"");
            auto f = m_cHeaders.find(it.first.c_str());
            if (f == m_cHeaders.end())
            {
                wstr = TToW(it.first + it.second);
                HttpAddRequestHeadersW(m_ReqContext.RequestHandle, wstr.data(), wstr.size(), HTTP_ADDREQ_FLAG_ADD);
            }
        }

    Exit:

        return Error;
    }

    VOID
    WaitForRequestCompletion()
    /*++

    Routine Description:
        Wait for the request to complete or timeout to occur

    Arguments:
        ReqContext - Pointer to request context structure

    Return Value:
        None.

    --*/
    {
        DWORD SyncResult;

        //
        // The preferred method of doing timeouts is to
        // use the timeout options through InternetSetOption,
        // but this overall timeout is being used to show 
        // the correct way to abort and close a request.
        //

        SyncResult = WaitForSingleObject(m_ReqContext.CompletionEvent, m_Configuration.UserTimeout);              // Wait until we receive the completion

        switch (SyncResult)
        {
        case WAIT_OBJECT_0:

            printf("Done!\n");
            break;

        case WAIT_TIMEOUT:

            fprintf(stderr,
                "Timeout while waiting for completion event (request will be cancelled)\n");
            break;

        case WAIT_FAILED:

            fprintf(stderr,
                "Wait failed with Error %d while waiting for completion event (request will be cancelled)\n",
                GetLastError());
            break;

        default:
            // Not expecting any other error codes
            break;


        }

        return;
    }

    VOID
    CleanUpRequestContext()
    /*++

    Routine Description:
        Used to cleanup the request context before exiting.

    Arguments:
        ReqContext - Pointer to request context structure

    Return Value:
        None.

    --*/
    {
        if (m_ReqContext.RequestHandle)
        {
            CloseRequestHandle();

            //
            // Wait for the closing of the handle to complete
            // (waiting for all async operations to complete)
            //
            // This is the only safe way to get rid of the context
            //

            (VOID)WaitForSingleObject(m_ReqContext.CleanUpEvent, INFINITE);
        }

        if (m_ReqContext.ConnectHandle)
        {
            //
            // Remove the callback from the ConnectHandle since
            // we don't want the closing notification
            // The callback was inherited from the session handle
            //
            (VOID)InternetSetStatusCallbackW(m_ReqContext.ConnectHandle, NULL);

            (VOID)InternetCloseHandle(m_ReqContext.ConnectHandle);

            m_ReqContext.ConnectHandle = NULL;
        }

        if (m_ReqContext.UploadFile != INVALID_HANDLE_VALUE)
        {
            CloseHandle(m_ReqContext.UploadFile);
            m_ReqContext.UploadFile = INVALID_HANDLE_VALUE;
        }

        if (m_ReqContext.DownloadFile != INVALID_HANDLE_VALUE)
        {
            CloseHandle(m_ReqContext.DownloadFile);
            m_ReqContext.DownloadFile = INVALID_HANDLE_VALUE;
        }

        if (m_ReqContext.CompletionEvent)
        {
            CloseHandle(m_ReqContext.CompletionEvent);
            m_ReqContext.CompletionEvent = NULL;
        }

        if (m_ReqContext.CleanUpEvent)
        {
            CloseHandle(m_ReqContext.CleanUpEvent);
            m_ReqContext.CleanUpEvent = NULL;
        }

        if (m_ReqContext.CritSecInitialized)
        {
            DeleteCriticalSection(&m_ReqContext.CriticalSection);
            m_ReqContext.CritSecInitialized = FALSE;
        }

    Exit:
        return;
    }



    VOID
    CleanUpSessionHandle()
    /*++

    Routine Description:
        Used to cleanup session before exiting.

    Arguments:
        SessionHandle - Wininet session handle

    Return Value:
        None.

    --*/
    {

        if (m_SessionHandle != NULL)
        {
            //
            // Remove the callback from the SessionHandle since
            // we don't want the closing notification
            //
            (VOID)InternetSetStatusCallbackW(m_SessionHandle, NULL);
            //
            // Call InternetCloseHandle and do not wait for the closing notification 
            // in the callback function
            //
            (VOID)InternetCloseHandle(m_SessionHandle);

            m_SessionHandle = NULL;
        }

        return;
    }


    DWORD
    OpenFiles(/*__in DWORD Method,
        __in const std::wstring& InputFileName,
        __in const std::wstring& OutputFileName*/
    )
    /*++

    Routine Description:
        This routine opens files, one to post data from, and
        one to write response into

    Arguments:
        ReqContext - Pointer to request context structure
        Method - GET or POST - do we need to open the input file
        InputFileName - Input file name
        OutputFileName - output file name

    Return Value:
        Error Code for the operation.

    --*/
    {
        DWORD Error = ERROR_SUCCESS;

        if (m_Configuration.Method == METHOD_POST)
        {
            if (!m_Configuration.InputFileName.empty())
            {
                // Open input file
                m_ReqContext.UploadFile = CreateFileW(STRING_PTR(m_Configuration.InputFileName),
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    NULL,                   // handle cannot be inherited
                    OPEN_ALWAYS,            // if file exists, open it
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);                  // No template file

                if (m_ReqContext.UploadFile == INVALID_HANDLE_VALUE)
                {
                    Error = GetLastError();
                    LogSysError(Error, L"CreateFile for input file");
                    goto Exit;
                }
            }
        }

        if (STRING_PTR(m_Configuration.OutputFileName) != NULL)
        {
            // Open output file
            m_ReqContext.DownloadFile = CreateFileW(STRING_PTR(m_Configuration.OutputFileName),
                GENERIC_WRITE,
                0,                        // Open exclusively
                NULL,                     // handle cannot be inherited
                CREATE_ALWAYS,            // if file exists, delete it
                FILE_ATTRIBUTE_NORMAL,
                NULL);                    // No template file

            if (m_ReqContext.DownloadFile == INVALID_HANDLE_VALUE)
            {
                Error = GetLastError();
                LogSysError(Error, L"CreateFile for output file");
                goto Exit;
            }
        }

    Exit:
        return Error;
    }


    DWORD
    AllocateAndInitializeRequestContext()
    /*++

    Routine Description:
        Allocate the request context and initialize it values

    Arguments:
        ReqContext - Pointer to Request context structure
        Configuration - Pointer to configuration structure
        SessionHandle - Wininet session handle to use when creating
                        connect handle

    Return Value:
        Error Code for the operation.

    --*/
    {
        DWORD Error = ERROR_SUCCESS;
        BOOL Success;

        m_ReqContext.Instance = this;
        m_ReqContext.RequestHandle = NULL;
        m_ReqContext.ConnectHandle = NULL;
        m_ReqContext.DownloadedBytes = 0;
        m_ReqContext.WrittenBytes = 0;
        m_ReqContext.ReadBytes = 0;
        m_ReqContext.UploadFile = INVALID_HANDLE_VALUE;
        m_ReqContext.DownloadFile = INVALID_HANDLE_VALUE;
        m_ReqContext.FileSize = 0;
        m_ReqContext.HandleUsageCount = 0;
        m_ReqContext.Closing = FALSE;
        m_ReqContext.Method = m_Configuration.Method;
        m_ReqContext.CompletionEvent = NULL;
        m_ReqContext.CleanUpEvent = NULL;
        memset(m_ReqContext.OutputBuffer, 0, sizeof(m_ReqContext.OutputBuffer));
        m_ReqContext.State = (m_ReqContext.Method == METHOD_GET) ? REQ_STATE_SEND_REQ : REQ_STATE_SEND_REQ_WITH_BODY;
        m_ReqContext.CritSecInitialized = FALSE;


        // initialize critical section

        Success = InitializeCriticalSectionAndSpinCount(&m_ReqContext.CriticalSection, SPIN_COUNT);

        if (!Success)
        {
            Error = GetLastError();
            LogSysError(Error, L"InitializeCriticalSectionAndSpinCount");
            goto Exit;
        }

        m_ReqContext.CritSecInitialized = TRUE;

        // create events
        m_ReqContext.CompletionEvent = CreateEventW(NULL,  // Sec attrib
            FALSE, // Auto reset
            FALSE, // Initial state unsignalled
            NULL); // Name
        if (m_ReqContext.CompletionEvent == NULL)
        {
            Error = GetLastError();
            LogSysError(Error, L"CreateEvent CompletionEvent");
            goto Exit;
        }

        // create events
        m_ReqContext.CleanUpEvent = CreateEventW(NULL,  // Sec attrib
            FALSE, // Auto reset
            FALSE, // Initial state unsignalled
            NULL); // Name
        if (m_ReqContext.CleanUpEvent == NULL)
        {
            Error = GetLastError();
            LogSysError(Error, L"CreateEvent CleanUpEvent");
            goto Exit;
        }

        // Open the file to dump the response entity body and
        // if required the file with the data to post
        Error = OpenFiles();

        if (Error != ERROR_SUCCESS)
        {
            fprintf(stderr, "OpenFiles failed with %d\n", Error);
            goto Exit;
        }

        // Verify if we've opened a file to post and get its size
        if (m_ReqContext.UploadFile != INVALID_HANDLE_VALUE)
        {
            m_ReqContext.FileSize = GetFileSize(m_ReqContext.UploadFile, NULL);
            if (m_ReqContext.FileSize == INVALID_FILE_SIZE)
            {
                Error = GetLastError();
                LogSysError(Error, L"GetFileSize");
                goto Exit;
            }
        }
        else
        {
            m_ReqContext.LeftSize = m_ReqContext.FileSize = m_Configuration.PostField.size();
        }


        Error = CreateWininetHandles();

        if (Error != ERROR_SUCCESS)
        {
            fprintf(stderr, "CreateWininetHandles failed with %d\n", Error);
            goto Exit;
        }

    Exit:

        if (Error != ERROR_SUCCESS)
        {
            CleanUpRequestContext();
        }

        return Error;
    }


    DWORD
    SendRequest()
    /*++

    Routine Description:
        Send the request using HttpSendRequest

    Arguments:
        ReqContext - Pointer to request context structure

    Return Value:
        Error code for the operation.

    --*/
    {
        BOOL Success;
        DWORD Error = ERROR_SUCCESS;

        Success = AcquireRequestHandle();
        if (!Success)
        {
            Error = ERROR_OPERATION_ABORTED;
            goto Exit;
        }

        Success = HttpSendRequestW(m_ReqContext.RequestHandle,
            NULL,                   // do not provide additional Headers
            0,                      // dwHeadersLength 
            NULL,                   // Do not send any data 
            0);                     // dwOptionalLength 

        ReleaseRequestHandle();

        if (!Success)
        {
            Error = GetLastError();

            if (Error != ERROR_IO_PENDING)
            {
                LogInetError(Error, L"HttpSendRequest");
            }
            goto Exit;
        }

    Exit:

        return Error;
    }


    DWORD
    SendRequestWithBody()
    /*++

    Routine Description:
        Send the request with entity-body using HttpSendRequestEx

    Arguments:
        ReqContext - Pointer to request context structure

    Return Value:
        Error code for the operation.

    --*/
    {
        BOOL Success;
        INTERNET_BUFFERS BuffersIn;
        DWORD Error = ERROR_SUCCESS;

        //
        // HttpSendRequest can also be used also to post data to a server, 
        // to do so, the data should be provided using the lpOptional
        // parameter and it's size on dwOptionalLength.
        // Here we decided to depict the use of HttpSendRequestEx function.
        //

        //Prepare the Buffers to be passed to HttpSendRequestEx
        ZeroMemory(&BuffersIn, sizeof(INTERNET_BUFFERS));
        BuffersIn.dwStructSize = sizeof(INTERNET_BUFFERS);
        BuffersIn.lpvBuffer = NULL;
        BuffersIn.dwBufferLength = 0;
        BuffersIn.dwBufferTotal = m_ReqContext.FileSize; // content-length of data to post


        Success = AcquireRequestHandle();
        if (!Success)
        {
            Error = ERROR_OPERATION_ABORTED;
            goto Exit;
        }

        Success = HttpSendRequestExW(m_ReqContext.RequestHandle,
            &BuffersIn,
            NULL,                 // Do not use output buffers
            0,                    // dwFlags reserved
            (DWORD_PTR)&m_ReqContext);

        ReleaseRequestHandle();

        if (!Success)
        {
            Error = GetLastError();

            if (Error != ERROR_IO_PENDING)
            {
                LogInetError(Error, L"HttpSendRequestEx");
            }

            goto Exit;
        }

    Exit:

        return Error;
    }
    VOID
    ProcessRequest(__in DWORD Error)
    /*++

    Routine Description:
        Process the request context - Sending the request and
        receiving the response

    Arguments:
        ReqContext - Pointer to request context structure
        Error - error returned from last asynchronous call

    Return Value:
        None.

    --*/
    {
        BOOL Eof = FALSE;

        while (Error == ERROR_SUCCESS && m_ReqContext.State != REQ_STATE_COMPLETE)
        {

            switch (m_ReqContext.State)
            {
            case REQ_STATE_SEND_REQ:

                m_ReqContext.State = REQ_STATE_RESPONSE_RECV_DATA;
                Error = SendRequest();

                break;

            case REQ_STATE_SEND_REQ_WITH_BODY:

                m_ReqContext.State = REQ_STATE_POST_GET_DATA;
                Error = SendRequestWithBody();

                break;

            case REQ_STATE_POST_GET_DATA:

                m_ReqContext.State = REQ_STATE_POST_SEND_DATA;
                Error = GetDataToPost();

                break;

            case REQ_STATE_POST_SEND_DATA:

                m_ReqContext.State = REQ_STATE_POST_GET_DATA;
                Error = PostDataToServer(&Eof);

                if (Eof)
                {
                    m_ReqContext.State = REQ_STATE_POST_COMPLETE;
                }

                break;

            case REQ_STATE_POST_COMPLETE:

                m_ReqContext.State = REQ_STATE_RESPONSE_RECV_DATA;
                Error = CompleteRequest();

                break;

            case REQ_STATE_RESPONSE_RECV_DATA:

                m_ReqContext.State = REQ_STATE_RESPONSE_WRITE_DATA;
                Error = RecvResponseData();

                break;

            case REQ_STATE_RESPONSE_WRITE_DATA:

                m_ReqContext.State = REQ_STATE_RESPONSE_RECV_DATA;
                Error = WriteResponseData(&Eof);

                if (Eof)
                {
                    m_ReqContext.State = REQ_STATE_COMPLETE;
                }

                break;

            default:

                break;
            }
        }

        if (Error != ERROR_IO_PENDING)
        {
            //
            // Everything has been procesed or has failed. 
            // In either case, the signal processing has
            // completed
            //

            SetEvent(m_ReqContext.CompletionEvent);
        }

        return;
    }

    static VOID CALLBACK
    InternetCallback(
        __in HINTERNET hInternet,
        __in DWORD_PTR dwContext,
        __in DWORD dwInternetStatus,
        __in_bcount(dwStatusInformationLength) LPVOID lpvStatusInformation,
        __in DWORD dwStatusInformationLength
    )
    /*++

    Routine Description:
        Callback routine for asynchronous WinInet operations

    Arguments:
         hInternet - The handle for which the callback function is called.
         dwContext - Pointer to the application defined context.
         dwInternetStatus - Status code indicating why the callback is called.
         lpvStatusInformation - Pointer to a buffer holding callback specific data.
         dwStatusInformationLength - Specifies size of lpvStatusInformation buffer.

    Return Value:
        None.

    --*/
    {
        InternetCookieHistory cookieHistory;
        RequestContext* ReqContext = (RequestContext*)dwContext;
        CHttpTool* thiz = (CHttpTool *)ReqContext->Instance;

        UNREFERENCED_PARAMETER(dwStatusInformationLength);

        fprintf(stderr, "Callback Received for Handle %p \t", hInternet);

        switch (dwInternetStatus)
        {
        case INTERNET_STATUS_COOKIE_SENT:
            fprintf(stderr, "Status: Cookie found and will be sent with request\n");
            break;

        case INTERNET_STATUS_COOKIE_RECEIVED:
            fprintf(stderr, "Status: Cookie Received\n");
            break;

        case INTERNET_STATUS_COOKIE_HISTORY:

            fprintf(stderr, "Status: Cookie History\n");

            cookieHistory = *((InternetCookieHistory*)lpvStatusInformation);

            if (cookieHistory.fAccepted)
            {
                fprintf(stderr, "Cookie Accepted\n");
            }
            if (cookieHistory.fLeashed)
            {
                fprintf(stderr, "Cookie Leashed\n");
            }
            if (cookieHistory.fDowngraded)
            {
                fprintf(stderr, "Cookie Downgraded\n");
            }
            if (cookieHistory.fRejected)
            {
                fprintf(stderr, "Cookie Rejected\n");
            }


            break;

        case INTERNET_STATUS_CLOSING_CONNECTION:
            fprintf(stderr, "Status: Closing Connection\n");
            break;

        case INTERNET_STATUS_CONNECTED_TO_SERVER:
            fprintf(stderr, "Status: Connected to Server\n");
            break;

        case INTERNET_STATUS_CONNECTING_TO_SERVER:
            fprintf(stderr, "Status: Connecting to Server\n");
            break;

        case INTERNET_STATUS_CONNECTION_CLOSED:
            fprintf(stderr, "Status: Connection Closed\n");
            break;

        case INTERNET_STATUS_HANDLE_CLOSING:
            fprintf(stderr, "Status: Handle Closing\n");

            //
            // Signal the cleanup routine that it is
            // safe to cleanup the request context
            //

            SetEvent(thiz->m_ReqContext.CleanUpEvent);

            break;

        case INTERNET_STATUS_HANDLE_CREATED:
            fprintf(stderr,
                "Handle %x created\n",
                ((LPINTERNET_ASYNC_RESULT)lpvStatusInformation)->dwResult);

            break;

        case INTERNET_STATUS_INTERMEDIATE_RESPONSE:
            fprintf(stderr, "Status: Intermediate response\n");
            break;

        case INTERNET_STATUS_RECEIVING_RESPONSE:
            fprintf(stderr, "Status: Receiving Response\n");
            break;

        case INTERNET_STATUS_RESPONSE_RECEIVED:
            fprintf(stderr, "Status: Response Received (%d Bytes)\n", *((LPDWORD)lpvStatusInformation));

            break;

        case INTERNET_STATUS_REDIRECT:
            fprintf(stderr, "Status: Redirect\n");
            break;

        case INTERNET_STATUS_REQUEST_COMPLETE:
            fprintf(stderr, "Status: Request complete\n");

            thiz->ProcessRequest(((LPINTERNET_ASYNC_RESULT)lpvStatusInformation)->dwError);

            break;

        case INTERNET_STATUS_REQUEST_SENT:

            fprintf(stderr, "Status: Request sent (%d Bytes)\n", *((LPDWORD)lpvStatusInformation));
            break;

        case INTERNET_STATUS_DETECTING_PROXY:
            fprintf(stderr, "Status: Detecting Proxy\n");
            break;

        case INTERNET_STATUS_RESOLVING_NAME:
            fprintf(stderr, "Status: Resolving Name\n");
            break;

        case INTERNET_STATUS_NAME_RESOLVED:
            fprintf(stderr, "Status: Name Resolved\n");
            break;

        case INTERNET_STATUS_SENDING_REQUEST:
            fprintf(stderr, "Status: Sending request\n");
            break;

        case INTERNET_STATUS_STATE_CHANGE:
            fprintf(stderr, "Status: State Change\n");
            break;

        case INTERNET_STATUS_P3P_HEADER:
            fprintf(stderr, "Status: Received P3P header\n");
            break;

        default:
            fprintf(stderr, "Status: Unknown (%d)\n", dwInternetStatus);
            break;
        }

        return;
    }

    DWORD
    GetDataToPost()
    /*++

    Routine Description:
        Reads data from a file

    Arguments:
        ReqContext - Pointer to request context structure

    Return Value:
        Error code for the operation.

    --*/
    {
        DWORD Error = ERROR_SUCCESS;
        BOOL Success;


        //
        //
        // ReadFile is done inline here assuming that it will return quickly
        // I.E. the file is on disk
        //
        // If you plan to do blocking/intensive operations they should be
        // queued to another thread and not block the callback thread
        //
        //
        if (m_ReqContext.UploadFile != INVALID_HANDLE_VALUE)
        {
            Success = ReadFile(m_ReqContext.UploadFile,
                m_ReqContext.OutputBuffer,
                BUFFER_LEN,
                &m_ReqContext.ReadBytes,
                NULL);
            if (!Success)
            {
                Error = GetLastError();
                LogSysError(Error, L"ReadFile");
                goto Exit;
            }
        }
        else
        {
            if (m_ReqContext.LeftSize > 0)
            {
                m_ReqContext.ReadBytes = m_Configuration.PostField.size();
                memcpy(m_ReqContext.OutputBuffer, m_Configuration.PostField.data(), m_ReqContext.ReadBytes);
                m_ReqContext.LeftSize -= m_ReqContext.ReadBytes;
            }
            else
            {
                m_ReqContext.ReadBytes = 0;
            }
        }

    Exit:

        return Error;
    }

    DWORD
    PostDataToServer(__out PBOOL Eof)
    /*++

    Routine Description:
        Post data in the http request

    Arguments:
        Eof - Done posting data to server

    Return Value:
        Error code for the operation.

    --*/
    {
        DWORD Error = ERROR_SUCCESS;
        BOOL Success;

        *Eof = FALSE;

        if (m_ReqContext.ReadBytes == 0)
        {
            *Eof = TRUE;
            goto Exit;
        }


        Success = AcquireRequestHandle();
        if (!Success)
        {
            Error = ERROR_OPERATION_ABORTED;
            goto Exit;
        }


        //
        // The lpdwNumberOfBytesWritten parameter will be
        // populated on async completion, so it must exist
        // until INTERNET_STATUS_REQUEST_COMPLETE.
        // The same is true of lpBuffer parameter.
        //

        Success = InternetWriteFile(m_ReqContext.RequestHandle,
            m_ReqContext.OutputBuffer,
            m_ReqContext.ReadBytes,
            &m_ReqContext.WrittenBytes);


        ReleaseRequestHandle();

        if (!Success)
        {
            Error = GetLastError();

            if (Error == ERROR_IO_PENDING)
            {
                fprintf(stderr, "Waiting for InternetWriteFile to complete\n");
            }
            else
            {
                LogInetError(Error, L"InternetWriteFile");
            }

            goto Exit;

        }

    Exit:
        return Error;
    }


    DWORD
    CompleteRequest()
    /*++

    Routine Description:
        Perform completion of asynchronous post.

    Arguments:
        ReqContext - Pointer to request context structure

    Return Value:
        Error Code for the operation.

    --*/
    {

        DWORD Error = ERROR_SUCCESS;
        BOOL Success;

        fprintf(stderr, "Finished posting file\n");

        Success = AcquireRequestHandle();
        if (!Success)
        {
            Error = ERROR_OPERATION_ABORTED;
            goto Exit;
        }

        Success = HttpEndRequestW(m_ReqContext.RequestHandle, NULL, 0, 0);

        ReleaseRequestHandle();

        if (!Success)
        {
            Error = GetLastError();
            if (Error == ERROR_IO_PENDING)
            {
                fprintf(stderr, "Waiting for HttpEndRequest to complete \n");
            }
            else
            {
                LogInetError(Error, L"HttpEndRequest");
                goto Exit;
            }
        }

    Exit:

        return Error;
    }



    DWORD
    RecvResponseData()
    /*++

    Routine Description:
         Receive response

    Arguments:
         ReqContext - Pointer to request context structure

    Return Value:
         Error Code for the operation.

    --*/
    {
        DWORD Error = ERROR_SUCCESS;
        BOOL Success;

        Success = AcquireRequestHandle();
        if (!Success)
        {
            Error = ERROR_OPERATION_ABORTED;
            goto Exit;
        }

        //
        // The lpdwNumberOfBytesRead parameter will be
        // populated on async completion, so it must exist
        // until INTERNET_STATUS_REQUEST_COMPLETE.
        // The same is true of lpBuffer parameter.
        //
        // InternetReadFile will block until the buffer
        // is completely filled or the response is exhausted.
        //


        Success = InternetReadFile(m_ReqContext.RequestHandle,
            m_ReqContext.OutputBuffer,
            BUFFER_LEN,
            &m_ReqContext.DownloadedBytes);

        ReleaseRequestHandle();

        if (!Success)
        {
            Error = GetLastError();
            if (Error == ERROR_IO_PENDING)
            {
                fprintf(stderr, "Waiting for InternetReadFile to complete\n");
            }
            else
            {
                LogInetError(Error, L"InternetReadFile");
            }

            goto Exit;
        }


    Exit:

        return Error;
    }


    DWORD
    WriteResponseData(__out PBOOL Eof)
    /*++

    Routine Description:
         Write response to a file

    Arguments:
         ReqContext - Pointer to request context structure
         Eof - Done with response

    Return Value:
         Error Code for the operation.

    --*/
    {
        DWORD Error = ERROR_SUCCESS;
        DWORD BytesWritten = 0;

        BOOL Success;

        *Eof = FALSE;

        //
        // Finished receiving response
        //

        if (m_ReqContext.DownloadedBytes == 0)
        {
            *Eof = TRUE;
            goto Exit;

        }

        //
        //
        // WriteFile is done inline here assuming that it will return quickly
        // I.E. the file is on disk
        //
        // If you plan to do blocking/intensive operations they should be
        // queued to another thread and not block the callback thread
        //
        //
        if (m_ReqContext.DownloadFile != INVALID_HANDLE_VALUE)
        {
            Success = WriteFile(m_ReqContext.DownloadFile,
                m_ReqContext.OutputBuffer,
                m_ReqContext.DownloadedBytes,
                &BytesWritten,
                NULL);

            if (!Success)
            {
                Error = GetLastError();

                LogSysError(Error, L"WriteFile");
                goto Exit;;
            }
        }
        else
        {
            m_ReqContext.RespData.append(m_ReqContext.OutputBuffer, m_ReqContext.DownloadedBytes);
        }       

    Exit:

        return Error;
    }

    VOID
    CloseRequestHandle()
    /*++

    Routine Description:
        Safely  close the request handle by synchronizing
        with all threads using the handle.

        When this function returns no more calls can be made with the
        handle.

    Arguments:
        ReqContext - Pointer to Request context structure
    Return Value:
        None.

    --*/
    {
        BOOL Close = FALSE;

        EnterCriticalSection(&m_ReqContext.CriticalSection);

        //
        // Current implementation only supports the main thread
        // kicking off the request handle close
        //
        // To support multiple threads the lifetime 
        // of the request context must be carefully controlled
        // (most likely guarded by refcount/critsec)
        // so that they are not trying to abort a request
        // where the context has already been freed.
        //

        m_ReqContext.Closing = TRUE;

        if (m_ReqContext.HandleUsageCount == 0)
        {
            Close = TRUE;
        }

        LeaveCriticalSection(&m_ReqContext.CriticalSection);

        if (Close)
        {
            //
            // At this point there must be the guarantee that all calls
            // to wininet with this handle have returned with some value
            // including ERROR_IO_PENDING, and none will be made after
            // InternetCloseHandle.
            //        
            (VOID)InternetCloseHandle(m_ReqContext.RequestHandle);
            m_ReqContext.RequestHandle = NULL;
        }

        return;
    }
private:
    HINTERNET m_SessionHandle = NULL;
    RequestContext m_ReqContext = {};
    Configuration m_Configuration = {};

    const std::unordered_map<TSTRING, TSTRING> m_cHeaders = {
           {TEXT("Accept:"),TEXT("*,*/*")},
           {TEXT("Accept-Language:"),TEXT("zh-cn")},
           {TEXT("User-Agent:"),TEXT("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/89.0.4389.72 Safari/537.36 Edg/89.0.774.45")},
           {TEXT("Content-Type:"),TEXT("application/x-www-form-urlencoded")},
           {TEXT("Accept-Encoding:"),TEXT("deflate")},
    };
};