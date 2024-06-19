#pragma once
#include "pch.h"
#include <atomic>
#include <vector>
#include <mutex>
#include <Windows.h>

class ThreadFuncBase{}; // �û�ͨ���̳����������д�Զ��Ĳ�ͬ���̺߳���
typedef int (ThreadFuncBase::* FUNCTYPE)(); // ThreadFuncBase��ĳ�Ա����ָ�룺 int (ThreadFuncBase::*)() ����int �޲���

class ThreadWorker { // �����߳��࣬��ҪĿ������������һ���߳���ִ��ָ���ĳ�Ա����(�û���д��)
public:
	ThreadWorker() :thiz(NULL), func(NULL) {} // Ĭ�Ϲ��캯��
	ThreadWorker(void* obj, FUNCTYPE f) :thiz((ThreadFuncBase*)obj), func(f){} // ���ι��캯��
	ThreadWorker(const ThreadWorker& worker) // ���ƹ��캯��
	{
		thiz = worker.thiz;
		func = worker.func;
	}
	ThreadWorker& operator=(const ThreadWorker& worker) // ���ظ�ֵ�����
	{
		if (this != &worker)
		{
			thiz = worker.thiz;
			func = worker.func;
		}
		return *this;
	}

	int operator()() // ���غ������������ 
	{
		if (IsValid())
		{
			return (thiz->*func)(); // ���ó�Ա����ָ�� func ��ָ��ĺ�������������
		}
		return -1;
	}
	bool IsValid() const
	{
		return (thiz != NULL) && (func != NULL);
	}

private:
	ThreadFuncBase* thiz; // ThreadFuncBase��Ķ���ָ��
	FUNCTYPE func; // ThreadFuncBase��ĳ�Ա����ָ��  int (ThreadFuncBase::*)() ---> ����ֵΪ int���޲εģ�ָ�� ThreadFuncBase ���Ա�����ĺ���ָ��
};

class CEdoyunThread // �߳��� ����ִ�� ThreadWorker ����ָ���ĳ�Ա����
	{
	public:
	CEdoyunThread() {
		m_hThread = NULL;
		m_bStatus = false;
	}
	~CEdoyunThread() {
		Stop();
	}
	bool Start() 
	{ // �����߳� ture ��ʾ�����ɹ�
		m_hThread = (HANDLE)_beginthread(&CEdoyunThread::ThreadEntry, 0, this);
		m_bStatus = IsValid();
		return m_bStatus;
	}
	bool IsValid()
	{ // �߳��Ƿ���Ч true��ʾ��Ч��false��ʾ�߳��쳣����ֹ
		if (m_hThread == NULL || (m_hThread == INVALID_HANDLE_VALUE)) return false; // ��Ч���߳̾��
		return WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT;
	}
	bool Stop()
	{
		if (m_bStatus == false) return true; // �Ѿ���ֹ��ֱ�ӷ���
		m_bStatus = false;
		DWORD ret = WaitForSingleObject(m_hThread, 1000);
		if (ret == WAIT_TIMEOUT)
		{
			TerminateThread(m_hThread, -1);
		}
		UpdateWorker();
		return ret == WAIT_TIMEOUT;
	}
	void UpdateWorker(const ::ThreadWorker& worker = ::ThreadWorker()) // �����߳�ִ�е�����
	{	
		if (m_worker.load() != NULL && (m_worker.load() != &worker))
		{
			::ThreadWorker* pWorker = m_worker.load();
			m_worker.store(NULL);
			delete pWorker;
		}
		if (m_worker.load() == &worker) return;
		if (!worker.IsValid())
		{
			m_worker.store(NULL);
			return;
		}
		::ThreadWorker* pWorker = new ::ThreadWorker(worker);
		m_worker.store(pWorker);
	}
	bool IsIdle() // �ж��߳��Ƿ���� true��ʾ���� false��ʾ���߳��Ѿ������˹���
	{
		if (m_worker.load() == NULL) return true;
		return !m_worker.load()->IsValid();
	}
private:
	void ThreadWorker()
	{ // �̹߳�������
		while (m_bStatus) // �߳���Чʱ����ִ�й���
		{
			if (m_worker.load() == NULL) // ��鹤���̶߳����Ƿ�Ϊ��
			{
				Sleep(1);
				continue;
			}
			::ThreadWorker worker = *m_worker.load(); // ���ڻ�ȡԭ�ӱ���(m_worker) ��ֵ ����һ�������߳���(::ThreadWorker)����
			if (worker.IsValid()) 
			{
				if (WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT) // ����߳��������У�WaitForSingleObject ���������� WAIT_TIMEOUT����Ϊ��ʱʱ���� 0����ʾ���ȴ�
				{
					// 1. �߳�����Ҫִ�еĺ����Ѿ���ȫ�������̣߳����û���ThreadFuncBase�̳л���ȥʵ���Զ���ĺ���(accept send recv...)���� 
					int ret = worker(); // ��Ϊ�����˺��������������ʹ�����ʵ�������������һ�������� ����worker()��ʾ���ù����̺߳���
					if (ret != 0)
					{
						CString str;
						str.Format(_T("thread found warning code %d\r\n"), ret);
						OutputDebugString(str);
					}
					if (ret < 0)
					{ // ��Դ�ͷ�
						::ThreadWorker* pWorker = m_worker.load();
						m_worker.store(NULL); // ��������ԭ�ӱ���(m_worker) ��ֵ
						delete pWorker;
					}
				}	
			}
			else
			{
				Sleep(1);
			}
		}
	}
	static void ThreadEntry(void* arg)
	{ // �߳���ں��� �ô�:1.����ȷ���̹߳�������ִ��������ȷ����Դ�ͷţ���Ϊ����ڹ������������_endthread()�����кܶ���Դ�������ͷ� 2.���ʼ� 
		CEdoyunThread* thiz = (CEdoyunThread*)arg;
		if (thiz)
		{
			thiz->ThreadWorker();  // �̹߳�������   
		}
		_endthread();
	}
private:
	HANDLE m_hThread; // �߳̾��
	bool m_bStatus; // false��ʾ�߳̽�Ҫ�رգ�true��ʾ�߳����ڽ���
	// std::atomic �� C++11 �ṩ��ԭ�Ӳ������ͣ�����ȷ�����̻߳����µĲ�����ԭ����
	std::atomic<::ThreadWorker*> m_worker; // ԭ��ָ�룬�洢��һ�������߳���(::ThreadWorker) ��ָ��(�̰߳�ȫ��)
};

class CEdoyunThreadPool { // �̳߳���
public:
	CEdoyunThreadPool(size_t size)
	{
		m_threads.resize(size);
		for (size_t i = 0; i < size; i++)
			m_threads[i] = new CEdoyunThread();
	}
	CEdoyunThreadPool(){}
	~CEdoyunThreadPool() {
		Stop();
		for (size_t i = 0; i < m_threads.size(); i++)
		{
			delete m_threads[i];
			m_threads[i] = NULL;
		}
		m_threads.clear();
	}
	bool Invoke() // �����̳߳�
	{
		bool ret = true;
		for (size_t i = 0; i < m_threads.size(); i++)
		{
			if (m_threads[i]->Start() == false)
			{
				ret = false;
				break;
			}
		}
		if (ret == false)
		{
			for (size_t i = 0; i < m_threads.size(); i++)
			{
				m_threads[i]->Stop();
			}
		}
		return ret;
	}
	void Stop() // �����̳߳� 
	{ 
		for (size_t i = 0; i < m_threads.size(); i++)
		{
			m_threads[i]->Stop();
		}
	}
	int DispatchWorker(const ThreadWorker& worker) // �����߳�ִ������
	{
		int index = -1;
		m_lock.lock(); // ��������������ֹ���̷߳���ʱ������ͻ
		for (size_t i = 0; i < m_threads.size(); i++) // �����̳߳��е��߳�
		{  // �Ż�˼·��1.�������̵߳ı��ȫ���ŵ�һ���б��У�����ĳ���߳������Ŵ��б��Ƴ� 2.ͨ���ж��б��size�Ƿ�>0��֪����û�п����߳� 3.size>0����б���ȡ���̱߳�ŷ�������
			if (m_threads[i]->IsIdle()) // ���߳̿���
			{
				m_threads[i]->UpdateWorker(worker); // ��worker�������������߳�
				index = i;
				break;
			}
		}
		m_lock.unlock();
		return index; // ���� -1 ˵��û�п����̣߳��������ʧ�ܣ� ����ɹ��򷵻ض�Ӧ�߳����̳߳��е����
	}
	bool CheckThreadValid(size_t index) // �����Ϊindex�߳���Ч��
	{
		if (index < m_threads.size())
		{
			return m_threads[index]->IsValid();
		}
		return false;
	}
private:
	std::vector<CEdoyunThread*> m_threads;
	std::mutex m_lock; 
	
};


