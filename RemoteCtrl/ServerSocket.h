#pragma once
#include "pch.h"
#include "framework.h"
#include <list>
#include "Packet.h"

// ����һ������Ϊ void(*)(void*) �ĺ���ָ�����ͣ����������������Ϊ SOCKET_CALLBACK��
// std::list<CPacket>& lstPacket ��һ�����ã�ָ��һ���洢�� CPacket ����� std::list ������std::list ��C++��׼���е�һ��˫�������������洢��һϵ�е� CPacket ����
typedef void(*SOCKET_CALLBACK)(void* arg, int status, std::list<CPacket>& lstPacket, CPacket& packet);

class CServerSocket // �������˵��׽�����
{
public:
	// ʹ�þ�̬��������������Է��ʵ��������˽�г�Ա ���ʷ�ʽ����������::������ �磺CServerSocket::getInstance()
	static CServerSocket* getInstance()
	{
		if (m_instance == NULL) // ��̬����û�� this ָ�룬�����޷�ֱ�ӷ��ʳ�Ա����
		{
			m_instance = new CServerSocket();
		}
		return m_instance;
	}

	// �ú����Ƿ���˺�����ں��������ڶ���ʹ�õĽӿ�
	// para1:Command�ľ�̬������ para2:ִ������Ķ��󣬼�Command���� para3:�˿ںţ�Ĭ��9527 ����ֵ��ʧ�ܷ���-1
	// ���������¼�����Ӧ������
	// �ٳ�ʼ������⣬������InitSocket()
	// �����ӿͻ��ˣ�������AcceptClient()
	// �۴���ͻ��˷��������ݣ��������������DealCommand()
	// �ܽ������Command�������������������
	// �ݴӳ�ԱlstPackets����ȡ�ô�Command����������(���ܺ���push_back�����ݣ��ο�Command��Ĺ��ܺ���)��Ȼ�����Send���ͻؿͻ���
	int Run(SOCKET_CALLBACK callback, void* arg, short port = 9527){
		m_callback = callback;
		m_arg = arg;
		std::list<CPacket> lstPackets; // �洢 CPacket �����˫����������
		bool ret = InitSocket(port); // �����ʼ��
		if (ret == false) return -1; // ��ʼ��ʧ�� ���� -1
		int count = 0; // ���½������ ������½���3��
		while (true){
			if (AcceptClient() == false){
				if (count >= 3) return -2;
				count++;
			}
			int ret = DealCommand();
			if (ret > 0){
				// ����ص����� m_callback() ��CCommand�����RunCommand()
				// para1:main()��ʵ����CCommand������ this ָ�� para2:���� 
				// para3:�洢���ݰ������������������е��������ݰ�Ҫ���͸��ͻ��ˣ��ͻὫ�������ӵ�������
				// para4:���ݴ����������Ҫ�õ��Ŀͻ��˷���������(���ļ�·��������¼���)
				// ����֮ǰ�������������漰��������ͨ�Ź������ڴ��������ͬʱ���
				// ����֮��ͨ��para3��para4������������ֻ��Ҫ�������ݣ�����ͨ�Ź���ȫ������CServerSocket����
				m_callback(m_arg, ret, lstPackets, m_packet); // �������Command�����m_arg�� ��صĹ���ִ�к󷵻ص����ݰ���ӵ�lstPacketβ��
				while (lstPackets.size() > 0) { // ����������ݰ�Ҫ���͸��ͻ���
					Send(lstPackets.front()); // �������ݰ�������֮ǰ��������ִ�й�������ɵģ�
					lstPackets.pop_front(); // ��������ɾ�����͵����ݰ�
				}
			}
			CloseCient();
		}
		return 0;
	}

protected:
	bool InitSocket(short port = 9527) // ���������׽��ֳ�ʼ��
	{
		if (m_sock == -1) return false;

		// ��ʼ�����÷������׽��ֵ�ַ��Ϣ
		sockaddr_in serv_adr; // ����һ�� sockaddr_in ���͵ı��� serv_adr���ýṹ�����ڴ洢 IPv4 ��ַ�Ͷ˿���Ϣ
		memset(&serv_adr, 0, sizeof(serv_adr)); // ʹ�� memset ������ serv_adr �ṹ����ڴ�����ȫ������Ϊ�㡣����һ�ֳ�ʼ���ṹ��ĳ�����������ȷ���ṹ��������ֶζ�����ȷ����
		serv_adr.sin_family = AF_INET; // Э���� IPv4
		// ���÷������� IP ��ַ��INADDR_ANY ��ʾ�������������κν�������ӣ�������ͨ���κο��õ�����ӿڽ���ͨ�š���ʹ�÷������ܹ��ڶ������ӿ��ϼ�������
		serv_adr.sin_addr.s_addr = htonl(INADDR_ANY); 
		// ���÷����������Ķ˿ں�Ϊ 9527��htons �������ڽ������ֽ���host byte order���Ķ˿ں�ת��Ϊ�����ֽ���network byte order����ȷ���ڲ�ͬϵͳ�ϵ���ȷ����
		serv_adr.sin_port = htons(port); 

		// �� 
		// bind �����������ǽ��׽������ض��� IP ��ַ�Ͷ˿ںŹ������Ա���������ַ�Ͷ˿��ϵ���������
		// �ڷ������˳����У�bind ͨ���ڴ����׽��ֺ��������ã���ȷ���������������ĸ���ַ�Ͷ˿ڡ�
		// �ڵ��� bind �󣬷���������ͨ�� listen ������ʼ�����������󣬵ȴ��ͻ��˵�����
		// 'para1' �������׽��� 'para2'  �� serv_adr �ṹ�����͵ĵ�ַǿ��ת��Ϊͨ�õ� sockaddr �ṹ������ 
		// 'para3' ָ���˴���ĵ�ַ�ṹ��Ĵ�С��ȷ�� bind ������ȷʶ����ĵ�ַ��Ϣ�ĳ���
		if (bind(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr)) == -1) return false;

		// ����
		// ʹ�������׽��ִ��ڼ���״̬��׼�����ܿͻ��˵���������
		// 'para1' �������׽��� 
		// 'para2' ָ���˴����������������е���󳤶ȡ����������Ϊ 1 ��ʾ�����������Դ���һ���ȴ����ӵĿͻ��ˡ�����ж���ͻ���ͬʱ�������ӣ�����ָ���Ķ��г��ȵ��������󽫱��ܾ�
		// ���� listen �����󣬷������׽��־ʹ��ڱ�������״̬���ȴ��ͻ��˵���������
		// ��ʱ������������ͨ�� accept �������ܿͻ��˵����ӣ�����һ���µ��׽���������ͻ��˽���ͨ�š�
		// ͨ�����ڵ��� listen ֮�󣬷����������һ��ѭ�������ϵ��� accept ���������ͻ��˵���������
		if (listen(m_sock, 1) == -1) return false;

		return true;
	}

	bool AcceptClient() // ������ accept
	{
		sockaddr_in client_adr; // ����һ���ṹ����������ڴ洢�ͻ��˵ĵ�ַ��Ϣ
		int cli_sz = sizeof(client_adr); // ����һ���������������ڴ洢 client_adr �ṹ��Ĵ�С��
		m_client = accept(m_sock, (sockaddr*)&client_adr, &cli_sz); // ���� accept �������ܿͻ��˵��������� ����һ���µ��׽��֣����׽���������ո����ӵĿͻ��˽���ͨ��
		if (m_client == -1) return false; // �������ʧ�ܣ�accept ���� INVALID_SOCKET
		return true;
	}

#define BUFFER_SIZE 4096
	int  DealCommand() // �������˴����������Ŀ�н��ܿط���Ϊ�������ˣ����Ʒ���Ϊ�ͻ��ˣ������տͻ��˷��͹�������Ϣ��������Ϣ���ݽ��������
	{
		if (m_client == -1) return -1; // δ��ͻ��˽�������
		char* buffer = new char[BUFFER_SIZE]; // �������˻�����
		if (buffer == NULL)
		{
			TRACE("�ڴ治��! \r\n"); 
			return -2;
		}
		memset(buffer, 0, sizeof(buffer)); // ��ʼ����ջ�����
		size_t index = 0; // ������ǻ����������ݵ�β��,��buffer��д�����ݵĳ��ȵĳ���
		while (true)
		{
			// �������ݣ���ŵ���ʼλ����buffer��û��д�����ݵĵ�ַ�����յĳ���Ϊbufferʣ��ûд�����ݵĵ�ַ
			size_t len = recv(m_client, buffer + index, BUFFER_SIZE - index, 0); // ���سɹ����յ������ݳ���
			if (len <= 0)
			{
				delete[]buffer; // ��ֹ�ڴ�й¶
				return -1;
			}
			// ���
			index    += len;  // �������ɹ���ȡ�� len ���ֽ����ݣ��� buffer ��д�����ݳ��� + len
			len      = index; // �������������������ݣ��ʽ� index ��ֵ�� len ��Ϊ�������ݸ����� Cpacket ��Ĺ��캯��
			m_packet = CPacket((BYTE*)buffer, len); // ʹ�� CPacket ��Ĺ��캯����ʼ�� ����ʹ�õĽ���Ĺ��캯������ֵ�� m_packet
			if (len > 0) // CPacket�Ľ�����캯�����ص� len �ǽ���������õ������ݣ�> 0 ˵������ɹ�
			{
				// memmove(para1��para2��para3)����һ���ڴ��������ڴ����ƶ����Ը���֮ǰ�Ĳ���  
				// para1��Դ�ڴ��������ʼ��ַ para2��Ŀ���ڴ��������ʼ��ַ para3��Ҫ�ƶ����ֽ��� 
				memmove(buffer, buffer + len, BUFFER_SIZE - len); // ������������ݸ��ǣ������»�������������������������
				index -= len; // ����������� len ���ֽ����ݣ��� buffer ��д�����ݵĳ��� - len
				delete[]buffer; // ��ֹ�ڴ�й¶
				return m_packet.sCmd; // ���ճɹ������ؽ��յ�������
			}
		}
		delete[]buffer; // ��ֹ�ڴ�й¶
		return -1;
	}

	bool Send(const char* pData, int nSize) // �������˷�����Ϣ pData��Ϊbuffer�ĵ�ַ nSize ��Ҫ������Ϣ�ĳ���
	{
		if (m_client == -1) return false; // δ��ͻ��˽�������

		return send(m_client, pData, nSize, 0) > 0;
	}

	bool Send(CPacket& pack) // �������˷������ݰ�
	{
		if (m_client == -1) return false; // δ��ͻ��˽�������

		return send(m_client, pack.Data(), pack.Size(), 0) > 0;
	}

	// ��������������������ò�����
	
	//bool GetFilePath(std::string& strPath)
	//{
	//	if ((m_packet.sCmd == 2) || (m_packet.sCmd == 3) || (m_packet.sCmd == 4) || (m_packet.sCmd == 9)) // ���2����ʾ�鿴ָ��Ŀ¼�µ��ļ� ��3��ִ���ļ� '4'�����ļ� '9'ɾ���ļ�
	//	{
	//		strPath = m_packet.strData; // ���ذ��е����ݣ����ļ�·��
	//		return true;
	//	}
	//	
	//	return false;
	//}

	//bool GetMouseEvent(MOUSEEV& mouse) 
	//{
	//	if (m_packet.sCmd == 5)
	//	{
	//		memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEV)); // �洢�ͻ��˷���������¼�
	//		return true;
	//	}
	//	return false;
	//}

	//CPacket& GetPacket()
	//{
	//	return m_packet;
	//}

	void CloseCient()
	{
		if (m_client != INVALID_SOCKET)
		{
			closesocket(m_client);
			m_client = INVALID_SOCKET;
		}	
	}

private:
	SOCKET_CALLBACK m_callback; // �ص���������������Ϊ SOCKET_CALLBACK �ĺ���ָ�룬�������ָ�����ָ��һ������void������һ�� void* ���Ͳ����ĺ���
	void* m_arg; // �ص������Ĳ�����������һ��ָ�� void ���͵�ָ�룬���� m_callback �Ĳ���
	CPacket m_packet; // �������ݰ���ĳ�Ա����
	SOCKET m_sock; // �������������׽���Ϊ��ĳ�Ա����
	SOCKET m_client; // ������ͻ��˽������ӵ��׽���Ϊ��ĳ�Ա����

	// �� CServerSocket ��Ĺ��캯�������ƹ��캯���Լ���ֵ�����������Ϊ˽��
	// ����ζ����ֹ����ĸ��ƺ͸�ֵ����������Ϊ��ȷ���ڸ����ʵ���в��ᷢ������ĸ��ƺ͸�ֵ��ͨ������Ϊ�׽��ֵ���Դ������Ҫ���⴦�����ʺ�ͨ��Ĭ�ϵĸ��ƺ͸�ֵ����
	CServerSocket& operator=(const CServerSocket& ss) // ��ֵ�����
	{

	}
	CServerSocket(const CServerSocket& ss) // ���ƹ��캯��
	{
		m_sock = ss.m_sock;
		m_client = ss.m_client;
	}
	// ���캯��
	CServerSocket() {
		m_client = INVALID_SOCKET;
		if (InitSockEnv() == FALSE) // �����绷����ʼ��ʧ�ܣ���ʾ������Ϣ��Ȼ����� 'exit(0)' 
		{
			MessageBox(NULL, _T("�޷���ʼ���׽��ֻ�����������������"), _T("��ʼ������"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		// ����һ������ IPv4 ����ʽ�׽��� 'PF_INET' ָ����Э���壨Protocol Family��Ϊ IPv4  
		// 'SOCK_STREAM' ָ�����׽��ֵ����ͣ���������ʽ�׽��֣�stream socket������ʽ�׽�����һ���ṩ�������ӵġ��ɿ��ġ������ֽ������׽������ͣ�ͨ������ TCP Э��
		// '0' ���׽��ֵ�Э�������ͨ��Ϊ 0����ʾ����ǰ���������Զ�ѡ����ʵ�Э��
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
	}

	// ��������
	~CServerSocket() {
		//������������Ҳ��Ҫ������private��
	    //��Ϊ������Ҫ���ʵ���ڳ������е����������ж�����
	    //�������ǲ�����ʵ���Լ������������������ͷŶ���
		WSACleanup(); // �������� Winsock �����Դ������һ���ڲ�����Ҫʹ�� Winsock ʱ���е��������
		closesocket(m_sock);
	}

	// ��ʼ�����绷��������2.0�汾�����; ��ʼ���ɹ�����TRUE,ʧ�ܷ���FALSE
	BOOL InitSockEnv()
	{
		// �����д����������� Windows ��ʹ�� Winsock API ���������̵ĳ�ʼ������
		WSADATA data; // ������һ����Ϊ data �ı���������Ϊ WSADATA��WSADATA ��һ���ṹ�����ڴ洢�й� Windows Sockets ʵ�ֵ���Ϣ
		// if (WSAStartup(MAKEWORD(1, 1), &data) != 0)  // WSAStartup ������ʼ�� Winsock �⡣MAKEWORD(1, 1) ������ָ��������� Winsock �汾������������汾 1.1 
		if (WSAStartup(MAKEWORD(2, 0), &data) != 0) // ����"WSA_FLAG_OVERLAPPED"������ʵ���첽����Ҫ����汾 2.0
		{
			return FALSE; // ��ʼ�����绷��ʧ��
		}
		return TRUE; // ��ʼ�����绷���ɹ�
	}

	static CServerSocket* m_instance; // ����

	static void releaseInstance()
	{
		if (m_instance != NULL)
		{
			CServerSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp;
		}
	}

	// ����ģʽ����һ��������ģʽ�������˼����ȷ������Ӧ�ó��������������ֻ��һ��ʵ�������ṩһ��ȫ�ֵķ��ʵ㡣
	//         ����ζ�����ۺ�ʱ�ε�����������ʵ��������������ͬ�Ķ��󡣵���ģʽͨ�����ڹ���ȫ����Դ������������Ϣ�������ṩΨһ�ķ��ʵ�������ĳЩ��Դ
	// �������󣺵���ģʽ�µ���ʵ������Ϊ�����������������Ӧ�ó�����ֻ��һ��ʵ�����ڣ������ṩ��һ��ȫ�ֵķ��ʵ㣬���������ֵĴ�����Ի�ȡ�����Ψһ��ʵ��

	// CHelper ����һ��Ƕ���ࣨnested class������������Ϊ CServerSocket ���˽���ڲ���
	class CHelper
	{
	public:
		CHelper() // ���캯�������ڶ��󴴽�ʱ���� CServerSocket::getInstance() ��̬������
		{         // ��Ϊ CHelper ���� CServerSocket ���Ƕ���࣬���������Է��� CServerSocket ���˽�г�Ա�;�̬��Ա   
			CServerSocket::getInstance(); // ������ CHelper ��Ķ���ʱ�������Զ����ù��캯�����Ӷ����ڲ����� CServerSocket::getInstance() ������
		}
		~CHelper() 
		{		  // ͬ������Ϊ CHelper ���� CServerSocket ���Ƕ���࣬���������Է��� CServerSocket ���˽�г�Ա�;�̬��Ա
			CServerSocket::releaseInstance(); //  CHelper ��Ķ����뿪������ʱ�������Զ����������������Ӷ����ڲ����� CServerSocket::releaseInstance() ����
		}
	};
	//����һ���ڲ���ľ�̬����
	//���ö������ٵ�ʱ�򣬵�����������˳���������ǵĵ�������
	static CHelper m_helper; // ���ó����ڽ���ʱ����ȫ�ֱ��������ԣ�ѡ�����յ��ͷ�ʱ��
	
	
	// m_helper �� CHelper ��ľ�̬��Ա�������� CServerSocket ��������Ϊ static

};

// extern �ؼ��ֱ�ʾ�ñ����������ļ��ж��壬���ڵ�ǰ�ļ��н���Ϊ������ʵ�ʵĶ�������������ļ��У����� server �ͱ����һ��ȫ�ֱ����������������Ƕ���
// ����������Դ�ļ��ڰ������ͷ�ļ�(ServerSocket.h)ʱ�����ܹ����ʵ� server ���ȫ�ֱ����������������������ظ�����Ĵ���
// ������Դ�ļ��У������Ҫʹ�� server������ͨ��������ͬ��ͷ�ļ�����ȡ�� server �������������ᵼ�¶�ζ��塣
// Ȼ��ʵ�ʵĶ���Ӧ�ó�����һ��Դ�ļ��У�ȷ��ֻ��һ���ط��� server ������ʵ�ʵķ���ͳ�ʼ��
// ����������Ϊ��֧��ģ�黯��̣�����ͬ��Դ�ļ�����ͬһȫ�ֱ������������ͻ
extern CServerSocket server; // ��Ϊ RemoteCtrl.cpp ��ֻ������ ServerSocket.h,Ϊ������ɷ��� ServerSocket.cpp �еı��������ｫ server ����Ϊȫ�ֱ���

