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
	CESockaddrIn() { // 默认构造函数
		memset(&m_addr, 0, sizeof(m_addr));
		m_port = -1;
	}
	CESockaddrIn(sockaddr_in addr) {
		memcpy(&m_addr, &addr, sizeof(addr));
		m_ip = inet_ntoa(m_addr.sin_addr);
		m_port = ntohs(m_addr.sin_port);
	}
	CESockaddrIn(UINT nIP, short nPort) { // 构造函数
		m_addr.sin_family = AF_INET;
		m_addr.sin_port = htons(nPort); // 16位主机字节序转网络字节序
		m_addr.sin_addr.s_addr = htonl(nIP); // 32位主机字节序转网络字节序
		m_ip = inet_ntoa(m_addr.sin_addr); // 将IPv4地址从二进制格式转换为点分十进制字符串格式的函数
		m_port = nPort;
	}
	CESockaddrIn(const std::string& strIP, short nPort) { // 构造函数
		m_ip = strIP;
		m_port = nPort;
		m_addr.sin_family = AF_INET;
		m_addr.sin_port = htons(nPort);
		m_addr.sin_addr.s_addr = inet_addr(strIP.c_str()); //  是一个用于将点分十进制字符串格式的IPv4地址转换为网络字节顺序的32位无符号整数
	}
	CESockaddrIn(const CESockaddrIn& addr) { // 复制构造函数
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
	operator sockaddr* () const { return (sockaddr*)&m_addr; } // 重载 sockaddr*() 类型转换
	operator void* () const { return (void*)&m_addr; } // 重载 void*()类型转换
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

class CEBuffer: public std::string // 自定义的 buffer 类
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
class CESocket // 自定义的 socket 类
{
public:
	CESocket(ETYPE nType = ETYPE::ETypeTCP, int nProtocol = 0) { // 初始化 默认TCP
		m_sock = socket(PF_INET, (int)nType, nProtocol);
	}
	CESocket(const CESocket& sock) { // 复制构造函数 注意套接字不能直接复制，拿到的只是一个资源句柄，实际的对象在操作系统内核中
		m_sock = socket(PF_INET, (int)sock.m_type, sock.m_protocol);
		m_type = sock.m_type;
		m_protocol = sock.m_protocol;
		m_addr = sock.m_addr;
	}
	CESocket& operator= (CESocket& sock) { // 重载赋值运算符
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
	// 加 const 修饰的成员函数：
	// 1.不修改成员变量： 在成员函数声明后加上 const 关键字表示该函数不会修改任何成员变量的值。这意味着在该函数中，不能修改对象的任何非 mutable 成员变量。这种声明方式可以提高代码的可读性，并且能够确保在该函数内部不会意外地修改对象的状态。
	// 2.适用于常量对象： 在将成员函数声明为 const 之后，它就可以被常量对象调用。常量对象是指用 const 修饰的对象，它们不允许调用非 const 成员函数，但可以调用 const 成员函数。这样可以使得在常量对象上调用成员函数时，仅允许访问而不允许修改对象的成员变量，从而增加了对象的只读特性。
	// 总之，将成员函数声明为 const 可以提供更好的代码安全性和可读性，同时还可以增加对常量对象的支持。
	operator SOCKET() const { return m_sock; } // 重载 SOCKET()类型转换
	bool operator==(SOCKET sock) const { // 重载 == 运算符
		return m_sock == sock;
	}
	int listen(int backlog = 5) {
		if (m_type != ETYPE::ETypeTCP) return -1; // udp 不需要listen()
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

typedef std::shared_ptr<CESocket> CESOCKET; // CESocket对象的智能指针

