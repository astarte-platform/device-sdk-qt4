#include "endpoint.h"
#include "endpoint_p.h"

#include "crypto.h"

#include <QtCore/QDir>
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QSettings>
#include <QtCore/QTimer>

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>

namespace Astarte {

Endpoint::Endpoint(EndpointPrivate &dd, const QUrl &endpoint, QObject* parent)
    : AsyncInitObject(parent)
    , d_h_ptr(&dd)
{
    Q_D(Endpoint);
    d->endpoint = endpoint;
}

Endpoint::~Endpoint()
{
    delete d_h_ptr;
}

QUrl Endpoint::endpoint() const
{
    Q_D(const Endpoint);
    return d->endpoint;
}

QString Endpoint::endpointVersion() const
{
    return QString();
}

}
