#pragma once
#include "pch.h"
#include "framework.h"
#include <string>
#include <vector>
#include <map>
#include <list>
#include <mutex>

#define WM_SEND_PACK (WM_USER + 1)     // �������������Ϣ
#define WM_SEND_PACK_ACK (WM_USER + 2) // ����Ӧ�������Ϣ

// ���ķ�װ��ƣ�1-2���ֽڵİ�ͷ + 2/4���ֽڴ洢���ĳ���(���ݰ��Ĵ�С��ѡ��) + ���崫�ݵ�����(ǰ1/2���ֽ���Ϊ�����1/2���ֽ���ΪУ�飬�м伴Ϊ��ʽ������)
// ��ͷ���ݾ���һ������Ϊ��FF FE / FE FF ��Ϊ�������ֽڳ��ֵĸ��ʺ�С
// �����ļ��飺��У�飬�������崫�ݵ��������(�����ֽ�/�����Σ��������)�������ӵĽ����ΪУ�����ڰ���ĩβ
class CPacket // ���ݰ���
{
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}  // ���캯������Ա������ʼ��Ϊ 0
	CPacket(const CPacket& pack) // �������캯��
	{
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
	}
	CPacket& operator=(const CPacket& pack) // ���ظ�ֵ�����
	{
		if (this != &pack) // eg: a = b; ���ָ�ֵ���
		{
			sHead = pack.sHead;
			nLength = pack.nLength;
			sCmd = pack.sCmd;
			strData = pack.strData;
			sSum = pack.sSum;
		}
		return *this; // eg��a = a; ���ָ�ֵ���
	}
	// ���ݰ�������캯�������ڰ����ݷ�װ�����ݰ�  // size_t �Ǻ궨�壬�� typedef unsigned int size_t, Ϊ�޷������� 4���ֽ�; BYTE �� unsigned char �ĺ궨��
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize) // para1������  para2������ָ��  para3�����ݴ�С para4:�¼�
	{
		sHead = 0xFEFF;
		nLength = nSize + 2 + 2; // ����2 + ������nSize + ��У��2
		sCmd = nCmd;
		if (nSize > 0)
		{
			strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize); // ��������
		}
		else
		{
			strData.clear(); // ���������
		}

		sSum = 0;
		for (size_t j = 0; j < strData.size(); j++) // ����У��λ
		{
			sSum += BYTE(strData[j]) & 0xFF;
		}
	}

	// ���ݰ�������캯���������е����ݷ��䵽��Ա������
	CPacket(const BYTE* pData, size_t& nSize) // para1������ָ�� para2�����ݴ�С ����ʱ nSize ����������ж��ٸ��ֽڵ����ݣ����ʱ���ص� nSize ���������ڽ���������õ��˶��ٸ��ֽ�
	{
		size_t i = 0; // �������ݰ�
		for (; i < nSize; i++)
		{
			if (*(WORD*)(pData + i) == 0xFEFF) // ʶ���ͷ
			{
				sHead = *(WORD*)(pData + i); // ȡ��ͷ
				i += 2; // WORD Ϊ�޷��Ŷ����ͣ�ռ�����ֽ�,ȡ����ͷ�����ָ�� i ������λ 
				break;
			}
		}
		if (i + 4 + 2 + 2 > nSize) // �����ݿ��ܲ�ȫ������ ��ͷi + ����4 + ����2 + У��2 ����ȫ
		{
			nSize = 0;
			return; // ����ʧ��
		}

		nLength = *(DWORD*)(pData + i); i += 4; // �����ĳ���
		if (nLength + i > nSize) // ��δ��ȫ����
		{
			nSize = 0;
			return; // ����ʧ��
		}

		sCmd = *(WORD*)(pData + i); i += 2; // ��ȡ����

		if (nLength > (2 + 2)) // ���ݵĳ��ȴ��� ����2 + У��2 ˵�����������Ҫ��ȡ������
		{
			strData.resize(nLength - 2 - 2); // ���� string �Ĵ�С
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);  // ��ȡ����
			i += nLength - 2 - 2; // ��ȡ�����ݺ��ƶ�����ָ�� i �����µ�λ��(��ʱ�ƶ���У���뿪ʼ��λ��)
		}

		sSum = *(WORD*)(pData + i); i += 2;  // ��У����
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++)
		{
			sum += BYTE(strData[j]) & 0xFF;
		}

		if (sum == sSum) // У��
		{
			nSize = i; // ͷ + ���� + ���� + ���� + У��   nSize ��������(&)�ķ�ʽ���Σ����ﷵ�ص� nSize �������������õ��˶�������
			return;
		}

		nSize = 0;
	}

	~CPacket() {} // ��������

	int Size() // ����������Ĵ�С
	{
		return nLength + 4 + 2; //  ���������� + ������ + ��У��)nLength + ���nLength�Ĵ�С 4 + ��ͷ 2 
	}
	const char* Data(std::string& strOut) const// ���ڽ��������������뻺���� �������ݰ����������ݸ�ֵ�������Ա strOut
	{
		strOut.resize(nLength + 4 + 2);
		BYTE* pData = (BYTE*)strOut.c_str();
		*(WORD*)pData = sHead;    pData += 2;
		*(DWORD*)pData = nLength;  pData += 4;
		*(WORD*)pData = sCmd;     pData += 2;
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();
		*(WORD*)pData = sSum;

		return strOut.c_str(); // ����������
	}
public:
	WORD sHead;    // ��ͷ �̶�Ϊ FE FF // WORD �Ǻ궨�壬�� typedef unsigned short WORD, Ϊ�޷��Ŷ����� 2���ֽ�
	DWORD nLength; // �������ݵĳ��� // DWORD �Ǻ궨�壬�� typedef unsigned long WORD, Ϊ�޷��ų����� 4���ֽ�(64λ��8���ֽ�) ������ָ���棺�������� + ������ + ��У�� �ĳ���
	WORD sCmd;     // ��������
	std::string strData; // ������
	WORD sSum;     // ��У��
	// std::string strOut; // ������������
};

// ���һ�������Ϣ�����ݽṹ�����ڴ�ſͻ�������������
typedef struct MouseEvent
{
	MouseEvent() // �ṹ��Ĺ��캯�� ����ʵ�����ṹ��ʱ�����ʼ��
	{
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction; // 0��ʾ������1��ʾ˫����2��ʾ���£�3��ʾ�ſ���4��������
	WORD nButton; // 0��ʾ�����1��ʾ�Ҽ���2��ʾ�м���4û�а���
	POINT ptXY; // ���� x y ���� �ṹ������������� x ����� y ����

}MOUSEEV, * PMOUSEEV;


std::string GetErrInfo(int wsaErrCode);// ͨ����������ܹ���ȡ��Ӧ������ɶ��Ĵ�����Ϣ ���� Windows �׽��ִ�����룬������������˴��������صĴ�����Ϣ

// ���ڴ洢�ܿط���ȡ�����ļ���Ϣ
typedef struct file_info
{
	file_info() // �ṹ��Ĺ��캯�� ����ʵ�����ṹ��ʱ�����ʼ��
	{
		IsInvalid = FALSE; // Ĭ����Ч
		IsDirectory = FALSE;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvalid; // ��־�Ƿ���Ч��0-��Ч 1-��Ч
	BOOL IsDirectory; // ��־�Ƿ�ΪĿ¼ 0-����Ŀ¼ 1-��Ŀ¼
	BOOL HasNext; // �Ƿ��к����������ļ��л��ļ� 0-û�� 1-��
	char szFileName[256]; // �ļ�/Ŀ¼ ��

}FILEINFO, * PFILEINFO;

enum {
	CSM_AUTOCLOSE = 1, // CSM = Client Socket Mode �Զ��ر�ģʽ��������һ����������յ�һ��Ӧ�����͹ر��׽���
};

// ����Ϣ�����л��õ��ģ��Զ�������ݽṹ�����ڸ� WM_SEND_PACK ��Ϣ����Ӧ���� SendPack() ����
typedef struct PacketData
{
	std::string strData;  // ����
	UINT        nMode;    // ͨ��ģʽ��Ӧ��һ�ξ͹ر��׽��ֻ��Ƕ��
	WPARAM      wParam;   // ��������
	PacketData(const char* pData, size_t nLen, UINT mode, WPARAM nParam = 0) // Ĭ�Ϲ��캯��
	{
		strData.resize(nLen);
		memcpy((char*)strData.c_str(), pData, nLen);
		nMode = mode;
		wParam = nParam;
	}

	PacketData(const PacketData& data) // �������캯��
	{
		strData = data.strData;
		nMode = data.nMode;
		wParam = data.wParam;
	}

	PacketData& operator=(const PacketData& data) // ���ظ�ֵ�����
	{
		if (this != &data)
		{
			strData = data.strData;
			nMode = data.nMode;
			wParam = data.wParam;
		}
		return *this;
	}

}PACKET_DATA;

class CClientSocket // �ͻ��˵��׽�����
{
public:
	// ʹ�þ�̬��������������Է��ʵ��������˽�г�Ա ���ʷ�ʽ����������::������ �磺CClientSocket::getInstance()
	static CClientSocket* getInstance()
	{
		if (m_instance == NULL) // ��̬����û�� this ָ�룬�����޷�ֱ�ӷ��ʳ�Ա����
		{
			m_instance = new CClientSocket();
		}
		return m_instance;
	}

	bool InitSocket(); // ��ʼ���ͻ���socket����÷�������ַ�Ͷ˿ں����ӵ��������� para1:������ip��ַ para2:�������˿� �ɹ����� ture ʧ�� false
	
	
#define BUFFER_SIZE 4096*1024 // 4M������
	int DealCommand() // ���շ���˷��������ݣ����ҽ����ݷ��뻺����m_packet�� �ɹ����ؽ����������ĳԭ���жϷ���-1
	{
		if (m_sock == -1) return false; // δ��������ɹ�����

		char* buffer = m_buffer.data(); // �ͻ��˻����� new char[BUFFER_SIZE];
		
		// index ����׷�ٻ������д��������ݵ�λ�á�ͨ����������Ϊ��̬���������ں�������֮�䱣����ֵ����������ÿ�κ�������ʱ���³�ʼ����
		// ��������ȷ���ڶ�ε��ú���ʱ��index ���������һ�ε��ý���ʱ��λ�ÿ�ʼ��������ÿ�ζ����㿪ʼ��
		static size_t index = 0; // �˴��þ�̬��Ϊ��������ѭ���б���index��ֵ 

		while (true)
		{
			// Ŀ���ǽ����еİ������뻺����
			size_t len = recv(m_sock, buffer + index, BUFFER_SIZE - index, 0); // ʵ�ʽ��յ��ĳ���
			if (((int)len <= 0) && ((int)index <= 0)) return -1; // 

			// ���
			index += len;  // ��������ȡ�� len ���ֽ����ݣ��� buffer ���� + len
			len = index; // �������������������ݣ��ʽ� index ��ֵ�� len ��Ϊ�������ݸ����� Cpacket ��Ĺ��캯��
			m_packet = CPacket((BYTE*)buffer, len); // ʹ�� CPacket ��Ĺ��캯����ʼ�� ����ʹ�õĽ���Ĺ��캯������ֵ�� m_packet
			if (len > 0) // CPacket�Ľ�����캯�����ص� len �ǽ���������õ������ݣ�> 0 ˵������ɹ�
			{
				// memmove(para1��para2��para3)����һ���ڴ��������ڴ����ƶ����Ը���֮ǰ�Ĳ���   para1��Դ�ڴ��������ʼ��ַ para2��Ŀ���ڴ��������ʼ��ַ para3��Ҫ�ƶ����ֽ��� 
				memmove(buffer, buffer + len, index - len); // ������������ݸ��ǣ������»�������������������������
				index -= len; // ������������ len ���ֽ����ݣ��� buffer ���� - len
				return m_packet.sCmd; // ���ճɹ������ؽ��յ�������
			}

		}
		return -1; // �����ж���շ��� -1 
	}

	bool GetFilePath(std::string& strPath)
	{
		if ((m_packet.sCmd == 2) || (m_packet.sCmd == 3) || (m_packet.sCmd == 4)) // ���2����ʾ�鿴ָ��Ŀ¼�µ��ļ� ��3��ִ���ļ� '4'�����ļ�
		{
			strPath = m_packet.strData; // ���ذ��е����ݣ����ļ�·��
			return true;
		}

		return false;
	}

	bool GetMouseEvent(MOUSEEV& mouse)
	{
		if (m_packet.sCmd == 5)
		{
			memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEV)); // �洢�ͻ��˷���������¼�
			return true;
		}
		return false;
	}

	CPacket& GetPacket()
	{
		return m_packet;
	}

	void CloseSocket()
	{
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
	}

	void UpdateAddress(int nIP, int nPort) // ֪ͨClientSocket��������UpdateAddress()�����ڸ���Զ�̷����IP�Ͷ˿�
	{
		if ((m_nIP != nIP) || (m_nPort != nPort))
		{
			m_nIP = nIP;
			m_nPort = nPort;
		}
	}

	// bool SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks, bool isAutoClosed = true); // �¼����ƽ��ͨ��
	bool SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClosed = true, WPARAM wParam = 0); // ��Ϣ���ƽ��ͨ�ų�ͻ
	

private:

	typedef void(CClientSocket::* MSGFUNC)(UINT uMsgm, WPARAM wParam, LPARAM lParam); // �Զ����������� MSGFUNC ����ص���Ϣ��Ӧ����(SendPack())��ָ��
	std::map<UINT, MSGFUNC> m_mapFunc; // ��Ϣ��Ӧ����ӳ�������ϢID����Ӧ����Ӧ������ӳ��
	UINT m_nThreadID; // ͨ���߳�ID
	HANDLE m_eventInvoke; // ����֪ͨͨ���߳��������¼�
	HANDLE m_hThread; // ͨ���߳̾��
	
	bool m_bAutoClose;
	std::list<CPacket> m_lstSend; // �����͵İ�
	std::map<HANDLE, std::list<CPacket>&> m_mapAck; // HANDLE + Զ�̷�����Ӧ�����list
	std::map<HANDLE, bool> m_mapAutoClosed; // HANDLE + �Ƿ��Զ��ر��׽���ͨ��(����Ӧ������ͨ��һ�μ�����ɻ�����Ҫͨ�Ŷ�Σ��ļ������Ҫ��Σ�)
	std::mutex m_lock;
	int				  m_nIP;    // Զ�̷�������IP��ַ
	int				  m_nPort;  // Զ�̷������Ķ˿�
	SOCKET            m_sock;   // �����ͻ��� socket ����
	CPacket           m_packet; // �������ݰ���ĳ�Ա����
	std::vector<char> m_buffer; // �ͻ��˻�����

	// �� CServerSocket ��Ĺ��캯�������ƹ��캯���Լ���ֵ�����������Ϊ˽��
	// ����ζ����ֹ����ĸ��ƺ͸�ֵ����������Ϊ��ȷ���ڸ����ʵ���в��ᷢ������ĸ��ƺ͸�ֵ��ͨ������Ϊ�׽��ֵ���Դ������Ҫ���⴦�����ʺ�ͨ��Ĭ�ϵĸ��ƺ͸�ֵ����
	CClientSocket& operator=(const CClientSocket& ss) // ��ֵ�����
	{
	}
	CClientSocket(const CClientSocket& ss); // ���ƹ��캯��
	CClientSocket(); // Ĭ�Ϲ��캯��������InitSockEnv()��ʼ�����绷��

	// ��������
	~CClientSocket() {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		WSACleanup(); // �������� Winsock �����Դ������һ���ڲ�����Ҫʹ�� Winsock ʱ���е��������
	}

	// ��ʼ�����绷��������1.1�汾�����; ��ʼ���ɹ�����TRUE,ʧ�ܷ���FALSE
	BOOL InitSockEnv()
	{
		// �����д����������� Windows ��ʹ�� Winsock API ���������̵ĳ�ʼ������
		WSADATA data; // ������һ����Ϊ data �ı���������Ϊ WSADATA��WSADATA ��һ���ṹ�����ڴ洢�й� Windows Sockets ʵ�ֵ���Ϣ
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0)  // WSAStartup ������ʼ�� Winsock �⡣MAKEWORD(1, 1) ������ָ��������� Winsock �汾������������汾 1.1
		{
			return FALSE; // ��ʼ�����绷��ʧ��
		}
		return TRUE; // ��ʼ�����绷���ɹ�
	}

	

	static CClientSocket* m_instance;

	static void releaseInstance()
	{
		if (m_instance != NULL)
		{
			CClientSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp;
		}
	}

	// ����ģʽ����һ��������ģʽ�������˼����ȷ������Ӧ�ó��������������ֻ��һ��ʵ�������ṩһ��ȫ�ֵķ��ʵ㡣
	//         ����ζ�����ۺ�ʱ�ε�����������ʵ��������������ͬ�Ķ��󡣵���ģʽͨ�����ڹ���ȫ����Դ������������Ϣ�������ṩΨһ�ķ��ʵ�������ĳЩ��Դ
	// �������󣺵���ģʽ�µ���ʵ������Ϊ�����������������Ӧ�ó�����ֻ��һ��ʵ�����ڣ������ṩ��һ��ȫ�ֵķ��ʵ㣬���������ֵĴ�����Ի�ȡ�����Ψһ��ʵ��

	// CHelper ����һ��Ƕ���ࣨnested class������������Ϊ CServerSocket ���˽���ڲ���
	// CHelper ����������� CServerSocket ������������ڴ���һ������������ȷ�� CServerSocket ��ĵ���ģʽ��ʵ��
	class CHelper
	{
	public:
		CHelper() // ���캯�������ڶ��󴴽�ʱ���� CServerSocket::getInstance() ��̬��������Ϊ CHelper ���� CServerSocket ���Ƕ���࣬���������Է��� CServerSocket ���˽�г�Ա�;�̬��Ա
		{         // ������ CHelper ��Ķ���ʱ�������Զ����ù��캯�����Ӷ����ڲ����� CServerSocket::getInstance() ������
			CClientSocket::getInstance();
		}
		~CHelper() // �������������ڶ�������ʱ���� CServerSocket::releaseInstance() ��̬������ͬ������Ϊ CHelper ���� CServerSocket ���Ƕ���࣬���������Է��� CServerSocket ���˽�г�Ա�;�̬��Ա
		{		   //  CHelper ��Ķ����뿪������ʱ�������Զ����������������Ӷ����ڲ����� CServerSocket::releaseInstance() ����
			CClientSocket::releaseInstance();
		}
	};
	// CHelper ��Ĺ��캯���������������ڶ���Ĵ���������ʱ�Զ������ã����Խ� m_helper ����Ϊ��̬��Ա����ȷ���� CServerSocket �������������ʼ�մ���һ�� CHelper ��Ķ���
	// �����Ϳ���ȷ���ڳ���ִ�й�����ʼ�մ���һ�� CServerSocket ��ĵ������󣬴Ӷ�ʵ���˵���ģʽ��Ч��
	static CHelper m_helper; // m_helper �� CHelper ��ľ�̬��Ա�������� CServerSocket ��������Ϊ static

	// ������߳���Ϊ�˽��������MVC�ܹ��ع��ͻ��˺�Զ���������ڱ��ؿͻ��˵�һ���̣߳�����¼�������һ���̣߳����̲߳������������Զ�̷����������ͨ�ų�ͻ����
	// �����ͻ����Ϊ:��һ�������ʼ��socket�����շ�����Ӧ�𣬻�δ�����꣬��һ��������initsocket���Ὣ��һ������ͨ�ŵ��׽��ֹر�
	//              ͬʱ�������ڽ���Ӧ����Ļ�����������ܴ��ڷ�����Ӧ����һ������Ĳ��ְ���Ӧ��ǰ����İ����޷�������
	// ����������ĳ������������¼�����:
	// 1.���ͻ��˷��͵Ķ������ŵ�һ�����е���
	// 2.���洴�����̸߳������δ�������е�������Ҫ�Ƿ��Ϳ��������Զ�̷�������Ȼ�����Զ�̷��������ص����ݰ�
	// 3.�����굱ǰ����ص����ݰ���ͨ�� hEvent ��֪������������̣߳���ǰ�������������
	// 4.������������е���һ������
	// ���ֻ��Ʊ�֤���ڴ�����(��������->����Ӧ��)��һ������֮ǰ�����ᷢ�͵�ǰ����������Զ�̷������������˶��̷߳�������ĳ�ͻ����
	static unsigned __stdcall threadEntry(void* arg); // �߳���ں���
	void threadFunc(); // ʵ��ִ�е��̺߳��������ڽ��շ���˴����������ݰ�

	// �����������ֻ���Ҳ���ã�ʵ���������鷳���ʽ������߳��¼������ع�Ϊ��Ϣ���� ---240408
	// �¼����ƴ��ڵ����⣺
	// 1. ����Ի���Ҫʹ������ģ�飺watchdlg remotedlg controller �ļ�����... 
	// 2. ����ÿ���Ի��������ģ�鷢��ͨ������ʱ����Ҫ������ģ�齨���¼���������
	// 3. ���¼��������������1��1�ģ���ÿ���Ի���֮������ٶ�����ϵ
	// 4. ���������ʹ���¼�����ʵ������ͬ���ģ������ļ����أ��Ի����Ҫһֱ�����ȴ�����ģ�齫event����Ϊ���ź�״̬
	// 5. ��������Ϣ���ƾͿ���ʵ���첽���Ի�������Ϣ������ģ�鴦������ٷ���Ӧ����Ϣ
	// �µĽ�����̲߳�����ͻ����Ļ��ƣ�
	// 1.�ͻ���dlg��������Ϣ(SendMessage)������ģ�飬����ģ���յ���������SendPack()���ͨ�ţ�Ȼ����Ӧ����Ϣ��dlg(SendMessage)
	// 2.���������߳��¼����ƣ��ͻ��˺�����ģ��֮��Ҫʱʱ�̱̿�֤����ͬ��(ͨ��hEvent)�����鷳��������ûص��ķ�ʽ�Ͳ���ʱʱ�̱̿�֤����ͬ����
	// 3.�ͻ���dlg���͵�������Ϣ������һ����Ϣ�����ݽṹ�����ݺ����ݳ��ȣ�����һ��Ӧ��͹رջ��ǽ��ն��Ӧ���ٹرգ� �ص���Ϣ�����ݽṹ��HWND MESSAGE��
	void threadFunc2();
	void SendPack(UINT nMsg, WPARAM wParam, LPARAM lParam); // WM_SEND_PACK ��Ϣ����Ӧ������ ��Ҫ���ڷ������ݵ�����ˣ�Ȼ����մӷ���˷��ص���Ϣ������SendMessage()����Ӧ�Ľ���ص�������

	
	bool Send(const char* pData, int nSize) // para1:������Ϣ�Ļ�����ָ�� para2:��Ϣ��С �ɹ����� true ʧ�ܷ��� false
	{
		if (m_sock == -1) return false; // δ��ͻ��˽�������

		return send(m_sock, pData, nSize, 0) > 0;
	}
	bool Send(const CPacket& pack); // para1:�Զ������ݰ������ô���  �ɹ����� true ʧ�ܷ��� false



};

// extern �ؼ��ֱ�ʾ�ñ����������ļ��ж��壬���ڵ�ǰ�ļ��н���Ϊ������ʵ�ʵĶ�������������ļ��У����� server �ͱ����һ��ȫ�ֱ����������������Ƕ���
// ����������Դ�ļ��ڰ������ͷ�ļ�(ServerSocket.h)ʱ�����ܹ����ʵ� server ���ȫ�ֱ����������������������ظ�����Ĵ���
// ������Դ�ļ��У������Ҫʹ�� server������ͨ��������ͬ��ͷ�ļ�����ȡ�� server �������������ᵼ�¶�ζ��塣
// Ȼ��ʵ�ʵĶ���Ӧ�ó�����һ��Դ�ļ��У�ȷ��ֻ��һ���ط��� server ������ʵ�ʵķ���ͳ�ʼ��
// ����������Ϊ��֧��ģ�黯��̣�����ͬ��Դ�ļ�����ͬһȫ�ֱ������������ͻ
extern CClientSocket server; // ��Ϊ RemoteCtrl.cpp ��ֻ������ ServerSocket.h,Ϊ������ɷ��� ServerSocket.cpp �еı��������ｫ server ����Ϊȫ�ֱ���






