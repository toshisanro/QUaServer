#ifndef QUASERVER_H
#define QUASERVER_H

#include <type_traits>

#include <QTimer>

#include <QUaTypesConverter>
#include <QUaFolderObject>
#include <QUaBaseDataVariable>
#include <QUaProperty>
#include <QUaBaseObject>

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
class QUaBaseEvent;
class QUaGeneralModelChangeEvent;
#endif // UA_ENABLE_SUBSCRIPTIONS_EVENTS

#ifdef UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS
class QUaRefreshStartEvent;
class QUaRefreshEndEvent;
#endif // UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS

#ifdef UA_ENABLE_HISTORIZING
#include <QUaHistoryBackend>
#endif // UA_ENABLE_HISTORIZING

// Enum Stuff
typedef qint64 QUaEnumKey;
struct QUaEnumEntry
{
	QByteArray strDisplayName;
	QByteArray strDescription;
};
Q_DECLARE_METATYPE(QUaEnumEntry);
inline bool operator==(const QUaEnumEntry& lhs, const QUaEnumEntry& rhs) 
{ 
	return lhs.strDisplayName == rhs.strDisplayName && lhs.strDescription == rhs.strDescription;
}
typedef QMap<QUaEnumKey, QUaEnumEntry> QUaEnumMap;
typedef QMapIterator<QUaEnumKey, QUaEnumEntry> QUaEnumMapIter;

// User validation
typedef std::function<bool(const QString &, const QString &)> QUaValidationCallback;

// Class whose only pupose is emit signals
class QUaSignaler : public QObject
{
	Q_OBJECT
public:
    explicit QUaSignaler(QObject *parent = nullptr) 
        : QObject(parent) 
    { 
        m_processing = false;
        QObject::connect(
            this,
            &QUaSignaler::sendEvent,
            this,
            &QUaSignaler::on_sendEvent,
            Qt::QueuedConnection
        );
    };
    template <typename M1 = const std::function<void(void)>&>
    inline void execLater(M1&& func)
    {
        m_funcs.enqueue(func);
        if (m_processing)
        {
            return;
        }
        m_processing = true;
        emit this->sendEvent(QPrivateSignal());
    };
    inline bool processing() const
    {
        return m_processing;
    };
signals:
	void signalNewInstance(QUaNode *node);
    // can only be emitted internally
    void sendEvent(QPrivateSignal);
private slots:
    inline void on_sendEvent()
    {
        Q_ASSERT(m_processing);
        if (m_funcs.isEmpty())
        {
            m_processing = false;
            return;
        }
        m_funcs.dequeue()();
        emit this->sendEvent(QPrivateSignal());
    };
private:
    bool m_processing;
    QQueue<std::function<void(void)>> m_funcs;
};

class QUaSession : public QObject
{
	friend class QUaServer;
	Q_OBJECT

	Q_PROPERTY(QString   sessionId       READ sessionId      )
	Q_PROPERTY(QString   userName        READ userName       )
	Q_PROPERTY(QString   applicationName READ applicationName)
	Q_PROPERTY(QString   applicationUri  READ applicationUri )
	Q_PROPERTY(QString   productUri      READ productUri     )
	Q_PROPERTY(QString   address         READ address        )
	Q_PROPERTY(quint16   port            READ port           )
    Q_PROPERTY(QDateTime timestamp       READ timestamp      )

public:

    explicit QUaSession(QObject* parent = nullptr);

	QString   sessionId     () const;
	QString   userName       () const;
	QString   applicationName() const;
	QString   applicationUri () const;
	QString   productUri     () const;
	QString   address        () const;
	quint16   port           () const;
    QDateTime timestamp      () const;

private:
	QString   m_strSessionId;
	QString   m_strUserName;
	QString   m_strApplicationName;
	QString   m_strApplicationUri;
	QString   m_strProductUri;
	QString   m_strAddress;
	quint16   m_intPort;
    QDateTime m_timestamp;
};

class QUaServer : public QObject
{
	friend class QUaNode;
	friend class QUaBaseVariable;
	friend class QUaBaseObject;
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
	friend class QUaBaseEvent;
#endif // UA_ENABLE_SUBSCRIPTIONS_EVENTS
#ifdef UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS
    friend class QUaStateVariable;
    friend class QUaTwoStateVariable;
    friend class QUaCondition;
#endif // UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS
#ifdef UA_ENABLE_HISTORIZING
    friend class QUaHistoryBackend;
#endif // UA_ENABLE_HISTORIZING
	template <typename ClassType, typename R, bool IsMutable, typename... Args> friend struct QUaMethodTraitsBase;

	Q_OBJECT

	// Qt properties

	Q_PROPERTY(quint16    port              READ port              WRITE setPort              NOTIFY portChanged             )
	Q_PROPERTY(QByteArray certificate       READ certificate       WRITE setCertificate       NOTIFY certificateChanged      )
#ifdef UA_ENABLE_ENCRYPTION
	Q_PROPERTY(QByteArray privateKey        READ privateKey        WRITE setPrivateKey        NOTIFY privateKeyChanged       )
#endif
	Q_PROPERTY(quint16    maxSecureChannels READ maxSecureChannels WRITE setMaxSecureChannels NOTIFY maxSecureChannelsChanged)
	Q_PROPERTY(quint16    maxSessions       READ maxSessions       WRITE setMaxSessions       NOTIFY maxSessionsChanged      )
	Q_PROPERTY(bool       isRunning         READ isRunning         WRITE setIsRunning         NOTIFY isRunningChanged        )
	Q_PROPERTY(QString    applicationName   READ applicationName   WRITE setApplicationName   NOTIFY applicationNameChanged  )
	Q_PROPERTY(QString    applicationUri    READ applicationUri    WRITE setApplicationUri    NOTIFY applicationUriChanged   )
	Q_PROPERTY(QString    productName       READ productName       WRITE setProductName       NOTIFY productNameChanged      )
	Q_PROPERTY(QString    productUri        READ productUri        WRITE setProductUri        NOTIFY productUriChanged       )
	Q_PROPERTY(QString    manufacturerName  READ manufacturerName  WRITE setManufacturerName  NOTIFY manufacturerNameChanged )
	Q_PROPERTY(QString    softwareVersion   READ softwareVersion   WRITE setSoftwareVersion   NOTIFY softwareVersionChanged  )
	Q_PROPERTY(QString    buildNumber       READ buildNumber       WRITE setBuildNumber       NOTIFY buildNumberChanged      )

	Q_PROPERTY(bool anonymousLoginAllowed READ anonymousLoginAllowed WRITE setAnonymousLoginAllowed NOTIFY buildNumberChanged)

public:

    explicit QUaServer(QObject* parent = nullptr);
	
	~QUaServer();

	// Server Config API

	quint16 port() const;
	void    setPort(const quint16 &intPort); // NOTE : only updates after server restart

	QByteArray certificate() const;
	void       setCertificate(const QByteArray& byteCertificate);

#ifdef UA_ENABLE_ENCRYPTION
	QByteArray privateKey() const;
	void       setPrivateKey(const QByteArray& bytePrivateKey);
#endif

	// Server Description API

	QString applicationName() const;
	void    setApplicationName(const QString &strApplicationName);
	QString applicationUri() const;
	void    setApplicationUri(const QString &strApplicationUri);
	QString productName() const;
	void    setProductName(const QString &strProductName);
	QString productUri() const;
	void    setProductUri(const QString &strProductUri);
	QString manufacturerName() const;
	void    setManufacturerName(const QString &strManufacturerName);
	QString softwareVersion() const;
	void    setSoftwareVersion(const QString &strSoftwareVersion);
	QString buildNumber() const;
	void    setBuildNumber(const QString &strBuildNumber);

	// Server LifeCycle API

	void start();
	void stop();
	bool isRunning() const;
	void setIsRunning(const bool &running); // same as start/stop, just to complete Qt property

	// Server Limits API

	quint16 maxSecureChannels() const;
	void    setMaxSecureChannels(const quint16 &maxSecureChannels);

	quint16 maxSessions() const;
	void    setMaxSessions(const quint16 &maxSessions);

	// Instance Creation API

	// register type in order to assign it a typeNodeId
	template<typename T>
	void registerType(const QString &strNodeId = "");
	// get all instances of a type
	template<typename T>
	QList<T*> typeInstances();
	// subscribe to instance of a type added
	template<typename T, typename M>
	QMetaObject::Connection instanceCreated(const M &callback);
	// same but pass a QObject pointer to disconnect callback when the QObject is deleted
	template<typename T, typename M>
	QMetaObject::Connection instanceCreated(const QObject * pObj, const M &callback);
	// same but pass a QObject pointer and member function as a callback
	template<typename T, typename TFunc1>
	QMetaObject::Connection instanceCreated(typename QtPrivate::FunctionPointer<TFunc1>::Object* pObj,
		                                    TFunc1 callback);
	// register enum in order to use it as data type
	template<typename T>
	void registerEnum(const QString &strNodeId = "");
	void registerEnum(const QString &strEnumName, const QUaEnumMap &enumMap, const QString &strNodeId = "");
	// enum helpers
	bool        isEnumRegistered(const QString &strEnumName) const;
	QUaEnumMap  enumMap         (const QString &strEnumName) const;
	void        setEnumMap      (const QString &strEnumName, const QUaEnumMap &enumMap);
	void        updateEnumEntry (const QString &strEnumName, const QUaEnumKey &enumValue, const QUaEnumEntry &enumEntry);
	void        removeEnumEntry (const QString &strEnumName, const QUaEnumKey &enumValue);
	
	// register custom non-hierarchical reference type
	bool registerReferenceType(const QUaReferenceType &refType, const QString& strNodeId = "");
	// get list of registered reference types
	const QList<QUaReferenceType> referenceTypes() const;
	// check if a reference type is already registered
	bool referenceTypeRegistered(const QUaReferenceType& refType) const;

    // check if a node id is available (not used), not necessarily by an instance, but could be a type for example
    bool isNodeIdUsed(const QString& strNodeId) const;
	// create instance of a given (variable or object) type
	template<typename T>
	T* createInstance(QUaNode * parentNode, const QString &strNodeId = "");
	// get objects folder
	QUaFolderObject * objectsFolder() const;
	// get node reference by node id and cast to type (nullptr if node id does not exist)
	template<typename T>
	T* nodeById(const QString &strNodeId);
	// get node reference by node id (nullptr if node id does not exist)
	QUaNode * nodeById(const QString &strNodeId);
	// check if a type with type name (C++ class name) is registered
	bool isTypeNameRegistered(const QString &strTypeName) const;
	// test if node id format is valid (does not check if instance exist though)
	static bool isIdValid(const QString &strNodeId);

	// Browse API
	// (* actually browses using QObject tree)

	template<typename T>
	T* browsePath(const QStringList &strBrowsePath) const;
	// specialization
	QUaNode * browsePath(const QStringList &strBrowsePath) const;

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
	// Events API

	// create instance of a given event type
	template<typename T>
	T* createEvent();

#endif // UA_ENABLE_SUBSCRIPTIONS_EVENTS

	// Access Control API

	// anonymous login is enabled by default
	bool        anonymousLoginAllowed() const;
	void        setAnonymousLoginAllowed(const bool &anonymousLoginAllowed);
	// if user already exists, it updates password
	void        addUser(const QString &strUserName, const QString & strKey);
	// if user does not exist, it does nothing
	void        removeUser(const QString &strUserName);
	// get the key associated to the user
	QString     userKey(const QString &strUserName) const;
	// number of users
	int         userCount();
	// get all user names
	QStringList userNames() const;
	// check if user already exists
	bool        userExists(const QString &strUserName) const;
	// add a validation callback for user key, defaults checks key == password
	template<typename M>
	void        setUserValidationCallback(const M &callback);

	// Sessions API

    QList<const QUaSession*> sessions() const;

#ifdef UA_ENABLE_HISTORIZING
    // Historizing API
    template<typename T>
    void setHistorizer(T& historizer);
#endif // UA_ENABLE_HISTORIZING

signals:
	void isRunningChanged            (const bool       &running           );
	void portChanged                 (const quint16    &port              );
	void certificateChanged          (const QByteArray &byteCertificate   );
#ifdef UA_ENABLE_ENCRYPTION		     									  
	void privateKeyChanged           (const QByteArray &bytePrivateKey    );
#endif							     									  
	void maxSecureChannelsChanged    (const quint16 &maxSecureChannels    );
	void maxSessionsChanged          (const quint16 &maxSessions          );
	void applicationNameChanged      (const QString &strApplicationName   );
	void applicationUriChanged       (const QString &strApplicationUri    );
	void productNameChanged          (const QString &strProductName       );
	void productUriChanged           (const QString &strProductUri        );
	void manufacturerNameChanged     (const QString &strManufacturerName  );
	void softwareVersionChanged      (const QString &strSoftwareVersion   );
	void buildNumberChanged          (const QString &strBuildNumber       );
	void anonymousLoginAllowedChanged(const bool    &anonymousLoginAllowed);
	
	// Log API
	void logMessage(const QUaLog &log);

	// Sessions API
    void clientConnected   (const QUaSession * session);
    void clientDisconnected(const QUaSession * session);

	// NOTE : private signal
	void iterateServer(QPrivateSignal);

public slots:
	

private:
	UA_Server             * m_server;
	quint16                 m_port;
	quint16                 m_maxSecureChannels;
	quint16                 m_maxSessions;
	UA_Boolean              m_running;
	QTimer                  m_iterWaitTimer;
	QByteArray              m_byteCertificate;
	QByteArray              m_byteCertificateInternal; // NOTE : needs to exists as long as server instance
	bool                    m_anonymousLoginAllowed;
	QUaFolderObject       * m_pobjectsFolder;
	QByteArray              m_logBuffer;

#ifdef UA_ENABLE_ENCRYPTION
	QByteArray m_bytePrivateKey;
	QByteArray m_bytePrivateKeyInternal; // NOTE : needs to exists as long as server instance
#endif

	QByteArray m_byteApplicationName;
	QByteArray m_byteApplicationUri;

	QByteArray m_byteProductName;
	QByteArray m_byteProductUri;
	QByteArray m_byteManufacturerName;
	QByteArray m_byteSoftwareVersion;
	QByteArray m_byteBuildNumber; 

	QHash<QString         , QString      > m_hashUsers;
	QHash<UA_NodeId       , QUaSession*  > m_hashSessions;
	QMap <QString         , UA_NodeId    > m_mapTypes;
	QHash<QString         , QMetaObject  > m_hashMetaObjects;
	QHash<QString         , UA_NodeId    > m_hashEnums;
	QHash<QUaReferenceType, UA_NodeId    > m_hashRefTypes;
	QHash<QUaReferenceType, UA_NodeId    > m_hashHierRefTypes;
	QHash<UA_NodeId       , QUaSignaler* > m_hashSignalers;
    QHash<QString         , QStringList  > m_hashMandatoryChildren;

	QUaValidationCallback m_validationCallback;

    const QUaSession* m_currentSession;

    static QString m_anonUser;
    static QString m_anonUserToken;
    static QStringList m_anonUsers;

	// change event instance to notify client when nodes added or removed
#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
    QUaSignaler m_changeEventSignaler;
	QUaGeneralModelChangeEvent * m_changeEvent;
	QUaChangesList m_listChanges; // buffer
	void addChange(const QUaChangeStructureDataType& change);
#endif // UA_ENABLE_SUBSCRIPTIONS_EVENTS

#ifdef UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS
    QUaRefreshStartEvent* m_refreshStartEvent;
    QUaRefreshEndEvent  * m_refreshEndEvent;
    QHash<QUaNode*, QSet<QUaCondition*>> m_retainedConditions;
#endif // UA_ENABLE_SUBSCRIPTIONS_ALARMS_CONDITIONS

#ifdef UA_ENABLE_HISTORIZING
    UA_HistoryDatabase m_historDatabase;
    QUaHistoryBackend  m_historBackend;
    UA_HistoryDataGathering getGathering() const;
#endif // UA_ENABLE_HISTORIZING

	// reset open62541 config
	void resetConfig();

	// parse and validate certificate
	static UA_ByteString * parseCertificate(const QByteArray &inByteCert, 
		                                    UA_ByteString    &outUaCert, 
		                                    QByteArray       &outByteCert);
	void setupServer();
	UA_Logger getLogger();
	// types
    template<typename T>
    void registerSpecificationType(const UA_NodeId& typeNodeId, const bool abstract = false);
	void registerTypeInternal(const QMetaObject &metaObject, const QString &strNodeId = "");
	QList<QUaNode*> typeInstances(const QMetaObject &metaObject);
	template<typename T, typename M>
	QMetaObject::Connection instanceCreated(
		const QMetaObject &metaObject,
		const QObject * targetObject,
		const M &callback
	);
	// enums
	void       registerEnum(const QMetaEnum &metaEnum, const QString &strNodeId = "");
	UA_NodeId  enumValuesNodeId(const UA_NodeId &enumNodeId) const;
	UA_Variant enumValues(const UA_NodeId &enumNodeId) const;
	void       updateEnum(const UA_NodeId &enumNodeId, const QUaEnumMap &mapEnum);
	// lifecycle
    void registerTypeLifeCycle(const UA_NodeId &typeNodeId, const QMetaObject &metaObject);
    void registerTypeDefaults (const UA_NodeId &typeNodeId, const QMetaObject &metaObject);
	// meta
	void registerMetaEnums(const QMetaObject &metaObject);
	void addMetaProperties(const QMetaObject &metaObject);
	void addMetaMethods   (const QMetaObject &metaObject);

	UA_NodeId createInstanceInternal(
        const QMetaObject &metaObject, 
        QUaNode * parentNode, 
        const QString &strNodeId
    );

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
	// create instance of a given event type
	UA_NodeId createEventInternal(
        const QMetaObject &metaObject
    );
#endif // UA_ENABLE_SUBSCRIPTIONS_EVENTS

	void bindCppInstanceWithUaNode(QUaNode * nodeInstance, UA_NodeId &nodeId);

	bool isMetaObjectRegistered(const QString& strClassName) const;
	QMetaObject getRegisteredMetaObject(const QString& strClassName) const;

	QHash< UA_NodeId, std::function<
        /* 
        RetCode(nodeId, nodeContext), captures QUaServer*, metaObject -> calls
        QUaServer::uaConstructor(this, instanceNodeId, nodeContext, metaObject) -> calls
        metaObject.newInstance 
        */
        UA_StatusCode(const UA_NodeId *nodeId, void ** nodeContext)>
    > m_hashConstructors;
	QHash< UA_NodeId, std::function<
        /* RetCode(objectContext, input, output) captures QUaServer*, methIdx  -> calls
        QUaServer::callMetaMethod(this, object, metaMethod, input, output) -> calls
        metaMethod.invoke */
        UA_StatusCode(void *, const UA_Variant*, UA_Variant*)>
    > m_hashMethods;

    UA_StatusCode m_methodRetStatusCode;

	static UA_NodeId getReferenceTypeId(
        const QMetaObject &parentMetaObject, 
        const QMetaObject &childMetaObject
    );

	static UA_StatusCode uaConstructor(
        UA_Server        *server,
        const UA_NodeId  *sessionId, 
        void             *sessionContext,
        const UA_NodeId  *typeNodeId, 
        void             *typeNodeContext,
        const UA_NodeId  *nodeId, 
        void            **nodeContext
    );

	static void uaDestructor(
        UA_Server        *server,
        const UA_NodeId  *sessionId, 
        void             *sessionContext,
        const UA_NodeId  *typeNodeId, 
        void             *typeNodeContext,
        const UA_NodeId  *nodeId, 
        void            **nodeContext
    );

	static UA_StatusCode uaConstructor(
        QUaServer         *server,
        const UA_NodeId   *nodeId, 
        void             **nodeContext,
        const QMetaObject &metaObject
    );

	static UA_StatusCode methodCallback(
        UA_Server        *server,
        const UA_NodeId  *sessionId,
        void             *sessionContext,
        const UA_NodeId  *methodId,
        void             *methodContext,
        const UA_NodeId  *objectId,
        void             *objectContext,
        size_t            inputSize,
        const UA_Variant *input,
        size_t            outputSize,
        UA_Variant       *output
    );

    static UA_StatusCode callMetaMethod(
        QUaServer         *server,
        QUaBaseObject     *object, 
        const QMetaMethod &metaMethod,
        const UA_Variant  *input, 
        UA_Variant        *output
    );

    static void bindMethod(
        QUaServer       *server,
        const UA_NodeId *methodNodeId,
        const int       &metaMethodIndex
    );

    static QHash<QString, int> metaMethodIndexes(const QMetaObject& metaObject);

	static bool isNodeBound(const UA_NodeId &nodeId, UA_Server *server);

	struct QOpcUaEnumValue
	{
		UA_Int64         Value;
		UA_LocalizedText DisplayName;
		UA_LocalizedText Description;
	};

	static QUaServer * getServerNodeContext(UA_Server * server);

	static UA_StatusCode addEnumValues(
        UA_Server             *server, 
        UA_NodeId             *parent, 
        const UA_UInt32        numEnumValues, 
        const QOpcUaEnumValue *enumValues
    );

	static UA_StatusCode activateSession(UA_Server                    *server, 
		                                 UA_AccessControl             *ac,
		                                 const UA_EndpointDescription *endpointDescription,
		                                 const UA_ByteString          *secureChannelRemoteCertificate,
		                                 const UA_NodeId              *sessionId,
		                                 const UA_ExtensionObject     *userIdentityToken,
		                                 void                        **sessionContext);

	static void newSession(QUaServer* server, 
		                   const UA_NodeId* sessionId);

	static void closeSession(UA_Server        *server, 
		                     UA_AccessControl *ac, 
		                     const UA_NodeId  *sessionId, 
		                     void             *sessionContext);

	static UA_UInt32 getUserRightsMask(UA_Server        *server,
		                               UA_AccessControl *ac,
		                               const UA_NodeId  *sessionId,
		                               void             *sessionContext,
		                               const UA_NodeId  *nodeId,
		                               void             *nodeContext);

	static UA_Byte getUserAccessLevel(UA_Server        *server, 
		                              UA_AccessControl *ac,
		                              const UA_NodeId  *sessionId, 
		                              void             *sessionContext,
		                              const UA_NodeId  *nodeId, 
		                              void             *nodeContext);

	static UA_Boolean getUserExecutable(UA_Server        *server, 
		                                UA_AccessControl *ac,
		                                const UA_NodeId  *sessionId, 
		                                void             *sessionContext,
		                                const UA_NodeId  *methodId, 
		                                void             *methodContext);

	static UA_Boolean getUserExecutableOnObject(UA_Server        *server, 
		                                        UA_AccessControl *ac,
		                                        const UA_NodeId  *sessionId, 
		                                        void             *sessionContext,
		                                        const UA_NodeId  *methodId, 
		                                        void             *methodContext,
		                                        const UA_NodeId  *objectId, 
		                                        void             *objectContext);

	// NOTE : temporary values needed to instantiate node, used to simplify user API
	//        passed-in in QUaServer::uaConstructor and used in QUaNode::QUaNode
	const UA_NodeId   * m_newNodeNodeId;
	const QMetaObject * m_newNodeMetaObject;
};

template<typename T>
inline void QUaServer::registerType(const QString &strNodeId/* = ""*/)
{
	// call internal method
	this->registerTypeInternal(T::staticMetaObject, strNodeId);
}

template<typename T>
inline QList<T*> QUaServer::typeInstances()
{
	QList<T*> retList;
	auto nodeList = this->typeInstances(T::staticMetaObject);
	for (int i = 0; i < nodeList.count(); i++)
	{
		auto instance = qobject_cast<T*>(nodeList.at(i));
		Q_CHECK_PTR(instance);
		retList << instance;
	}
	return retList;
}

template<typename T, typename M>
inline QMetaObject::Connection QUaServer::instanceCreated(const M & callback)
{
	return this->instanceCreated<T>(T::staticMetaObject, nullptr, callback);
}

template<typename T, typename M>
inline QMetaObject::Connection QUaServer::instanceCreated(const QObject * pObj, const M & callback)
{
	return this->instanceCreated<T>(T::staticMetaObject, pObj, callback);
}

template<typename T, typename TFunc1>
inline QMetaObject::Connection QUaServer::instanceCreated(
	typename QtPrivate::FunctionPointer<TFunc1>::Object * pObj, 
	TFunc1 callback)
{
	// create callback as std::function and bind
	std::function<void(T*)> f = std::bind(callback, pObj, std::placeholders::_1);
	return this->instanceCreated<T>(T::staticMetaObject, pObj, f);
}

template<typename T>
inline void QUaServer::registerSpecificationType(const UA_NodeId& typeNodeId, const bool abstract/* = false*/)
{
    auto& metaObject = T::staticMetaObject;
    m_mapTypes.insert(QString(metaObject.className()), typeNodeId);
    m_hashMetaObjects.insert(QString(metaObject.className()), metaObject);
    // register for default mandatory children and so on
    this->registerTypeDefaults(typeNodeId, metaObject);
    // set node context only if instatiable
    if (abstract)
    {
        return;
    }
    auto st = UA_Server_setNodeContext(m_server, typeNodeId, (void*)this);
    Q_ASSERT(st == UA_STATUSCODE_GOOD);
    // NOTE : cannot move all the registering stuff to templated versions
    //        because we need recursive upstream regitration to base classes
    //        in order to maintain a simple to use API
    this->registerTypeLifeCycle(typeNodeId, metaObject);
}

template<typename T, typename M>
inline QMetaObject::Connection QUaServer::instanceCreated(
	const QMetaObject & metaObject, 
	const QObject * targetObject, 
	const M & callback)
{
	// check if OPC UA relevant
	if (!metaObject.inherits(&QUaNode::staticMetaObject))
	{
		Q_ASSERT_X(false, "QUaServer::instanceCreated", "Unsupported base class. It must derive from QUaNode");
		return QMetaObject::Connection();
	}
	// try to get typeNodeId, if null, then register it
	QString   strClassName = QString(metaObject.className());
	UA_NodeId typeNodeId = m_mapTypes.value(strClassName, UA_NODEID_NULL);
	if (UA_NodeId_isNull(&typeNodeId))
	{
		this->registerTypeInternal(metaObject);
		typeNodeId = m_mapTypes.value(strClassName, UA_NODEID_NULL);
	}
	Q_ASSERT(!UA_NodeId_isNull(&typeNodeId));
	// check if there is already a signaler
	// NOTE : one signaler per registered ua type
	auto signaler = m_hashSignalers.value(typeNodeId, nullptr);
	if (!signaler)
	{
		m_hashSignalers[typeNodeId] = new QUaSignaler(this);
		signaler = m_hashSignalers.value(typeNodeId, nullptr);
	}
	Q_CHECK_PTR(signaler);
	// connect to signal
	return QObject::connect(signaler, &QUaSignaler::signalNewInstance, targetObject ? targetObject : this,
	[callback](QUaNode * node) {
		auto specializedNode = qobject_cast<T*>(node);
		Q_CHECK_PTR(specializedNode);
		callback(specializedNode);
	}, Qt::QueuedConnection);
}

template<typename T>
inline void QUaServer::registerEnum(const QString &strNodeId/* = ""*/)
{
	// call internal method
	this->registerEnum(QMetaEnum::fromType<T>(), strNodeId);
}

template<typename T>
inline T * QUaServer::createInstance(QUaNode * parentNode, const QString &strNodeId/* = ""*/)
{
	// instantiate first in OPC UA
	UA_NodeId newInstanceNodeId = this->createInstanceInternal(
        T::staticMetaObject, 
        parentNode, 
        strNodeId
    );
	if (UA_NodeId_isNull(&newInstanceNodeId))
	{
		UA_NodeId_clear(&newInstanceNodeId);
		return nullptr;
	}
	// get new c++ instance created in UA constructor
	auto tmp = QUaNode::getNodeContext(newInstanceNodeId, this);
	T * newInstance = qobject_cast<T*>(tmp);
	Q_CHECK_PTR(newInstance);
    Q_ASSERT(newInstance->parent() == parentNode);
	// return c++ instance
	UA_NodeId_clear(&newInstanceNodeId);
	return newInstance;
}

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
template<typename T>
inline T * QUaServer::createEvent()
{
	// instantiate first in OPC UA
	UA_NodeId newEventNodeId = this->createEventInternal(
        T::staticMetaObject
    );
	if (UA_NodeId_isNull(&newEventNodeId))
	{
		return nullptr;
	}
	// get new c++ instance created in UA constructor
	auto tmp = QUaNode::getNodeContext(newEventNodeId, this);
	T * newEvent = qobject_cast<T*>(tmp);
	Q_CHECK_PTR(newEvent);
    // set originator 
    newEvent->setSourceNode(
        QUaTypesConverter::nodeIdToQString(UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER))
    );
    newEvent->setSourceName(
        tr("Server")
    );
	// reparent qt
    newEvent->setParent(this);
    // TODO : this API is only for nodes not reachible in the address space
//#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
//    // add reference added change to buffer
//    this->addChange({
//        QUaTypesConverter::nodeIdToQString(UA_NODEID_NUMERIC(0, UA_NS0ID_SERVER)),
//        QUaTypesConverter::nodeIdToQString(UA_NODEID_NUMERIC(0, UA_NS0ID_SERVERTYPE)),
//        QUaChangeVerb::ReferenceAdded // UaExpert does not recognize QUaChangeVerb::NodeAdded
//    });
//#endif // UA_ENABLE_SUBSCRIPTIONS_EVENTS
	// return c++ event instance
	return newEvent;
}
#endif // UA_ENABLE_SUBSCRIPTIONS_EVENTS

template<typename T>
inline T * QUaServer::nodeById(const QString &strNodeId)
{
	return qobject_cast<T*>(this->nodeById(strNodeId));
}

template<typename T>
inline T * QUaServer::browsePath(const QStringList & strBrowsePath) const
{
	return qobject_cast<T*>(this->browsePath(strBrowsePath));
}

template<typename M>
inline void QUaServer::setUserValidationCallback(const M & callback)
{
	m_validationCallback = [callback](const QString &strUserName, const QString &strPassword) {
		return callback(strUserName, strPassword);
	};
}

#ifdef UA_ENABLE_HISTORIZING
template<typename T>
inline void QUaServer::setHistorizer(T& historizer)
{
    m_historBackend.setHistorizer<T>(historizer);
}
#endif // UA_ENABLE_HISTORIZING


// -------- OTHER TYPES --------------------------------------------------

template<typename T>
inline bool QUaNode::serialize(T& serializer, QQueue<QUaLog>& logOut)
{
    if (!this->serializeStart<T>(serializer, logOut))
    {
        for (auto& log : logOut)
        { emit this->server()->logMessage(log); }
        return false;
    }
    if (!this->serializeInternal<T>(serializer, logOut))
    {
        for (auto& log : logOut)
        { emit this->server()->logMessage(log); }
        return false;
    }
    if (!this->serializeEnd<T>(serializer, logOut))
    {
        for (auto& log : logOut)
        { emit this->server()->logMessage(log); }
        return false;
    }
    for (auto& log : logOut)
    { emit this->server()->logMessage(log); }
    return true;
}

template<typename T>
inline typename std::enable_if<QUaHasMethodSerializeStart<T>::value, bool>::type
QUaNode::serializeStart(T& serializer, QQueue<QUaLog>& logOut)
{
    return serializer.T::serializeStart(logOut);
}

template<typename T>
inline typename std::enable_if<!QUaHasMethodSerializeStart<T>::value, bool>::type
QUaNode::serializeStart(T& serializer, QQueue<QUaLog>& logOut)
{
    Q_UNUSED(serializer);
    Q_UNUSED(logOut);
    return true;
}

template<typename T>
inline typename std::enable_if<QUaHasMethodSerializeEnd<T>::value, bool>::type 
QUaNode::serializeEnd(T& serializer, QQueue<QUaLog>& logOut)
{
    return serializer.T::serializeEnd(logOut);
}

template<typename T>
inline typename std::enable_if<!QUaHasMethodSerializeEnd<T>::value, bool>::type
QUaNode::serializeEnd(T& serializer, QQueue<QUaLog>& logOut)
{
    Q_UNUSED(serializer);
    Q_UNUSED(logOut);
    return true;
}

template<typename T>
inline bool QUaNode::serializeInternal(T& serializer, QQueue<QUaLog> &logOut)
{
    if(!serializer.writeInstance(
        this->nodeId(),
        this->className(),
        this->serializeAttrs(),
        this->serializeRefs(),
        logOut
    ))
    {
        return false;
    }
    // recurse children (only hierarchical references)
    for (auto& refType : m_qUaServer->m_hashHierRefTypes.keys())
    {
        for (auto ref : this->findReferences(refType))
        {
            if (!ref->serializeInternal(serializer, logOut))
            {
                return false;
            }
        }
    }
    return true;
}

template<typename T>
inline bool QUaNode::deserialize(T& deserializer, QQueue<QUaLog> &logOut)
{
    if (!this->deserializeStart<T>(deserializer, logOut))
    {
        for (auto &log : logOut)
        { emit this->server()->logMessage(log); }
        // stop deserializing
        return false;
    }
    QString typeName = this->className();
    QMap<QString, QVariant> attrs;
    QList<QUaForwardReference> forwardRefs;
    if (!deserializer.readInstance(
        this->nodeId(),
        typeName,
        attrs,
        forwardRefs,
        logOut
    ))
    {
        for (auto &log : logOut)
        { emit this->server()->logMessage(log); }
        // stop deserializing
        return false;
    }
    // deserialize recursive
    QMap<QUaNode*, QList<QUaForwardReference>> nonHierRefs;
    if (!this->deserializeInternal<T>(
        deserializer,
        attrs,
        forwardRefs,
        nonHierRefs,
        logOut,
        this == m_qUaServer->objectsFolder()
    ))
    {
        for (auto &log : logOut)
        { emit this->server()->logMessage(log); }
        // stop deserializing
        return false;
    }
    // non-hierarchical at the end
    auto srcNodes = nonHierRefs.keys();
    for (auto srcNode : srcNodes)
    {
        for (auto nonHierRef : nonHierRefs[srcNode])
        {
            auto targetNode = this->server()->nodeById(nonHierRef.targetNodeId);
            if (targetNode)
            {
                srcNode->addReference(nonHierRef.refType, targetNode, true);
            }
            else
            {
                logOut.enqueue({
                    tr("Failed to add non-hierarchical reference (%1:%2) "
                       "from source node %3 to target node %4. Target node does not exist.")
                        .arg(nonHierRef.refType.strForwardName)
                        .arg(nonHierRef.refType.strInverseName)
                        .arg(srcNode->nodeId())
                        .arg(nonHierRef.targetNodeId),
                    QUaLogLevel::Error,
                    QUaLogCategory::Serialization
                });
            }
        }
    }
    if (!this->deserializeEnd<T>(deserializer, logOut))
    {
       for (auto &log : logOut)
        { emit this->server()->logMessage(log); }
        // stop deserializing
        return false;
    }
    for (auto& log : logOut)
    { emit this->server()->logMessage(log); }
    return true;
}

template<typename T>
inline typename std::enable_if<QUaHasMethodDeserializeStart<T>::value, bool>::type
QUaNode::deserializeStart(T& deserializer, QQueue<QUaLog>& logOut)
{
    return deserializer.T::deserializeStart(logOut);
}

template<typename T>
inline typename std::enable_if<!QUaHasMethodDeserializeStart<T>::value, bool>::type
QUaNode::deserializeStart(T& deserializer, QQueue<QUaLog>& logOut)
{
    Q_UNUSED(deserializer);
    Q_UNUSED(logOut);
    return true;
}

template<typename T>
inline typename std::enable_if<QUaHasMethodDeserializeEnd<T>::value, bool>::type
QUaNode::deserializeEnd(T& deserializer, QQueue<QUaLog>& logOut)
{
    return deserializer.T::deserializeEnd(logOut);
}

template<typename T>
inline typename std::enable_if<!QUaHasMethodDeserializeEnd<T>::value, bool>::type
QUaNode::deserializeEnd(T& deserializer, QQueue<QUaLog>& logOut)
{
    Q_UNUSED(deserializer);
    Q_UNUSED(logOut);
    return true;
}

template<typename T>
inline bool QUaNode::deserializeInternal(
    T& deserializer,
    const QMap<QString, QVariant> &attrs,
    const QList<QUaForwardReference> &forwardRefs,
    QMap<QUaNode*, QList<QUaForwardReference>>& nonHierRefs,
    QQueue<QUaLog>& logOut,
    const bool& isObjsFolder/* = false*/)
{
    // deserialize attrs for all nodes except objectsFolder
    if (!isObjsFolder)
    {
        // deserialize attrs (this can only generate warnings)
        this->deserializeAttrs(attrs, logOut);
    }
    // get existing children list to match hierachical forward references
    auto existingChildren = this->browseChildren();
    QHash<QString, QUaNode*> mapExistingChildrenNodeIds;
    QHash<QString, QList<QUaNode*>> mapExistingChildrenBrowseName;
    for (auto child : existingChildren)
    {
        mapExistingChildrenNodeIds[child->nodeId()] = child;
        mapExistingChildrenBrowseName[child->browseName()] << child;
    }
    // loop deserialized forward references
    for (auto &forwRef : forwardRefs)
    {
        // leave non-hierarchical refs for the end (nonHierRefs)
        if (!m_qUaServer->m_hashHierRefTypes.contains(forwRef.refType))
        {
            nonHierRefs[this] << forwRef;
            continue;
        }
        // validate ref child typeName
        QString typeName = forwRef.targetType;
        if (!this->server()->isMetaObjectRegistered(typeName))
        {
            logOut.enqueue({
                tr("Type name %1 for deserialized node %2 is not registered. Ignoring.")
                    .arg(typeName)
                    .arg(forwRef.targetNodeId),
                QUaLogLevel::Error,
                QUaLogCategory::Serialization
            });
            continue;
        }
        // deserialize hierarchical ref child
        QMap<QString, QVariant> attrs;
        QList<QUaForwardReference> forwardRefs;
        if (!deserializer.readInstance(
            forwRef.targetNodeId,
            typeName,
            attrs,
            forwardRefs,
            logOut
        ))
        {
            // stop deserializing
            return false;
        }
        // check if ref child already exists by browse name
        if (attrs.contains("browseName") && !attrs.value("browseName").toString().isEmpty())
        {
            QString strBrowseName = attrs.value("browseName").toString();
            auto existingBrowseName = mapExistingChildrenBrowseName.value(strBrowseName, QList<QUaNode*>());
            // deserialize existing
            if (existingBrowseName.count() > 0)
            {
                QUaNode* instance = nullptr;
                if (existingBrowseName.count() > 1)
                {
                    // if existingChildren does not contain an instance it meas it has already been deserialized
                    while (
                        !existingChildren.isEmpty() &&
                        !existingChildren.contains(existingBrowseName.first()) && 
                        existingBrowseName.count() > 0
                        )
                    {
                        existingBrowseName.takeFirst();
                    }
                    if (existingBrowseName.count() == 0)
                    {
                        logOut.enqueue({
                        tr("All children of %1 with browseName %2 have been deserialized. "
                        "Ignoring deserialization of forward reference to %3 (type %4).")
                                .arg(this->nodeId())
                                .arg(strBrowseName)
                                .arg(forwRef.targetNodeId)
                                .arg(forwRef.targetType),
                            QUaLogLevel::Error,
                            QUaLogCategory::Serialization
                        });
                        continue;
                    }
                    instance = existingBrowseName.takeFirst();
                    // get the first one with the same type
                    while (instance->className().compare(forwRef.targetType, Qt::CaseSensitive) != 0 &&
                        existingBrowseName.count() > 0)
                    {
                        instance = existingBrowseName.takeFirst();
                    }
                    if (existingBrowseName.count() == 0 && 
                        instance->className().compare(forwRef.targetType, Qt::CaseSensitive) != 0)
                    {
                        logOut.enqueue({
                        tr("Could not find a child of %1 with browseName %2 that matches the type %3. "
                        "Ignoring deserialization of forward reference to %4.")
                                .arg(this->nodeId())
                                .arg(strBrowseName)
                                .arg(forwRef.targetType)
                                .arg(forwRef.targetNodeId),
                            QUaLogLevel::Error,
                            QUaLogCategory::Serialization
                        });
                        continue;
                    }
                    if (existingBrowseName.count() > 0)
                    {
                        logOut.enqueue({
                            tr("Node %1 contains more than one children with the same browseName attribute %2 and type %3. "
                            "Deserializing over instance with node id %4.")
                                .arg(this->nodeId())
                                .arg(strBrowseName)
                                .arg(forwRef.targetType)
                                .arg(instance->nodeId()),
                            QUaLogLevel::Warning,
                            QUaLogCategory::Serialization
                        });
                    }
                }
                else
                {
                    // get only child instance with matching browse name
                    instance = existingBrowseName.takeFirst();
                    if (instance->className().compare(forwRef.targetType, Qt::CaseSensitive) != 0)
                    {
                        logOut.enqueue({
                        tr("Could not find a child of %1 with browseName %2 that matches the type %3. "
                        "Ignoring deserialization of forward reference to %4.")
                                .arg(this->nodeId())
                                .arg(strBrowseName)
                                .arg(forwRef.targetType)
                                .arg(forwRef.targetNodeId),
                            QUaLogLevel::Error,
                            QUaLogCategory::Serialization
                        });
                        continue;
                    }
                }
                // remove from existingChildren to mark it as deserialized
                existingChildren.removeOne(instance);
                // deserialize (recursive)
                bool ok = instance->deserializeInternal<T>(
                    deserializer,
                    attrs,
                    forwardRefs,
                    nonHierRefs,
                    logOut,
                    instance == m_qUaServer->objectsFolder()
                );
                if (!ok)
                {
                    return ok;
                }
                // continue next ref child
                continue;
            }
        } // end by browse name
        // check if child already exists by node id and type matches
        if (mapExistingChildrenNodeIds.contains(forwRef.targetNodeId) &&
            mapExistingChildrenNodeIds[forwRef.targetNodeId]->className().compare(forwRef.targetType, Qt::CaseSensitive) == 0)
        {
            QUaNode* instance = mapExistingChildrenNodeIds.take(forwRef.targetNodeId);
            // remove from existingChildren to mark it as deserialized
            existingChildren.removeOne(instance);
            // deserialize (recursive)
            bool ok = instance->deserializeInternal<T>(
                deserializer,
                attrs,
                forwardRefs,
                nonHierRefs,
                logOut,
                instance == m_qUaServer->objectsFolder()
            );
            if (!ok)
            {
                return ok;
            }
            // continue next ref child
            continue;
        } // end by node id
        // no existing children matched by browse name nor by node id and type
        // so we must create a new one but before, check node id does not exist
        QString strTargetNodeId = forwRef.targetNodeId;
        if (this->server()->isNodeIdUsed(strTargetNodeId))
        {
            logOut.enqueue({
                tr("Forward reference of %1 with node id %2 (type %3) already exists elsewhere in server. Instantiating with random node id.")
                        .arg(this->nodeId())
                        .arg(strTargetNodeId)
                        .arg(forwRef.targetType),
                    QUaLogLevel::Warning,
                    QUaLogCategory::Serialization
            });
            // empty node id means use random node id
            strTargetNodeId.clear();
        }
        // to create new first get metaobject
        QMetaObject metaObject = this->server()->getRegisteredMetaObject(typeName);
        // instantiate first in OPC UA
        UA_NodeId newInstanceNodeId = this->server()->createInstanceInternal(
            metaObject, 
            this, 
            strTargetNodeId
        );
        if (UA_NodeId_isNull(&newInstanceNodeId))
        {
            UA_NodeId_clear(&newInstanceNodeId);
            logOut.enqueue({
            tr("Failed to instantiate child instance of %1 with node id %2. Ignoring.")
                    .arg(this->nodeId())
                    .arg(forwRef.targetNodeId),
                QUaLogLevel::Error,
                QUaLogCategory::Serialization
            });
            continue;
        }
        // get new c++ instance created in UA constructor
        auto instance = QUaNode::getNodeContext(newInstanceNodeId, this->server());
        // return c++ instance
        UA_NodeId_clear(&newInstanceNodeId);
        // deserialize (recursive)
        bool ok = instance->deserializeInternal<T>(
            deserializer,
            attrs,
            forwardRefs,
            nonHierRefs,
            logOut,
            instance == m_qUaServer->objectsFolder()
        );
        if (!ok)
        {
            return ok;
        }
    }
    // TODO : what to do with non-matching existing children
    if (existingChildren.count() > 0)
    {
        logOut.enqueue({
            tr("Node %1 contains %2 existing children which did not match any if its deserialized forward references.")
                .arg(this->nodeId())
                .arg(existingChildren.count()),
            QUaLogLevel::Warning,
            QUaLogCategory::Serialization
        });
    }
    // success
    return true;
}

template<typename T>
inline T * QUaBaseObject::addChild(const QString &strNodeId/* = ""*/)
{
    return m_qUaServer->createInstance<T>(this, strNodeId);
}

#ifdef UA_ENABLE_SUBSCRIPTIONS_EVENTS
template<typename T>
inline T * QUaBaseObject::createEvent()
{
    // instantiate first in OPC UA
    UA_NodeId newEventNodeId = m_qUaServer->createEventInternal(
        T::staticMetaObject
    );
    if (UA_NodeId_isNull(&newEventNodeId))
    {
        return nullptr;
    }
    // get new c++ instance created in UA constructor
    auto tmp = QUaNode::getNodeContext(newEventNodeId, m_qUaServer);
    T * newEvent = qobject_cast<T*>(tmp);
    Q_CHECK_PTR(newEvent);
    // set originator 
    newEvent->setSourceNode(
        QUaTypesConverter::nodeIdToQString(m_nodeId)
    );
    newEvent->setSourceName(
        this->displayName()
    );
    // reparent qt
    newEvent->setParent(this);
    // return c++ event instance
    return newEvent;
}
#endif // UA_ENABLE_SUBSCRIPTIONS_EVENTS

template<typename T>
inline T * QUaBaseDataVariable::addChild(const QString &strNodeId/* = ""*/)
{
    return m_qUaServer->createInstance<T>(this, strNodeId);
}

template<typename T>
inline void QUaBaseVariable::setDataTypeEnum()
{
	// register if not registered
	m_qUaServer->registerEnum<T>();
	this->setDataTypeEnum(QMetaEnum::fromType<T>());
}

// NOTE : had to remove template template parameters because is c++17
template <typename T>
struct container_traits : std::false_type {};

template <typename T>
struct container_traits<QList<T>> : std::true_type
{
	using inner_type = T;
};

template <typename T>
struct container_traits<QVector<T>> : std::true_type
{
	using inner_type = T;
};

template <typename ClassType, typename R, bool IsMutable, typename... Args>
struct QUaMethodTraitsBase
{
    inline static bool getIsMutable()
    {
        return IsMutable;
    }

    inline static const size_t getNumArgs()
    {
        return sizeof...(Args);
    }

    template<typename T>
    inline static QString getTypeName()
    {
        return QString(typeid(T).name());
    }

    inline static QString getRetType()
    {
        return getTypeName<R>();
    }

    inline static QStringList getArgTypes()
    {
        return { getTypeName<Args>()... };
    }

	// https://stackoverflow.com/questions/6627651/enable-if-method-specialization
	template<typename T>
	inline static UA_Argument getTypeUaArgument(QUaServer * uaServer, const int &iArg = 0)
	{
        return getTypeUaArgumentInternalArray<T>(container_traits<T>(), uaServer, iArg);
	}

	template<typename T>
	inline static UA_Argument getTypeUaArgumentInternalArray(std::false_type, QUaServer * uaServer, const int &iArg = 0)
	{
		return getTypeUaArgumentInternalEnum<T>(std::is_enum<T>(), uaServer, iArg);
	}

	template<typename T>
	inline static UA_Argument getTypeUaArgumentInternalArray(std::true_type, QUaServer * uaServer, const int &iArg = 0)
	{
        UA_Argument arg = getTypeUaArgumentInternalEnum<typename container_traits<T>::inner_type>
            (std::is_enum<typename container_traits<T>::inner_type>(), uaServer, iArg);
		arg.valueRank = UA_VALUERANK_ONE_DIMENSION;
		return arg;
	}

    template<typename T>
    inline static UA_Argument getTypeUaArgumentInternalEnum(std::false_type, QUaServer * uaServer, const int &iArg = 0)
    {
        Q_UNUSED(uaServer);
        UA_NodeId nodeId = QUaTypesConverter::uaTypeNodeIdFromCpp<T>();
        return getTypeUaArgumentInternal<T>(nodeId, iArg);
    }

    template<typename T>
    inline static UA_Argument getTypeUaArgumentInternalEnum(std::true_type, QUaServer * uaServer, const int &iArg = 0)
    {
        QMetaEnum metaEnum = QMetaEnum::fromType<T>();
        // compose enum name
        #if QT_VERSION >= 0x051200
            QString strEnumName = QString("%1::%2").arg(metaEnum.scope()).arg(metaEnum.enumName());
        #else
            QString strEnumName = QString("%1::%2").arg(metaEnum.scope()).arg(metaEnum.name());
        #endif
        // register if not exists
        if (!uaServer->m_hashEnums.contains(strEnumName))
        {
            uaServer->registerEnum(metaEnum);
        }
        Q_ASSERT(uaServer->m_hashEnums.contains(strEnumName));
        // pass in enum nodeid
        UA_NodeId nodeId = uaServer->m_hashEnums.value(strEnumName);
        return getTypeUaArgumentInternal<T>(nodeId, iArg);
    }

    template<typename T>
    inline static UA_Argument getTypeUaArgumentInternal(const UA_NodeId &nodeId, const int &iArg = 0)
    {
        UA_Argument inputArgument;
        UA_Argument_init(&inputArgument);
        // create n-th argument with name "Arg" + number
        inputArgument.description = UA_LOCALIZEDTEXT((char *)"", (char *)"Method Argument");
        inputArgument.name        = QUaTypesConverter::uaStringFromQString(QObject::trUtf8("Arg%1").arg(iArg));
        inputArgument.dataType    = nodeId;
        inputArgument.valueRank   = UA_VALUERANK_SCALAR;
        // return
        return inputArgument;
    }

    inline static const bool isRetUaArgumentVoid()
    {
        return std::is_same<R, void>::value;
    }

	inline static UA_Argument getRetUaArgument()
	{
        return getRetUaArgumentArray<R>(container_traits<R>());
	}

	template<typename T>
    inline static UA_Argument getRetUaArgumentArray(std::false_type)
    {
        if (isRetUaArgumentVoid()) return UA_Argument();
        // create output argument
        UA_Argument outputArgument;
        UA_Argument_init(&outputArgument);
        outputArgument.description = UA_LOCALIZEDTEXT((char *)"",
                                                      (char *)"Result Value");
        outputArgument.name        = QUaTypesConverter::uaStringFromQString((char *)"Result");
        outputArgument.dataType    = QUaTypesConverter::uaTypeNodeIdFromCpp<T>();
        outputArgument.valueRank   = UA_VALUERANK_SCALAR;
        return outputArgument;
    }

	template<typename T>
	inline static UA_Argument getRetUaArgumentArray(std::true_type)
	{
        UA_Argument arg = getRetUaArgumentArray<typename container_traits<T>::inner_type>(container_traits<typename container_traits<T>::inner_type>());
		arg.valueRank = UA_VALUERANK_ONE_DIMENSION;
		return arg;
	}

    inline static QVector<UA_Argument> getArgsUaArguments(QUaServer * uaServer)
    {
        int iArg = 0;
        const size_t nArgs = getNumArgs();
        if (nArgs <= 0) return QVector<UA_Argument>();
        return { getTypeUaArgument<Args>(uaServer, iArg++)... };
    }

    template<typename T>
    inline static T convertArgType(const UA_Variant * input, const int &iArg)
    {
        return convertArgTypeArray<T>(container_traits<T>(), input, iArg);
    }

	template<typename T>
	inline static T convertArgTypeArray(std::false_type, const UA_Variant * input, const int &iArg)
	{
		QVariant varQt = QUaTypesConverter::uaVariantToQVariant(input[iArg]);
		return varQt.value<T>();
	}

	template<typename T>
	inline static T convertArgTypeArray(std::true_type, const UA_Variant * input, const int &iArg)
	{
		T retArr;
		auto varQt = QUaTypesConverter::uaVariantToQVariant(input[iArg]);
		auto iter  = varQt.value<QSequentialIterable>();
		for (const QVariant &v : iter)
		{
            retArr << v.value<typename container_traits<T>::inner_type>();
		}
		return retArr;
	}

    template<typename M>
    inline static UA_Variant execCallback(const M &methodCallback, const UA_Variant * input)
    {
        // call method
        // NOTE : arguments inverted when calling "methodCallback"? only x++ and x-- work (i.e. not --x)?
        int iArg = (int)getNumArgs() - 1;
        // call method
        QVariant varResult = QVariant::fromValue(methodCallback(convertArgType<Args>(input, iArg--)...));
        // set result
        UA_Variant retVar = QUaTypesConverter::uaVariantFromQVariant(varResult);

        // TODO : cleanup? UA_Variant_deleteMembers(&retVar);

        return retVar;
    }
};
// general case
template<typename T>
struct QOpcUaMethodTraits : QOpcUaMethodTraits<decltype(&T::operator())>
{};
// specialization - const
template <typename ClassType, typename R, typename... Args>
struct QOpcUaMethodTraits< R(ClassType::*)(Args...) const > : QUaMethodTraitsBase<ClassType, R, false, Args...>
{};
// specialization - mutable
template <typename ClassType, typename R, typename... Args>
struct QOpcUaMethodTraits< R(ClassType::*)(Args...) > : QUaMethodTraitsBase<ClassType, R, true, Args...>
{};
// specialization - const | no return value
template <typename ClassType, typename... Args>
struct QOpcUaMethodTraits< void(ClassType::*)(Args...) const > : QUaMethodTraitsBase<ClassType, void, false, Args...>
{
    template<typename M>
    inline static UA_Variant execCallback(const M &methodCallback, const UA_Variant * input)
    {
        // call method
        // NOTE : arguments inverted when calling "methodCallback"? only x++ and x-- work (i.e. not --x)?
        int iArg = (int)QOpcUaMethodTraits<M>::getNumArgs() - 1;
        // call method
        methodCallback(QOpcUaMethodTraits<M>::template convertArgType<Args>(input, iArg--)...);
        // set result
        return UA_Variant();
    }
};
// specialization - mutable | no return value
template <typename ClassType, typename... Args>
struct QOpcUaMethodTraits< void(ClassType::*)(Args...) > : QUaMethodTraitsBase<ClassType, void, true, Args...>
{
    template<typename M>
    inline static UA_Variant execCallback(const M &methodCallback, const UA_Variant * input)
    {
        // call method
        // NOTE : arguments inverted when calling "methodCallback"? only x++ and x-- work (i.e. not --x)?
        int iArg = QOpcUaMethodTraits<M>::getNumArgs() - 1;
        // call method
        methodCallback(QOpcUaMethodTraits<M>::template convertArgType<Args>(input, iArg--)...);
        // set result
        return UA_Variant();
    }
};
// specialization - function pointer
template <typename R, typename... Args>
struct QOpcUaMethodTraits< R(*)(Args...) > : QUaMethodTraitsBase<void, R, true, Args...>
{};
// specialization - function pointer | no return value
template <typename... Args>
struct QOpcUaMethodTraits< void(*)(Args...) > : QUaMethodTraitsBase<void, void, true, Args...>
{
    template<typename M>
    inline static UA_Variant execCallback(const M &methodCallback, const UA_Variant * input)
    {
        // call method
        // NOTE : arguments inverted when calling "methodCallback"? only x++ and x-- work (i.e. not --x)?
        int iArg = QOpcUaMethodTraits<M>::getNumArgs() - 1;
        // call method
        methodCallback(QOpcUaMethodTraits<M>::template convertArgType<Args>(input, iArg--)...);
        // set result
        return UA_Variant();
    }
};

template<typename M>
inline void QUaBaseObject::addMethod(const QString & strMethodName, const M & methodCallback, const QString & strNodeId)
{
    // create input arguments
    UA_Argument * p_inputArguments = nullptr;
    QVector<UA_Argument> listInputArguments;
    if (QOpcUaMethodTraits<M>::getNumArgs() > 0)
    {
        listInputArguments = QOpcUaMethodTraits<M>::getArgsUaArguments(m_qUaServer);
        p_inputArguments = listInputArguments.data();
    }
    // create output arguments
    UA_Argument * p_outputArgument = nullptr;
    UA_Argument outputArgument;
    if (!QOpcUaMethodTraits<M>::isRetUaArgumentVoid())
    {
        outputArgument = QOpcUaMethodTraits<M>::getRetUaArgument();
        p_outputArgument = &outputArgument;
    }
    // add method node
    QByteArray byteMethodName = strMethodName.toUtf8();
    UA_NodeId methNodeId = this->addMethodNodeInternal(
        byteMethodName,
        strNodeId,
        QOpcUaMethodTraits<M>::getNumArgs(),
        p_inputArguments,
        p_outputArgument
    );
    // store method with node id hash as key
    Q_ASSERT_X(!m_hashMethods.contains(methNodeId), "QUaBaseObject::addMethodInternal", "Method already exists, callback will be overwritten.");
    m_hashMethods[methNodeId] = 
        [this, methodCallback](const UA_Variant * input, UA_Variant * output) {
        // NOTE : save server ref because method might delete this instance
        // setMethodReturnStatusCode is just syntax sugar tu use within QUaBaseObject::
        QUaServer* srv = m_qUaServer;
        // reset status code
        srv->m_methodRetStatusCode = UA_STATUSCODE_GOOD;
        // call method
        if (QOpcUaMethodTraits<M>::isRetUaArgumentVoid())
        {
            QOpcUaMethodTraits<M>::execCallback(methodCallback, input);
        }
        else
        {
            *output = QOpcUaMethodTraits<M>::execCallback(methodCallback, input);
        }
        // copy return status code
        UA_StatusCode retStatusCode = srv->m_methodRetStatusCode;
        // reset status code
        srv->m_methodRetStatusCode = UA_STATUSCODE_GOOD;
        // return success status
        return retStatusCode;
    };
}

#endif // QUASERVER_H
