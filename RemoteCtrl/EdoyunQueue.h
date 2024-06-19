#pragma once
#include "pch.h"
#include <atomic>
#include <list>
#include "EdoyunThread.h"

template<class T> // ��ģ�棬����֧�ָ�����������
class CEdoyunQueue 
{ // �̰߳�ȫ�Ķ��У�����IOCPʵ�֣�
public:
	
	typedef struct IocpParam
	{// ʵ��IOCP��һЩ������Ҫ�õ��Ĳ����ṹ��
		size_t nOperator; // ����
		T Data; // ����
		HANDLE hEvent; // pop������Ҫ�õ����¼����
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
	}PPARAM; // Post Parameter ����Ͷ����Ϣ�Ľṹ��
	enum {
		EQNone, // û��
		EQPush, // д
		EQPop, // ��
		EQSize, // ���С
		EQClear // ���
	};
	
	
public:
	CEdoyunQueue() {
		m_lock = false;
		m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1); // ����һ����ɶ˿ڶ���
		m_hThread = INVALID_HANDLE_VALUE;
		if (m_hCompletionPort != NULL)
		{
			m_hThread = (HANDLE)_beginthread(&CEdoyunQueue<T>::threadEntry, 0, this); // ���������̣߳�worker)		
		}
	}
	virtual ~CEdoyunQueue() {
		if (m_lock) return; // ���������� ֱ�ӷ��� ����������
		m_lock = true;
		PostQueuedCompletionStatus(m_hCompletionPort, 0, NULL, NULL); // ���Զ�������״̬��Ϣ��Completion Status����ӵ�ָ������ɶ˿ڵ���Ϣ�����У�ĳ���첽�����Ѿ���ɣ��Ӷ�������Ӧ�Ĵ����߼� ���﷢�͵Ľ�����Ϣ
		WaitForSingleObject(m_hThread, INFINITE); // ȷ���߳���������
		if (m_hCompletionPort != NULL)
		{
			HANDLE hTemp = m_hCompletionPort; // �����Ա��
			m_hCompletionPort = NULL;
			CloseHandle(hTemp);
		}
	}

	bool PushBack(const T& data) {
		
		IocpParam* pParm = new IocpParam(EQPush, data); // Ͷ�ݵ���Ϣ
		if (m_lock)  // ��������ʱ������д������
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
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL); // ͬ���¼������ڵȴ���������������
		IocpParam Param = IocpParam(EQPop, data, hEvent); // Ͷ�ݵ���Ϣ
		if (m_lock)
		{
			if (hEvent) CloseHandle(hEvent);
			return false; // ��������ʱ������д������
		}
		bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM), (ULONG_PTR)&Param, NULL);
		if (ret == false)
		{
			CloseHandle (hEvent);
			return false;
		}
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0; // �ȴ��������
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
			return -1; // ��������ʱ�������С 
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
		if (m_lock) return false; // ��������ʱ������д������
		IocpParam* pParm = new IocpParam(EQClear, T()); // Ͷ�ݵ���Ϣ
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
		case EQPush: // ��list����׷������
		{
			m_lstData.push_back(pParam->Data);
			delete pParam; // ����pushʱ��3��������new�����ģ��ʽ��պ�Ҫdelete
		}
		break;
		case EQPop: // ��list����ȡ����
		{
			if (m_lstData.size() > 0) // list��Ϊ�գ�������
			{
				pParam->Data = m_lstData.front(); // ȡ����
				m_lstData.pop_front();
			}
			if (pParam->hEvent != NULL)
			{
				SetEvent(pParam->hEvent); // �����¼����������pop����
			}
		}
		break;
		case EQSize:
		{
			pParam->nOperator = m_lstData.size(); // ����С����� nOperator ��
			if (pParam->hEvent != NULL)
			{
				SetEvent(pParam->hEvent); // �����¼����������size����
			}
		}
		break;

		case EQClear:
		{
			m_lstData.clear();
			delete pParam; // �������ʱ��3��������new�����ģ��ʽ��պ�Ҫdelete
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
		while (GetQueuedCompletionStatus(m_hCompletionPort, &dwTransferred, &CompletionKey, &pOverlapped, INFINITE)) // ��������
		{
			if ((dwTransferred == 0) || (CompletionKey == NULL)) // ����ǽ�����Ϣ
			{
				printf("thread is prpare to exit!\r\n");
				break;
			}
			// Ͷ����Ϣ���PostQueuedCompletionStatus(hIOCP, sizeof(IOCP_PARAM), (ULONG_PTR)new IOCP_PARAM(IocpListPush, "hello world"), NULL); //������Ϣ����ɶ˿�
			pParam = (PPARAM*)CompletionKey; // ��������õ�������ĵ�3������
			DealParam(pParam);
		}
		while (GetQueuedCompletionStatus(m_hCompletionPort, &dwTransferred, &CompletionKey, &pOverlapped, 0)) // �����Ա�̣��ڹر���ɶ˿�ǰ����Ƿ���δ���������
		{
			if ((dwTransferred == 0) || (CompletionKey == NULL)) // ����ǽ�����Ϣ
			{
				printf("thread is prpare to exit!\r\n");
				continue;
			}
			pParam = (PPARAM*)CompletionKey; // ��������õ�������ĵ�3������
			DealParam(pParam);
		}
		if (m_hCompletionPort != NULL)
		{
			HANDLE hTemp = m_hCompletionPort;
			m_hCompletionPort = NULL;
			CloseHandle(hTemp); // �ر�IOCP������
		}
	}

protected:
	std::list<T> m_lstData;
	HANDLE m_hCompletionPort; // ��ɶ˿ڵľ��
	HANDLE m_hThread; // �����߳�(worker)���
	std::atomic<bool> m_lock; // ������������
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
		typename CEdoyunQueue<T>::IocpParam* Param = new typename CEdoyunQueue<T>::IocpParam(CEdoyunQueue<T>::EQPop, T()); // Ͷ�ݵ���Ϣ
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
		case CEdoyunQueue<T>::EQPush: // ��list����׷������
		{
			CEdoyunQueue<T>::m_lstData.push_back(pParam->Data);
			delete pParam; // ����pushʱ��3��������new�����ģ��ʽ��պ�Ҫdelete
		}
		break;
		case CEdoyunQueue<T>::EQPop: // ��list����ȡ����
		{
			if (CEdoyunQueue<T>::m_lstData.size() > 0) // list��Ϊ�գ�������
			{
				pParam->Data = CEdoyunQueue<T>::m_lstData.front(); // ȡ����
				if ((m_base->*m_callback)(pParam->Data) == 0) // ���ͳɹ�
					CEdoyunQueue<T>::m_lstData.pop_front();
			}
			delete pParam;
		}
		break;
		case CEdoyunQueue<T>::EQSize:
		{
			pParam->nOperator = CEdoyunQueue<T>::m_lstData.size(); // ����С����� nOperator ��
			if (pParam->hEvent != NULL)
			{
				SetEvent(pParam->hEvent); // �����¼����������size����
			}
		}
		break;

		case CEdoyunQueue<T>::EQClear:
		{
			CEdoyunQueue<T>::m_lstData.clear();
			delete pParam; // �������ʱ��3��������new�����ģ��ʽ��պ�Ҫdelete
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