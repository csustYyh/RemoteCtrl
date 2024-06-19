// CWatchDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "CWatchDialog.h"
#include "afxdialogex.h"
#include "ClientController.h"


// CWatchDialog 对话框

IMPLEMENT_DYNAMIC(CWatchDialog, CDialog)

CWatchDialog::CWatchDialog(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DIALOG_WATCH, pParent)
{
	m_isFull     = false;
	m_nObjWidth  = -1; // 默认 -1
	m_nObjHeight = -1; // 默认 -1
}

CWatchDialog::~CWatchDialog()
{
}

void CWatchDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_WATCH, m_picture);
}


BEGIN_MESSAGE_MAP(CWatchDialog, CDialog)
	ON_WM_TIMER()
	ON_WM_TIMER()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_STN_CLICKED(IDC_WATCH, &CWatchDialog::OnStnClickedWatch)
	ON_BN_CLICKED(IDC_BTN_LOCK, &CWatchDialog::OnBnClickedBtnLock)
	ON_BN_CLICKED(IDC_BTN_UNLOCK, &CWatchDialog::OnBnClickedBtnUnlock)
	ON_MESSAGE(WM_SEND_PACK_ACK, &CWatchDialog::OnSendPackAck)
END_MESSAGE_MAP()


// CWatchDialog 消息处理程序


// 将客户端本地远控界面的鼠标坐标转换为远程服务器的桌面坐标
CPoint CWatchDialog::UserPoint2RemoteScreenPoint(CPoint& point, bool isScreen)
{// 远控界面窗口大小 1199*730

	CRect clientRect;
	//if (isScreen) m_picture.ScreenToClient(&point); // 将屏幕坐标（屏幕上的点的坐标）转换为客户区坐标（窗口客户区内的点的坐标）即客户端本地屏幕坐标转换为本地远控窗口的坐标
	//else
	//{
	//	// 以下就是为了减去远程桌面dlg y 轴上空出一段放置两个按钮带来的坐标偏差 
	//	ClientToScreen(&point); // 将相对于远程桌面dlg的坐标转换为相对于屏幕左上角的坐标
	//	m_picture.ScreenToClient(&point); // 将相对于屏幕的坐标转换为显示远程桌面窗口的坐标(相对于m_picture控件的坐标)
	//}
	
	// 上面注释代码优化逻辑后：
	if (!isScreen) ClientToScreen(&point); // 将相对于远程桌面dlg的坐标转换为相对于屏幕左上角的坐标;
	m_picture.ScreenToClient(&point); // 将相对于屏幕的坐标转换为显示远程桌面窗口的坐标(相对于m_picture控件的坐标)


	m_picture.GetWindowRect(clientRect); // m_picture：远程dlg里面用于显示远程桌面的静态界面变量
	int width0 = clientRect.Width(); // 本地用于远程桌面的宽
	int height0 = clientRect.Height(); // 本地用于远程桌面的高
	//TRACE("%d\r\n", width0);
	//TRACE("%d\r\n", height0);
	TRACE("%d\r\n", point.x);
	TRACE("%d\r\n", point.y);
	// 本地远控窗口的坐标转换为远程桌面的坐标
	int x = point.x * m_nObjWidth / width0; // 鼠标坐标换算
	int y = (point.y) * m_nObjHeight / height0; // 鼠标坐标换算
	// 为什么要 -57：
	// 1.CWatchDialog设置中: 远程dlg大小为：799 * 487 显示远程界面的大小为：799 * 449 （因为y轴上空出一段放置两个按钮） 487 - 449 = 38
	// 2.本地(我的显示器)用于显示远程界面的大小为：1199*730
	// 3.由于point的位置是以远程dlg为坐标原点，故折算到dlg上，需要在y轴 - 38 * （730 / 487） = 56.96

 	return CPoint(x, y);
}

BOOL CWatchDialog::OnInitDialog() // 重写远程桌面dlg的初始化函数
{
	CDialog::OnInitDialog();

	// TODO:  在此添加额外的初始化
	m_isFull = false; // 远程桌面的缓存是否有数据的标记，初始化为false，即没有数据
	// SetTimer(0, 45, NULL); // para1:计时器名称 para2:时间间隔 单位ms para3:使用onTime函数 在使用消息机制来处理通信冲突后，OnTimer不再需要？ 我认为还是要的 不然只会读一次图片--0408
	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}


// 定时器 用于定时从m_image中读取图片数据，显示远程桌面
void CWatchDialog::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 0)
	{
		// 重构之前
		// CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent(); // 拿到父窗口(即客户端主窗口)的指针
	
		// 重构之后
		CClientController* pParent = CClientController::getInstance();
		if (m_isFull) // 若用于存放远程桌面的缓存有数据  则显示
		{
			CRect rect;
			m_picture.GetWindowRect(rect);
			m_nObjWidth = m_image.GetWidth(); // 等于图片大小
			m_nObjHeight = m_image.GetHeight(); // 等于图片大小
			TRACE("%d\r\n", m_nObjWidth);
			TRACE("%d\r\n", m_nObjHeight);

			// DC：DeviceContext 设备上下文 在MFC中对应 CDC 类，用于文字、图像、绘制、背景等(即用于显示相关)
			m_image.StretchBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), SRCCOPY);
			m_picture.InvalidateRect(NULL); // 重绘
			m_image.Destroy(); // 销毁
			m_isFull = false; // 设置标志为false 表示远程桌面的缓存已读取
		}
	}
	
	CDialog::OnTimer(nIDEvent);
}



LRESULT CWatchDialog::OnSendPackAck(WPARAM wParam, LPARAM lParam)
{
	// ::SendMessage(hWnd, WM_SEND_PACK_ACK, (WPARAM) new CPacket(pack), 0); 这里是网络通信线程发过来的应答消息
	if (lParam == -1|| (lParam == -2)) // 有错误
	{
		//TODO：错误处理
	}
	else if (lParam == 1) // 有警告
	{
		//TODO：对方关闭了套接字
	}
	else 
	{
		CPacket* pPacket = (CPacket*)wParam; // 拿到应答包
		if (pPacket != NULL)
		{
			CPacket head = *(CPacket*)wParam; // 拿到应答包
			delete (CPacket*)wParam; // 释放网络通信线程为应答包new的内存空间，避免内存泄漏
			switch (head.sCmd){
			case 6: // 远程桌面操作的应答， watchdlg 需要显示画面
			{
				CEdoyunTool::Bytes2Image(m_image, head.strData); // 内存数据转为图片
				CRect rect;
				m_picture.GetWindowRect(rect);
				m_nObjWidth = m_image.GetWidth(); // 等于图片大小
				m_nObjHeight = m_image.GetHeight(); // 等于图片大小
				TRACE("%d\r\n", m_nObjWidth);
				TRACE("%d\r\n", m_nObjHeight);

				// DC：DeviceContext 设备上下文 在MFC中对应 CDC 类，用于文字、图像、绘制、背景等(即用于显示相关)
				m_image.StretchBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), SRCCOPY);
				m_picture.InvalidateRect(NULL); // 重绘
				TRACE("更新图片完成%d %d %08X\r\n", m_nObjWidth, m_nObjHeight, (HBITMAP)m_image);
				m_image.Destroy(); // 销毁
				break;
			}
			case 5: // 鼠标操作的应答 watchdlg 无需额外操作
				TRACE("远程服务器应答了鼠标操作\r\n");
				break;
			case 7: // 锁屏操作的应答 watchdlg 无需额外操作
			case 8: // 解锁操作的应答  watchdlg 无需额外操作
			default:
				break;
			}
		}
	}
	return 0;
}

void CWatchDialog::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) // 当收到远程桌面数据后，才会发送鼠标命令
	{
		// 坐标转换
		CPoint remote = UserPoint2RemoteScreenPoint(point); // 得到本地坐标转换后的远程桌面的坐标

		// 封装
		MOUSEEV           event;  // 鼠标事件结构体
		event.ptXY = remote; // 鼠标坐标
		event.nButton = 0;      // 左键
		event.nAction = 1;      // 双击

		// 发送 重构之前需要通过父窗口实现通信
		// CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent(); // 拿到父窗口，才能与远程服务器通信
		// pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (LPARAM) & event); // 通过父窗口的消息响应函数来发送远程控制命令
		
		// 重构之后通过C层实现通信
		CClientController::getInstance()->SendCommandPack(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
	}
	
	CDialog::OnLButtonDblClk(nFlags, point);
}


void CWatchDialog::OnLButtonDown(UINT nFlags, CPoint point) // 这里point拿到的就是基于CWatchDialog的坐标
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) // 当收到远程桌面数据后，才会发送鼠标命令
	{
		// 坐标转换
		CPoint remote = UserPoint2RemoteScreenPoint(point); // 得到本地坐标转换后的远程桌面的坐标  注意这里输入的point坐标本身就是针对窗口而非屏幕而言的，故转换时para2使用默认的false

		// 封装
		MOUSEEV           event;  // 鼠标事件结构体
		event.ptXY = remote; // 鼠标坐标
		event.nButton = 0;      // 左键
		event.nAction = 2;      // 按下

		// 发送 因为socket的初始化只能在CRemoteClientDlg里面的SendCommandPacket()里面执行，这里其实是一个设计缺陷 只有CRemoteClientDlg才有远程服务器的IP和端口，这样相当于将通信和主窗口绑定耦合了
		// CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent(); // 拿到父窗口，才能与远程服务器通信 TODO：解耦主窗口与远程服务器的网络通信
		// pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (LPARAM) & event); // 通过父窗口的消息响应函数来发送远程控制命令

		// 重构之后通过C层实现通信
		CClientController::getInstance()->SendCommandPack(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
	}

	CDialog::OnLButtonDown(nFlags, point);
}


void CWatchDialog::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) // 当收到远程桌面数据后，才会发送鼠标命令
	{
		// 坐标转换
		CPoint remote = UserPoint2RemoteScreenPoint(point); // 得到本地坐标转换后的远程桌面的坐标

		// 封装
		MOUSEEV           event;  // 鼠标事件结构体
		event.ptXY = remote; // 鼠标坐标
		event.nButton = 1;      // 右键
		event.nAction = 1;      // 双击

		// 发送
		//CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent(); // 拿到父窗口，才能与远程服务器通信
		//pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (LPARAM) & event); // 通过父窗口的消息响应函数来发送远程控制命令

		// 重构之后通过C层实现通信
		CClientController::getInstance()->SendCommandPack(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
	}

	CDialog::OnRButtonDblClk(nFlags, point);
}


void CWatchDialog::OnRButtonDown(UINT nFlags, CPoint point)
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) // 当收到远程桌面数据后，才会发送鼠标命令
	{
		// 坐标转换
		CPoint remote = UserPoint2RemoteScreenPoint(point); // 得到本地坐标转换后的远程桌面的坐标

		// 封装
		MOUSEEV           event;  // 鼠标事件结构体
		event.ptXY = remote; // 鼠标坐标
		event.nButton = 1;      // 右键
		event.nAction = 2;      // 按下

		// 发送
		//CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent(); // 拿到父窗口，才能与远程服务器通信
		//pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (LPARAM) & event); // 通过父窗口的消息响应函数来发送远程控制命令

		// 重构之后通过C层实现通信
		CClientController::getInstance()->SendCommandPack(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
	}

	CDialog::OnRButtonDown(nFlags, point);
}


void CWatchDialog::OnLButtonUp(UINT nFlags, CPoint point)
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) // 当收到远程桌面数据后，才会发送鼠标命令
	{
		// 坐标转换
		CPoint remote = UserPoint2RemoteScreenPoint(point); // 得到本地坐标转换后的远程桌面的坐标

		// 封装
		MOUSEEV           event;  // 鼠标事件结构体
		event.ptXY = remote; // 鼠标坐标
		event.nButton = 0;      // 左键
		event.nAction = 3;      // 放开

		// 发送
		//CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent(); // 拿到父窗口，才能与远程服务器通信
		//pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (LPARAM) & event); // 通过父窗口的消息响应函数来发送远程控制命令

		// 重构之后通过C层实现通信
		CClientController::getInstance()->SendCommandPack(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
	}

	CDialog::OnLButtonUp(nFlags, point);
}


void CWatchDialog::OnRButtonUp(UINT nFlags, CPoint point)
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) // 当收到远程桌面数据后，才会发送鼠标命令
	{
		// 坐标转换
		CPoint remote = UserPoint2RemoteScreenPoint(point); // 得到本地坐标转换后的远程桌面的坐标

		// 封装
		MOUSEEV           event;  // 鼠标事件结构体
		event.ptXY = remote; // 鼠标坐标
		event.nButton = 1;      // 右键
		event.nAction = 3;      // 放开

		// 发送
		// CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent(); // 拿到父窗口，才能与远程服务器通信
		// pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (LPARAM) & event); // 通过父窗口的消息响应函数来发送远程控制命令
		
		// 重构之后通过C层实现通信
		CClientController::getInstance()->SendCommandPack(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
	}
	
	CDialog::OnRButtonUp(nFlags, point);
}


void CWatchDialog::OnMouseMove(UINT nFlags, CPoint point)
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) // 当收到远程桌面数据后，才会发送鼠标命令
	{
		// 坐标转换
		CPoint remote = UserPoint2RemoteScreenPoint(point); // 得到本地坐标转换后的远程桌面的坐标

		// 封装
		MOUSEEV           event;  // 鼠标事件结构体
		event.ptXY = remote; // 鼠标坐标
		event.nButton = 4;      // 没有按键
		event.nAction = NULL;   // 移动

		// 发送 
		//CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent(); // 拿到父窗口，才能与远程服务器通信 
		//pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (LPARAM) & event); // 通过父窗口的消息响应函数来发送远程控制命令

		// 重构之后通过C层实现通信
		CClientController::getInstance()->SendCommandPack(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
	}
	

	CDialog::OnMouseMove(nFlags, point);
}


void CWatchDialog::OnStnClickedWatch() // 添加左键单击的事件处理程序； MFC dialog 属性设置中，没有自带单击消息
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) // 当收到远程桌面数据后，才会发送鼠标命令
	{
		CPoint point;
		GetCursorPos(&point);

		// 坐标转换
		// 注意下面para2 = true  因为OnStnClickedWatch()这个函数是和 STN_WATCH 控件绑定的，这个控件就是本地的监视窗口 m_picture
		// 所以这个函数中的鼠标坐标都要先从相对于本地显示器的坐标转为相对于监视窗口的坐标，故para2 = true
		// 而在上面的鼠标事件中，我们用的是 MFC dialog 属性设置中自带的消息，本身坐标就是相对于监视窗口而言的
		// 所以只有这里 para2=true 进行本地桌面到监视窗口的坐标转换，上面的函数 para2 都是使用默认参数 false
		CPoint remote = UserPoint2RemoteScreenPoint(point, true); // 得到本地坐标转换后的远程桌面的坐标

		// 封装
		MOUSEEV           event;  // 鼠标事件结构体
		event.ptXY = remote; // 鼠标坐标
		event.nButton = 0;      // 左键
		event.nAction = 0;      // 单击

		// 发送
		//CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent(); // 拿到父窗口，才能与远程服务器通信
		//pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (LPARAM) & event); // 通过父窗口的消息响应函数来发送远程控制命令

		// 重构之后通过C层实现通信
		CClientController::getInstance()->SendCommandPack(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
	}	

}


void CWatchDialog::OnOK() // 重写远程桌面dlg的初始化函数 避免监控时本地按下回车键就直接退出监控窗口
{
	// TODO: 在此添加专用代码和/或调用基类

	// CDialog::OnOK() 是 MFC 中的一个成员函数，用于处理对话框中 "确定" 按钮（OK 按钮）的点击事件。当用户点击对话框中的 "确定" 按钮时，系统会调用 OnOK() 函数
	// 默认情况下，OnOK() 函数的行为是关闭对话框，并且将对话框的返回值设置为 IDOK，通知程序用户点击了 "确定" 按钮。
	// 这意味着程序可以通过检查对话框的返回值来确定用户是否点击了 "确定" 按钮
	
	// CDialog::OnOK(); // 这里直接屏蔽了 OnOK() 即按下回车不做任何反应
}


void CWatchDialog::OnBnClickedBtnLock()
{
	// 发送 
	//CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent(); // 拿到父窗口，才能与远程服务器通信 
	//pParent->SendMessage(WM_SEND_PACKET, 7 << 1 | 1); // 通过父窗口的消息响应函数来发送锁机命令

	// 重构之后通过C层实现通信
	CClientController::getInstance()->SendCommandPack(GetSafeHwnd(), 7);
}


void CWatchDialog::OnBnClickedBtnUnlock()
{
	// 发送 
	//CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent(); // 拿到父窗口，才能与远程服务器通信 
	//pParent->SendMessage(WM_SEND_PACKET, 8 << 1 | 1); // 通过父窗口的消息响应函数来发送解锁命令

	// 重构之后通过C层实现通信
	CClientController::getInstance()->SendCommandPack(GetSafeHwnd(), 8);
}
