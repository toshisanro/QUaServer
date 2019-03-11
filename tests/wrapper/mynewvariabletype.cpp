#include "mynewvariabletype.h"

MyNewVariableType::MyNewVariableType(QOpcUaServer *server, const UA_NodeId &nodeId, const QMetaObject & metaObject)
	: QOpcUaBaseDataVariable(server, nodeId, metaObject)
{
	this->getMyVar()->set_value(123);
}

QOpcUaBaseDataVariable * MyNewVariableType::getMyVar()
{
	return this->findChild<QOpcUaBaseDataVariable*>("myVar");
}

QOpcUaBaseObject * MyNewVariableType::getMyObj()
{
	return this->findChild<QOpcUaBaseObject*>("myObj");
}

MyOtherNewVariableType * MyNewVariableType::getMyOtherVar()
{
	return this->findChild<MyOtherNewVariableType*>("myOtherVar");
}

MyOtherNewVariableType * MyNewVariableType::getMyOtherTwo()
{
	return this->findChild<MyOtherNewVariableType*>("myOtherTwo");
}

// ---

MyOtherNewVariableType::MyOtherNewVariableType(QOpcUaServer *server, const UA_NodeId &nodeId, const QMetaObject & metaObject)
	: QOpcUaBaseDataVariable(server, nodeId, metaObject)
{
	this->set_value("equis");
}

QOpcUaBaseDataVariable * MyOtherNewVariableType::getMyVarTwo()
{
	return this->findChild<QOpcUaBaseDataVariable*>("myVarTwo");
}

// ---

MyNewVariableSubType::MyNewVariableSubType(QOpcUaServer *server, const UA_NodeId &nodeId, const QMetaObject & metaObject)
	: MyNewVariableType(server, nodeId, metaObject)
{
	//qDebug() << "MyNewVariableSubType::MyNewVariableSubType";
}

QOpcUaBaseObject * MyNewVariableSubType::getMyObjSub()
{
	return this->findChild<QOpcUaBaseObject*>("myObjSub");
}

// ---

MyNewVariableSubSubType::MyNewVariableSubSubType(QOpcUaServer *server, const UA_NodeId &nodeId, const QMetaObject & metaObject)
	: MyNewVariableSubType(server, nodeId, metaObject)
{
	//qDebug() << "MyNewVariableSubSubType::MyNewVariableSubSubType";
}

QOpcUaBaseObject * MyNewVariableSubSubType::getMyObjSubSub()
{
	return this->findChild<QOpcUaBaseObject*>("myObjSubSub");
}

