#include "pch.h"
#include "ServerSocket.h"

// ���о�̬��Ա��Ҫʹ����ʽ�ط�ʽ��ʼ�������Ǿ�̬��Աһ����ڹ��캯���г�ʼ��
// ����̬��Աһ�㲻������Ĺ��캯���г�ʼ������Ϊ��̬��Ա�����������ʵ�����Ķ����������ʵ�����Ķ����õ�
// ��̬��Ա���������ڳ�ʼ���������������Ϊ���⣬��Ҫ���ھ�̬������Ա�����ֵ�����Ǳ���ʱ�������ʽ��constexpr��
// //���ڷǾ�̬������Ա���const���͵ľ�̬��Ա����Ҫ�����ⲿ���г�ʼ����
// ��������ڳ�ʼ����̬���ݳ�Ա���������� constexpr ���ͻ����ǳ������ͣ�������ö�����͡�
CServerSocket* CServerSocket::m_instance = NULL;				// ����ʽ 
// CServerSocket* CServerSocket::m_instance = new CServerSocket(); // ����ʽ

CServerSocket::CHelper CServerSocket::m_helper;

CServerSocket* pserver = CServerSocket::getInstance();