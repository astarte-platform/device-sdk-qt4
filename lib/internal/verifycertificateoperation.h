#ifndef VERIFYCERTIFICATEOPERATION_H
#define VERIFYCERTIFICATEOPERATION_H

#include "endpoint.h"
#include "crypto.h"

#include <QtCore/QUrl>

#include <QtNetwork/QSslConfiguration>

#include "utils/hemeraoperation.h"

class QFile;

namespace Astarte {

class HTTPEndpoint;

class VerifyCertificateOperation : public Hemera::Operation
{
    Q_OBJECT
    Q_DISABLE_COPY(VerifyCertificateOperation)

public:
    explicit VerifyCertificateOperation(QFile &certFile, HTTPEndpoint *parent);
    explicit VerifyCertificateOperation(const QByteArray &certificate, HTTPEndpoint *parent);
    explicit VerifyCertificateOperation(const QSslCertificate &certificate, HTTPEndpoint *parent);
    virtual ~VerifyCertificateOperation();

protected:
    void startImpl();

private Q_SLOTS:
    void onReplyFinished();
    void verify();

private:
    QByteArray m_certificate;
    HTTPEndpoint *m_endpoint;
};
}


#endif // VERIFYCERTIFICATEOPERATION_H
