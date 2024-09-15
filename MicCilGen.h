#ifndef MICCILGEN_H
#define MICCILGEN_H

/*
** Copyright (C) 2024 Rochus Keller (me@rochus-keller.ch)
**
** This file is part of the Micron language project.
**
**
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
*/

#include <Micron/MicMilLoader.h>

namespace Mic
{
    class CilGen : public MilRenderer
    {
    public:
        CilGen(MilLoader*l);
        ~CilGen();

        void beginModule( const QByteArray& moduleName, const QString& sourceFile, const QByteArrayList& );
        void endModule();

        void addImport(const QByteArray& path );

        void addVariable( const MilQuali& typeRef, QByteArray name, bool isPublic );
        void addConst(const MilQuali& typeRef, const QByteArray& name, const QVariant& val );
        void addProcedure(const MilProcedure& method );

        void beginType(const QByteArray& name, bool isPublic, quint8 typeKind);
        void endType();
        void addType( const QByteArray& name, bool isPublic, const MilQuali& baseType,
                      quint8 typeKind, quint32 len = 0);

        void addField( const QByteArray& fieldName,
                       const MilQuali& typeRef,
                       bool isPublic = true );
    private:
        class Imp;
        Imp* imp;
    };
}

#endif // MICCILGEN_H