#ifndef HEMERA_OPERATION_P_H
#define HEMERA_OPERATION_P_H

#include "hemeraoperation.h"
#include <QtCore/QDebug>

namespace Hemera {

class OperationPrivate
{
public:
    enum PrivateExecutionOption {
        NoOptions = 0,
        SetStartedExplicitly = 1
    };
    Q_DECLARE_FLAGS(PrivateExecutionOptions, PrivateExecutionOption)
    Q_FLAGS(PrivateExecutionOptions)

    OperationPrivate(Operation *q, PrivateExecutionOptions pO = NoOptions) : q_ptr(q), privateOptions(pO), started(false), finished(false) {}

    void genericInit(Operation::ExecutionOptions options);

    Q_DECLARE_PUBLIC(Operation)
    Operation * const q_ptr;

    QString errorName;
    QString errorMessage;
    Operation::ExecutionOptions options;
    PrivateExecutionOptions privateOptions;
    bool started : 1;
    bool finished : 1;

    // Private slots
    void emitFinished();
    void setStarted();
};

}

#endif
