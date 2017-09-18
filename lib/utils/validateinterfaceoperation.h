#ifndef VALIDATEINTERFACEOPERATION_H
#define VALIDATEINTERFACEOPERATION_H

#include "hemeraoperation.h"

class ValidateInterfaceOperation : public Hemera::Operation
{
    Q_OBJECT
    Q_DISABLE_COPY(ValidateInterfaceOperation)

public:
    explicit ValidateInterfaceOperation(const QString &interface, QObject *parent = 0);
    virtual ~ValidateInterfaceOperation();

protected:
    virtual void startImpl();

private:
    QString m_path;
};


#endif // VALIDATEINTERFACEOPERATION_H
