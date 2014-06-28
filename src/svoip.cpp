#include "svoip.h"

SVoIP::SVoIP(QObject *parent):
    QObject(parent),
    mContactDB(NULL),
    mRsaKeyring(NULL),
    mContactListWindow(NULL),
    mPasswordWindow(NULL)
{
    QSettings settings("settings.ini", QSettings::IniFormat);
    QByteArray salt = QByteArray::fromBase64(settings.value("encryption/salt").toByteArray());
    QByteArray pwdHash = QByteArray::fromBase64(settings.value("encryption/password_hash").toByteArray());

    if(salt.isEmpty()){
        settings.setValue("encryption/salt", generateSalt());
    }
    //256 bits IV for GCM
    mFileKey.second = salt;

    if(pwdHash.isEmpty()){
        checkParameters();
    }else{
        mPasswordWindow = new PasswordWindow(pwdHash, salt);
        connect(mPasswordWindow, SIGNAL(validate(QByteArray)),
                this, SLOT(checkParameters(QByteArray)));
    }
}

void SVoIP::checkParameters(QByteArray key){
    QSettings settings("settings.ini", QSettings::IniFormat);
    mFileKey.first = key;
    mRsaKeyring = new RsaKeyring(&mFileKey);
    if(!mRsaKeyring->hasPrivateKey() ||
       settings.value("network/listen_port").isNull()
      ){
        displayFirstStartWizard();
    }else
        startProgram();
}

void SVoIP::displayFirstStartWizard(){


    m_wizard = new ConfWizard(mFileKey);


    connect(m_wizard,SIGNAL(accepted()),this,SLOT(startProgram()));
    connect(m_wizard,SIGNAL(rejected()),qApp,SLOT(quit()));

}

void SVoIP::startProgram(){
    disconnect(mRsaKeyring, SIGNAL(privateKeyGenerationFinished(QByteArray)), this, 0);
    mContactDB = new ContactDB(&mFileKey, this);
    mContactListWindow = new ContactListWindow(mContactDB, mRsaKeyring, &mFileKey);

    restartListener();
    connect(&mListener, SIGNAL(newConnection()),
            this, SLOT(onNewConnection()));
    connect(&mIpFilter, SIGNAL(accepted(QTcpSocket*)),
            this, SLOT(onIpAccepted(QTcpSocket*)));
    connect(mContactListWindow, SIGNAL(settingsUpdated()),
            this, SLOT(restartListener()));
    connect(mContactListWindow, SIGNAL(contactEvent(int,Contact::Event)),
            this, SLOT(onContactEvent(int,Contact::Event)));
    connect(mContactListWindow, SIGNAL(startApp(QList<Contact*>,AppType)),
            this,SLOT(startApp(QList<Contact*>,AppType)));

    //start a NetworkManager for each contact
    QList<Contact*> contactList = mContactDB->getAllContacts();
    foreach(Contact *contact, contactList){
        NetworkManager* networkManager = new NetworkManager(contact, mContactDB, mRsaKeyring, &mIpFilter, this);
        connectNetworkManagerSignals(networkManager);
        mNetworkManagerList.insert(contact->getId(), networkManager);
    }
}

void SVoIP::onIpAccepted(QTcpSocket *socket){
    //negative unique id for hanshaking network managers
    int id = -1;
    while(mNetworkManagerList.contains(id))
        id--;
    NetworkManager* networkManager = new NetworkManager(socket,mContactDB, mRsaKeyring, &mIpFilter, this);
    connectNetworkManagerSignals(networkManager);
    mNetworkManagerList.insert(id, networkManager);
}

void SVoIP::onNewConnection(){
    mIpFilter.filter(mListener.nextPendingConnection());
}

void SVoIP::onNetworkManagerDestroy(NetworkManager* networkManager){
        mNetworkManagerList.remove(mNetworkManagerList.key(networkManager));
}

void SVoIP::updateNetworkManagerId(int newId){
    NetworkManager *networkManager = dynamic_cast<NetworkManager*>(sender());
    if(networkManager){
        int oldId = mNetworkManagerList.key(networkManager, 0);
        if(oldId){
            mNetworkManagerList.remove(oldId);
            mNetworkManagerList.insert(newId, networkManager);
        }
    }
}

void SVoIP::updateContactStatus(int id, Contact::Status status){
    if(id)
        mContactListWindow->setContactStatusIcon(id, status);
}

void SVoIP::onContactEvent(int id, Contact::Event event){
    NetworkManager* networkManager;
    switch(event){
    case Contact::Added:
        networkManager = new NetworkManager(mContactDB->findById(id), mContactDB, mRsaKeyring, &mIpFilter, this);
        connectNetworkManagerSignals(networkManager);
        mNetworkManagerList.insert(id, networkManager);
        break;
    case Contact::Deleted:
        delete mNetworkManagerList.value(id);
        break;
    case Contact::Updated:
        mNetworkManagerList.value(id)->onContactEvent(event);
        break;
    }
}

void SVoIP::restartListener(){
    mListener.close();
    qint16 port = QSettings("settings.ini", QSettings::IniFormat).value("network/listen_port").toInt();
    mListener.listen(QHostAddress::Any, port);
}

void SVoIP::connectNetworkManagerSignals(NetworkManager *networkManager){
    connect(networkManager, SIGNAL(contactStatusChanged(int,Contact::Status)),
            this, SLOT(updateContactStatus(int, Contact::Status)));
    connect(networkManager, SIGNAL(destroyed(NetworkManager*)),
            this, SLOT(onNetworkManagerDestroy(NetworkManager*)));
    connect(networkManager, SIGNAL(newContactId(int)),
            this, SLOT(updateNetworkManagerId(int)));
    connect(networkManager,SIGNAL(startRootApp(Contact*)),
            this,SLOT(startRootApp(Contact*)));
}

QString SVoIP::generateSalt(){
    const uint blockSize = 32;
    byte randomBlock[blockSize];
    std::string encodedBlock;
    CryptoPP::AutoSeededRandomPool rng;
    rng.GenerateBlock(randomBlock,blockSize);
    CryptoPP::ArraySource(randomBlock,
                          blockSize,
                          true,
                          new CryptoPP::Base64Encoder(
                              new CryptoPP::StringSink(encodedBlock),
                              false));
    return QString::fromStdString(encodedBlock);
}

void SVoIP::startApp(QList<Contact*> contactList, AppType appType){
    //TODO: see if templated factory may works here...
    //TODO: retrieve the right app for the right contact group
    AbstractApp::AppUID appUID(appType);
    if(mAppList.contains(appUID) && appType != Root){
        mAppList.value(appUID)->show();
    }else{
        AbstractApp *app = NULL;
        if(appType == Root){
            app = new RootApp(contactList);
        }else if(appType == Messenger){
            app = new MessengerApp(contactList);
        }else{
            emit error(InvalidAppID);
        }
        if(app){
            foreach(AbstractApp::AppUID existingAppUID, mAppList.keys()){
                if(existingAppUID == appUID)
                    appUID.instanceID++;
            }
            foreach(Contact* contact, contactList)
                mNetworkManagerList.value(contact->getId())->registerApp(appUID, app);
            mAppList.insert(appUID, app);
        }
    }
}

void SVoIP::startRootApp(Contact *contact){
    QList<Contact*> contactList;
    contactList.append(contact);
    startApp(contactList, Root);
}

SVoIP::~SVoIP(){
    if(mContactListWindow) delete mContactListWindow;
    if(mContactDB) delete mContactDB;
    if(mPasswordWindow) delete mPasswordWindow;
    qDeleteAll(mAppList);
    qDeleteAll(QMap<int,NetworkManager*>(mNetworkManagerList)); //copy list to avoid networkManager self remove
}
