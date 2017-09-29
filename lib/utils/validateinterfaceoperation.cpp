/*
 * Copyright (C) 2017 Ispirata Srl
 *
 * This file is part of Astarte.
 * Astarte is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * Astarte is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Astarte.  If not, see <http://www.gnu.org/licenses/>.
 */

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
