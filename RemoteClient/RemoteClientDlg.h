
// RemoteClientDlg.h: 头文件
//

#pragma once
#include "ClientSocket.h"
#include "StatusDlg.h"
#include "CWatchDialog.h"
#ifndef WM_SEND_PACK_ACK
#define WM_SEND_PACK_ACK (WM_USER + 2) // 发送应答包的消息
#endif
// 下面自定义消息是因为未使用MVC设计之前，窗口和窗口耦合，子窗口需要用到主窗口的IP和端口控件信息
// 解耦后窗口与窗口之间不再耦合，都通过C层来交互,故不再需要自定义消息

// 自定义消息的流程：
// 1.自定义消息 ID .h
// 2.定义自定义消息响应函数 .h
// 3.在消息映射表中注册消息 .cpp
// 4.实现消息响应函数 .cpp
// #define WM_SEND_PACKET (WM_USER + 1) // 自定义的发送数据包的消息类型 消息ID 


// CRemoteClientDlg 对话框
class CRemoteClientDlg : public CDialogEx
{
// 构造
public:
	CRemoteClientDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_REMOTECLIENT_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

public:
private:
	bool   m_isClosed; // 用于标记远程监控窗口是否已关闭，避免点击监控按钮->关闭远程窗口->再次点击监控按钮时因为重复开启监控线程，而两个线程又访问同一个m_image而报错
private:
	void InitUIData(); // 初始化对话框
	void DealCommand(WORD nCmd, const std::string& strData, LPARAM lParam); // 处理通信线程发来的应答包 
	void Str2Tree(const std::string& drivers, CTreeCtrl& tree); // 获取驱动磁盘信息的实现
	void UpdateFileInfo(const FILEINFO& finfo, HTREEITEM hParent); // 获取文件信息的实现
	void UpdateDownloadFile(const std::string& strData, FILE* pFile); // 下载文件的实现	

	// 解耦后，不需要在V层做下载文件功能的具体实现，转交给C层做,故下面四个函数不再需要
	// static void threadEntryForDownFile(void* arg); // 下载文件时的线程处理函数 静态函数不能使用this指针 故不能直接访问成员变量/函数
	// void threadDownFile(); // 下载文件 成员函数可以使用this指针，即可以访问类内成员变量/函数 
	// static void threadEntryForWatch(void* arg); // 远程桌面时的线程处理函数
	// void threadWatchData(); // 远程桌面

	void LoadFileCurrent(); // 用于刷新文件列表
	
	CString GetPath(HTREEITEM hTree);
	void DeleteTreeChildrenItem(HTREEITEM hTree);

	// para1:命令 para2:是否自动关闭 para3:操作文件的路径，命令如果是文件操作则为文件路径，其他操作不填 para4：文件路径长度
	// 正常返回命令，错误返回-1
	// 命令1：查看磁盘分区 命令2：查看指定目录下的文件 命令3：打开文件 命令4：下载文件
	// 解耦后V层不再需要直接与M层通信，故下面函数也就不再需要
	//int SendCommandPacket(int nCmd, bool bAutoClose = true, BYTE* pData = NULL, size_t nLength = 0); // 客户端(控制端)发包函数
	// 实现
protected:
	HICON m_hIcon;
	CStatusDlg m_dlgStatus;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	void LoadFileInfo();
	afx_msg LRESULT OnSendPackAck(WPARAM wParam, LPARAM lParam); // 自定义 WM_SEND_PACK_ACK 消息响应函数 返回值类型为LRESULT
	afx_msg void OnBnClickedBynTest();
	DWORD m_server_address;
	CString m_nPort;
	afx_msg void OnBnClickedBtnFileinfo();
	CTreeCtrl m_Tree;
	afx_msg void OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	// 显示文件
	CListCtrl m_List;
	afx_msg void OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDownloadFile();
	afx_msg void OnDeleteFile();
	afx_msg void OnRunFile();
	// afx_msg LRESULT OnSendPacket(WPARAM wParam, LPARAM lParam); // ②消息响应函数 用于处理 WM_SEND_PACKET  LRESULT 是消息处理函数的返回值类型，表示消息处理的结果。WPARAM 和 LPARAM 是消息参数，用于传递消息的附加信息 如果需要更多的信息，通常可以通过这两个参数中的值来进一步获取或解析
	afx_msg void OnBnClickedBtnStartWatch();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnIpnFieldchangedIpaddressServ(NMHDR* pNMHDR, LRESULT* pResult); // 主窗口IP地址改变
	afx_msg void OnEnChangeEditPort();
};
