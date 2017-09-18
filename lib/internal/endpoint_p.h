#ifndef ENDPOINT_P_H
#define ENDPOINT_P_H

#include "endpoint.h"

namespace Astarte {

class EndpointPrivate {
public:
    EndpointPrivate(Endpoint *q) : q_ptr(q) {}

    Q_DECLARE_PUBLIC(Endpoint)
    Endpoint * const q_ptr;

    QUrl endpoint;
};

}


#endif // ENDPOINT_P_H
