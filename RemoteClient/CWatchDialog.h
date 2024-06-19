#pragma once
#ifndef WM_SEND_PACK_ACK
#define WM_SEND_PACK_ACK (WM_USER + 2) // 发送应答包的消息
#endif

// CWatchDialog 对话框

class CWatchDialog : public CDialog
{
	DECLARE_DYNAMIC(CWatchDialog)

public:
	CWatchDialog(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CWatchDialog();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_WATCH };
#endif

public:
	int m_nObjWidth;  // 远程服务器桌面的宽，即监视时远程服务器发过来图片的宽
	int m_nObjHeight; // 远程服务器桌面的高
	CImage m_image; // 存放远程桌面 用于监控远程桌面功能的缓存
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	DECLARE_MESSAGE_MAP()

	bool   m_isFull;   // 用于标记本地存放远程桌面的缓存是否有数据  true-有 false-无


public:
	// // 将客户端本地远控界面的鼠标坐标转换为远程服务器的桌面坐标 para1:参数使用引用，方便传参，避免不必要的 point 拷贝   para2:传入的point是否为屏幕(全局)坐标，默认false
	CPoint	UserPoint2RemoteScreenPoint(CPoint& point, bool isScreen = false); 

	virtual BOOL OnInitDialog();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	CStatic m_picture;
	afx_msg LRESULT OnSendPackAck(WPARAM wParam, LPARAM lParam); // 自定义 WM_SEND_PACK_ACK 消息响应函数 返回值类型为LRESULT
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnStnClickedWatch();
	virtual void OnOK();
	afx_msg void OnBnClickedBtnLock();
	afx_msg void OnBnClickedBtnUnlock();

	bool isFull() const {  // 此成员函数用于远程桌面dlg访问 CRem私有成员变量  const 关键字表示isFull()函数不会修改任何成员变量
		return m_isFull;
	}
	void SetImageStatus(bool isFull = false) { // 用于远程dlg显示完远程桌面后调用，默认参数设置为false 表示远程桌面缓存已无数据
		m_isFull = isFull;
	}
	CImage& GetImage() {
		return m_image;
	}
};
