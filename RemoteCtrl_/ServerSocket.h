#pragma once
#include "pch.h"
#include "framework.h"
#include <list>
#include "Packet.h"

// 定义一个类型为 void(*)(void*) 的函数指针类型，并将这个类型命名为 SOCKET_CALLBACK。
// std::list<CPacket>& lstPacket 是一个引用，指向一个存储了 CPacket 对象的 std::list 容器。std::list 是C++标准库中的一个双向链表容器，存储了一系列的 CPacket 对象
typedef void(*SOCKET_CALLBACK)(void* arg, int status, std::list<CPacket>& lstPacket, CPacket& packet);

class CServerSocket // 服务器端的套接字类
{
public:
	// 使用静态函数，让类外可以访问到类里面的私有成员 访问方式：作用域名::函数名 如：CServerSocket::getInstance()
	static CServerSocket* getInstance()
	{
		if (m_instance == NULL) // 静态函数没有 this 指针，所以无法直接访问成员变量
		{
			m_instance = new CServerSocket();
		}
		return m_instance;
	}

	// 该函数是服务端核心入口函数，用于对外使用的接口
	// para1:Command的静态处理函数 para2:执行命令的对象，即Command对象 para3:端口号，默认9527 返回值：失败返回-1
	// 其做出以下几个响应动作：
	// ①初始化网络库，即调用InitSocket()
	// ②连接客户端，即调用AcceptClient()
	// ③处理客户端发来的数据，返回命令，即调用DealCommand()
	// ④将命令交给Command类对象，让其完成命令操作
	// ⑤从成员lstPackets表中取得从Command处理后的数据(功能函数push_back的数据，参考Command类的功能函数)，然后调用Send发送回客户端
	int Run(SOCKET_CALLBACK callback, void* arg, short port = 9527){
		m_callback = callback;
		m_arg = arg;
		std::list<CPacket> lstPackets; // 存储 CPacket 对象的双向链表容器
		bool ret = InitSocket(port); // 网络初始化
		if (ret == false) return -1; // 初始化失败 返回 -1
		int count = 0; // 重新接入次数 最大重新接入3次
		while (true){
			if (AcceptClient() == false){
				if (count >= 3) return -2;
				count++;
			}
			int ret = DealCommand();
			if (ret > 0){
				// 这个回调函数 m_callback() 即CCommand里面的RunCommand()
				// para1:main()中实例化CCommand类对象的 this 指针 para2:命令 
				// para3:存储数据包的容器，命令处理过程中但凡有数据包要发送给客户端，就会将这个包添加到容器中
				// para4:数据处理过程中需要用到的客户端发来的数据(如文件路径、鼠标事件等)
				// 解耦之前：命令处理过程中涉及到的网络通信工作都在处理命令的同时完成
				// 解耦之后：通过para3和para4，命令处理过程中只需要处理数据，网络通信工作全部交由CServerSocket负责
				m_callback(m_arg, ret, lstPackets, m_packet); // 将命令交给Command类对象m_arg， 相关的功能执行后返回的数据包会加到lstPacket尾部
				while (lstPackets.size() > 0) { // 服务端有数据包要发送给客户端
					Send(lstPackets.front()); // 发送数据包（解耦之前是在命令执行过程中完成的）
					lstPackets.pop_front(); // 在容器中删除发送的数据包
				}
			}
			CloseCient();
		}
		return 0;
	}

protected:
	bool InitSocket(short port = 9527) // 服务器端套接字初始化
	{
		if (m_sock == -1) return false;

		// 初始化配置服务器套接字地址信息
		sockaddr_in serv_adr; // 声明一个 sockaddr_in 类型的变量 serv_adr，该结构体用于存储 IPv4 地址和端口信息
		memset(&serv_adr, 0, sizeof(serv_adr)); // 使用 memset 函数将 serv_adr 结构体的内存内容全部设置为零。这是一种初始化结构体的常见方法，以确保结构体的所有字段都被正确设置
		serv_adr.sin_family = AF_INET; // 协议族 IPv4
		// 设置服务器的 IP 地址。INADDR_ANY 表示服务器将接受任何进入的连接，即可以通过任何可用的网络接口进行通信。这使得服务器能够在多个网络接口上监听连接
		serv_adr.sin_addr.s_addr = htonl(INADDR_ANY); 
		// 设置服务器监听的端口号为 9527。htons 函数用于将主机字节序（host byte order）的端口号转换为网络字节序（network byte order），确保在不同系统上的正确传输
		serv_adr.sin_port = htons(port); 

		// 绑定 
		// bind 函数的作用是将套接字与特定的 IP 地址和端口号关联，以便监听这个地址和端口上的连接请求。
		// 在服务器端程序中，bind 通常在创建套接字后立即调用，以确定服务器将监听哪个地址和端口。
		// 在调用 bind 后，服务器可以通过 listen 函数开始监听连接请求，等待客户端的连接
		// 'para1' 服务器套接字 'para2'  将 serv_adr 结构体类型的地址强制转换为通用的 sockaddr 结构体类型 
		// 'para3' 指定了传入的地址结构体的大小，确保 bind 函数正确识别传入的地址信息的长度
		if (bind(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr)) == -1) return false;

		// 监听
		// 使服务器套接字处于监听状态，准备接受客户端的连接请求
		// 'para1' 服务器套接字 
		// 'para2' 指定了待处理的连接请求队列的最大长度。在这里，设置为 1 表示服务器最多可以处理一个等待连接的客户端。如果有多个客户端同时尝试连接，超过指定的队列长度的连接请求将被拒绝
		// 调用 listen 函数后，服务器套接字就处于被动监听状态，等待客户端的连接请求。
		// 此时，服务器可以通过 accept 函数接受客户端的连接，创建一个新的套接字用于与客户端进行通信。
		// 通常，在调用 listen 之后，服务器会进入一个循环，不断调用 accept 来处理多个客户端的连接请求
		if (listen(m_sock, 1) == -1) return false;

		return true;
	}

	bool AcceptClient() // 服务器 accept
	{
		sockaddr_in client_adr; // 声明一个结构体变量，用于存储客户端的地址信息
		int cli_sz = sizeof(client_adr); // 声明一个整数变量，用于存储 client_adr 结构体的大小。
		m_client = accept(m_sock, (sockaddr*)&client_adr, &cli_sz); // 调用 accept 函数接受客户端的连接请求 返回一个新的套接字，该套接字用于与刚刚连接的客户端进行通信
		if (m_client == -1) return false; // 如果连接失败，accept 返回 INVALID_SOCKET
		return true;
	}

#define BUFFER_SIZE 4096
	int  DealCommand() // 服务器端处理命令（此项目中将受控方做为服务器端，控制方作为客户端，即接收客户端发送过来的消息，根据消息内容进行命令处理）
	{
		if (m_client == -1) return -1; // 未与客户端建立连接
		char* buffer = new char[BUFFER_SIZE]; // 服务器端缓冲区
		if (buffer == NULL)
		{
			TRACE("内存不足! \r\n"); 
			return -2;
		}
		memset(buffer, 0, sizeof(buffer)); // 初始化清空缓冲区
		size_t index = 0; // 用来标记缓冲区有数据的尾部,即buffer已写入数据的长度的长度
		while (true)
		{
			// 接收数据，存放的起始位置是buffer还没有写入数据的地址，接收的长度为buffer剩余没写入数据的地址
			size_t len = recv(m_client, buffer + index, BUFFER_SIZE - index, 0); // 返回成功接收到的数据长度
			if (len <= 0)
			{
				delete[]buffer; // 防止内存泄露
				return -1;
			}
			// 解包
			index    += len;  // 缓冲区成功读取了 len 个字节数据，故 buffer 已写入数据长度 + len
			len      = index; // 解析缓冲区的所有数据，故将 index 赋值给 len 作为参数传递给下面 Cpacket 类的构造函数
			m_packet = CPacket((BYTE*)buffer, len); // 使用 CPacket 类的构造函数初始化 这里使用的解包的构造函数并赋值给 m_packet
			if (len > 0) // CPacket的解包构造函数返回的 len 是解包过程中用掉的数据，> 0 说明解包成功
			{
				// memmove(para1，para2，para3)：将一段内存数据在内存中移动，以覆盖之前的部分  
				// para1：源内存区域的起始地址 para2：目标内存区域的起始地址 para3：要移动的字节数 
				memmove(buffer, buffer + len, BUFFER_SIZE - len); // 将解包掉的数据覆盖，即更新缓冲区，将解析过的数据清理
				index -= len; // 缓冲区解包了 len 个字节数据，故 buffer 已写入数据的长度 - len
				delete[]buffer; // 防止内存泄露
				return m_packet.sCmd; // 接收成功，返回接收到的命令
			}
		}
		delete[]buffer; // 防止内存泄露
		return -1;
	}

	bool Send(const char* pData, int nSize) // 服务器端发送消息 pData即为buffer的地址 nSize 是要发送信息的长度
	{
		if (m_client == -1) return false; // 未与客户端建立连接

		return send(m_client, pData, nSize, 0) > 0;
	}

	bool Send(CPacket& pack) // 服务器端发送数据包
	{
		if (m_client == -1) return false; // 未与客户端建立连接

		return send(m_client, pack.Data(), pack.Size(), 0) > 0;
	}

	// 解耦后以下三个函数均用不到了
	
	//bool GetFilePath(std::string& strPath)
	//{
	//	if ((m_packet.sCmd == 2) || (m_packet.sCmd == 3) || (m_packet.sCmd == 4) || (m_packet.sCmd == 9)) // 命令‘2’表示查看指定目录下的文件 ‘3’执行文件 '4'下载文件 '9'删除文件
	//	{
	//		strPath = m_packet.strData; // 返回包中的数据，即文件路径
	//		return true;
	//	}
	//	
	//	return false;
	//}

	//bool GetMouseEvent(MOUSEEV& mouse) 
	//{
	//	if (m_packet.sCmd == 5)
	//	{
	//		memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEV)); // 存储客户端发来的鼠标事件
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
	SOCKET_CALLBACK m_callback; // 回调函数，声明类型为 SOCKET_CALLBACK 的函数指针，这个函数指针可以指向一个返回void、接受一个 void* 类型参数的函数
	void* m_arg; // 回调函数的参数，声明了一个指向 void 类型的指针，用作 m_callback 的参数
	CPacket m_packet; // 声明数据包类的成员变量
	SOCKET m_sock; // 声明服务器端套接字为类的成员变量
	SOCKET m_client; // 声明与客户端建立连接的套接字为类的成员变量

	// 将 CServerSocket 类的构造函数，复制构造函数以及赋值运算符都声明为私有
	// 这意味着阻止了类的复制和赋值操作。这是为了确保在该类的实例中不会发生意外的复制和赋值，通常是因为套接字的资源管理需要特殊处理，不适合通过默认的复制和赋值操作
	CServerSocket& operator=(const CServerSocket& ss) // 赋值运算符
	{

	}
	CServerSocket(const CServerSocket& ss) // 复制构造函数
	{
		m_sock = ss.m_sock;
		m_client = ss.m_client;
	}
	// 构造函数
	CServerSocket() {
		m_client = INVALID_SOCKET;
		if (InitSockEnv() == FALSE) // 若网络环境初始化失败，显示错误信息，然后调用 'exit(0)' 
		{
			MessageBox(NULL, _T("无法初始化套接字环境，请检查网络设置"), _T("初始化错误"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		// 创建一个基于 IPv4 的流式套接字 'PF_INET' 指定了协议族（Protocol Family）为 IPv4  
		// 'SOCK_STREAM' 指定了套接字的类型，这里是流式套接字（stream socket）。流式套接字是一种提供面向连接的、可靠的、基于字节流的套接字类型，通常用于 TCP 协议
		// '0' 是套接字的协议参数，通常为 0，表示根据前两个参数自动选择合适的协议
		m_sock = socket(PF_INET, SOCK_STREAM, 0);
	}

	// 析构函数
	~CServerSocket() {
		//析构函数我们也需要声明成private的
	    //因为我们想要这个实例在程序运行的整个过程中都存在
	    //所以我们不允许实例自己主动调用析构函数释放对象
		WSACleanup(); // 用于清理 Winsock 库的资源。这是一个在不再需要使用 Winsock 时进行的清理操作
		closesocket(m_sock);
	}

	// 初始化网络环境，采用2.0版本网络库; 初始化成功返回TRUE,失败返回FALSE
	BOOL InitSockEnv()
	{
		// 这两行代码是用于在 Windows 中使用 Winsock API 进行网络编程的初始化步骤
		WSADATA data; // 声明了一个名为 data 的变量，类型为 WSADATA。WSADATA 是一个结构，用于存储有关 Windows Sockets 实现的信息
		// if (WSAStartup(MAKEWORD(1, 1), &data) != 0)  // WSAStartup 函数初始化 Winsock 库。MAKEWORD(1, 1) 宏用于指定所请求的 Winsock 版本，这里是请求版本 1.1 
		if (WSAStartup(MAKEWORD(2, 0), &data) != 0) // 设置"WSA_FLAG_OVERLAPPED"参数来实现异步后，需要请求版本 2.0
		{
			return FALSE; // 初始化网络环境失败
		}
		return TRUE; // 初始化网络环境成功
	}

	static CServerSocket* m_instance; // 单例

	static void releaseInstance()
	{
		if (m_instance != NULL)
		{
			CServerSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp;
		}
	}

	// 单例模式：是一种软件设计模式，其核心思想是确保类在应用程序的生命周期内只有一个实例，并提供一个全局的访问点。
	//         这意味着无论何时何地请求这个类的实例，都将返回相同的对象。单例模式通常用于管理全局资源、共享配置信息、或者提供唯一的访问点来控制某些资源
	// 单例对象：单例模式下的类实例被称为单例对象。这个对象在应用程序中只有一个实例存在，而且提供了一个全局的访问点，让其他部分的代码可以获取到这个唯一的实例

	// CHelper 类是一个嵌套类（nested class），它被声明为 CServerSocket 类的私有内部类
	class CHelper
	{
	public:
		CHelper() // 构造函数用于在对象创建时调用 CServerSocket::getInstance() 静态函数。
		{         // 因为 CHelper 类是 CServerSocket 类的嵌套类，所以它可以访问 CServerSocket 类的私有成员和静态成员   
			CServerSocket::getInstance(); // 当创建 CHelper 类的对象时，它会自动调用构造函数，从而在内部调用 CServerSocket::getInstance() 函数。
		}
		~CHelper() 
		{		  // 同样，因为 CHelper 类是 CServerSocket 类的嵌套类，所以它可以访问 CServerSocket 类的私有成员和静态成员
			CServerSocket::releaseInstance(); //  CHelper 类的对象离开作用域时，它会自动调用析构函数，从而在内部调用 CServerSocket::releaseInstance() 函数
		}
	};
	//定义一个内部类的静态对象
	//当该对象销毁的时候，调用析构函数顺便销毁我们的单例对象
	static CHelper m_helper; // 利用程序在结束时析构全局变量的特性，选择最终的释放时机
	
	
	// m_helper 是 CHelper 类的静态成员变量，在 CServerSocket 类中声明为 static

};

// extern 关键字表示该变量在其他文件中定义，而在当前文件中仅作为声明。实际的定义可能在其他文件中，这样 server 就变成了一个全局变量的声明，而不是定义
// 这样，其他源文件在包含这个头文件(ServerSocket.h)时，就能够访问到 server 这个全局变量的声明，而不会引发重复定义的错误。
// 在其他源文件中，如果需要使用 server，可以通过包含相同的头文件来获取对 server 的声明，而不会导致多次定义。
// 然后，实际的定义应该出现在一个源文件中，确保只有一个地方对 server 进行了实际的分配和初始化
// 这种做法是为了支持模块化编程，允许不同的源文件共享同一全局变量而不引起冲突
extern CServerSocket server; // 因为 RemoteCtrl.cpp 中只会引用 ServerSocket.h,为了让其可访问 ServerSocket.cpp 中的变量，这里将 server 声明为全局变量

