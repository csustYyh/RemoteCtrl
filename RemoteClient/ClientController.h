#pragma once
#include "ClientSocket.h"
#include "RemoteClientDlg.h"
#include "CWatchDialog.h"
#include "StatusDlg.h"
#include <map>
#include "resource.h"
#include "EdoyunTool.h"

// �Զ�����Ϣ�����̣�
// 1.�Զ�����Ϣ ID .h
// 2.�����Զ�����Ϣ��Ӧ���� .h
// 3.����Ϣӳ�����ע����Ϣ .cpp
// 4.ʵ����Ϣ��Ӧ���� .cpp
//#define WM_SEND_PACK   (WM_USER + 1) // �������ݰ�����ϢID 
// #define WM_SEND_DATA   (WM_USER + 2) // �������ݵ���ϢID 
#define WM_SHOW_STATUS (WM_USER + 3) // չʾ״̬����ϢID 
#define WM_SHOW_WATCH  (WM_USER + 4) // Զ�̼�ص���ϢID 
#define WM_SEND_MESSAGE (WM_USER + 0x1000) // �Զ�����Ϣ�������ϢID

class CClientController
{
public:
	static CClientController* getInstance(); // ���ڻ�ȡ�������ȫ��Ψһ��������
	int InitController(); // ��ʼ�� ����Ὺ��һ���߳�������Ϣѭ��
	int Invoke(CWnd*& pMainWnd); // ���� ����Ϊָ�� CWnd ����ָ�������
	
	//LRESULT SendMessage(MSG msg); // ������Ϣ �������Լ�����ĺ����������� Windows API �е� SendMessage �����������������ȵ����Զ���ĺ����������Ҫ���� Windows API �е� SendMessage ��������Ҫʹ�������������ռ���ָ�������� ::SendMessage
	//LRESULT OnSendPack(UINT msg, WPARAM wParam, LPARAM lParam); // ��Ϣ��Ӧ���� ���ڴ��� WM_SEND_PACK ��Ϣ
	//LRESULT OnSendData(UINT msg, WPARAM wParam, LPARAM lParam); // ��Ϣ��Ӧ���� ���ڴ��� WM_SEND_PACK ��Ϣ
	LRESULT OnShowStatus(UINT msg, WPARAM wParam, LPARAM lParam); // ��Ϣ��Ӧ���� ���ڴ��� WM_SHOW_STATUS ��Ϣ
	LRESULT OnShowWatch(UINT msg, WPARAM wParam, LPARAM lParam); // ��Ϣ��Ӧ���� ���ڴ��� WM_SHOW_WATCH ��Ϣ
	// ��������������ĵ�ַ�Ͷ˿�(֪ͨCClientScoket��������UpdateAddress())
	void UpdataAddress(int nIP, int nPort)
	{
		CClientSocket::getInstance()->UpdateAddress(nIP, nPort);
	}
	// ��������(֪ͨCClientScoket��������DealCommand())
	int DealCommand()
	{
		return CClientSocket::getInstance()->DealCommand(); // ����ͻ��˷���������
	}
	// �ر��׽���(֪ͨCClientScoket��������CloseSocket())
	void CloseSocket()
	{
		CClientSocket::getInstance()->CloseSocket(); // �ر��׽���
	}
	// �����������ݰ�(֪ͨCClientSocket��������SendPacket()�����������ݰ�)
	// para1:����ͨ������Ĵ��� para2:���� para3:������Ϻ��Ƿ��Զ��ر��׽���
	// para4:��Ҫ���͵����ݰ� para5:���ݳ��� para6:���ڴ����ļ���ָ�룬�����ļ�����ʱʹ�ã��������Ϊ0
	// ����ֵ:�ɹ����� true�� ʧ�ܷ��� false
	bool SendCommandPack(HWND hWnd, int nCmd, bool bAutoClose = true, BYTE* pData = NULL, size_t nLength = 0, WPARAM wParam = 0); // ��Ϣ���ƽ��ͨ�ų�ͻ
	

	// int SendCommandPacket(int nCmd, bool bAutoClose = true, BYTE* pData = NULL, size_t nLength = 0, std::list<CPacket>* plstPacks = NULL); // �¼����ƴ���ͨ�ų�ͻ

	
	// ���ڴ��ȡԶ������ͼƬ���ݣ�����ʹ�ù��߿�CTool���ֽ�ת��ΪͼƬ��ʽ����
	int GetImage(CImage& image)
	{
		CClientSocket* pClient = CClientSocket::getInstance();
		// ���ù������еĺ�����ʵ���ڴ����ݵ�ͼƬ��ת��
		// ��Ϊ�ڴ����ݵ�ͼƬ��ת���Ǹ������ܺ������������Ŀ����ҵ���߼�û��ֱ�ӹ��������Է�װ��
		return CEdoyunTool::Bytes2Image(image, pClient->GetPacket().strData);
	}
	// ����CfileDialog������ȡ���ر���·��������SendCommandPack()���������������ݰ������ù��λ�ú͵ȴ������������ش���
	// para:Ŀ���ļ���·�� ����ֵ���ɹ����� 0 ʧ�ܷ��� -1
	int DownFile(CString strPath);
	void DownloadEnd();

	// ����Զ��ͼ���̣߳�Ȼ����ʾͼ������
	void StartWatchScreen();
protected: 
	// void threadDownloadFile(); // ʵ��ִ�е��ļ������̺߳���������Զ�������ļ�
	// static void threadDownloadEntry(void* arg); // �ļ������߳���ں���
	void threadWatchScreen(); // ʵ��ִ�е�Զ�������̺߳���������Զ�������ļ�
	static void threadWatchScreenEntry(void* arg); // Զ�������߳���ں���

	CClientController():
		m_statusDlg(&m_remoteDlg), // ��Ա��ʼ���б�
		m_watchDlg(&m_remoteDlg)   // ��Ա��ʼ���б�
	{ // ���캯�� ��ʼ����Ա������ָ��Զ�����桢���ش��ڵĸ�����Ϊ�����洰��
		m_hThread = INVALID_HANDLE_VALUE;
		m_nThreadID = -1;
		//m_hThreadDownload = INVALID_HANDLE_VALUE;
		m_hThreadWatch = INVALID_HANDLE_VALUE;
		m_isClosed = true;
	}
	~CClientController() { // ��������
		WaitForSingleObject(m_hThread, 100);
	}
	
	// Ϊʲô�̺߳�������Ϊ��̬��Ա��������ȫ�ֺ���
	// �� Windows ����У��̺߳�������Ϊ��̬������������Ϊ�̺߳�����Ҫ���� C ���Եĺ���ָ���Ҫ�󣬶� C ���Եĺ���ָ�벻֧�ַǾ�̬��Ա������
	// ��ˣ������ʹ�� C++ ��д Windows �����̺߳���������ȫ�ֺ�����������ľ�̬��Ա�������������ǷǾ�̬��Ա������
	// ������Ϊ��̬��Ա������ȫ�ֺ����ĵ��÷�ʽ�����Ƶģ����Ƕ�����Ҫ������ this ָ�룬���Ǿ�̬��Ա������Ҫͨ��������ã�������Ҫ������ this ָ�롣
	// �����̺߳�����ͨ������ָ�������õģ�������ָ���޷����������� this ָ�룬��˷Ǿ�̬��Ա��������ֱ����Ϊ�̺߳�����

	// �����̴߳���ҵ���߼������̣�
	// 1.��ȫ�ַ��� _beginthread() ���õ��̺߳��� threadEntry() ����Ϊ��̬��ͬʱ����thisָ����Ϊ����
	// 2.��ʵ��Ҫ���߳���ִ�е�ҵ���߼����� threadFunc() ����Ϊ�Ǿ�̬
	// 3.��threadEntry() ʹ��thisָ����� threadFunc()��������ʵ��������һ���߳�ִ��ҵ���߼�
	void threadFunc(); // ʵ��ִ�е��̺߳��� ���ڽ���CClientController����Ϣѭ������
	static unsigned __stdcall threadEntry(void* arg); // ��Ϣѭ���߳���ں��� Ϊ�����õ�thisָ�������ָ����̺߳��� �̺߳�����Ҫ����Ϊ��̬��ȫ��
	static void releaseInstance()
	{
		if (m_instance != NULL)
		{
			delete m_instance;
			m_instance = NULL;
			//CClientController* tmp = m_instance;
			//m_instance = NULL;
			//delete tmp;
		}
	}

private:

	typedef struct MsgInfo
	{
		MSG     msg;    // ��Ϣ
		LRESULT result; // ���ڼ�¼��Ϣ�����ķ���ֵ
		MsgInfo(MSG m) // ���캯��
		{
			result = 0;
			memcpy(&msg, &m, sizeof(MSG));
		}
		MsgInfo& operator=(const MsgInfo& m) // ���ظ�ֵ�����
		{
			if (this != &m)
			{
				result = m.result;
				memcpy(&msg, &m.msg, sizeof(MSG));
			}
			return *this;
		}
		MsgInfo(const MsgInfo& m) // ���ƹ��캯��
		{
			result = m.result;
			memcpy(&msg, &m.msg, sizeof(MSG));
		}
	} MSGINFO;

	// ��Ϣӳ���
	typedef LRESULT(CClientController::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParm); // MSGFUNC��Ա����ָ�� 
	static std::map<UINT, MSGFUNC> m_mapFunc; // ��Ա����ӳ��� ����ϢID����Ӧ�Ĺ��ܺ�����ӳ�� ����ʹ��������Ϊ����������Ӧ�ĳ�Ա����ָ��

	// ������ʵ��
	static CClientController* m_instance; // ��������
	// �����Ľ��ͣ�https://www.zhihu.com/question/639483371/answer/3383218426
	class CHelper
	{
	public:
		CHelper() 
		{         
			// CClientController::getInstance();
		}
		~CHelper() 
		{		    
			CClientController::releaseInstance();
		}
	};
	static CHelper m_helper; 

	// �ع������жԻ����ǿ�����ĳ�Ա
	CWatchDialog		m_watchDlg;  // Զ������Ի���
	CRemoteClientDlg	m_remoteDlg; // ���Ի���
	CStatusDlg			m_statusDlg; // ״̬�Ի���

	// �߳����
	HANDLE	   m_hThread;      // ��Ϣѭ���߳̾��
	HANDLE     m_hThreadWatch; // Զ�������߳̾��
	//HANDLE     m_hThreadDownload; // �����ļ��߳̾��
	unsigned   m_nThreadID;    // ��Ϣѭ���߳�ID

	// �������
	CString    m_strRemote; // Զ�������ļ�·��
	CString    m_strLocal;  // ���ر����ļ�·��

	// ��־���
	//bool       m_isFull;  // ���ڱ�ʾ�����ͷ������� true �У� false û��
	bool       m_isClosed; // ���ڱ�ʾ�߳��Ƿ�ر� true �رգ� false δ�ر�
};

