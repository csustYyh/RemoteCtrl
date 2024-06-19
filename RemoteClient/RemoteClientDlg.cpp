
// RemoteClientDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"
#include "ClientController.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
public:
	
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CRemoteClientDlg 对话框



CRemoteClientDlg::CRemoteClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REMOTECLIENT_DIALOG, pParent)
	, m_server_address(0)
	, m_nPort(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRemoteClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_IPAddress(pDX, IDC_IPADDRESS_SERV, m_server_address);
	DDX_Text(pDX, IDC_EDIT_PORT, m_nPort);
	DDX_Control(pDX, IDC_TREE_DIR, m_Tree);
	DDX_Control(pDX, IDC_LIST_FILE, m_List);
}

void CRemoteClientDlg::InitUIData()
{
	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	UpdateData();
	//m_server_address = 0x7F000001; // 远程服务器地址
	m_server_address = 0xC0A83865; // 调试用虚拟机的地址：192.168.56.101
	m_server_address = 0x7F000001; // 调试用本机的地址：127.0.0.1
	m_nPort = _T("9527"); // 远程服务器端口
	CClientController* pController = CClientController::getInstance();
	pController->UpdataAddress(m_server_address, atoi((LPCTSTR)m_nPort)); // 同步后的变量用来更新地址，还是MVC模式，通过控制层来调用
	UpdateData(FALSE);
	m_dlgStatus.Create(IDD_DLG_STATUS, this); // 创建一个名为 m_dlgStatus 的对话框对象，并使用资源 ID IDD_DLG_STATUS 来定义对话框的外观和布局，并将该对话框作为当前窗口的子窗口嵌入到当前窗口中
	m_dlgStatus.ShowWindow(SW_HIDE); // 隐藏名为 m_dlgStatus 的对话框窗口

}

void CRemoteClientDlg::DealCommand(WORD nCmd, const std::string& strData, LPARAM lParam)
{
	switch (nCmd) {
	case 1: // 获取驱动信息操作的应答
		Str2Tree(strData, m_Tree);
		break;
	case 2: // 获取文件信息的应答 
		UpdateFileInfo(*((PFILEINFO)strData.c_str()), (HTREEITEM)lParam);
		break;
	case 3: // 运行文件的应答 remotedlg无需额外操作
		TRACE("run file done!\r\n");
		MessageBox("打开文件完成！", "操作完成", MB_ICONINFORMATION);
		break;
	case 4: // 下载操作的应答 
		UpdateDownloadFile(strData, (FILE*)lParam);
		break;
	case 9: // 删除文件的应答
		TRACE("delete file done!\r\n");
		MessageBox("删除文件完成！", "操作完成", MB_ICONINFORMATION);
		break;
	case 1981: // 测试按钮的应答
		TRACE("test connnection sucess!\r\n");
		MessageBox("连接测试成功！", "连接成功", MB_ICONINFORMATION);
		break;
	default:
		TRACE("unknow data received! %d\r\n", nCmd);
		break;
	}
}

void CRemoteClientDlg::Str2Tree(const std::string& drivers, CTreeCtrl& tree)
{
	std::string dr; // 用于构建驱动器字符串

	tree.DeleteAllItems(); // 先清空 清空树状控件中的所有项
	for (size_t i = 0; i < drivers.size(); i++)
	{
		if (drivers[i] == ',') // C,D,E,F, ==> C: D: E: F:
		{
			dr += ":"; // 将冒号添加到驱动器字符串，形成如"C:"的格式
			HTREEITEM hTemp = tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST); // 插入显示  在树状控件中插入驱动器项
			tree.InsertItem("", hTemp, TVI_LAST); //	在当前驱动器项下插入一个子项（此处为NULL）即磁盘下肯定有子节点 文件夹一定能点进去，不管里面是否为空
			dr.clear(); // 清空驱动器字符串，准备处理下一个驱动器
			continue;
		}
		dr += drivers[i]; // 构建驱动器字符串
	}
}

void CRemoteClientDlg::UpdateFileInfo(const FILEINFO& finfo, HTREEITEM hParent)
{
	if (finfo.HasNext == FALSE) return;
	if (finfo.IsDirectory) // 如果是目录
	{
		if (CString(finfo.szFileName) == "." || CString(finfo.szFileName) == "..") // 忽略"."和".."目录，继续处理下一个命令
		{
			return;
		}
		HTREEITEM hTemp = m_Tree.InsertItem(finfo.szFileName, hParent, TVI_LAST); // 插入 即展示受控方传回的信息 在执行命令2时，返回的lParam中存放了 hTreeSelected
		m_Tree.InsertItem("", hTemp, TVI_LAST); // 如果是目录 则给其建立一个子节点
		m_Tree.Expand(hParent, TVE_EXPAND); // 展开树形控件中指定的树节点
	}

	else  // 文件插入
	{
		m_List.InsertItem(0, finfo.szFileName);
	}
}

void CRemoteClientDlg::UpdateDownloadFile(const std::string& strData, FILE* pFile)
{
	static LONGLONG length = 0, index = 0;
	if (length == 0) // 下载操作的第一个应答包里面是文件大小
	{
		length = *(long long*)strData.c_str();
		if (length == 0) // 返回文件大小是0
		{
			CClientController::getInstance()->DownloadEnd();
		}
	}
	else if (length > 0 && (index >= length)) // 文件下载完毕
	{
		fclose(pFile);
		length = 0;
		index = 0;
		CClientController::getInstance()->DownloadEnd();
	}
	else // 下载操作从第二个应答包开始都是文件数据,故写文件
	{
		fwrite(strData.c_str(), 1, strData.size(), pFile);
		index += strData.size();
		if (index >= length) // 下载完毕
		{
			fclose(pFile);
			length = 0;
			index = 0;
			CClientController::getInstance()->DownloadEnd();
		}
	}
}

// 重构之前：这里的处理逻辑将视图层和模型层(数据)绑定了，导致子窗口，如远程桌面窗口不能直接使用CClientSocket通信，而是必须发消息给主对话框RemoteClientDlg来实现通信
//         因为ip地址和端口这两个用于通信的关键数据只有在主对话框才能拿到(UpdateData())
//         所以需要MVC设计，将应用程序的数据、业务逻辑(M)和用户界面(V)分离开来，以便更好地管理和维护代码，提高代码的可重用性和可扩展性，并促进团队的协作开发
// 重构之后：通过C层(CClientController类)来实现MV层之间的通信，故下面这个函数也就不再需要
//int CRemoteClientDlg::SendCommandPacket(int nCmd, bool bAutoClose, BYTE* pData, size_t nLength)
//{
//	// 在对话框控件的值更改后，调用UpdateData(TRUE)，将控件的值更新到相应的变量
//    // UpdateData(FALSE) 则将变量的值更新到相应的控件
//	// UpdateData(); // 用于将控件的数据与变量进行同步 参数默认为 TRUE MVC解耦后不会在这里更新
//
//	// MVC设计模式，这里是V层(RemoteClientDlg)，通过C层(CClientController)来与M层(CClientSocket)通信 
//	// 减少或消除V与M的耦合
//	return CClientController::getInstance()->SendCommandPacket(nCmd, bAutoClose, pData, nLength);
//}

// 消息映射表 用于将特定的消息（如鼠标点击、键盘输入、窗口绘制等）与相应的消息处理函数关联起来
// 通过消息映射表，程序员可以指定在接收到特定消息时应该调用哪个消息处理函数来处理该消息
BEGIN_MESSAGE_MAP(CRemoteClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BYN_TEST, &CRemoteClientDlg::OnBnClickedBynTest)
	ON_BN_CLICKED(IDC_BTN_FILEINFO, &CRemoteClientDlg::OnBnClickedBtnFileinfo)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMDblclkTreeDir)
	ON_NOTIFY(NM_CLICK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMClickTreeDir)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_FILE, &CRemoteClientDlg::OnNMRClickListFile)
	ON_COMMAND(ID_DOWNLOAD_FILE, &CRemoteClientDlg::OnDownloadFile)
	ON_COMMAND(ID_DELETE_FILE, &CRemoteClientDlg::OnDeleteFile)
	ON_COMMAND(ID_RUN_FILE, &CRemoteClientDlg::OnRunFile)
	// 解耦后，窗口与窗口之间不再耦合，都通过C层来交互，故不再需要消息机制
	// ON_MESSAGE(WM_SEND_PACKET, &CRemoteClientDlg::OnSendPacket) // ③注册消息 告诉系统消息 ID WM_SEND_PACKET 对应的响应函数是 OnSendPacket()
	ON_BN_CLICKED(IDC_BTN_START_WATCH, &CRemoteClientDlg::OnBnClickedBtnStartWatch)
	ON_WM_TIMER()
	ON_NOTIFY(IPN_FIELDCHANGED, IDC_IPADDRESS_SERV, &CRemoteClientDlg::OnIpnFieldchangedIpaddressServ)
	ON_EN_CHANGE(IDC_EDIT_PORT, &CRemoteClientDlg::OnEnChangeEditPort)
	ON_MESSAGE(WM_SEND_PACK_ACK, &CRemoteClientDlg::OnSendPackAck)
END_MESSAGE_MAP()


// CRemoteClientDlg 消息处理程序

BOOL CRemoteClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// TODO: 在此添加额外的初始化代码
	InitUIData();
		
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CRemoteClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CRemoteClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CRemoteClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



LRESULT CRemoteClientDlg::OnSendPackAck(WPARAM wParam, LPARAM lParam)
{
	// ::SendMessage(hWnd, WM_SEND_PACK_ACK, (WPARAM) new CPacket(pack), 0); 这里是网络通信线程发过来的应答消息
	if (lParam == -1 || (lParam == -2)) // 有错误
	{
		//TODO：错误处理
		TRACE("socket is error %d\r\n", lParam);
	}
	else if (lParam == 1) // 有警告
	{
		//TODO：对方关闭了套接字
		TRACE("socket is closed!\r\n");
	}
	else
	{	
		if (wParam != NULL) // 应答不为空
		{
			CPacket head = *(CPacket*)wParam; // 拿到应答包
			delete (CPacket*)wParam; // 释放网络通信线程为应答包new的内存空间，避免内存泄漏
			DealCommand(head.sCmd, head.strData, lParam);
		}
	}
	return 0;
}

void CRemoteClientDlg::OnBnClickedBynTest()
{
	// 解耦MV前，这里还要调用V层(CRemoteClientDlg类)中定义的发送命令函数来与M层(CClientSocket类)通信
	// SendCommandPacket(1981);
	// 解耦MV后，V层通过C层(CClientController类)来与M层通信
	CClientController::getInstance()->SendCommandPack(GetSafeHwnd(), 1981);
}


// 发送命令为1的数据给服务器端，接收服务器返回的驱动器信息，然后在MFC对话框中的树状控件中显示这些驱动器信息
// 每个驱动器的格式是类似于"C:"，并作为树状控件的顶层项，每个驱动器下面再插入一个子项（此处为NULL）
void CRemoteClientDlg::OnBnClickedBtnFileinfo()
{
	// TODO: 在此添加控件通知处理程序代码
	// 解耦MV前，这里还要调用V层(CRemoteClientDlg类)中定义的发送命令函数来与M层(CClientSocket类)通信
	// int ret = SendCommandPacket(1); // 发送命令为1的数据
	// 解耦MV后，V层通过C层(CClientController类)来与M层通信
	std::list<CPacket> lstPackets; // 用以存放远程服务器的应答包(远程服务器返回的文件信息)
	int ret = CClientController::getInstance()->SendCommandPack(GetSafeHwnd(), 1, true, NULL, 0);
	if (ret == 0)
	{
		AfxMessageBox(_T("命令处理失败!"));
		return;
	}
	// 解耦之前
	//CClientSocket* pClient = CClientSocket::getInstance(); // 获取单例模式下的客户端套接字实例
	//std::string drivers = pClient->GetPacket().strData; // 获取包数据
	// 解耦之后
	//CPacket& head = lstPackets.front(); // 拿到应答包
	//std::string drivers = head.strData; // 拿到应答包的数据(即文件信息)
	//
	//std::string dr; // 用于构建驱动器字符串

	//m_Tree.DeleteAllItems(); // 先清空 清空树状控件中的所有项
	//for (size_t i = 0; i < drivers.size(); i++)
	//{
	//	if (drivers[i] == ',') // C,D,E,F, ==> C: D: E: F:
	//	{
	//		dr += ":"; // 将冒号添加到驱动器字符串，形成如"C:"的格式
	//		HTREEITEM hTemp = m_Tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST); // 插入显示  在树状控件中插入驱动器项
	//		m_Tree.InsertItem("", hTemp, TVI_LAST); //	在当前驱动器项下插入一个子项（此处为NULL）即磁盘下肯定有子节点 文件夹一定能点进去，不管里面是否为空
	//		dr.clear(); // 清空驱动器字符串，准备处理下一个驱动器
	//		continue;
	//	}
	//	dr += drivers[i]; // 构建驱动器字符串
	//}
}



// 获取树状控件中指定节点（hTree）的路径，并以CString形式返回
CString CRemoteClientDlg::GetPath(HTREEITEM hTree) 
{
	CString strRet, strTmp;
	do  // 从指定节点开始，迭代获取节点路径
	{
		strTmp = m_Tree.GetItemText(hTree); // 获取当前节点的文本内容
		strRet = strTmp + '\\' + strRet; // 将当前节点的文本内容添加到路径字符串中，并在前面加上反斜杠
		hTree = m_Tree.GetParentItem(hTree); // 获取当前节点的父节点
	} while (hTree != NULL); // 继续迭代直到达到根节点
	return strRet; // 返回完整路径
}

// 删除文件树项的子项
void CRemoteClientDlg::DeleteTreeChildrenItem(HTREEITEM hTree)
{
	HTREEITEM hSub = NULL;
	do // 使用循环删除指定节点的所有子节点
	{
		hSub = m_Tree.GetChildItem(hTree); // 获取指定节点的第一个子节点
		if (hSub != NULL) m_Tree.DeleteItem(hSub); // 删除子节点
	} while (hSub != NULL); // 继续循环，直到没有子节点为止

}

// 获取磁盘树列表
void CRemoteClientDlg::LoadFileInfo()
{
	CPoint ptMouse;
	GetCursorPos(&ptMouse); // 获取当前鼠标位置 这里是相对于屏幕
	m_Tree.ScreenToClient(&ptMouse); // 转化为相对于客户端界面的鼠标位置

	HTREEITEM hTreeSelected = m_Tree.HitTest(ptMouse, 0);
	if (hTreeSelected == NULL) // 如果没有双击到磁盘树节点
		return;

	// 在多次单/双击的情况下，不会多次在磁盘树和文件列表插入
	DeleteTreeChildrenItem(hTreeSelected); 
	m_List.DeleteAllItems();

	// 如果双击到磁盘树节点则获取节点信息
	CString strPath = GetPath(hTreeSelected); // 获取节点完整路径

	// 解耦MV前，这里还要调用V层(CRemoteClientDlg类)中定义的发送命令函数来与M层(CClientSocket类)通信
	// int nCmd = SendCommandPacket(2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength()); // 发送路径给受控端
	// 解耦MV后，V层通过C层(CClientController类)来与M层通信
	// std::list<CPacket> lstPackets; // 存放远程服务器应答结果
	CClientController::getInstance()->SendCommandPack(GetSafeHwnd(), 2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength(), (WPARAM)hTreeSelected);
	//if (lstPackets.size() > 0) // 存在应答结果 迭代获取
	//{
	//	std::list<CPacket>::iterator it = lstPackets.begin();
	//	for (; it != lstPackets.end(); it++)
	//	{
	//		PFILEINFO pInfo = (PFILEINFO)(*it).strData.c_str();
	//		if (pInfo->HasNext == FALSE) continue;
	//		if (pInfo->IsDirectory) // 如果是目录
	//		{
	//			if (CString(pInfo->szFileName) == "." || CString(pInfo->szFileName) == "..") // 忽略"."和".."目录，继续处理下一个命令
	//			{
	//				continue;
	//			}
	//			HTREEITEM hTemp = m_Tree.InsertItem(pInfo->szFileName, hTreeSelected, TVI_LAST); // 插入 即展示受控方传回的信息
	//			m_Tree.InsertItem("", hTemp, TVI_LAST); // 如果是目录 则给其建立一个子节点
	//		}

	//		else  // 文件插入
	//		{
	//			m_List.InsertItem(0, pInfo->szFileName);
	//		}
	//	}	
	//}

	// 重构之前
	// PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str(); // 接收受控方返回的信息
	// CClientSocket* pClient = CClientSocket::getInstance();
	//PFILEINFO pInfo = (PFILEINFO)lstPackets.front().strData.c_str();
	//while (pInfo->HasNext) // 数据可能过多 一次发不完 故循环处理
	//{
	//	if (pInfo->IsDirectory) // 如果是目录
	//	{
	//		if (CString(pInfo->szFileName) == "." || CString(pInfo->szFileName) == "..") // 忽略"."和".."目录，继续处理下一个命令
	//		{
	//			// int cmd = pClient->DealCommand(); // 继续处理 重构之前
	//			int cmd = CClientController::getInstance()->DealCommand(); // 重构之后
	//			if (cmd < 0) break;
	//			pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	//			continue;
	//		}
	//		HTREEITEM hTemp = m_Tree.InsertItem(pInfo->szFileName, hTreeSelected, TVI_LAST); // 插入 即展示受控方传回的信息
	//		m_Tree.InsertItem("", hTemp, TVI_LAST); // 如果是目录 则给其建立一个子节点
	//	}

	//	else  // 文件插入
	//	{
	//		//HTREEITEM hTemp = m_Tree.InsertItem(pInfo->szFileName, hTreeSelected, TVI_LAST); // 插入 即展示受控方传回的信息
	//		m_List.InsertItem(0, pInfo->szFileName);
	//	}
	//	// int cmd = pClient->DealCommand(); // 继续处理 重构之前
	//	int cmd = CClientController::getInstance()->DealCommand(); // 重构之后
	//	if (cmd < 0) break;
	//	pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	//}
	// pClient->CloseSocket(); // 重构之前
	// CClientController::getInstance()->CloseSocket(); // 重构之后

}

// 磁盘树列表的双击事件 
void CRemoteClientDlg::OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	LoadFileInfo();
}


void CRemoteClientDlg::OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	LoadFileInfo();
}


void CRemoteClientDlg::OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;

	CPoint ptMouse, ptList;
	GetCursorPos(&ptMouse);
	ptList = ptMouse;
	m_List.ScreenToClient(&ptList);
	int ListSelected = m_List.HitTest(ptList);
	if (ListSelected < 0) return;
	CMenu menu;
	menu.LoadMenu(IDR_MENU_RCLICK);
	CMenu* pPupup = menu.GetSubMenu(0);
	if (pPupup != NULL)
	{
		pPupup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, ptMouse.x, ptMouse.y, this);
	}
}

// 刷新文件列表
void CRemoteClientDlg::LoadFileCurrent()
{
	HTREEITEM hTree = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hTree);
	m_List.DeleteAllItems(); // 当前文件列表全部删除

	// 解耦MV前，这里还要调用V层(CRemoteClientDlg类)中定义的发送命令函数来与M层(CClientSocket类)通信
	// int nCmd SendCommandPacket(2, false, (BYTE*)(LPCSTR)strPath, strPath.GetLength()); // 发送查看指定目录下的文件命令
	// 解耦MV后，V层通过C层(CClientController类)来与M层通信
	int nCmd = CClientController::getInstance()->SendCommandPack(GetSafeHwnd(), 2, false, (BYTE*)(LPCSTR)strPath, strPath.GetLength());

	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str(); // 接收受控方返回的信息

	// CClientSocket* pClient = CClientSocket::getInstance(); // 重构之前
	while (pInfo->HasNext) // 数据可能过多 一次发不完 故循环处理
	{
		if (!pInfo->IsDirectory) // 如果不是目录
		{
			m_List.InsertItem(0, pInfo->szFileName);
		}
		// int cmd = pClient->DealCommand(); // 继续处理 重构之前
		int cmd = CClientController::getInstance()->DealCommand(); // 重构之后
		if (cmd < 0) break;
		pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	}
	// pClient->CloseSocket(); // 重构之前
	// CClientController::getInstance()->CloseSocket(); // 重构之后
}

// 解耦后，下面的四个函数(文件下载&远程桌面的线程入口函数+线程函数)不再需要在V层做具体实现

// 真正的下载函数，首先得到文件存储的位置，这里使用CFileDialog类来完成

//void CRemoteClientDlg::threadDownFile()
//{
//	int nListSelected = m_List.GetSelectionMark(); // 获取文件列表中选中的项的索引
//	CString strFile = m_List.GetItemText(nListSelected, 0); // 获取文件列表中选中的文件名
//
//	// 创建了一个文件对话框
//	// para1:表示文件对话框是用来打开文件，而不是保存文件。如果值为 TRUE，则表示用来保存文件
//	// para2:表示文件对话框将显示所有类型的文件, 也可以传入特定的文件扩展名，如 "*.txt"，表示只显示 .txt 格式的文件
//	// para3:用于指定文件对话框中显示的默认文件名 para4:指定了文件对话框在用户指定的文件已经存在时是否显示覆盖提示,这里的参数表示会弹出提示框询问用户是否覆盖
//	// para5:指定了对话框的过滤器，通常用于限制用户可以选择的文件类型,由于 para2 使用 "*" 参数表示显示所有类型的文件，因此过滤器为空，表示不限制文件类型
//	// para6:这个参数表示对话框的父窗口，通常是指向当前窗口的指针
//	CFileDialog dlg(FALSE, "*", strFile, OFN_OVERWRITEPROMPT, NULL, this); // 创建了一个文件对话框
//	// 显示文件对话框（模态），并等待用户的操作，直到用户关闭对话框为止
//	// DoModal()会暂停当前程序的执行，直到用户完成对话框的操作，比如选择了一个文件并点击了对话框中的打开按钮（或者点击了取消按钮），然后返回用户的操作结果
//	// 返回值表示用户的操作结果，常用的返回值有 IDOK（用户点击了确定按钮）和 IDCANCEL（用户点击了取消按钮）。
//	if (dlg.DoModal() == IDOK) // 用户点击确定
//	{
//		// 创建文件流用于保存待下载文件
//		FILE* pFile = fopen(dlg.GetPathName(), "wb+");
//		if (pFile == NULL)
//		{
//			// 在 C++ 中，_T("中文") 是宏定义的一种用法，用于在 Unicode 和 ANSI 字符串之间进行转换，以便在不同的编译模式下（Unicode 或 ANSI）使用相同的代码
//			// _T("中文")会在 Unicode 编译模式下被展开为 L"中文"，在 ANSI 编译模式下被展开为 "中文"。
//			// 在 Unicode 编译模式下，L 前缀表示字符串是 Unicode 编码的，每个字符占两个字节；
//			// 而在 ANSI 编译模式下，字符串不带前缀，表示使用本地的 ANSI 字符集，每个字符占一个字节
//			AfxMessageBox(_T("本地没有权限保存该文件，或者文件无法创建！")); // _T("中文") 的作用是根据编译模式选择正确的字符串类型，以确保代码的兼容性 
//			m_dlgStatus.ShowWindow(SW_HIDE);
//			EndWaitCursor();
//			return;
//		}
//
//		// 01 获取待下载文件路径
//		HTREEITEM hSelected = m_Tree.GetSelectedItem(); // 获取文件夹树中选中的项，即当前选中文件的上级目录
//		strFile = GetPath(hSelected) + strFile; // 调用在本.cpp文件中实现的获取树状控件中指定节点（hTree）的路径函数来获取父目录的路径，再与文件名拼接即可得到当前选中文件的完整路径
//		TRACE("%s\r\n", LPCSTR(strFile));
//		CClientSocket* pClient = CClientSocket::getInstance();
//
//		// 02 发送下载命令给受控方(服务器)
//		do
//		{
//			// int ret = SendCommandPacket(4, FALSE, (BYTE*)(LPCSTR)strFile, strFile.GetLength()); // para2=false，因为下载文件时需要不断接收，不能自动关闭通信 不另起线程执行下载文件功能时
//			// 当创建子线程执行下载任务时，不能在子线程中直接调用SendCommandPacket()
//			// MFC 中 CWnd 是 CThreadCmd 的一个子类，即所有的窗口都是依赖于一个线程的
//			// 因为SendCommandPacket()执行了UpdateData()，而UpdateData()是属于主线程的函数 调用时会 assert 当前进程id与函数所属进程id是否匹配，不匹配则崩溃，避免子线程和主线程访问同一控件冲突 
//			//DWORD threadId = ::GetCurrentThreadId(); // 获取当前线程的线程 ID
//			//TRACE(_T("我是子线程，线程号为：%lu\n"), threadId); // 输出线程 ID 到输出窗口 // 11560
//		
//			// 故这里采用发送消息来让主线程执行 SendCommandPacket()，然后主线程给子线程一个返回值，在这里即 ret = 4	
//			// int ret = SendMessage(WM_SEND_PACKET, 4 << 1 | 0, (LPARAM)(LPCSTR)strFile); // 通过消息处理函数来发送下载命令
// 
//			// 解耦MCV后，V层通过C层(CClientController类)来与M层通信
//			int ret = CClientController::getInstance()->SendCommandPacket(4, false, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
//			
//			if (ret < 0) // 下载失败
//			{int ret = SendMessage(WM_SEND_PACKET, 4 << 1 | 0, (LPARAM)(LPCSTR)strFile); // 通过消息处理函数来发送下载命令
//				AfxMessageBox("执行下载命令失败！");
//				TRACE("执行下载失败, ret = %d\r\n", ret);
//				break;
//			}
//
//			// 03 服务器返回待下载文件的大小
//			// 服务器接收到下载文件命令后返回来的第一个包中 .strData 是下载文件的大小长度
//			long long nLength = *(long long*)CClientSocket::getInstance()->GetPacket().strData.c_str(); // 待下载文件的长度
//			if (nLength == 0)
//			{
//				AfxMessageBox("文件长度为0或者无法读取文件！");
//				break;
//			}
//
//			// 04 接收服务器发送过来的文件，下载保存到GetPathName()路径下面
//			long long nCount = 0; // 记录客户端已接收文件的大小
//			while (nCount < nLength)
//			{
//				ret = pClient->DealCommand();
//				if (ret < 0)
//				{
//					AfxMessageBox("文件长度为0或者无法读取文件！");
//					TRACE("传输失败： ret = %d\r\n", ret);
//					break;
//				}
//				fwrite(pClient->GetPacket().strData.c_str(), 1, pClient->GetPacket().strData.size(), pFile);
//				nCount += pClient->GetPacket().strData.size();
//			}
//		} while (false);
//		fclose(pFile);
//		pClient->CloseSocket();
//	}
//	m_dlgStatus.ShowWindow(SW_HIDE);
//	EndWaitCursor();
//	MessageBox(_T("下载完成!"), _T("完成")); // 下载完成后弹出提示
//}

//// 开启线程下载文件
//// 下载文件操作，该操作需要另起线程来完成下载操作，由于启动线程的特殊要求，首先，启动的线程不能带有this指针，也就说该线程函数为静态函数
//// 但是在线程函数中需要用到this指针来操作（成员函数、成员变量等），因此需要中转机制完成，把this指针当成参数传给线程，再通过this指针调用成员函数
//// ---------------------------线程函数--------------------------------------------------------
//// windows线程函数必须为全局函数或者类的静态成员函数  因为线程函数需要满足 C 语言的函数指针的要求，而 C 语言的函数指针不支持非静态成员函数
//// 因为类的非静态成员函数只能通过类的对象去调用，但是创建线程时从哪里能获得类的对象而去调用类的成员函数呢
//// 类的静态成员函数是类所有，不专属于类的任何一个对象，所以不创建类的对象也可以调用
//// 静态成员函数则属于类, 不属于对象.  而this指针是指向对象自己的指针,即有了对象才有this指针.  
//// 所以你不能在线程函数中直接使用this指针, 因为线程函数中根本就没有这个东西,所以需要传进去,然后进行类型转换
//// ---------------------------this 指针--------------------------------------------------------
//// this指针, 并不是"成员变量", 而是从函数中传进去的. 即所有的非静态成员函数都会被加上一个this指针参数, 这是编译器自己加的
//// 在C++中，this是一个指向当前对象的指针，它是一个隐藏的参数，用于指示当前正在执行成员函数的对象的地址
//// this指针在非静态成员函数中才有效，因为静态成员函数是与类本身相关联的，而不是与类的实例相关联的
//// 当调用一个非静态成员函数时，编译器会自动将当前对象的地址作为隐式参数传递给函数，即将this指针传递给该函数。
//// this指针指向当前对象的地址，可以用于访问当前对象的成员变量和成员函数

//void CRemoteClientDlg::threadEntryForDownFile(void* arg)
//{
//	CRemoteClientDlg* thiz = (CRemoteClientDlg*)arg; // 这里传入的是 this 指针，才能调用类的下载函数
//	thiz->threadDownFile(); // 调用下载函数
//	_endthread(); // 结束线程
//}

// 真正的远程桌面函数

//void CRemoteClientDlg::threadWatchData()
//{
//	Sleep(50);
//	// 解耦V和M之前，还需要在V这里调用M(CClientSocket)从数据包拿数据到图片
//	//CClientSocket* pClient = NULL;
//	//do
//	//{
//	//	pClient = CClientSocket::getInstance();
//	//} while (pClient == NULL); // 用do while 循环来确保拿到 CClientSocket 类对象 pClient
//	// 解耦之后，利用C层(CClientController)实现与M层的通信
//	CClientController* pCtrl = CClientController::getInstance();
//	while (!m_isClosed) // 当前窗口还存在时
//	{
//		if (m_isFull == false) // 更新服务器发来的截屏数据到缓存 若缓存没被读取数据(即定时器没触发将缓存显示到远程dlg上)，则丢帧
//		{
//			// int ret = SendMessage(WM_SEND_PACKET, 6 << 1 | 0); // 解耦之前：通过消息响应函数来发送远程桌面命令
//			int ret = pCtrl->SendCommandPacket(6, false); // 命令为6，其余使用默认参数
//			if (ret == 6)
//			{
//				if (pCtrl->GetImage(m_image) == 0) // 通过控制层成功读取到图像
//				{
//					m_isFull = true; // 表示缓存中已有数据
//				}
//				else
//				{
//					TRACE("获取图片失败！\r\n");
//				}
//			}
//			else Sleep(1);
//		}
//		else Sleep(1);	
//	}
//}
//// 开启线程远程桌面
//void CRemoteClientDlg::threadEntryForWatch(void* arg)
//{
//	CRemoteClientDlg* thiz = (CRemoteClientDlg*)arg; // 这里传入的是 this 指针，才能调用类的下载函数
//	thiz->threadWatchData(); // 调用远程桌面函数
//	_endthread(); // 结束线程
//}

// 文件下载功能的实现
void CRemoteClientDlg::OnDownloadFile()
{
	int nListSelected = m_List.GetSelectionMark(); // 获取文件列表中选中的项的索引
	CString strFile = m_List.GetItemText(nListSelected, 0); // 获取文件列表中选中的文件名
	// 01 获取待下载文件路径
	HTREEITEM hSelected = m_Tree.GetSelectedItem(); // 获取文件夹树中选中的项，即当前选中文件的上级目录
	strFile = GetPath(hSelected) + strFile; // 调用在本.cpp文件中实现的获取树状控件中指定节点（hTree）的路径函数来获取父目录的路径，再与文件名拼接即可得到当前选中文件的完整路径
	TRACE("%s\r\n", LPCSTR(strFile));
	int ret = CClientController::getInstance()->DownFile(strFile); // 通过控制层来下载文件 解耦前是在V层(CRemoteClientDlg类)里实现的
	if (ret != 0) // 下载出现错误
	{
		MessageBox(_T("下载失败！"));
		TRACE("下载失败 ret =  %d\r\n", ret);
	}
}

// 文件删除功能的实现
void CRemoteClientDlg::OnDeleteFile() // 删除文件操作，右键菜单，删除事件按钮，获得选择文件的信息，发送数据包命令为9，进行删除，删除后调用刷新文件函数进行刷新
{
	HTREEITEM	 hSelected = m_Tree.GetSelectedItem();			// 拿到文件夹树坐标
	CString		 strPath   = GetPath(hSelected);				// 拿到文件夹坐标的路径
	int			 nSelected = m_List.GetSelectionMark();			// 拿到文件名坐标
	CString		 strFile   = m_List.GetItemText(nSelected, 0);	// 得到文件名
				 strFile   = strPath + strFile;					// 组合得到完整的文件路径

	// 解耦MV前，这里还要调用V层(CRemoteClientDlg类)中定义的发送命令函数来与M层(CClientSocket类)通信
	// int ret = SendCommandPacket(9, true, (BYTE*)(LPCSTR)strFile, strFile.GetLength()); // 发送删除文件命令
	// 解耦MV后，V层通过C层(CClientController类)来与M层通信
	int ret = CClientController::getInstance()->SendCommandPack(GetSafeHwnd(), 9, true, (BYTE*)(LPCSTR)strFile, strFile.GetLength());

	if (ret < 0)
	{
		AfxMessageBox("删除文件命令执行失败！");
		return;
	}

	// 刷新文件列表
	LoadFileCurrent();
}

// 文件打开功能的实现
void CRemoteClientDlg::OnRunFile() // 运行文件操作。鼠标选中文件右键菜单，运行文件。发送命令3，以及指定运行文件的路径到服务器
{
	HTREEITEM	 hSelected = m_Tree.GetSelectedItem();			// 拿到文件夹树坐标
	CString		 strPath   = GetPath(hSelected);				// 拿到文件夹坐标的路径
	int			 nSelected = m_List.GetSelectionMark();			// 拿到文件名坐标
	CString		 strFile   = m_List.GetItemText(nSelected, 0);	// 得到文件名
				 strFile   = strPath + strFile;					// 组合得到完整的文件路径

	// 解耦MV前，这里还要调用V层(CRemoteClientDlg类)中定义的发送命令函数来与M层(CClientSocket类)通信
	// int ret = SendCommandPacket(3, true, (BYTE*)(LPCSTR)strFile, strFile.GetLength()); // 发送打开文件命令
	// 解耦MV后，V层通过C层(CClientController类)来与M层通信
	int ret = CClientController::getInstance()->SendCommandPack(GetSafeHwnd(), 3, true, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
	if (ret < 0)
	{
		AfxMessageBox("打开文件命令执行失败！");
		return;
	}
}

// ④实现消息响应函数 WM_SEND_PACKET() 解耦后不再需要

//LRESULT CRemoteClientDlg::OnSendPacket(WPARAM wParam, LPARAM lParam) // wParam 和 lParam 分别表示消息的参数
//{
//	int ret = 0;
//	int cmd = wParam >> 1;
//	switch (cmd){
//	case 4: // 下载文件
//		{
//			CString strFile = (LPCSTR)lParam;
//			
//			// 解耦MV前，这里还要调用V层(CRemoteClientDlg类)中定义的发送命令函数来与M层(CClientSocket类)通信
//			// ret = SendCommandPacket(cmd, wParam & 1, (BYTE*)(LPCSTR)strFile, strFile.GetLength()); // para2=false，因为下载文件时需要不断接收，不能自动关闭通信
//			// 解耦MV后，V层通过C层(CClientController类)来与M层通信
//			ret = CClientController::getInstance()->SendCommandPacket(cmd, wParam & 1, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
//
//		}
//		break;
//	case 5: // 鼠标操作
//		{
//			// 解耦MV前，这里还要调用V层(CRemoteClientDlg类)中定义的发送命令函数来与M层(CClientSocket类)通信
//			// ret = SendCommandPacket(cmd, wParam & 1, (BYTE*)lParam, sizeof(MOUSEEV));
//			// 解耦MV后，V层通过C层(CClientController类)来与M层通信
//			ret = CClientController::getInstance()->SendCommandPacket(cmd, wParam & 1, (BYTE*)lParam, sizeof(MOUSEEV));
//		}
//		break;
//	case 6: // 远程桌面
//		{
//			// 解耦MV前，这里还要调用V层(CRemoteClientDlg类)中定义的发送命令函数来与M层(CClientSocket类)通信
//			// ret = SendCommandPacket(cmd, wParam & 1, NULL, 0);
//			// 解耦MV后，V层通过C层(CClientController类)来与M层通信
//			ret = CClientController::getInstance()->SendCommandPacket(cmd, wParam & 1, NULL, 0);
//		}
//		break;
//	case 7: // 锁机
//	{
//		// 解耦MV前，这里还要调用V层(CRemoteClientDlg类)中定义的发送命令函数来与M层(CClientSocket类)通信
//		// ret = SendCommandPacket(cmd, wParam & 1, NULL, 0);
//		// 解耦MV后，V层通过C层(CClientController类)来与M层通信
//		ret = CClientController::getInstance()->SendCommandPacket(cmd, wParam & 1, NULL, 0);
//	}
//	break;
//	case 8: // 解锁
//	{
//		// 解耦MV前，这里还要调用V层(CRemoteClientDlg类)中定义的发送命令函数来与M层(CClientSocket类)通信
//		// ret = SendCommandPacket(cmd, wParam & 1, NULL, 0);
//		// 解耦MV后，V层通过C层(CClientController类)来与M层通信
//		ret = CClientController::getInstance()->SendCommandPacket(cmd, wParam & 1, NULL, 0);
//	}
//	break;
//	default:
//		ret = -1;
//	}
//	
//	//DWORD threadId = ::GetCurrentThreadId(); // 获取当前线程的线程 ID
//	//DWORD mainThreadId = AfxGetApp()->m_nThreadID; // 获取主线程 ID
//	//TRACE(_T("我当前线程号为：%lu\n"), threadId); // 输出线程 ID 到输出窗口 // 16764
//	//TRACE(_T("主线程号为：%lu\n"), mainThreadId); // 输出主线程 ID 到输出窗口 // 16764
//	return ret;
//}


// 远程监控按钮按下事件对应的事件处理函数，即开始远程桌面
void CRemoteClientDlg::OnBnClickedBtnStartWatch() 
{
	CClientController::getInstance()->StartWatchScreen(); // 通过控制层来执行远程桌面功能
	
	// 解耦后，不需要再在V层实现业务逻辑，转交控制层实现，故下面的代码不再需要
	//m_isClosed = false;
	//CWatchDialog dlg(this); // 远程桌面dlg parent设置为this
	//HANDLE hThread = (HANDLE)_beginthread(CRemoteClientDlg::threadEntryForWatch, 0, this);
	//dlg.DoModal(); // 设置为模态，直到用户完成对话框的操作，
	//m_isClosed = true;
	//WaitForSingleObject(hThread, 500);
}


void CRemoteClientDlg::OnTimer(UINT_PTR nIDEvent) // 添加一个定时器用以控制远程
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnTimer(nIDEvent);
}


// 如果界面的远程服务器 IP 地址发生了改变
void CRemoteClientDlg::OnIpnFieldchangedIpaddressServ(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMIPADDRESS pIPAddr = reinterpret_cast<LPNMIPADDRESS>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;

	// 在对话框控件的值更改后，调用UpdateData(TRUE)，将控件的值更新到相应的变量
	// UpdateData(FALSE) 则将变量的值更新到相应的控件
	UpdateData(); // 用于将远程服务器地址和端口控件的数据与变量进行同步
	CClientController* pController = CClientController::getInstance();
	pController->UpdataAddress(m_server_address, atoi((LPCTSTR)m_nPort)); // 同步后的变量用来更新地址，还是MVC模式，通过控制层来调用
}

// 如果界面的远程服务器 端口 发生了改变
void CRemoteClientDlg::OnEnChangeEditPort()
{
	UpdateData(); // 用于将远程服务器地址和端口控件的数据与变量进行同步
	CClientController* pController = CClientController::getInstance();
	pController->UpdataAddress(m_server_address, atoi((LPCTSTR)m_nPort)); // 同步后的变量用来更新地址，还是MVC模式，通过控制层来调用

}
