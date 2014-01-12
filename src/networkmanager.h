#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H




#include <QTcpSocket>
#include <QHostAddress>
#include <QObject>
#include <QDebug>
#include "voip.h"
#include "message.h"
#include "messengerwindow.h"
#include "contact.h"
#include "cryptopp/aes.h"
#include "cryptopp/modes.h"
#include "starter.h"

using namespace CryptoPP;

class NetworkManager : public QObject
{

    Q_OBJECT

public:
    NetworkManager(QTcpSocket *socket,  QObject *parent=0);
    NetworkManager(QString name,  QObject *parent=0);
    ~NetworkManager();


public slots :
    void readIncomingData();
    void sendData(QByteArray);
    void error(QAbstractSocket::SocketError);
    void voipCall();
    void onConnect();


private :
    QTcpSocket *m_Socket;
    VoIP *voip;
    MessengerWindow *m_MessengerWindow;
    QByteArray mAesKey;
    QByteArray mAesIv;
    CFB_Mode<AES>::Encryption mCfbAesEnc;
    CFB_Mode<AES>::Decryption mCfbAesDec;




};

#endif // NETWORKMANAGER_H