#ifndef HTTPENDPOINT_P_H
#define HTTPENDPOINT_P_H

#include <QtCore/QObject>
#include <QtNetwork/QNetworkAccessManager>

#include "httpendpoint.h"
#include "endpoint_p.h"

namespace Astarte {

class HTTPEndpointPrivate : public EndpointPrivate {

public:
    HTTPEndpointPrivate(HTTPEndpoint *q) : EndpointPrivate(q) {}

    Q_DECLARE_PUBLIC(HTTPEndpoint)

    QString endpointName;
    QString endpointVersion;
    QString configurationFile;
    QString persistencyDir;
    QUrl mqttBroker;
    QNetworkAccessManager *nam;
    QByteArray hardwareId;

    QByteArray agentKey;
    QString brokerCa;
    bool ignoreSslErrors;

    QSslConfiguration sslConfiguration;

    void connectToEndpoint();
    void onConnectionEstablished();
    void processCryptoStatus();
};


class PairOperation : public Hemera::Operation
{
    Q_OBJECT
    Q_DISABLE_COPY(PairOperation)

public:
    explicit PairOperation(HTTPEndpoint *parent);
    virtual ~PairOperation();

protected:
    virtual void startImpl();

private Q_SLOTS:
    void onGenerationFinished(Hemera::Operation*);
    void initiatePairing();
    void onDeviceIDPayloadFinished();
    void performFakeAgentPairing();
    void performPairing();
    void onPairingReply();

private:
    HTTPEndpoint *m_endpoint;
};

}

#endif // HTTPENDPOINT_P_H
