#include "AddInNative.h"

#define TIME_LEN 65
#define BASE_ERRNO 7

static const wchar_t* g_PropNames[] = {
	L"Host", L"Port", L"Mode"
};
static const wchar_t* g_MethodNames[] = {
	L"SendMessage"
};

static const wchar_t* g_PropNamesRu[] = {
	L"Адрес", L"Порт", L"Режим"
};
static const wchar_t* g_MethodNamesRu[] = {
	L"ОтправитьСообщение"
};

static const wchar_t g_kClassNames[] = L"AddInNativeNet";
static IAddInDefBase* pAsyncEvent = NULL;

uint32_t convToShortWchar(WCHAR_T** Dest, const wchar_t* Source, uint32_t len = 0);
uint32_t convFromShortWchar(wchar_t** Dest, const WCHAR_T* Source, uint32_t len = 0);
uint32_t getLenShortWcharStr(const WCHAR_T* Source);
static AppCapabilities g_capabilities = eAppCapabilitiesInvalid;
static WcharWrapper s_names(g_kClassNames);
//---------------------------------------------------------------------------//
long GetClassObject(const WCHAR_T* wsName, IComponentBase** pInterface)
{
	if (!*pInterface)
	{
		*pInterface = new CAddInNative;
		return (long)* pInterface;
	}
	return 0;
}
//---------------------------------------------------------------------------//
AppCapabilities SetPlatformCapabilities(const AppCapabilities capabilities)
{
	g_capabilities = capabilities;
	return eAppCapabilitiesLast;
}
//---------------------------------------------------------------------------//
long DestroyObject(IComponentBase** pIntf)
{
	if (!*pIntf)
		return -1;

	delete* pIntf;
	*pIntf = 0;
	return 0;
}
//---------------------------------------------------------------------------//
const WCHAR_T* GetClassNames()
{
	return s_names;
}
//---------------------------------------------------------------------------//
VOID CALLBACK MyTimerProc(PVOID lpParam, BOOLEAN TimerOrWaitFired);
// CAddInNative
//---------------------------------------------------------------------------//
CAddInNative::CAddInNative()
{
	m_iMemory = 0;
	m_iConnect = 0;
	m_hTimerQueue = 0;
	m_Mode = Mode::TCP;
	m_Port = 0;
}
//---------------------------------------------------------------------------//
CAddInNative::~CAddInNative()
{
}
//---------------------------------------------------------------------------//
bool CAddInNative::Init(void* pConnection)
{
	m_iConnect = (IAddInDefBase*)pConnection;
	return m_iConnect != NULL;
}
//---------------------------------------------------------------------------//
long CAddInNative::GetInfo()
{
	// Component should put supported component technology version 
	// This component supports 2.0 version
	return 2000;
}
//---------------------------------------------------------------------------//
void CAddInNative::Done()
{
	if (m_hTimerQueue)
	{
		DeleteTimerQueue(m_hTimerQueue);
		m_hTimerQueue = 0;
	}
}
/////////////////////////////////////////////////////////////////////////////
// ILanguageExtenderBase
//---------------------------------------------------------------------------//
bool CAddInNative::RegisterExtensionAs(WCHAR_T** wsExtensionName)
{
	const wchar_t* wsExtension = g_kClassNames;
	int iActualSize = ::wcslen(wsExtension) + 1;
	WCHAR_T* dest = 0;

	if (m_iMemory)
	{
		if (m_iMemory->AllocMemory((void**)wsExtensionName, iActualSize * sizeof(WCHAR_T)))
			::convToShortWchar(wsExtensionName, wsExtension, iActualSize);
		return true;
	}

	return false;
}
//---------------------------------------------------------------------------//
long CAddInNative::GetNProps()
{
	// You may delete next lines and add your own implementation code here
	return ePropLast;
}
//---------------------------------------------------------------------------//
long CAddInNative::FindProp(const WCHAR_T* wsPropName)
{
	long plPropNum = -1;
	wchar_t* propName = 0;

	::convFromShortWchar(&propName, wsPropName);
	plPropNum = findName(g_PropNames, propName, ePropLast);

	if (plPropNum == -1)
		plPropNum = findName(g_PropNamesRu, propName, ePropLast);

	delete[] propName;

	return plPropNum;
}
//---------------------------------------------------------------------------//
const WCHAR_T* CAddInNative::GetPropName(long lPropNum, long lPropAlias)
{
	if (lPropNum >= ePropLast)
		return NULL;

	wchar_t* wsCurrentName = NULL;
	WCHAR_T* wsPropName = NULL;
	int iActualSize = 0;

	switch (lPropAlias)
	{
	case 0: // First language
		wsCurrentName = (wchar_t*)g_PropNames[lPropNum];
		break;
	case 1: // Second language
		wsCurrentName = (wchar_t*)g_PropNamesRu[lPropNum];
		break;
	default:
		return 0;
	}

	iActualSize = (int)wcslen(wsCurrentName) + 1;

	if (m_iMemory && wsCurrentName)
	{
		if (m_iMemory->AllocMemory((void**)& wsPropName, iActualSize * sizeof(WCHAR_T)))
			::convToShortWchar(&wsPropName, wsCurrentName, iActualSize);
	}

	return wsPropName;
}
//---------------------------------------------------------------------------//
bool CAddInNative::GetPropVal(const long lPropNum, tVariant* pvarPropVal)
{
	switch (lPropNum) {
	case ePropHost:
		wstring_to_variant(m_Host, pvarPropVal);
		break;
	case ePropPort:
		TV_VT(pvarPropVal) = VTYPE_UI2;
		TV_UI2(pvarPropVal) = m_Port;
		break;
	case ePropMode:
		TV_VT(pvarPropVal) = VTYPE_UI1;
		TV_UI1(pvarPropVal) = (std::uint8_t)m_Mode;
		break;
	default:
		return false;
	}

	return true;
}
//---------------------------------------------------------------------------//
bool CAddInNative::SetPropVal(const long lPropNum, tVariant* varPropVal)
{
	switch (lPropNum) {
	case ePropHost:
		if (TV_VT(varPropVal) != VTYPE_PWSTR) return false;
		m_Host = TV_WSTR(varPropVal);
		break;
	case ePropPort:
		m_Port = TV_UI2(varPropVal);
		break;
	case ePropMode:
		if (TV_UI1(varPropVal) == 1)
			m_Mode = Mode::UDP;
		break;
	default:
		return false;
	}

	return true;
}
//---------------------------------------------------------------------------//
bool CAddInNative::IsPropReadable(const long lPropNum)
{
	return true;
}
//---------------------------------------------------------------------------//
bool CAddInNative::IsPropWritable(const long lPropNum)
{
	switch (lPropNum) {
	case ePropPort:
		return true;
	case ePropMode:
		return true;
	case ePropHost:
		return true;
	default:
		return false;
	}
}
//---------------------------------------------------------------------------//
long CAddInNative::GetNMethods()
{
	return eMethLast;
}
//---------------------------------------------------------------------------//
long CAddInNative::FindMethod(const WCHAR_T* wsMethodName)
{
	long plMethodNum = -1;
	wchar_t* name = 0;

	::convFromShortWchar(&name, wsMethodName);

	plMethodNum = findName(g_MethodNames, name, eMethLast);

	if (plMethodNum == -1)
		plMethodNum = findName(g_MethodNamesRu, name, eMethLast);

	delete[] name;

	return plMethodNum;
}
//---------------------------------------------------------------------------//
const WCHAR_T* CAddInNative::GetMethodName(const long lMethodNum, const long lMethodAlias)
{
	if (lMethodNum >= eMethLast)
		return NULL;

	wchar_t* wsCurrentName = NULL;
	WCHAR_T* wsMethodName = NULL;
	int iActualSize = 0;

	switch (lMethodAlias)
	{
	case 0: // First language
		wsCurrentName = (wchar_t*)g_MethodNames[lMethodNum];
		break;
	case 1: // Second language
		wsCurrentName = (wchar_t*)g_MethodNamesRu[lMethodNum];
		break;
	default:
		return 0;
	}

	iActualSize = (int)wcslen(wsCurrentName) + 1;

	if (m_iMemory && wsCurrentName)
	{
		if (m_iMemory->AllocMemory((void**)& wsMethodName, iActualSize * sizeof(WCHAR_T)))
			::convToShortWchar(&wsMethodName, wsCurrentName, iActualSize);
	}

	return wsMethodName;
}
//---------------------------------------------------------------------------//
long CAddInNative::GetNParams(const long lMethodNum)
{
	switch (lMethodNum)
	{
	case eMethSendMessage:
		return 1;
	default:
		return 0;
	}
}
//---------------------------------------------------------------------------//
bool CAddInNative::GetParamDefValue(const long lMethodNum, const long lParamNum,
	tVariant* pvarParamDefValue)
{
	TV_VT(pvarParamDefValue) = VTYPE_EMPTY;

	switch (lMethodNum)
	{
	case eMethSendMessage:
		return true;
	default:
		return false;
	}
}
//---------------------------------------------------------------------------//
bool CAddInNative::HasRetVal(const long lMethodNum)
{
	switch (lMethodNum)
	{
	case eMethSendMessage:
		return true;
	default:
		return false;
	}
}
//---------------------------------------------------------------------------//
bool CAddInNative::CallAsProc(const long lMethodNum, tVariant* paParams, const long lSizeArray)
{
	switch (lMethodNum)
	{
	case eMethLast:
		return true;
	default:
		return false;
	}
}
//---------------------------------------------------------------------------//
bool CAddInNative::CallAsFunc(const long lMethodNum, tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray)
{
	tVariant& pParam0 = paParams[0];
	std::wstring wstr = pParam0.pwstrVal;
	std::string message = Utils::narrow_string(wstr);
	auto str_host = Utils::narrow_string(m_Host);

	switch (lMethodNum)
	{
	case eMethSendMessage:
		switch (m_Mode)
		{
		case CAddInNative::TCP: {
			return tcp_send(message, str_host, pvarRetValue);
		}
		case CAddInNative::UDP: {
			return udp_send(message, str_host, pvarRetValue);
		}
		default:
			return false;
		}
	default:
		return false;
	}
}
//---------------------------------------------------------------------------//
// This code will work only on the client!
VOID CALLBACK MyTimerProc(PVOID lpParam, BOOLEAN TimerOrWaitFired)
{
	if (!pAsyncEvent)
		return;
	DWORD dwTime = 0;
	wchar_t* who = L"ComponentNative", * what = L"Timer";

	wchar_t* wstime = new wchar_t[TIME_LEN];
	if (wstime)
	{
		wmemset(wstime, 0, TIME_LEN);
		time_t vtime;
		time(&vtime);
		::_ui64tow_s(vtime, wstime, TIME_LEN, 10);
		pAsyncEvent->ExternalEvent(who, what, wstime);
		delete[] wstime;
	}
}
//---------------------------------------------------------------------------//
void CAddInNative::SetLocale(const WCHAR_T* loc)
{
	_wsetlocale(LC_ALL, L"");
}
/////////////////////////////////////////////////////////////////////////////
// LocaleBase
//---------------------------------------------------------------------------//
bool CAddInNative::setMemManager(void* mem)
{
	m_iMemory = (IMemoryManager*)mem;
	return m_iMemory != 0;
}
//---------------------------------------------------------------------------//
void CAddInNative::addError(uint32_t wcode, const wchar_t* source, const wchar_t* descriptor, long code)
{
	if (m_iConnect)
	{
		WCHAR_T* err = 0;
		WCHAR_T* descr = 0;

		::convToShortWchar(&err, source);
		::convToShortWchar(&descr, descriptor);

		m_iConnect->AddError(wcode, err, descr, code);
		delete[] err;
		delete[] descr;
	}
}

bool CAddInNative::tcp_send(const std::string& message, const std::string& host, tVariant* pvarRetValue) {

	trantor::EventLoop loop;
	trantor::InetAddress server_address(host, m_Port);

	auto client = std::make_shared<trantor::TcpClient>(&loop, server_address, "Avito: 1C Enterprise");

	client->setConnectionCallback(
		[&loop, &message, pvarRetValue, this](const trantor::TcpConnectionPtr& conn) {
			if (conn->connected()) {
				conn->send(message);
				conn->shutdown();
			}
			else {
				string_to_retVariant(Utils::stringToJson("success"), pvarRetValue);
				loop.quit();
			}
		});

	client->setConnectionErrorCallback([&loop, pvarRetValue, this]() {
		string_to_retVariant(Utils::stringToJson("failed", "Not connection to the server"), pvarRetValue);
	loop.quit();
		});

	client->connect();
	loop.loop();

	return true;
}

bool CAddInNative::udp_send(const std::string& message, const std::string& host, tVariant* pvarRetValue) {

	WSADATA ws_data;

	if (WSAStartup(MAKEWORD(2, 2), &ws_data) != 0)
	{
		auto result = string_to_retVariant(Utils::stringToJson("failed", "Not Initialising Winsocket"), pvarRetValue);
		return false;
	}

	sockaddr_in server;
	int client_socket;

	if ((client_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR)
	{
		auto result = string_to_retVariant(Utils::stringToJson("failed", "Not Create Winsocket"), pvarRetValue);
		return false;
	}

	memset((char*)&server, 0, sizeof(server));

	server.sin_family = AF_INET;
	server.sin_port = htons(m_Port);
	server.sin_addr.S_un.S_addr = inet_addr(host.c_str());

	if (sendto(client_socket, message.c_str(), strlen(message.c_str()), 0, (sockaddr*)&server, sizeof(sockaddr_in)) == SOCKET_ERROR)
	{
		auto result = string_to_retVariant(Utils::stringToJson("failed", "Not send message"), pvarRetValue);
		return false;
	}

	closesocket(client_socket);
	WSACleanup();
	auto result = string_to_retVariant(Utils::stringToJson("success"), pvarRetValue);

	return true;
}
//---------------------------------------------------------------------------//
long CAddInNative::findName(const wchar_t* names[], const wchar_t* name, const uint32_t size) const
{
	long ret = -1;
	for (uint32_t i = 0; i < size; i++)
	{
		if (!wcscmp(names[i], name))
		{
			ret = i;
			break;
		}
	}
	return ret;
}
//---------------------------------------------------------------------------//
uint32_t convToShortWchar(WCHAR_T** Dest, const wchar_t* Source, uint32_t len)
{
	if (!len)
		len = (int)::wcslen(Source) + 1;

	if (!*Dest)
		* Dest = new WCHAR_T[len];

	WCHAR_T* tmpShort = *Dest;
	wchar_t* tmpWChar = (wchar_t*)Source;
	uint32_t res = 0;

	::memset(*Dest, 0, len * sizeof(WCHAR_T));

	for (; len; --len, ++res, ++tmpWChar, ++tmpShort)
	{
		*tmpShort = (WCHAR_T)* tmpWChar;
	}

	return res;
}
//---------------------------------------------------------------------------//
uint32_t convFromShortWchar(wchar_t** Dest, const WCHAR_T* Source, uint32_t len)
{
	if (!len)
		len = getLenShortWcharStr(Source) + 1;

	if (!*Dest)
		* Dest = new wchar_t[len];

	wchar_t* tmpWChar = *Dest;
	WCHAR_T* tmpShort = (WCHAR_T*)Source;
	uint32_t res = 0;

	::memset(*Dest, 0, len * sizeof(wchar_t));

	for (; len; --len, ++res, ++tmpWChar, ++tmpShort)
	{
		*tmpWChar = (wchar_t)* tmpShort;
	}

	return res;
}
//---------------------------------------------------------------------------//
uint32_t getLenShortWcharStr(const WCHAR_T* Source)
{
	uint32_t res = 0;
	WCHAR_T* tmpShort = (WCHAR_T*)Source;

	while (*tmpShort++)
		++res;

	return res;
}

//---------------------------------------------------------------------------//
WcharWrapper::WcharWrapper(const wchar_t* str) : m_str_wchar(NULL)
{
	if (str)
	{
		int len = (int)wcslen(str);
		m_str_wchar = new wchar_t[len + 1];
		memset(m_str_wchar, 0, sizeof(wchar_t) * (len + 1));
		memcpy(m_str_wchar, str, sizeof(wchar_t) * len);
	}

}
//---------------------------------------------------------------------------//
WcharWrapper::~WcharWrapper()
{
	if (m_str_wchar)
	{
		delete[] m_str_wchar;
		m_str_wchar = NULL;
	}
}
//---------------------------------------------------------------------------//

bool CAddInNative::wstring_to_variant(std::wstring& str, tVariant* val) {

	char* t1;

	TV_VT(val) = VTYPE_PWSTR;

	m_iMemory->AllocMemory((void**)&t1, (str.length() + 1) * sizeof(WCHAR_T));
	memcpy(t1, str.c_str(), (str.length() + 1) * sizeof(WCHAR_T));

	val->pstrVal = t1;
	val->strLen = str.length();
	
	return true;
}

bool CAddInNative::string_to_variant(std::string& str, tVariant* val)
{
	WCHAR_T* wsPropValue = 0;

	TV_VT(val) = VTYPE_PWSTR;

	std::wstring wTemp(str.begin(), str.end());

	int isActualSize = wTemp.length() + 1;

	if (m_iMemory)
	{
		if (m_iMemory->AllocMemory((void**)&wsPropValue, isActualSize * sizeof(WCHAR_T)))
			::convToShortWchar(&wsPropValue, wTemp.c_str(), isActualSize);
	}

	TV_WSTR(val) = wsPropValue;
	val->wstrLen = isActualSize;

	return false;
}

bool CAddInNative::variant_to_string(std::string& str, tVariant* val)
{
	std::wstring wstr = TV_WSTR(val);
	str = Utils::narrow_string(wstr);

	return true;
}

bool CAddInNative::string_to_retVariant(std::string& str, tVariant* retValue)
{
	bool result = false;

	if (m_iMemory->AllocMemory((void**)&retValue->pstrVal, (str.length() + 1) * sizeof(CHAR)))
	{
		memcpy(retValue->pstrVal, str.c_str(), str.length() * sizeof(CHAR));
		TV_VT(retValue) = VTYPE_PSTR;
		retValue->strLen = str.length();
		result = true;
	}

	return result;
}

bool CAddInNative::string_to_retVariant(std::string&& str, tVariant* retValue)
{
	bool result = false;

	if (m_iMemory->AllocMemory((void**)&retValue->pstrVal, (str.length() + 1) * sizeof(CHAR)))
	{
		memcpy(retValue->pstrVal, str.c_str(), str.length() * sizeof(CHAR));
		TV_VT(retValue) = VTYPE_PSTR;
		retValue->strLen = str.length();
		result = true;
	}

	return result;
}
