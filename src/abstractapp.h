#ifndef ABSTRACTAPP_H
#define ABSTRACTAPP_H

#include <QWidget>

#include "typesenums.h"
#include "contact.h"
#include "contactdb.h"

class AbstractApp : public QWidget
{
    Q_OBJECT
public:
    enum Error{
        IncompatibleVersion //TODO: add more generic errors
    };

    struct AppUID{
        AppType type;
        quint16 instanceID;
        AppUID();
        AppUID(AppType typeId, quint16 instanceId = 0);
        bool operator<(const AppUID &second) const;
    };

    AbstractApp(ContactDB *contactDB, QWidget *parent=0);
    AbstractApp(QList<Contact*> contactList, ContactDB *contactDB, QWidget *parent=0);
    virtual ~AbstractApp();

    inline QList<Contact*> getContactList(){ return m_ContactList; }

public slots :
    virtual void readIncommingData(QByteArray &data) = 0;

signals :
    void sendData(int contactID, LinkType linkType, QByteArray &data);
    void requestStartApp(AppType type, int contactID);
    void error(AbstractApp::Error);

protected:
    QList<Contact*> m_ContactList;
    ContactDB *m_ContactDB;
};

QDataStream &operator<<(QDataStream &out, const AbstractApp::AppUID& appUID);
QDataStream &operator>>(QDataStream &in, AbstractApp::AppUID& appUID);

#endif // ABSTRACTAPP_H
