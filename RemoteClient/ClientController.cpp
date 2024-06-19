#include "pch.h"
#include "ClientController.h"
#include "ClientSocket.h"

std::map<UINT, CClientController::MSGFUNC> CClientController::m_mapFunc; // .h�ļ���������δ��ʼ���ľ�̬��Ա��������.cpp�ļ�����Ҫ����ͳ�ʼ��

// �����Ľ��ͣ�https://www.zhihu.com/question/639483371/answer/3383218426
CClientController* CClientController::m_instance = NULL; 

CClientController::CHelper CClientController::m_helper;

CClientController* CClientController::getInstance()
{
	if (m_instance == NULL) // ��̬����û�� this ָ�룬�����޷�ֱ�ӷ��ʳ�Ա����
	{
		m_instance = new CClientController();

		// ��ʼ����Ϣӳ���
		struct
		{
			UINT     msg;  // ��ϢID
			MSGFUNC  func; // ��Ӧ����Ӧ����
		}MsgFuncs[] =
		{
			//{WM_SEND_PACK, &CClientController::OnSendPack},
			//{WM_SEND_DATA, &CClientController::OnSendData},
			{WM_SHOW_STATUS, &CClientController::OnShowStatus}, 
			{WM_SHOW_WATCH, &CClientController::OnShowWatch},

			{(UINT)-1, NULL}, // ��Ч
		};
		for (int i = 0; MsgFuncs[i].msg != -1; i++)
		{
			m_mapFunc.insert(std::pair<UINT, MSGFUNC>(MsgFuncs[i].msg, MsgFuncs[i].func));
		}
	}
	return m_instance;
}

int CClientController::InitController()
{
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientController::threadEntry, this, 0, &m_nThreadID);
	m_statusDlg.Create(IDD_DLG_STATUS, &m_remoteDlg); // ����״̬�Ի��� para1:MFC�е���ԴID para2:�����ڵ�ָ��
	return 0;
}

int CClientController::Invoke(CWnd*& pMainWnd)
{
	pMainWnd = &m_remoteDlg; // �� m_remoteDlg ����ĵ�ַ��ֵ������� pMainWnd ָ��
	return m_remoteDlg.DoModal(); // ��ʾģ̬�Ի��򣬲��ȴ��Ի���ر�
}

//LRESULT CClientController::SendMessage(MSG msg)
//{
//	MSGINFO info(msg);
//	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL); // ����һ���¼����� para2:�ֶ���λ para3����ʼ״̬�Ƿ� signaled �ġ�����ζ���κεȴ�����¼����߳����¼�������ʽ������Ϊ signaled ֮ǰ���ᱻ����
//	if (hEvent == NULL) return -2;// �¼�����ʧ��
//
//	// ����һ��WM_SEND_MESSAGE��Ϣ������߳�(������Ϣѭ�����߳�) ����������MSGINFO�ṹ��(MSG + result)��һ���¼�
//	PostThreadMessage(m_nThreadID, WM_SEND_MESSAGE, (WPARAM)&info, (LPARAM)hEvent); // ��һ���Զ�����Ϣ���͸���ʶΪ m_nThreadID ���߳�
//	WaitForSingleObject(hEvent, INFINITE); // ��windows�£�ͨ���¼���ʵ���̼߳��ͬ��
//	CloseHandle(hEvent);
//	return info.result;
//}

// ��Ϊ��Ϣ���ƺ��������������ļ����صĺ���������Ҫ���ļ�����ת�Ƶ���CRemoteDlg(view��)��ִ�У�������������û��ʵ��MVC�ܹ���---0409
//void CClientController::threadDownloadFile()
//{
//	// �����ļ������ڱ���������ļ�
//	FILE* pFile = fopen(m_strLocal, "wb+");
//	if (pFile == NULL)
//	{
//		// �� C++ �У�_T("����") �Ǻ궨���һ���÷��������� Unicode �� ANSI �ַ���֮�����ת�����Ա��ڲ�ͬ�ı���ģʽ�£�Unicode �� ANSI��ʹ����ͬ�Ĵ���
//		// _T("����")���� Unicode ����ģʽ�±�չ��Ϊ L"����"���� ANSI ����ģʽ�±�չ��Ϊ "����"��
//		// �� Unicode ����ģʽ�£�L ǰ׺��ʾ�ַ����� Unicode ����ģ�ÿ���ַ�ռ�����ֽڣ�
//		// ���� ANSI ����ģʽ�£��ַ�������ǰ׺����ʾʹ�ñ��ص� ANSI �ַ�����ÿ���ַ�ռһ���ֽ�
//		AfxMessageBox(_T("����û��Ȩ�ޱ�����ļ��������ļ��޷�������")); // _T("����") �������Ǹ��ݱ���ģʽѡ����ȷ���ַ������ͣ���ȷ������ļ����� 
//		m_statusDlg.ShowWindow(SW_HIDE);
//		m_remoteDlg.EndWaitCursor();
//		return;
//	}
//	CClientSocket* pClient = CClientSocket::getInstance();
//	do {
//		// �����ļ����������M��(CClientSocket��)
//		int ret = SendCommandPack(m_remoteDlg, 4, false, (BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength(), (WPARAM)pFile);
//		// 03 ���������ش������ļ��Ĵ�С
//		// ���������յ������ļ�����󷵻����ĵ�һ������ .strData �������ļ��Ĵ�С����
//		long long nLength = *(long long*)CClientSocket::getInstance()->GetPacket().strData.c_str(); // �������ļ��ĳ���
//		if (nLength == 0)
//		{
//			AfxMessageBox("�ļ�����Ϊ0�����޷���ȡ�ļ���");
//			break;
//		}
//
//		// 04 ���շ��������͹������ļ������ر��浽GetPathName()·������
//		long long nCount = 0; // ��¼�ͻ����ѽ����ļ��Ĵ�С
//		while (nCount < nLength)
//		{
//			ret = pClient->DealCommand();
//			if (ret < 0)
//			{
//				AfxMessageBox("�ļ�����Ϊ0�����޷���ȡ�ļ���");
//				TRACE("����ʧ�ܣ� ret = %d\r\n", ret);
//				break;
//			}
//			fwrite(pClient->GetPacket().strData.c_str(), 1, pClient->GetPacket().strData.size(), pFile);
//			nCount += pClient->GetPacket().strData.size();
//		}
//	} while (false);
//	fclose(pFile);
//	pClient->CloseSocket();
//	m_statusDlg.ShowWindow(SW_HIDE);
//	m_remoteDlg.EndWaitCursor();
//	m_remoteDlg.MessageBox(_T("������ɣ�"), _T("���"));
//
//}
//
//void CClientController::threadDownloadEntry(void* arg)
//{
//	CClientController* thiz = (CClientController*)arg;
//	thiz->threadDownloadFile();
//	_endthread();
//}

void CClientController::threadWatchScreen()
{
	Sleep(50);
	ULONGLONG nTick = GetTickCount64();
	while (!m_isClosed) // ��ǰ���ڻ�����ʱ
	{
		if (m_watchDlg.isFull() == false) // ���·����������Ľ������ݵ����� ������û����ȡ����(����ʱ��û������������ʾ��Զ��dlg��)����֡
		{
			if (GetTickCount64() - nTick < 200) // ��֤Զ���������������Ƶ��Ϊ200ms/��  ��1s��ʾ5֡ (5FPS)
			{
				Sleep(200 - DWORD(GetTickCount64() - nTick));
			}
			nTick = GetTickCount64();
			int ret = SendCommandPack(m_watchDlg.GetSafeHwnd(), 6, false, NULL, 0); // ����Զ����������
			// TODO:���WM_SEND_PACK_ACK�����Ϣ����Ӧ����
			if (ret != 1)
			{
				TRACE("��ȡͼƬʧ�ܣ�ret = %d\r\n", ret);
			}
		}
		Sleep(1);
	}
}

void CClientController::threadWatchScreenEntry(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadWatchScreen();
	_endthread();
}

bool CClientController::SendCommandPack(HWND hWnd, int nCmd, bool bAutoClose, BYTE* pData, size_t nLength, WPARAM wParam)
{
	CClientSocket* pClient = CClientSocket::getInstance();
	bool ret = pClient->SendPacket(hWnd, CPacket(nCmd, pData, nLength), bAutoClose, wParam); // �����������ݰ�
	return ret;
}

/* // �¼����ƴ���ͨ�ų�ͻ
int CClientController::SendCommandPacket(int nCmd, bool bAutoClose, BYTE* pData, size_t nLength,
	std::list<CPacket>* plstPacks)
{
	CClientSocket* pClient = CClientSocket::getInstance();
	// ��ָ��Ͷ�����
	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	
	std::list<CPacket> lstPacks; // �е��������Ӧ������������һ��list���Ӧ����
	if (plstPacks == NULL) // �������plstPacksΪNULL��˵�����������Ӧ��
		plstPacks = &lstPacks;
	pClient->SendPacket(CPacket(nCmd, pData, nLength, hEvent), *plstPacks, bAutoClose);
	CloseHandle(hEvent); // �����¼���� ��ֹ��Դ�ľ�
	if (plstPacks->size() > 0) // ��Ӧ����
	{
		// ���ܿͻ��˷���������ز�����Ӧ���������Ƕ�ֱ�ӷ���ָ���
		// ��Ϊ������Ӧ�������ڶ�Ӧ��ҵ���߼�����SendCommandPacket()ʱ���ͻᴫ��plstPacks 
		// �磺threadWatchScreen Զ�������������͹���Ӧ��������һЩ��������������Ӧ����
		return plstPacks->front().sCmd; // ����Ӧ�����ָ��
	}
	// int cmd = DealCommand();
	// if (bAutoClose)
		// CloseSocket();
	// return cmd;
	return -1;
}
*/

int CClientController::DownFile(CString strPath)
{
	// strPath = "C:\\Users\\Yyh\\Documents\\RemoteCtrl.pdb";
	// ������һ���ļ��Ի���
		// para1:��ʾ�ļ��Ի������������ļ��������Ǳ����ļ������ֵΪ TRUE�����ʾ���������ļ�
		// para2:��ʾ�ļ��Ի�����ʾ�������͵��ļ�, Ҳ���Դ����ض����ļ���չ������ "*.txt"����ʾֻ��ʾ .txt ��ʽ���ļ�
		// para3:����ָ���ļ��Ի�������ʾ��Ĭ���ļ��� para4:ָ�����ļ��Ի������û�ָ�����ļ��Ѿ�����ʱ�Ƿ���ʾ������ʾ,����Ĳ�����ʾ�ᵯ����ʾ��ѯ���û��Ƿ񸲸�
		// para5:ָ���˶Ի���Ĺ�������ͨ�����������û�����ѡ����ļ�����,���� para2 ʹ�� "*" ������ʾ��ʾ�������͵��ļ�����˹�����Ϊ�գ���ʾ�������ļ�����
		// para6:���������ʾ�Ի���ĸ����ڣ�ͨ����ָ��ǰ���ڵ�ָ��
	CFileDialog dlg(FALSE, NULL, strPath, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, NULL, &m_remoteDlg); // ������һ���ļ��Ի���
	// ��ʾ�ļ��Ի���ģ̬�������ȴ��û��Ĳ�����ֱ���û��رնԻ���Ϊֹ
	// DoModal()����ͣ��ǰ�����ִ�У�ֱ���û���ɶԻ���Ĳ���������ѡ����һ���ļ�������˶Ի����еĴ򿪰�ť�����ߵ����ȡ����ť����Ȼ�󷵻��û��Ĳ������
	// ����ֵ��ʾ�û��Ĳ�����������õķ���ֵ�� IDOK���û������ȷ����ť���� IDCANCEL���û������ȡ����ť����
	if (dlg.DoModal() == IDOK) // �û����ȷ��
	{
		m_strRemote = strPath; // Զ�������ļ�·��
		m_strLocal = dlg.GetPathName(); // ���ر����ļ�·��

		FILE* pFile = fopen(m_strLocal, "wb+"); // ���ض����Ʒ�ʽ���ļ���������д���ļ���Ӳ����
		if (pFile == NULL)
		{
			// �� C++ �У�_T("����") �Ǻ궨���һ���÷��������� Unicode �� ANSI �ַ���֮�����ת�����Ա��ڲ�ͬ�ı���ģʽ�£�Unicode �� ANSI��ʹ����ͬ�Ĵ���
			// _T("����")���� Unicode ����ģʽ�±�չ��Ϊ L"����"���� ANSI ����ģʽ�±�չ��Ϊ "����"��
			// �� Unicode ����ģʽ�£�L ǰ׺��ʾ�ַ����� Unicode ����ģ�ÿ���ַ�ռ�����ֽڣ�
			// ���� ANSI ����ģʽ�£��ַ�������ǰ׺����ʾʹ�ñ��ص� ANSI �ַ�����ÿ���ַ�ռһ���ֽ�
			AfxMessageBox(_T("����û��Ȩ�ޱ�����ļ��������ļ��޷�������")); // _T("����") �������Ǹ��ݱ���ģʽѡ����ȷ���ַ������ͣ���ȷ������ļ����� 
			return -1;
		}
		SendCommandPack(m_remoteDlg, 4, false, (BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength(), (WPARAM)pFile);
		//SendCommandPack(m_remoteDlg, 4, false, (BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength(), (WPARAM)pFile); // ����
		// m_hThreadDownload = (HANDLE)_beginthread(&CClientController::threadDownloadEntry, 0, this); // �����߳�ִ�������ļ�����
		/*if (WaitForSingleObject(m_hThreadDownload, 0) != WAIT_TIMEOUT)
		{
			return -1;
		}*/
		m_remoteDlg.BeginWaitCursor(); // ���������ڹ��Ϊ�ȴ�״̬(ɳ©)
		m_statusDlg.m_info.SetWindowText(_T("��������ִ���У�"));
		m_statusDlg.ShowWindow(SW_SHOW);
		m_statusDlg.CenterWindow(&m_remoteDlg); // ���������ھ���
		m_statusDlg.SetActiveWindow();
	}
	return 0;
}

void CClientController::DownloadEnd()
{
	m_statusDlg.ShowWindow(SW_HIDE);
	m_remoteDlg.EndWaitCursor();
	m_remoteDlg.MessageBox(_T("������ɣ�"), _T("���"));
}

void CClientController::StartWatchScreen()
{
	m_isClosed = false;
	//m_watchDlg.SetParent(&m_remoteDlg); // Զ������dlg parent����Ϊ������
	m_hThreadWatch = (HANDLE)_beginthread(&CClientController::threadWatchScreenEntry, 0, this);
	m_watchDlg.DoModal(); // ����Ϊģ̬��ֱ���û���ɶԻ���Ĳ�����
	m_isClosed = true;
	WaitForSingleObject(m_hThreadWatch, 500);
}

void CClientController::threadFunc()
{
	MSG msg;
	// ��Ϣѭ���Ǿ���� Windows ��Ϣ����ģʽ�������ֳ����ִ�У��ȴ��������û���ϵͳ��������Ϣ��ֻ���ڽ��յ��˳���Ϣ��ͨ���ǹرմ��ڵ���Ϣ��ʱ��ѭ���Ż��˳����������ִ��
	while (::GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		// ��ȡ��Ϣ
		if (msg.message == WM_SEND_MESSAGE)
		{
			MSGINFO* pmsg = (MSGINFO*)msg.wParam;
			HANDLE hEvent = (HANDLE)msg.lParam;
			std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);
			// �������Ϣѭ���߳��У������� WM_SEND_MESSAGE �����Ϣ�󣬽�����ֵ���봫�����Ľṹ��MSGINFO�е�result
			// ͬʱ�����¼����÷��������Ϣ���߳�(����Wait)֪����Ϣ�Ѵ������
			if (it != m_mapFunc.end()) // ����ҵ�����Ϣ��Ӧ�ĳ�Ա����ָ�룬��ͨ����Ա����ָ����ö�Ӧ�ĳ�Ա������������ msg ��Ϊ����������� (this->*it->second) ��ͨ����Ա����ָ����ó�Ա�������﷨��
			{
				pmsg->result = (this->*it->second) (pmsg->msg.message, pmsg->msg.wParam, pmsg->msg.lParam); // WM_SEND_MESSAGE��Ӧ����Ϣ��Ӧ�����ķ���ֵ
			}
			else
			{
				pmsg->result = -1; // �����ڶ�Ӧ����Ϣ
			}
			SetEvent(hEvent); // �ڵ�ǰ�߳�ִ����WM_SEND_MESSAGE��Ӧ���߼��������¼������ڸ��߷��������Ϣ���̣߳���Ϣ�Ѵ��� ������SendMessage()�оͿ���wait�����Ϣ�Ƿ���� ��ʵ���߳�ͬ��
		}
		else
		{
			std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);
			if (it != m_mapFunc.end()) // ����ҵ�����Ϣ��Ӧ�ĳ�Ա����ָ�룬��ͨ����Ա����ָ����ö�Ӧ�ĳ�Ա������������ msg ��Ϊ����������� (this->*it->second) ��ͨ����Ա����ָ����ó�Ա�������﷨��
			{
				(this->*it->second) (msg.message, msg.wParam, msg.lParam);
			}
		}
	}
}


unsigned __stdcall CClientController::threadEntry(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadFunc(); // �����̺߳���
	_endthreadex(0);
	return 0;
}

//LRESULT CClientController::OnSendPack(UINT msg, WPARAM wParam, LPARAM lParam)
//{
//	CClientSocket* pClient = CClientSocket::getInstance();
//	CPacket* pPacket = (CPacket*)wParam;
//	return pClient->Send(*pPacket); // �������ݰ�
//}

//LRESULT CClientController::OnSendData(UINT msg, WPARAM wParam, LPARAM lParam)
//{
//	CClientSocket* pClient = CClientSocket::getInstance();
//	char* pBuffer = (char*)wParam;
//	return pClient->Send(pBuffer, (int)lParam); // ��������
//}

LRESULT CClientController::OnShowStatus(UINT msg, WPARAM wParam, LPARAM lParam)
{
	return m_statusDlg.ShowWindow(SW_SHOW); // ��ʾ״̬�Ի��� 
}

LRESULT CClientController::OnShowWatch(UINT msg, WPARAM wParam, LPARAM lParam)
{
	return m_watchDlg.DoModal(); // Զ������Ի�������Ϊ����
}