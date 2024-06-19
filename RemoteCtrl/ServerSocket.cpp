#include "pch.h"
#include "ServerSocket.h"

// 类中静态成员需要使用显式地方式初始化，而非静态成员一般放在构造函数中初始化
// 而静态成员一般不能在类的构造函数中初始化，因为静态成员是所有这个类实例化的对象或其子类实例化的对象共用的
// 静态成员可以在类内初始化，但这种情况较为特殊，主要限于静态常量成员，其初值必须是编译时常量表达式（constexpr）
// //对于非静态常量成员或非const类型的静态成员，需要在类外部进行初始化。
// 如果在类内初始化静态数据成员，它必须是 constexpr 类型或者是常量类型，或者是枚举类型。
CServerSocket* CServerSocket::m_instance = NULL;				// 懒汉式 
// CServerSocket* CServerSocket::m_instance = new CServerSocket(); // 饿汉式

CServerSocket::CHelper CServerSocket::m_helper;

CServerSocket* pserver = CServerSocket::getInstance();