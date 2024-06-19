#pragma once
#include "pch.h"
#include <atomic>
#include <vector>
#include <mutex>
#include <Windows.h>

class ThreadFuncBase{}; // 用户通过继承这个类来重写自定的不同的线程函数
typedef int (ThreadFuncBase::* FUNCTYPE)(); // ThreadFuncBase类的成员函数指针： int (ThreadFuncBase::*)() 返回int 无参数

class ThreadWorker { // 工作线程类，主要目的是允许在另一个线程中执行指定的成员函数(用户重写的)
public:
	ThreadWorker() :thiz(NULL), func(NULL) {} // 默认构造函数
	ThreadWorker(void* obj, FUNCTYPE f) :thiz((ThreadFuncBase*)obj), func(f){} // 带参构造函数
	ThreadWorker(const ThreadWorker& worker) // 复制构造函数
	{
		thiz = worker.thiz;
		func = worker.func;
	}
	ThreadWorker& operator=(const ThreadWorker& worker) // 重载赋值运算符
	{
		if (this != &worker)
		{
			thiz = worker.thiz;
			func = worker.func;
		}
		return *this;
	}

	int operator()() // 重载函数调用运算符 
	{
		if (IsValid())
		{
			return (thiz->*func)(); // 调用成员函数指针 func 所指向的函数并返回其结果
		}
		return -1;
	}
	bool IsValid() const
	{
		return (thiz != NULL) && (func != NULL);
	}

private:
	ThreadFuncBase* thiz; // ThreadFuncBase类的对象指针
	FUNCTYPE func; // ThreadFuncBase类的成员函数指针  int (ThreadFuncBase::*)() ---> 返回值为 int，无参的，指向 ThreadFuncBase 类成员函数的函数指针
};

class CEdoyunThread // 线程类 可以执行 ThreadWorker 类中指定的成员函数
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
	{ // 启动线程 ture 表示启动成功
		m_hThread = (HANDLE)_beginthread(&CEdoyunThread::ThreadEntry, 0, this);
		m_bStatus = IsValid();
		return m_bStatus;
	}
	bool IsValid()
	{ // 线程是否有效 true表示有效，false表示线程异常或中止
		if (m_hThread == NULL || (m_hThread == INVALID_HANDLE_VALUE)) return false; // 无效的线程句柄
		return WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT;
	}
	bool Stop()
	{
		if (m_bStatus == false) return true; // 已经终止，直接返回
		m_bStatus = false;
		DWORD ret = WaitForSingleObject(m_hThread, 1000);
		if (ret == WAIT_TIMEOUT)
		{
			TerminateThread(m_hThread, -1);
		}
		UpdateWorker();
		return ret == WAIT_TIMEOUT;
	}
	void UpdateWorker(const ::ThreadWorker& worker = ::ThreadWorker()) // 更新线程执行的任务
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
	bool IsIdle() // 判断线程是否空闲 true表示空闲 false表示该线程已经分配了工作
	{
		if (m_worker.load() == NULL) return true;
		return !m_worker.load()->IsValid();
	}
private:
	void ThreadWorker()
	{ // 线程工作函数
		while (m_bStatus) // 线程有效时不断执行工作
		{
			if (m_worker.load() == NULL) // 检查工作线程对象是否为空
			{
				Sleep(1);
				continue;
			}
			::ThreadWorker worker = *m_worker.load(); // 用于获取原子变量(m_worker) 的值 就是一个工作线程类(::ThreadWorker)对象
			if (worker.IsValid()) 
			{
				if (WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT) // 如果线程仍在运行，WaitForSingleObject 会立即返回 WAIT_TIMEOUT，因为超时时间是 0，表示不等待
				{
					// 1. 线程里面要执行的函数已经完全脱离了线程，由用户从ThreadFuncBase继承基类去实现自定义的函数(accept send recv...)即可 
					int ret = worker(); // 因为重载了函数调用运算符，使得类的实例对象可以像函数一样被调用 这里worker()表示调用工作线程函数
					if (ret != 0)
					{
						CString str;
						str.Format(_T("thread found warning code %d\r\n"), ret);
						OutputDebugString(str);
					}
					if (ret < 0)
					{ // 资源释放
						::ThreadWorker* pWorker = m_worker.load();
						m_worker.store(NULL); // 用于设置原子变量(m_worker) 的值
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
	{ // 线程入口函数 好处:1.可以确保线程工作函数执行完后的正确的资源释放，因为如果在工作函数里面调_endthread()，会有很多资源来不急释放 2.见笔记 
		CEdoyunThread* thiz = (CEdoyunThread*)arg;
		if (thiz)
		{
			thiz->ThreadWorker();  // 线程工作函数   
		}
		_endthread();
	}
private:
	HANDLE m_hThread; // 线程句柄
	bool m_bStatus; // false表示线程将要关闭，true表示线程正在进行
	// std::atomic 是 C++11 提供的原子操作类型，用于确保多线程环境下的操作的原子性
	std::atomic<::ThreadWorker*> m_worker; // 原子指针，存储了一个工作线程类(::ThreadWorker) 的指针(线程安全的)
};

class CEdoyunThreadPool { // 线程池类
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
	bool Invoke() // 启动线程池
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
	void Stop() // 结束线程池 
	{ 
		for (size_t i = 0; i < m_threads.size(); i++)
		{
			m_threads[i]->Stop();
		}
	}
	int DispatchWorker(const ThreadWorker& worker) // 分配线程执行任务
	{
		int index = -1;
		m_lock.lock(); // 互斥体上锁，防止多线程分配时发生冲突
		for (size_t i = 0; i < m_threads.size(); i++) // 遍历线程池中的线程
		{  // 优化思路：1.将空闲线程的编号全部放到一个列表中，分配某个线程则将其编号从列表移除 2.通过判断列表的size是否>0就知道有没有空闲线程 3.size>0则从列表中取出线程编号分配任务
			if (m_threads[i]->IsIdle()) // 若线程空闲
			{
				m_threads[i]->UpdateWorker(worker); // 将worker任务分配给空闲线程
				index = i;
				break;
			}
		}
		m_lock.unlock();
		return index; // 返回 -1 说明没有空闲线程，任务分配失败； 分配成功则返回对应线程在线程池中的序号
	}
	bool CheckThreadValid(size_t index) // 检测编号为index线程有效性
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


