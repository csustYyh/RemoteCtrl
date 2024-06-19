#pragma once
#include "ClientSocket.h"
#include "RemoteClientDlg.h"
#include "CWatchDialog.h"
#include "StatusDlg.h"
#include <map>
#include "resource.h"
#include "EdoyunTool.h"

// 自定义消息的流程：
// 1.自定义消息 ID .h
// 2.定义自定义消息响应函数 .h
// 3.在消息映射表中注册消息 .cpp
// 4.实现消息响应函数 .cpp
//#define WM_SEND_PACK   (WM_USER + 1) // 发送数据包的消息ID 
// #define WM_SEND_DATA   (WM_USER + 2) // 发送数据的消息ID 
#define WM_SHOW_STATUS (WM_USER + 3) // 展示状态的消息ID 
#define WM_SHOW_WATCH  (WM_USER + 4) // 远程监控的消息ID 
#define WM_SEND_MESSAGE (WM_USER + 0x1000) // 自定义消息处理的消息ID

class CClientController
{
public:
	static CClientController* getInstance(); // 用于获取控制类的全局唯一单例对象
	int InitController(); // 初始化 这里会开启一个线程用于消息循环
	int Invoke(CWnd*& pMainWnd); // 启动 参数为指向 CWnd 对象指针的引用
	
	//LRESULT SendMessage(MSG msg); // 发送消息 这里是自己定义的函数，会隐藏 Windows API 中的 SendMessage 函数，编译器会优先调用自定义的函数。如果需要调用 Windows API 中的 SendMessage 函数，需要使用完整的命名空间来指定，例如 ::SendMessage
	//LRESULT OnSendPack(UINT msg, WPARAM wParam, LPARAM lParam); // 消息响应函数 用于处理 WM_SEND_PACK 消息
	//LRESULT OnSendData(UINT msg, WPARAM wParam, LPARAM lParam); // 消息响应函数 用于处理 WM_SEND_PACK 消息
	LRESULT OnShowStatus(UINT msg, WPARAM wParam, LPARAM lParam); // 消息响应函数 用于处理 WM_SHOW_STATUS 消息
	LRESULT OnShowWatch(UINT msg, WPARAM wParam, LPARAM lParam); // 消息响应函数 用于处理 WM_SHOW_WATCH 消息
	// 更新网络服务器的地址和端口(通知CClientScoket类对象调用UpdateAddress())
	void UpdataAddress(int nIP, int nPort)
	{
		CClientSocket::getInstance()->UpdateAddress(nIP, nPort);
	}
	// 处理命令(通知CClientScoket类对象调用DealCommand())
	int DealCommand()
	{
		return CClientSocket::getInstance()->DealCommand(); // 处理客户端发来的命令
	}
	// 关闭套接字(通知CClientScoket类对象调用CloseSocket())
	void CloseSocket()
	{
		CClientSocket::getInstance()->CloseSocket(); // 关闭套接字
	}
	// 发送命令数据包(通知CClientSocket类对象调用SendPacket()发送命令数据包)
	// para1:发送通信请求的窗口 para2:命令 para3:发送完毕后是否自动关闭套接字
	// para4:需要发送的数据包 para5:数据长度 para6:用于传递文件流指针，仅在文件下载时使用，其余情况为0
	// 返回值:成功返回 true， 失败返回 false
	bool SendCommandPack(HWND hWnd, int nCmd, bool bAutoClose = true, BYTE* pData = NULL, size_t nLength = 0, WPARAM wParam = 0); // 消息机制解决通信冲突
	

	// int SendCommandPacket(int nCmd, bool bAutoClose = true, BYTE* pData = NULL, size_t nLength = 0, std::list<CPacket>* plstPacks = NULL); // 事件机制处理通信冲突

	
	// 从内存获取远程桌面图片数据，并且使用工具库CTool将字节转化为图片格式返回
	int GetImage(CImage& image)
	{
		CClientSocket* pClient = CClientSocket::getInstance();
		// 利用工具类中的函数来实现内存数据到图片的转换
		// 因为内存数据到图片的转换是个纯功能函数，与这个项目其他业务逻辑没有直接关联，所以封装走
		return CEdoyunTool::Bytes2Image(image, pClient->GetPacket().strData);
	}
	// 创建CfileDialog类对象获取本地保存路径，调用SendCommandPack()发送下载命令数据包，设置光标位置和等待，并弹出下载窗口
	// para:目标文件的路径 返回值：成功返回 0 失败返回 -1
	int DownFile(CString strPath);
	void DownloadEnd();

	// 开启远程图传线程，然后显示图传界面
	void StartWatchScreen();
protected: 
	// void threadDownloadFile(); // 实际执行的文件下载线程函数，用于远程下载文件
	// static void threadDownloadEntry(void* arg); // 文件下载线程入口函数
	void threadWatchScreen(); // 实际执行的远程桌面线程函数，用于远程下载文件
	static void threadWatchScreenEntry(void* arg); // 远程桌面线程入口函数

	CClientController():
		m_statusDlg(&m_remoteDlg), // 成员初始化列表
		m_watchDlg(&m_remoteDlg)   // 成员初始化列表
	{ // 构造函数 初始化成员变量，指明远程桌面、下载窗口的父窗口为主界面窗口
		m_hThread = INVALID_HANDLE_VALUE;
		m_nThreadID = -1;
		//m_hThreadDownload = INVALID_HANDLE_VALUE;
		m_hThreadWatch = INVALID_HANDLE_VALUE;
		m_isClosed = true;
	}
	~CClientController() { // 析构函数
		WaitForSingleObject(m_hThread, 100);
	}
	
	// 为什么线程函数必须为静态成员函数或者全局函数
	// 在 Windows 编程中，线程函数必须为静态函数，这是因为线程函数需要满足 C 语言的函数指针的要求，而 C 语言的函数指针不支持非静态成员函数。
	// 因此，如果你使用 C++ 编写 Windows 程序，线程函数可以是全局函数或者是类的静态成员函数，但不能是非静态成员函数。
	// 这是因为静态成员函数和全局函数的调用方式是类似的，它们都不需要隐含的 this 指针，而非静态成员函数需要通过对象调用，并且需要隐含的 this 指针。
	// 由于线程函数是通过函数指针来调用的，而函数指针无法处理隐含的 this 指针，因此非静态成员函数不能直接作为线程函数。

	// 开启线程处理业务逻辑的流程：
	// 1.将全局方法 _beginthread() 调用的线程函数 threadEntry() 声明为静态，同时传入this指针作为参数
	// 2.将实际要在线程中执行的业务逻辑函数 threadFunc() 声明为非静态
	// 3.在threadEntry() 使用this指针调用 threadFunc()，这样就实现了另起一个线程执行业务逻辑
	void threadFunc(); // 实际执行的线程函数 用于建立CClientController的消息循环机制
	static unsigned __stdcall threadEntry(void* arg); // 消息循环线程入口函数 为的是拿到this指针后启动指针的线程函数 线程函数需要声明为静态或全局
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
		MSG     msg;    // 消息
		LRESULT result; // 用于记录消息处理后的返回值
		MsgInfo(MSG m) // 构造函数
		{
			result = 0;
			memcpy(&msg, &m, sizeof(MSG));
		}
		MsgInfo& operator=(const MsgInfo& m) // 重载赋值运算符
		{
			if (this != &m)
			{
				result = m.result;
				memcpy(&msg, &m.msg, sizeof(MSG));
			}
			return *this;
		}
		MsgInfo(const MsgInfo& m) // 复制构造函数
		{
			result = m.result;
			memcpy(&msg, &m.msg, sizeof(MSG));
		}
	} MSGINFO;

	// 消息映射表
	typedef LRESULT(CClientController::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParm); // MSGFUNC成员函数指针 
	static std::map<UINT, MSGFUNC> m_mapFunc; // 成员函数映射表 从消息ID到对应的功能函数的映射 可以使用整数作为键来检索对应的成员函数指针

	// 单例的实现
	static CClientController* m_instance; // 单例对象
	// 单例的解释：https://www.zhihu.com/question/639483371/answer/3383218426
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

	// 重构后所有对话框都是控制类的成员
	CWatchDialog		m_watchDlg;  // 远程桌面对话框
	CRemoteClientDlg	m_remoteDlg; // 主对话框
	CStatusDlg			m_statusDlg; // 状态对话框

	// 线程相关
	HANDLE	   m_hThread;      // 消息循环线程句柄
	HANDLE     m_hThreadWatch; // 远程桌面线程句柄
	//HANDLE     m_hThreadDownload; // 下载文件线程句柄
	unsigned   m_nThreadID;    // 消息循环线程ID

	// 下载相关
	CString    m_strRemote; // 远程下载文件路径
	CString    m_strLocal;  // 本地保存文件路径

	// 标志相关
	//bool       m_isFull;  // 用于表示缓存释放有数据 true 有， false 没有
	bool       m_isClosed; // 用于表示线程是否关闭 true 关闭， false 未关闭
};

