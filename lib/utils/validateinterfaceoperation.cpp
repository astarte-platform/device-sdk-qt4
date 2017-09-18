#include "validateinterfaceoperation.h"

#include <QtCore/QFile>

#include "rapidjson/document.h"
#include "rapidjson/schema.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;

ValidateInterfaceOperation::ValidateInterfaceOperation(const QString &path, QObject *parent)
    : m_path(path)
{
}

ValidateInterfaceOperation::~ValidateInterfaceOperation()
{
}

void ValidateInterfaceOperation::startImpl()
{
    QFile schemaFile(QString("%1/interface.json").arg(
                QLatin1String("/usr/share/astarte-sdk")));
    if (!schemaFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setFinishedWithError("Hemera::Literals::literal(Hemera::Literals::Errors::notFound())",
                             QString("Schema file %1 does not exist").arg(schemaFile.fileName()));
        return;
    }

    QFile interfaceFile(m_path);
    if (!interfaceFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setFinishedWithError("Hemera::Literals::literal(Hemera::Literals::Errors::notFound())",
                             QString("Interface file %1 does not exist").arg(interfaceFile.fileName()));
        return;
    }

    Document sd;
    if (sd.Parse(schemaFile.readAll().constData()).HasParseError()) {
        setFinishedWithError("wrong format", "could not parse schema!");
        return;
    }
    SchemaDocument schema(sd); // Compile a Document to SchemaDocument
    // sd is no longer needed here.

    Document d;
    if (d.Parse(interfaceFile.readAll().constData()).HasParseError()) {
        setFinishedWithError("wrong interface", "could not parse interface!");
        return;
    }
    SchemaValidator validator(schema);
    if (!d.Accept(validator)) {
        // Input JSON is invalid according to the schema
        // Output diagnostic information
        StringBuffer sb;
        validator.GetInvalidSchemaPointer().StringifyUriFragment(sb);
        QString errorString = QString("Invalid schema: %1\nInvalid keyword: %2\n").arg(sb.GetString()).arg(validator.GetInvalidSchemaKeyword());
        sb.Clear();
        validator.GetInvalidDocumentPointer().StringifyUriFragment(sb);
        errorString += QString("Invalid document: %1\n").arg(sb.GetString());

        setFinishedWithError("schema validation failed", errorString);
        return;
    }

    // Everything looks ok
    setFinished();
}
