#pragma once
#include <WinSock2.h>
#include <memory>
enum class ETYPE {
	ETypeTCP = 1, 
	ETypeUDP
};

class CESockaddrIn
{
public:
	CESockaddrIn() { // Ĭ�Ϲ��캯��
		memset(&m_addr, 0, sizeof(m_addr));
		m_port = -1;
	}
	CESockaddrIn(sockaddr_in addr) {
		memcpy(&m_addr, &addr, sizeof(addr));
		m_ip = inet_ntoa(m_addr.sin_addr);
		m_port = ntohs(m_addr.sin_port);
	}
	CESockaddrIn(UINT nIP, short nPort) { // ���캯��
		m_addr.sin_family = AF_INET;
		m_addr.sin_port = htons(nPort); // 16λ�����ֽ���ת�����ֽ���
		m_addr.sin_addr.s_addr = htonl(nIP); // 32λ�����ֽ���ת�����ֽ���
		m_ip = inet_ntoa(m_addr.sin_addr); // ��IPv4��ַ�Ӷ����Ƹ�ʽת��Ϊ���ʮ�����ַ�����ʽ�ĺ���
		m_port = nPort;
	}
	CESockaddrIn(const std::string& strIP, short nPort) { // ���캯��
		m_ip = strIP;
		m_port = nPort;
		m_addr.sin_family = AF_INET;
		m_addr.sin_port = htons(nPort);
		m_addr.sin_addr.s_addr = inet_addr(strIP.c_str()); //  ��һ�����ڽ����ʮ�����ַ�����ʽ��IPv4��ַת��Ϊ�����ֽ�˳���32λ�޷�������
	}
	CESockaddrIn(const CESockaddrIn& addr) { // ���ƹ��캯��
		memcpy(&m_addr, &addr, sizeof(addr));
		m_ip = inet_ntoa(m_addr.sin_addr);
		m_port = ntohs(m_addr.sin_port);
	}
	CESockaddrIn& operator=(const CESockaddrIn& addr) {
		if (this != &addr)
		{
			memcpy(&m_addr, &addr, sizeof(addr));
			m_ip = inet_ntoa(m_addr.sin_addr);
			m_port = ntohs(m_addr.sin_port);
		}
		return *this;
	}
	~CESockaddrIn() {}
	operator sockaddr* () const { return (sockaddr*)&m_addr; } // ���� sockaddr*() ����ת��
	operator void* () const { return (void*)&m_addr; } // ���� void*()����ת��
	std::string GetIP() const { return m_ip; }
	short GetPort() const { return m_port; }
	void updata() {
		m_ip = inet_ntoa(m_addr.sin_addr);
		m_port = ntohs(m_addr.sin_port);
	}

	inline int size()const { return sizeof(sockaddr_in); }
private:
	sockaddr_in m_addr;
	std::string m_ip;
	short m_port;
};

class CEBuffer: public std::string // �Զ���� buffer ��
{
public:
	CEBuffer(const char* str) {
		resize(strlen(str));
		memcpy((void*)c_str(), str, size());
	}
	CEBuffer(size_t size = 0) : std::string() {
		if (size > 0) {
			resize(size);
			memset(*this, 0, this->size());
		}
	}
	CEBuffer(void* buffer, size_t size) :std::string() {
		resize(size);
		memcpy((void*)c_str(), buffer, size);
	}
	~CEBuffer() {
		std::string::~basic_string();
	}
	operator char* () const { return (char*)c_str(); }
	operator const char* () const { return c_str(); }
	operator BYTE* () const { return (BYTE*)c_str(); }
	operator void* () const { return (void*)c_str(); }
	void Update(void* buffer, size_t size){
		resize(size);
		memcpy((void*)c_str(), buffer, size);
	}

};
class CESocket // �Զ���� socket ��
{
public:
	CESocket(ETYPE nType = ETYPE::ETypeTCP, int nProtocol = 0) { // ��ʼ�� Ĭ��TCP
		m_sock = socket(PF_INET, (int)nType, nProtocol);
	}
	CESocket(const CESocket& sock) { // ���ƹ��캯�� ע���׽��ֲ���ֱ�Ӹ��ƣ��õ���ֻ��һ����Դ�����ʵ�ʵĶ����ڲ���ϵͳ�ں���
		m_sock = socket(PF_INET, (int)sock.m_type, sock.m_protocol);
		m_type = sock.m_type;
		m_protocol = sock.m_protocol;
		m_addr = sock.m_addr;
	}
	CESocket& operator= (CESocket& sock) { // ���ظ�ֵ�����
		if (this != &sock)
		{
			m_sock = socket(PF_INET, (int)sock.m_type, sock.m_protocol);
			m_type = sock.m_type;
			m_protocol = sock.m_protocol;
			m_addr = sock.m_addr;
		}
		return *this;

	}
	~CESocket() {
		close();
	}
	// �� const ���εĳ�Ա������
	// 1.���޸ĳ�Ա������ �ڳ�Ա������������� const �ؼ��ֱ�ʾ�ú��������޸��κγ�Ա������ֵ������ζ���ڸú����У������޸Ķ�����κη� mutable ��Ա����������������ʽ������ߴ���Ŀɶ��ԣ������ܹ�ȷ���ڸú����ڲ�����������޸Ķ����״̬��
	// 2.�����ڳ������� �ڽ���Ա��������Ϊ const ֮�����Ϳ��Ա�����������á�����������ָ�� const ���εĶ������ǲ�������÷� const ��Ա�����������Ե��� const ��Ա��������������ʹ���ڳ��������ϵ��ó�Ա����ʱ����������ʶ��������޸Ķ���ĳ�Ա�������Ӷ������˶����ֻ�����ԡ�
	// ��֮������Ա��������Ϊ const �����ṩ���õĴ��밲ȫ�ԺͿɶ��ԣ�ͬʱ���������ӶԳ��������֧�֡�
	operator SOCKET() const { return m_sock; } // ���� SOCKET()����ת��
	bool operator==(SOCKET sock) const { // ���� == �����
		return m_sock == sock;
	}
	int listen(int backlog = 5) {
		if (m_type != ETYPE::ETypeTCP) return -1; // udp ����Ҫlisten()
		return ::listen(m_sock, backlog);
	}
	int bind(const std::string& ip, short port) {
		m_addr = CESockaddrIn(ip, port);
		return ::bind(m_sock, m_addr, m_addr.size());
	}
	int accept(){}
	int connect(const std::string& ip, short port){}
	int send(const CEBuffer& buffer) {
		return ::send(m_sock, buffer, buffer.size(), 0);
	}
	int recv(CEBuffer& buffer){
		return ::recv(m_sock, buffer, buffer.size(), 0);
	}
	int sendto(const CEBuffer& buffer, const CESockaddrIn& to) {
		return ::sendto(m_sock, buffer, buffer.size(), 0, to, to.size());
	}
	int recvfrom(CEBuffer& buffer, CESockaddrIn& from) {
		int len = from.size();
		int ret =  ::recvfrom(m_sock, buffer, buffer.size(), 0, from, &len);
		if (ret > 0){
			from.updata();
		}
		return ret;
	}
	void close(){
		if (m_sock != INVALID_SOCKET) {
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
		}		
	}
	
private:
	SOCKET m_sock;
	ETYPE m_type;
	int m_protocol;
	CESockaddrIn m_addr;
};

typedef std::shared_ptr<CESocket> CESOCKET; // CESocket���������ָ��

