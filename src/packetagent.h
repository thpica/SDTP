#ifndef PACKETAGENT_H
#define PACKETAGENT_H

#include <QString>
#include <QObject>
#include <QDebug>
#include <QPair>
#include <QMap>

#include "abstractapp.h"






class PacketAgent: public QObject
{

    Q_OBJECT

public:
    enum APPID{root,msgr,voip,vids,data}; // 0-10 are reserved IDs.

    PacketAgent(QPair<QByteArray,QByteArray> key);// key = key + IV
    PacketAgent();
    ~PacketAgent();

    void logApp(AbstractApp* app);

public slots :
    void incomingdata(QByteArray data);//link

signals:
    void senddata(QByteArray data);//link


private:

    void encrypt();
    void decrypt();
    void addheader();

    void addHash();
    void makeHash();

    void getHash();
    void parseHeader();
    bool verifyHash();

    QByteArray mContent;
    QByteArray mData;
    QByteArray mHash;
    QByteArray mKey;
    QByteArray mIV;

    QList<AbstractApp*> mAppList;
    QPair<int,int> appIdPair;
    QMap<QPair<int,int>,AbstractApp*> mAppMap;




};

#endif // PACKET_H
