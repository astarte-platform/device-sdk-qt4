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

#include <QtCore/QCoreApplication>
#include <QtCore/QStringList>
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QObject>
#include <QtCore/QTextStream>

#include "utils/validateinterfaceoperation.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    app.setApplicationName(QObject::tr("Astarte interface validator"));
    app.setOrganizationDomain(QLatin1String("com.ispirata.Hemera"));
    app.setOrganizationName(QLatin1String("Ispirata"));

    QStringList arguments = app.arguments();
    if (arguments.size() < 2 || arguments.size() > 3) {
        qWarning() << "Usage: astarte-validate-interface interface [-f]";
        return 1;
    }
    arguments.removeAt(0);
    bool force = false;
    if (arguments.contains("-f")) {
        force = true;
        arguments.removeOne("-f");
    }

    if (arguments.size() != 1) {
        qWarning() << "Usage: astarte-validate-interface interface [-f]";
        return 1;
    }

    QString interface = arguments.first();

    if (interface.isEmpty()) {
        QTextStream(stderr) << QObject::tr("You must supply an interface file to validate\n");
        return -1;
    }

    ValidateInterfaceOperation *op = new ValidateInterfaceOperation(interface);
    if (!op->synchronize()) {
        QTextStream(stderr) << QObject::tr("Validation failed:\n%1\n")
                               .arg(op->errorMessage());
        return 1;
    } else {
        QTextStream(stdout) << QObject::tr("Valid interface\n");
        return 0;
    }
}
