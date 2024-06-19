#include "pch.h"
#include "ClientSocket.h"

// ���о�̬��Ա��Ҫʹ����ʽ�ط�ʽ��ʼ�������Ǿ�̬��Աһ����ڹ��캯���г�ʼ��
// ����̬��Ա��������Ĺ��캯���г�ʼ������Ϊ��̬��Ա�����������ʵ�����Ķ����������ʵ�����Ķ����õ�
CClientSocket* CClientSocket::m_instance = NULL; // ��ʼ�� ServerSocket.h �������ľ�̬��Ա���� m_instance 

CClientSocket::CHelper CClientSocket::m_helper;

CClientSocket* pserver = CClientSocket::getInstance();

std::string GetErrInfo(int wsaErrCode) // ͨ����������ܹ���ȡ��Ӧ������ɶ��Ĵ�����Ϣ ���� Windows �׽��ִ�����룬������������˴��������صĴ�����Ϣ
{
	std::string ret;
	LPVOID lpMsgBuf = NULL; // ����һ��ָ����Ϣ��������ָ�룬����ʼ��ΪNULL
	FormatMessage( // ��ϵͳ�������ת��Ϊ��Ӧ�Ŀɶ��ַ���
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, // ʹ��ϵͳ������е���Ϣ | ��ϵͳΪ��Ϣ�����ڴ�
		NULL,
		wsaErrCode, // Ҫ��ȡ��Ϣ�Ĵ������
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // ָ�����Ա�ʶ���������������Ժ�Ĭ��������
		(LPTSTR)&lpMsgBuf, 0, NULL); //ָ�������Ϣ�Ļ��������Լ�����һЩ����
	ret = (char*)lpMsgBuf; // ����Ϣ������������ת��Ϊstd::string���͵��ַ���
	LocalFree(lpMsgBuf); // �ͷ���ǰ��FormatMessage������ڴ棬�Ա����ڴ�й©

	return ret;
}

bool CClientSocket::InitSocket()
{
	if (m_sock != INVALID_SOCKET) CloseSocket();
	m_sock = socket(PF_INET, SOCK_STREAM, 0); // ���� socket ����
	if (m_sock == -1) return false;

	// ��ʼ�����ÿͻ����׽��ֵ�ַ��Ϣ
	sockaddr_in serv_addr; // ����һ�� sockaddr_in ���͵ı��� serv_adr���ýṹ�����ڴ洢 IPv4 ��ַ�Ͷ˿���Ϣ
	memset(&serv_addr, 0, sizeof(serv_addr)); // ʹ�� memset ������ serv_adr �ṹ����ڴ�����ȫ������Ϊ�㡣����һ�ֳ�ʼ���ṹ��ĳ�����������ȷ���ṹ��������ֶζ�����ȷ����
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(m_nIP);
	serv_addr.sin_port = htons(m_nPort);

	if (serv_addr.sin_addr.s_addr == INADDR_NONE)
	{
		AfxMessageBox(_T("ָ����IP��ַ�����ڣ�"));
		return false;
	}
	int ret = connect(m_sock, (sockaddr*)&serv_addr, sizeof(serv_addr));
	if (ret == -1)
	{
		TRACE("����ʧ�ܣ�%d \r\n", errno);
		AfxMessageBox(_T("����ʧ�ܣ�"));
		TRACE("����ʧ�ܣ�%d %s \r\n", WSAGetLastError(), GetErrInfo(WSAGetLastError()).c_str());
		return false;
	}
	return true;
}

/* ����Ϣ���ƽ����ͻ֮ǰ���¼�����
bool CClientSocket::SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks, bool isAutoClosed)
{ // para1:�ͻ��˷��͵������  para2����¼��������Ӧ��� para3���Ƿ��Զ��ر��׽���ͨ�� ����Ӧ�������ĳ���(��Զ��������ļ�����)
	if (m_sock == INVALID_SOCKET && m_hThread == INVALID_HANDLE_VALUE) // ����2���ڱ���ÿ�η��������ʱ�ظ�����ͨ���߳�
	{
		m_hThread = (HANDLE)_beginthread(&CClientSocket::threadEntry, 0, this); // �����߳�������ͻ��˲�����ָ��
	}
	m_lock.lock(); // ���� �����̼߳�ͬ��	// para2��&����������һ��HANDLE��std::list<CPacket>*
	auto pr = m_mapAck.insert(std::pair<HANDLE, std::list<CPacket>&>(pack.hEvent, lstPacks)); // ���ص���һ�� pair<>, bool
	m_mapAutoClosed.insert(std::pair<HANDLE, bool>(pack.hEvent, isAutoClosed));
	m_lstSend.push_back(pack);
	m_lock.unlock(); // �ȴ�һ������ push_back ֮���ٽ���
	
	WaitForSingleObject(pack.hEvent, INFINITE); // �ȵ� hEvent Ϊ���ź�״̬��˵��Զ�̷���������Ӧ�����̺߳����������� hEvent
	std::map<HANDLE, std::list<CPacket>&>::iterator it;
	it = m_mapAck.find(pack.hEvent);
	if (it != m_mapAck.end()) // ����Ӧ����
	{
		m_lock.lock();
		m_mapAck.erase(it);
		m_lock.unlock();
		return true;
		
	}
	return false;
}
*/

bool CClientSocket::SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClosed, WPARAM wParam) // ��Ϣ���ƽ��ͨ�ų�ͻ
{
	UINT nMode = isAutoClosed ? CSM_AUTOCLOSE : 0;
	std::string strOut;
	pack.Data(strOut); // ���������
	PACKET_DATA* pData = new PACKET_DATA(strOut.c_str(), strOut.size(), nMode, wParam); // ����� PACKET_DATA ��������Ϣ��Ӧ����SendPack()����
	bool ret = PostThreadMessage(m_nThreadID, WM_SEND_PACK, (WPARAM)pData, (LPARAM)hWnd); // Ͷ����Ϣ�Լ�����Ϣ�йصĲ�����ͨ���߳� 
	if (ret == false) // new������������������ϢͶ�ݳɹ�������ͨ���߳�ɾ���������Լ�ɾ��
	{
		delete pData;
	}
	return ret;
}

unsigned CClientSocket::threadEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg;
	thiz->threadFunc2();
	_endthreadex(0);
	return 0;
}

CClientSocket::CClientSocket(const CClientSocket& ss)
{
	m_eventInvoke = ss.m_eventInvoke;
	m_bAutoClose = ss.m_bAutoClose;
	m_sock = ss.m_sock;
	m_nIP = ss.m_nIP;
	m_nPort = ss.m_nPort;
	std::map<UINT, CClientSocket::MSGFUNC>::const_iterator it = ss.m_mapFunc.begin();
	for (; it != ss.m_mapFunc.end(); it++)
	{
		m_mapFunc.insert(std::pair<UINT, MSGFUNC>(it->first, it->second));
	}
}

CClientSocket::CClientSocket():
	m_nIP(INADDR_ANY), m_nPort(0), m_sock(INVALID_SOCKET), m_bAutoClose(true), m_hThread(INVALID_HANDLE_VALUE)
{

	if (InitSockEnv() == FALSE) // �����绷����ʼ��ʧ�ܣ���ʾ������Ϣ��Ȼ����� 'exit(0)' 
	{
		MessageBox(NULL, _T("�޷���ʼ���׽��ֻ�����������������"), _T("��ʼ������"), MB_OK | MB_ICONERROR);
		exit(0);
	}

	// ͨ���߳�������
	m_eventInvoke = CreateEvent(NULL, TRUE, FALSE, NULL); // �˹����ã��������ResetEvent()���ܽ���ָ�Ϊ���ź�״̬���ҵ���Ϊ���ź�״̬ʱ���ȴ����¼�����������߳̾��ɻ���źű�Ϊ�ɵ����߳�
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientSocket::threadEntry, this, 0, &m_nThreadID); // �����߳�������ͻ��˸����Ի��򲢷�����Ϣ
	if (WaitForSingleObject(m_eventInvoke, 100) == WAIT_TIMEOUT) // �ȴ��¼���ʱ����ͨ���߳�δ�����ɹ�
	{
		TRACE("������Ϣ�����߳�����ʧ�ܣ�\r\n");
	}
	CloseHandle(m_eventInvoke); // �߳������ɹ����ر��¼�����

	m_buffer.resize(BUFFER_SIZE); // ���û�������С
	memset(m_buffer.data(), 0, BUFFER_SIZE); // ��ʼ��������Ϊȫ0	

	struct // ������Ϣ��Ӧ����ӳ���
	{
		UINT message;
		MSGFUNC func;
	}funcs[] =
	{
		{WM_SEND_PACK, &CClientSocket::SendPack},
		{0, NULL}
	};
	for (int i = 0; funcs[i].message != 0; i++)
	{
		m_mapFunc.insert(std::pair<UINT, MSGFUNC>(funcs[i].message, funcs[i].func));
	}
}

/*
void CClientSocket::threadFunc() 
{
	std::string strBuffer; // �ͻ��˽��ջ�����
	strBuffer.resize(BUFFER_SIZE);
	char* pBuffer = (char*)strBuffer.c_str(); // ��������ʼ��ַ
	int index = 0; // index ����׷�ٻ������д��������ݵ�λ��
	InitSocket();
	while (m_sock != INVALID_SOCKET)
	{
		if (m_lstSend.size() > 0) // �д����͵�����
		{
			TRACE("lstSend size: %d\r\n", m_lstSend.size());
			m_lock.lock();
			CPacket& head = m_lstSend.front(); // ���ն��������˳��������� ��ȡ��ͷ 
			m_lock.unlock();
			if (Send(head) == false) // ����ʧ��
			{
				TRACE("����ʧ��\r\n");
				continue;
			}
			std::map<HANDLE, std::list<CPacket>&>::iterator it;
			it = m_mapAck.find(head.hEvent);
			if (it != m_mapAck.end())
			{
				std::map<HANDLE, bool>::iterator it0;
				it0 = m_mapAutoClosed.find(head.hEvent);

				do {
					// ���ͳɹ���ȴ�Ӧ��
					int length = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0);
					if (length > 0 || (index > 0)) // ���������������ݣ����
					{
						index += length; // ��������ȡ�� len ���ֽ����ݣ��� buffer ���� + len
						size_t size = (size_t)index; // �������������������ݣ��ʽ� index ��ֵ�� len ��Ϊ�������ݸ����� Cpacket ��Ĺ��캯�� 
						CPacket pack((BYTE*)pBuffer, size);
						if (size > 0) // CPacket�Ľ�����캯�����ص� len �ǽ���������õ������ݣ�> 0 ˵������ɹ�
						{
							//TODO��	֪ͨ��Ӧ���¼� 
							pack.hEvent = head.hEvent;
							it->second.push_back(pack);
							memmove(pBuffer, pBuffer + size, index - size);
							index -= size;
							if (it0->second) // �����Զ��رգ�����ʱ����֪ͨ�Ѵ������
							{
								SetEvent(head.hEvent);
								break;
							}
						}
					}
					else if (length <= 0 && index <= 0) // ��������
					{
						CloseSocket();
						SetEvent(head.hEvent); // �ȵ�Զ�̷������ر�����֮����֪ͨ�������
						if (it0 != m_mapAutoClosed.end())
						{
							//m_mapAutoClosed.erase(it0);
							TRACE("SetEvent %d %d\r\n", head.sCmd, it0->second);
						}
						else
						{
							TRACE("�쳣�������û�ж�Ӧ��pair\r\n");
						}
						break; // ͨ����� ���ղ�����Ϣ �˳�ѭ��
					}
				} while (it0->second == false); // ��������Զ��رգ���һֱrecv()ֱ���׽��ֹر�
			}
			m_lock.lock(); // ���� ��ֹ�����߳�ͬʱ�� m_lstSend ���в��� 
			m_lstSend.pop_front();
			m_mapAutoClosed.erase(head.hEvent);
			m_lock.unlock(); // ����
			if (InitSocket() == false)
			{
				InitSocket();
			}
		}
		Sleep(1);
	}
	CloseSocket();
}
*/

void CClientSocket::threadFunc2()
{
	SetEvent(m_eventInvoke); // �߳�����ʱ���� m_eventInvoke Ϊ���ź�״̬
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) // ����Ϣ����������ͻ��˸���dlg������ͨ�ŵ����󣬱����ͻ
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		TRACE("Get Message : %08X \r\n", msg.message);
		if (m_mapFunc.find(msg.message) != m_mapFunc.end())
		{
			(this->*m_mapFunc[msg.message])(msg.message, msg.wParam, msg.lParam); // ���ö�Ӧ����Ϣ��Ӧ���� SendPack()
		}
	}
}

// para1:��Ϣ���� para2:������Ϣ(PacketData)��ָ�� para3:����������Ϣ�Ĵ��ھ��
void CClientSocket::SendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
{// TODO: ����һ����Ϣ�����ݽṹ�����ݺ����ݳ��ȣ�����һ��Ӧ��͹رջ��ǽ��ն��Ӧ���ٹر�[eg:�ļ�����]�� �ص���Ϣ�����ݽṹ��HWND MESSAGE��
	
	PACKET_DATA data = *(PACKET_DATA*)wParam; // ��������
	delete (PACKET_DATA*)wParam; // �����ڴ�й¶
	HWND hWnd = (HWND)lParam; // �õ�����������Ϣ�Ĵ��ھ��

	if (InitSocket() == true)
	{
		int ret = send(m_sock, (char*)data.strData.c_str(), (int)data.strData.size(), 0); // ��Զ�̷�����ͨ�ţ����ͻ��˵����󷢸�������
		if (ret > 0)
		{
			size_t index = 0; // ����׷�ٻ������д��������ݵ�λ��
			std::string strBuffer; // �ͻ��˽��ջ�����
			strBuffer.resize(BUFFER_SIZE); // ���û�������С
			char* pBuffer = (char*)strBuffer.c_str(); // ��������ʼ��ַ

			while (m_sock != INVALID_SOCKET) // ������ͨ��ģʽ��1.����һ��Ӧ��͹ر��׽��� 2.���ն��Ӧ���ٹر�[�ļ�����] ���� while ѭ��������
			{
				int length = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0); // ���ͳɹ������Զ�̷�������Ӧ��
				if (length > 0 || (index > 0)) // �յ�Ӧ��򻺳����л������� 
				{
					index += (size_t)length; // ��������ȡ�� len ���ֽ����ݣ��� buffer ���� + len 
					size_t nLen = (size_t)index; // �������������������ݣ��ʽ� index ��ֵ�� nLen ��Ϊ�������ݸ����� Cpacket ��Ĺ��캯�� 
					CPacket pack((BYTE*)pBuffer, nLen); // ��������������е����ݷ�������ݰ�pack�и�����Ա pack������ΪӦ���
					if (nLen > 0) // CPacket�Ľ�����캯�����ص� nLen �ǽ���������õ������ݣ�> 0 ˵������ɹ�
					{
						::SendMessage(hWnd, WM_SEND_PACK_ACK, (WPARAM) new CPacket(pack), data.wParam); // �õ�Ӧ����󣬸���Ӧ�Ĵ���(hWnd��������Ϣ��ͬʱ����Ӧ�����hWnd ����new��һ��pack�����ڽ��շ�(hWnd)��Ҫdelete pack
						if (data.nMode & CSM_AUTOCLOSE) // ������Զ��ر�ģʽ
						{
							CloseSocket();
							return;
						}
						memmove(pBuffer, pBuffer + nLen, index - nLen); // �����������ѱ������ nLen ������
						index -= nLen; // ����������������ݳ��� - nLen
					}
				}
				else // ͨ�Ž��� �׽��ֹرջ��������쳣
				{
					TRACE("recv failed! length: %d, index: %d\r\n", length, index);
					CloseSocket();
				}
			}
		}
		else
		{
			CloseSocket();
			// ������ֹ����
			::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -1); // ����Ӧ�Ĵ���(hWnd��������Ϣ NULL + -1 ��ʾ����ʧ��
		}
	}	
	else
	{
		::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -2); // ����Ӧ�Ĵ���(hWnd��������Ϣ NULL + -2 ��ʾ��ʼ���׽���ʧ��
	}
}

bool CClientSocket::Send(const CPacket& pack)
{
	if (m_sock == -1) return false;
	std::string strOut;
	pack.Data(strOut);
	return send(m_sock, strOut.c_str(), strOut.size(), 0) > 0; // ����
}

