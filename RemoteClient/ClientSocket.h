#pragma once
#include "pch.h"
#include "framework.h"
#include <string>
#include <vector>
#include <map>
#include <list>
#include <mutex>

#define WM_SEND_PACK (WM_USER + 1)     // 发送请求包的消息
#define WM_SEND_PACK_ACK (WM_USER + 2) // 发送应答包的消息

// 包的封装设计：1-2个字节的包头 + 2/4个字节存储包的长度(根据包的大小来选择) + 具体传递的数据(前1/2个字节作为命令，后1/2个字节作为校验，中间即为正式的数据)
// 包头根据经验一般设置为：FF FE / FE FF 因为这两个字节出现的概率很小
// 常见的检验：和校验，即将具体传递的数据相加(按照字节/短整形，溢出不管)，最后相加的结果作为校验存放在包的末尾
class CPacket // 数据包类
{
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}  // 构造函数，成员变量初始化为 0
	CPacket(const CPacket& pack) // 拷贝构造函数
	{
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
	}
	CPacket& operator=(const CPacket& pack) // 重载赋值运算符
	{
		if (this != &pack) // eg: a = b; 这种赋值情况
		{
			sHead = pack.sHead;
			nLength = pack.nLength;
			sCmd = pack.sCmd;
			strData = pack.strData;
			sSum = pack.sSum;
		}
		return *this; // eg：a = a; 这种赋值情况
	}
	// 数据包封包构造函数，用于把数据封装成数据包  // size_t 是宏定义，即 typedef unsigned int size_t, 为无符号整型 4个字节; BYTE 即 unsigned char 的宏定义
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize) // para1：命令  para2：数据指针  para3：数据大小 para4:事件
	{
		sHead = 0xFEFF;
		nLength = nSize + 2 + 2; // 命令2 + 包数据nSize + 和校验2
		sCmd = nCmd;
		if (nSize > 0)
		{
			strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize); // 拷贝数据
		}
		else
		{
			strData.clear(); // 无数据清空
		}

		sSum = 0;
		for (size_t j = 0; j < strData.size(); j++) // 计算校验位
		{
			sSum += BYTE(strData[j]) & 0xFF;
		}
	}

	// 数据包解包构造函数，将包中的数据分配到成员变量中
	CPacket(const BYTE* pData, size_t& nSize) // para1：数据指针 para2：数据大小 输入时 nSize 代表这个包有多少个字节的数据，输出时返回的 nSize 代表我们在解包过程中用掉了多少个字节
	{
		size_t i = 0; // 遍历数据包
		for (; i < nSize; i++)
		{
			if (*(WORD*)(pData + i) == 0xFEFF) // 识别包头
			{
				sHead = *(WORD*)(pData + i); // 取包头
				i += 2; // WORD 为无符号短整型，占两个字节,取出包头后遍历指针 i 后移两位 
				break;
			}
		}
		if (i + 4 + 2 + 2 > nSize) // 包数据可能不全，即连 包头i + 长度4 + 命令2 + 校验2 都不全
		{
			nSize = 0;
			return; // 解析失败
		}

		nLength = *(DWORD*)(pData + i); i += 4; // 读包的长度
		if (nLength + i > nSize) // 当未完全接收
		{
			nSize = 0;
			return; // 解析失败
		}

		sCmd = *(WORD*)(pData + i); i += 2; // 读取命令

		if (nLength > (2 + 2)) // 数据的长度大于 命令2 + 校验2 说明这个包中有要读取的数据
		{
			strData.resize(nLength - 2 - 2); // 设置 string 的大小
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);  // 读取数据
			i += nLength - 2 - 2; // 读取完数据后移动遍历指针 i 到最新的位置(此时移动到校验码开始的位置)
		}

		sSum = *(WORD*)(pData + i); i += 2;  // 读校验码
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++)
		{
			sum += BYTE(strData[j]) & 0xFF;
		}

		if (sum == sSum) // 校验
		{
			nSize = i; // 头 + 长度 + 命令 + 数据 + 校验   nSize 是以引用(&)的方式传参，这里返回的 nSize 代表解包过程中用掉了多少数据
			return;
		}

		nSize = 0;
	}

	~CPacket() {} // 析构函数

	int Size() // 获得整个包的大小
	{
		return nLength + 4 + 2; //  （控制命令 + 包数据 + 和校验)nLength + 存放nLength的大小 4 + 包头 2 
	}
	const char* Data(std::string& strOut) const// 用于解析包，将包送入缓冲区 即将数据包的所有数据赋值给包类成员 strOut
	{
		strOut.resize(nLength + 4 + 2);
		BYTE* pData = (BYTE*)strOut.c_str();
		*(WORD*)pData = sHead;    pData += 2;
		*(DWORD*)pData = nLength;  pData += 4;
		*(WORD*)pData = sCmd;     pData += 2;
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();
		*(WORD*)pData = sSum;

		return strOut.c_str(); // 返回整个包
	}
public:
	WORD sHead;    // 包头 固定为 FE FF // WORD 是宏定义，即 typedef unsigned short WORD, 为无符号短整型 2个字节
	DWORD nLength; // 包中数据的长度 // DWORD 是宏定义，即 typedef unsigned long WORD, 为无符号长整型 4个字节(64位下8个字节) 这里是指下面：控制命令 + 包数据 + 和校验 的长度
	WORD sCmd;     // 控制命令
	std::string strData; // 包数据
	WORD sSum;     // 和校验
	// std::string strOut; // 整个包的数据
};

// 设计一个鼠标信息的数据结构，用于存放客户端鼠标操作命令
typedef struct MouseEvent
{
	MouseEvent() // 结构体的构造函数 用于实例化结构体时将其初始化
	{
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction; // 0表示单击，1表示双击，2表示按下，3表示放开，4不作处理
	WORD nButton; // 0表示左键，1表示右键，2表示中键，4没有按键
	POINT ptXY; // 鼠标的 x y 坐标 结构体变量，里面有 x 坐标和 y 坐标

}MOUSEEV, * PMOUSEEV;


std::string GetErrInfo(int wsaErrCode);// 通过错误代码能够获取相应的人类可读的错误信息 输入 Windows 套接字错误代码，函数将返回与此错误代码相关的错误信息

// 用于存储受控方获取到的文件信息
typedef struct file_info
{
	file_info() // 结构体的构造函数 用于实例化结构体时将其初始化
	{
		IsInvalid = FALSE; // 默认有效
		IsDirectory = FALSE;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvalid; // 标志是否有效，0-有效 1-无效
	BOOL IsDirectory; // 标志是否为目录 0-不是目录 1-是目录
	BOOL HasNext; // 是否还有后续，即子文件夹或文件 0-没有 1-有
	char szFileName[256]; // 文件/目录 名

}FILEINFO, * PFILEINFO;

enum {
	CSM_AUTOCLOSE = 1, // CSM = Client Socket Mode 自动关闭模式，即发送一个命令包，收到一个应答包后就关闭套接字
};

// 在消息机制中会用到的，自定义的数据结构，用于给 WM_SEND_PACK 消息的响应函数 SendPack() 传参
typedef struct PacketData
{
	std::string strData;  // 数据
	UINT        nMode;    // 通信模式，应答一次就关闭套接字还是多次
	WPARAM      wParam;   // 其他参数
	PacketData(const char* pData, size_t nLen, UINT mode, WPARAM nParam = 0) // 默认构造函数
	{
		strData.resize(nLen);
		memcpy((char*)strData.c_str(), pData, nLen);
		nMode = mode;
		wParam = nParam;
	}

	PacketData(const PacketData& data) // 拷贝构造函数
	{
		strData = data.strData;
		nMode = data.nMode;
		wParam = data.wParam;
	}

	PacketData& operator=(const PacketData& data) // 重载赋值运算符
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

class CClientSocket // 客户端的套接字类
{
public:
	// 使用静态函数，让类外可以访问到类里面的私有成员 访问方式：作用域名::函数名 如：CClientSocket::getInstance()
	static CClientSocket* getInstance()
	{
		if (m_instance == NULL) // 静态函数没有 this 指针，所以无法直接访问成员变量
		{
			m_instance = new CClientSocket();
		}
		return m_instance;
	}

	bool InitSocket(); // 初始化客户端socket，填好服务器地址和端口后，连接到服务器， para1:服务器ip地址 para2:服务器端口 成功返回 ture 失败 false
	
	
#define BUFFER_SIZE 4096*1024 // 4M缓冲区
	int DealCommand() // 接收服务端发来的数据，并且将数据放入缓冲区m_packet中 成功返回解析出的命令，某原因中断返回-1
	{
		if (m_sock == -1) return false; // 未与服务器成功连接

		char* buffer = m_buffer.data(); // 客户端缓冲区 new char[BUFFER_SIZE];
		
		// index 用于追踪缓冲区中待处理数据的位置。通过将其声明为静态，它可以在函数调用之间保留其值，而不是在每次函数调用时重新初始化。
		// 这样可以确保在多次调用函数时，index 会继续从上一次调用结束时的位置开始，而不是每次都从零开始。
		static size_t index = 0; // 此处用静态，为了在下面循环中保留index的值 

		while (true)
		{
			// 目的是将所有的包都放入缓冲区
			size_t len = recv(m_sock, buffer + index, BUFFER_SIZE - index, 0); // 实际接收到的长度
			if (((int)len <= 0) && ((int)index <= 0)) return -1; // 

			// 解包
			index += len;  // 缓冲区读取了 len 个字节数据，故 buffer 长度 + len
			len = index; // 解析缓冲区的所有数据，故将 index 赋值给 len 作为参数传递给下面 Cpacket 类的构造函数
			m_packet = CPacket((BYTE*)buffer, len); // 使用 CPacket 类的构造函数初始化 这里使用的解包的构造函数并赋值给 m_packet
			if (len > 0) // CPacket的解包构造函数返回的 len 是解包过程中用掉的数据，> 0 说明解包成功
			{
				// memmove(para1，para2，para3)：将一段内存数据在内存中移动，以覆盖之前的部分   para1：源内存区域的起始地址 para2：目标内存区域的起始地址 para3：要移动的字节数 
				memmove(buffer, buffer + len, index - len); // 将解包掉的数据覆盖，即更新缓冲区，将解析过的数据清理
				index -= len; // 缓冲区清理了 len 个字节数据，故 buffer 长度 - len
				return m_packet.sCmd; // 接收成功，返回接收到的命令
			}

		}
		return -1; // 其他中断清空返回 -1 
	}

	bool GetFilePath(std::string& strPath)
	{
		if ((m_packet.sCmd == 2) || (m_packet.sCmd == 3) || (m_packet.sCmd == 4)) // 命令‘2’表示查看指定目录下的文件 ‘3’执行文件 '4'下载文件
		{
			strPath = m_packet.strData; // 返回包中的数据，即文件路径
			return true;
		}

		return false;
	}

	bool GetMouseEvent(MOUSEEV& mouse)
	{
		if (m_packet.sCmd == 5)
		{
			memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEV)); // 存储客户端发来的鼠标事件
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

	void UpdateAddress(int nIP, int nPort) // 通知ClientSocket类对象调用UpdateAddress()，用于更新远程服务端IP和端口
	{
		if ((m_nIP != nIP) || (m_nPort != nPort))
		{
			m_nIP = nIP;
			m_nPort = nPort;
		}
	}

	// bool SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks, bool isAutoClosed = true); // 事件机制解决通信
	bool SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClosed = true, WPARAM wParam = 0); // 消息机制解决通信冲突
	

private:

	typedef void(CClientSocket::* MSGFUNC)(UINT uMsgm, WPARAM wParam, LPARAM lParam); // 自定义数据类型 MSGFUNC 代表回调消息响应函数(SendPack())的指针
	std::map<UINT, MSGFUNC> m_mapFunc; // 消息响应函数映射表，从消息ID到对应的响应函数的映射
	UINT m_nThreadID; // 通信线程ID
	HANDLE m_eventInvoke; // 用于通知通信线程启动的事件
	HANDLE m_hThread; // 通信线程句柄
	
	bool m_bAutoClose;
	std::list<CPacket> m_lstSend; // 待发送的包
	std::map<HANDLE, std::list<CPacket>&> m_mapAck; // HANDLE + 远程服务器应答包的list
	std::map<HANDLE, bool> m_mapAutoClosed; // HANDLE + 是否自动关闭套接字通信(即对应的命令通信一次即可完成还是需要通信多次（文件传输就要多次）)
	std::mutex m_lock;
	int				  m_nIP;    // 远程服务器的IP地址
	int				  m_nPort;  // 远程服务器的端口
	SOCKET            m_sock;   // 声明客户端 socket 对象
	CPacket           m_packet; // 声明数据包类的成员变量
	std::vector<char> m_buffer; // 客户端缓冲区

	// 将 CServerSocket 类的构造函数，复制构造函数以及赋值运算符都声明为私有
	// 这意味着阻止了类的复制和赋值操作。这是为了确保在该类的实例中不会发生意外的复制和赋值，通常是因为套接字的资源管理需要特殊处理，不适合通过默认的复制和赋值操作
	CClientSocket& operator=(const CClientSocket& ss) // 赋值运算符
	{
	}
	CClientSocket(const CClientSocket& ss); // 复制构造函数
	CClientSocket(); // 默认构造函数，调用InitSockEnv()初始化网络环境

	// 析构函数
	~CClientSocket() {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		WSACleanup(); // 用于清理 Winsock 库的资源。这是一个在不再需要使用 Winsock 时进行的清理操作
	}

	// 初始化网络环境，采用1.1版本网络库; 初始化成功返回TRUE,失败返回FALSE
	BOOL InitSockEnv()
	{
		// 这两行代码是用于在 Windows 中使用 Winsock API 进行网络编程的初始化步骤
		WSADATA data; // 声明了一个名为 data 的变量，类型为 WSADATA。WSADATA 是一个结构，用于存储有关 Windows Sockets 实现的信息
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0)  // WSAStartup 函数初始化 Winsock 库。MAKEWORD(1, 1) 宏用于指定所请求的 Winsock 版本，这里是请求版本 1.1
		{
			return FALSE; // 初始化网络环境失败
		}
		return TRUE; // 初始化网络环境成功
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

	// 单例模式：是一种软件设计模式，其核心思想是确保类在应用程序的生命周期内只有一个实例，并提供一个全局的访问点。
	//         这意味着无论何时何地请求这个类的实例，都将返回相同的对象。单例模式通常用于管理全局资源、共享配置信息、或者提供唯一的访问点来控制某些资源
	// 单例对象：单例模式下的类实例被称为单例对象。这个对象在应用程序中只有一个实例存在，而且提供了一个全局的访问点，让其他部分的代码可以获取到这个唯一的实例

	// CHelper 类是一个嵌套类（nested class），它被声明为 CServerSocket 类的私有内部类
	// CHelper 类的作用是在 CServerSocket 类的生命周期内创建一个单例对象，以确保 CServerSocket 类的单例模式的实现
	class CHelper
	{
	public:
		CHelper() // 构造函数用于在对象创建时调用 CServerSocket::getInstance() 静态函数。因为 CHelper 类是 CServerSocket 类的嵌套类，所以它可以访问 CServerSocket 类的私有成员和静态成员
		{         // 当创建 CHelper 类的对象时，它会自动调用构造函数，从而在内部调用 CServerSocket::getInstance() 函数。
			CClientSocket::getInstance();
		}
		~CHelper() // 析构函数用于在对象销毁时调用 CServerSocket::releaseInstance() 静态函数。同样，因为 CHelper 类是 CServerSocket 类的嵌套类，所以它可以访问 CServerSocket 类的私有成员和静态成员
		{		   //  CHelper 类的对象离开作用域时，它会自动调用析构函数，从而在内部调用 CServerSocket::releaseInstance() 函数
			CClientSocket::releaseInstance();
		}
	};
	// CHelper 类的构造函数和析构函数会在对象的创建和销毁时自动被调用，所以将 m_helper 定义为静态成员可以确保在 CServerSocket 类的生命周期内始终存在一个 CHelper 类的对象。
	// 这样就可以确保在程序执行过程中始终存在一个 CServerSocket 类的单例对象，从而实现了单例模式的效果
	static CHelper m_helper; // m_helper 是 CHelper 类的静态成员变量，在 CServerSocket 类中声明为 static

	// 下面的线程是为了解决，根据MVC架构重构客户端后，远程桌面属于本地客户端的一个线程，鼠标事件属于另一个线程，多线程并发发送命令给远程服务器会出现通信冲突问题
	// 具体冲突表现为:第一个命令初始化socket，接收服务器应答，还未接收完，下一个命令又initsocket，会将上一个正在通信的套接字关闭
	//              同时本地用于接收应答包的缓冲区里面可能存在服务器应答上一个命令的部分包和应答当前命令的包，无法解析了
	// 这里解决问题的出发点是利用事件机制:
	// 1.将客户端发送的多个请求放到一个队列当中
	// 2.下面创建的线程负责依次处理队列中的请求，主要是发送控制命令给远程服务器，然后接收远程服务器返回的数据包
	// 3.接收完当前命令返回的数据包后，通过 hEvent 告知发送命令包的线程，当前命令请求处理完毕
	// 4.继续处理队列中的下一个请求
	// 这种机制保证了在处理完(发送命令->接收应答)上一个请求之前，不会发送当前请求的命令给远程服务器，避免了多线程发送命令的冲突问题
	static unsigned __stdcall threadEntry(void* arg); // 线程入口函数
	void threadFunc(); // 实际执行的线程函数，用于接收服务端传过来的数据包

	// 但是上面这种机制也不好，实现起来很麻烦，故将这种线程事件机制重构为消息机制 ---240408
	// 事件机制存在的问题：
	// 1. 多个对话框都要使用网络模块：watchdlg remotedlg controller 文件下载... 
	// 2. 这样每个对话框给网络模块发送通信请求时都需要与网络模块建立事件触发机制
	// 3. 而事件触发机制最好是1对1的，但每个对话框之间或多或少都有联系
	// 4. 这种情况下使用事件机制实际上是同步的，比如文件下载，对话框就要一直阻塞等待网络模块将event设置为有信号状态
	// 5. 而利用消息机制就可以实现异步，对话框发送消息，网络模块处理完后再发送应答消息
	// 新的解决多线程并发冲突问题的机制：
	// 1.客户端dlg发请求消息(SendMessage)给网络模块，网络模块收到请求后调用SendPack()完成通信，然后发送应答消息给dlg(SendMessage)
	// 2.上面那种线程事件机制，客户端和网络模块之间要时时刻刻保证数据同步(通过hEvent)，很麻烦，这里采用回调的方式就不用时时刻刻保证数据同步了
	// 3.客户端dlg发送的请求消息：定义一个消息的数据结构（数据和数据长度，接收一个应答就关闭还是接收多个应答再关闭） 回调消息的数据结构（HWND MESSAGE）
	void threadFunc2();
	void SendPack(UINT nMsg, WPARAM wParam, LPARAM lParam); // WM_SEND_PACK 消息的响应函数， 主要用于发送数据到服务端，然后接收从服务端返回的消息，并且SendMessage()到对应的界面回调函数中

	
	bool Send(const char* pData, int nSize) // para1:发送消息的缓冲区指针 para2:消息大小 成功返回 true 失败返回 false
	{
		if (m_sock == -1) return false; // 未与客户端建立连接

		return send(m_sock, pData, nSize, 0) > 0;
	}
	bool Send(const CPacket& pack); // para1:自定义数据包，引用传参  成功返回 true 失败返回 false



};

// extern 关键字表示该变量在其他文件中定义，而在当前文件中仅作为声明。实际的定义可能在其他文件中，这样 server 就变成了一个全局变量的声明，而不是定义
// 这样，其他源文件在包含这个头文件(ServerSocket.h)时，就能够访问到 server 这个全局变量的声明，而不会引发重复定义的错误。
// 在其他源文件中，如果需要使用 server，可以通过包含相同的头文件来获取对 server 的声明，而不会导致多次定义。
// 然后，实际的定义应该出现在一个源文件中，确保只有一个地方对 server 进行了实际的分配和初始化
// 这种做法是为了支持模块化编程，允许不同的源文件共享同一全局变量而不引起冲突
extern CClientSocket server; // 因为 RemoteCtrl.cpp 中只会引用 ServerSocket.h,为了让其可访问 ServerSocket.cpp 中的变量，这里将 server 声明为全局变量






