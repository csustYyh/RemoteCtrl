#pragma once
#include "pch.h"
#include <atomic>
#include <list>
#include "EdoyunThread.h"

template<class T> // 类模版，用以支持各种数据类型
class CEdoyunQueue 
{ // 线程安全的队列（利用IOCP实现）
public:
	
	typedef struct IocpParam
	{// 实现IOCP中一些函数需要用到的参数结构体
		size_t nOperator; // 操作
		T Data; // 数据
		HANDLE hEvent; // pop操作需要用到的事件句柄
		IocpParam(int op, const T& data, HANDLE hEve = NULL)
		{
			nOperator = op;
			Data = data;
			hEvent = hEve;
		}
		IocpParam()
		{
			nOperator = EQNone;
		}
	}PPARAM; // Post Parameter 用于投递信息的结构体
	enum {
		EQNone, // 没有
		EQPush, // 写
		EQPop, // 读
		EQSize, // 求大小
		EQClear // 清空
	};
	
	
public:
	CEdoyunQueue() {
		m_lock = false;
		m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1); // 创建一个完成端口对象
		m_hThread = INVALID_HANDLE_VALUE;
		if (m_hCompletionPort != NULL)
		{
			m_hThread = (HANDLE)_beginthread(&CEdoyunQueue<T>::threadEntry, 0, this); // 开启工作线程（worker)		
		}
	}
	virtual ~CEdoyunQueue() {
		if (m_lock) return; // 若正在析构 直接返回 避免多次析构
		m_lock = true;
		PostQueuedCompletionStatus(m_hCompletionPort, 0, NULL, NULL); // 将自定义的完成状态信息（Completion Status）添加到指定的完成端口的消息队列中，某个异步操作已经完成，从而触发相应的处理逻辑 这里发送的结束消息
		WaitForSingleObject(m_hThread, INFINITE); // 确保线程正常结束
		if (m_hCompletionPort != NULL)
		{
			HANDLE hTemp = m_hCompletionPort; // 防御性编程
			m_hCompletionPort = NULL;
			CloseHandle(hTemp);
		}
	}

	bool PushBack(const T& data) {
		
		IocpParam* pParm = new IocpParam(EQPush, data); // 投递的信息
		if (m_lock)  // 队列析构时不可再写入数据
		{
			delete pParm;
			return false;
		}
		bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM), (ULONG_PTR)pParm, NULL);
		if (ret == false) delete pParm;
		return ret; 
	}
	virtual bool PopFront(T& data)
	{
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL); // 同步事件，用于等待读数据请求的完成
		IocpParam Param = IocpParam(EQPop, data, hEvent); // 投递的信息
		if (m_lock)
		{
			if (hEvent) CloseHandle(hEvent);
			return false; // 队列析构时不可再写入数据
		}
		bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM), (ULONG_PTR)&Param, NULL);
		if (ret == false)
		{
			CloseHandle (hEvent);
			return false;
		}
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0; // 等待请求完成
		if (ret)
		{
			data = Param.Data;
		}
		return ret;
	}
	size_t Size() {
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL); 
		IocpParam Param = IocpParam(EQSize, T(), hEvent);
		if (m_lock)
		{
			if (hEvent) CloseHandle(hEvent);
			return -1; // 队列析构时不可求大小 
		}
		bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM), (ULONG_PTR)&Param, NULL);
		if (ret == false)
		{
			CloseHandle(hEvent);
			return -1;
		}
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
		if (ret)
		{
			return Param.nOperator;
		}
		return -1;
	}
	bool Clear() {
		if (m_lock) return false; // 队列析构时不可再写入数据
		IocpParam* pParm = new IocpParam(EQClear, T()); // 投递的信息
		bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM), (ULONG_PTR)pParm, NULL);
		if (ret == false) delete pParm;
		return ret;

	}
protected:
	static void threadEntry(void* arg) {
		CEdoyunQueue<T>* thiz = (CEdoyunQueue<T>*)arg;
		thiz->threadMain();
		_endthread();
	}
	virtual void DealParam(PPARAM* pParam)
	{
		switch (pParam->nOperator)
		{
		case EQPush: // 往list里面追加数据
		{
			m_lstData.push_back(pParam->Data);
			delete pParam; // 由于push时第3个参数是new出来的，故接收后要delete
		}
		break;
		case EQPop: // 从list里面取数据
		{
			if (m_lstData.size() > 0) // list不为空，有数据
			{
				pParam->Data = m_lstData.front(); // 取数据
				m_lstData.pop_front();
			}
			if (pParam->hEvent != NULL)
			{
				SetEvent(pParam->hEvent); // 设置事件表明已完成pop操作
			}
		}
		break;
		case EQSize:
		{
			pParam->nOperator = m_lstData.size(); // 将大小存放在 nOperator 中
			if (pParam->hEvent != NULL)
			{
				SetEvent(pParam->hEvent); // 设置事件表明已完成size操作
			}
		}
		break;

		case EQClear:
		{
			m_lstData.clear();
			delete pParam; // 由于清空时第3个参数是new出来的，故接收后要delete
		}
		break;

		default:
			OutputDebugStringA("unknown operator!\r\n");
			break;
		}
	}
	virtual void threadMain() {
		PPARAM* pParam = NULL;
		DWORD dwTransferred = 0;
		ULONG_PTR CompletionKey = 0;
		OVERLAPPED* pOverlapped = NULL;
		while (GetQueuedCompletionStatus(m_hCompletionPort, &dwTransferred, &CompletionKey, &pOverlapped, INFINITE)) // 阻塞函数
		{
			if ((dwTransferred == 0) || (CompletionKey == NULL)) // 如果是结束消息
			{
				printf("thread is prpare to exit!\r\n");
				break;
			}
			// 投递消息那里：PostQueuedCompletionStatus(hIOCP, sizeof(IOCP_PARAM), (ULONG_PTR)new IOCP_PARAM(IocpListPush, "hello world"), NULL); //发送消息到完成端口
			pParam = (PPARAM*)CompletionKey; // 这里就是拿到了上面的第3个参数
			DealParam(pParam);
		}
		while (GetQueuedCompletionStatus(m_hCompletionPort, &dwTransferred, &CompletionKey, &pOverlapped, 0)) // 防御性编程，在关闭完成端口前检查是否还有未处理的数据
		{
			if ((dwTransferred == 0) || (CompletionKey == NULL)) // 如果是结束消息
			{
				printf("thread is prpare to exit!\r\n");
				continue;
			}
			pParam = (PPARAM*)CompletionKey; // 这里就是拿到了上面的第3个参数
			DealParam(pParam);
		}
		if (m_hCompletionPort != NULL)
		{
			HANDLE hTemp = m_hCompletionPort;
			m_hCompletionPort = NULL;
			CloseHandle(hTemp); // 关闭IOCP对象句柄
		}
	}

protected:
	std::list<T> m_lstData;
	HANDLE m_hCompletionPort; // 完成端口的句柄
	HANDLE m_hThread; // 工作线程(worker)句柄
	std::atomic<bool> m_lock; // 队列正在析构
};

template<class T>
class EdoyunSendQueue :public CEdoyunQueue<T>, public ThreadFuncBase
{
public:
	typedef int (ThreadFuncBase::* EDYCALLBACK)(T& data);
	EdoyunSendQueue(ThreadFuncBase* obj, EDYCALLBACK callback)
		:CEdoyunQueue<T>(), m_base(obj), m_callback(callback) 
	{
		m_thread.Start();
		m_thread.UpdateWorker(::ThreadWorker(this, (FUNCTYPE)&EdoyunSendQueue<T>::threadTick));
	}
	virtual ~EdoyunSendQueue(){
		m_base = NULL;
		m_callback = NULL;
		m_thread.Stop();
	}

protected:
	virtual bool PopFront(T& data) { return false; }
	bool PopFront(){
		typename CEdoyunQueue<T>::IocpParam* Param = new typename CEdoyunQueue<T>::IocpParam(CEdoyunQueue<T>::EQPop, T()); // 投递的信息
		if (CEdoyunQueue<T>::m_lock)
		{
			delete Param;
			return false;
		}
		bool ret = PostQueuedCompletionStatus(CEdoyunQueue<T>::m_hCompletionPort, sizeof(*Param), (ULONG_PTR)&Param, NULL);
		if (ret == false)
		{
			delete Param;

			return false;
		}
		return ret;
	}
	int threadTick()
	{
		if (WaitForSingleObject(CEdoyunQueue<T>::m_hThread, 0) != WAIT_TIMEOUT) return 0;
		if (CEdoyunQueue<T>::m_lstData.size() > 0)
		{
			PopFront();
		}
		//Sleep(1);
		return 0;
	}
	virtual void DealParam(typename CEdoyunQueue<T>::PPARAM* pParam)
	{
		switch (pParam->nOperator)
		{
		case CEdoyunQueue<T>::EQPush: // 往list里面追加数据
		{
			CEdoyunQueue<T>::m_lstData.push_back(pParam->Data);
			delete pParam; // 由于push时第3个参数是new出来的，故接收后要delete
		}
		break;
		case CEdoyunQueue<T>::EQPop: // 从list里面取数据
		{
			if (CEdoyunQueue<T>::m_lstData.size() > 0) // list不为空，有数据
			{
				pParam->Data = CEdoyunQueue<T>::m_lstData.front(); // 取数据
				if ((m_base->*m_callback)(pParam->Data) == 0) // 发送成功
					CEdoyunQueue<T>::m_lstData.pop_front();
			}
			delete pParam;
		}
		break;
		case CEdoyunQueue<T>::EQSize:
		{
			pParam->nOperator = CEdoyunQueue<T>::m_lstData.size(); // 将大小存放在 nOperator 中
			if (pParam->hEvent != NULL)
			{
				SetEvent(pParam->hEvent); // 设置事件表明已完成size操作
			}
		}
		break;

		case CEdoyunQueue<T>::EQClear:
		{
			CEdoyunQueue<T>::m_lstData.clear();
			delete pParam; // 由于清空时第3个参数是new出来的，故接收后要delete
		}
		break;

		default:
			OutputDebugStringA("unknown operator!\r\n");
			break;
		}
	}

private:
	ThreadFuncBase* m_base;
	EDYCALLBACK m_callback;
	CEdoyunThread m_thread;
};

typedef EdoyunSendQueue<std::vector<char>>::EDYCALLBACK SENDCALLBACK;