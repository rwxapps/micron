/*
* Copyright 2024 Rochus Keller <mailto:me@rochus-keller.ch>
*
* This file is part of the Micron language project.
*
* The following is the license that applies to this copy of the
* file. For a license to use the file under conditions
* other than those described here, please email to me@rochus-keller.ch.
*
* GNU General Public License Usage
* This file may be used under the terms of the GNU General Public
* License (GPL) versions 2.0 or 3.0 as published by the Free Software
* Foundation and appearing in the file LICENSE.GPL included in
* the packaging of this file. Please review the following information
* to ensure GNU General Public Licensing requirements will be met:
* http://www.fsf.org/licensing/licenses/info/GPLv2.html and
* http://www.gnu.org/copyleft/gpl.html.
*/

#include "MicEiGen.h"
#include "MicMilLoader.h"

#include <QCoreApplication>
#include <QFile>
#include <QStringList>
#include <QtDebug>
#include <QFileInfo>
#include <QDir>
#include <QElapsedTimer>
#include <MicPpLexer.h>
#include <MicParser2.h>
#include <MicMilEmitter.h>
#include <MilLexer.h>
#include <MilParser.h>
#include <MilToken.h>
#include <QBuffer>
#include <QCommandLineParser>

QStringList collectFiles( const QDir& dir, const QStringList& suffix )
{
    QStringList res;
    QStringList files = dir.entryList( QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name );

    foreach( const QString& f, files )
        res += collectFiles( QDir( dir.absoluteFilePath(f) ), suffix );

    files = dir.entryList( suffix, QDir::Files, QDir::Name );
    foreach( const QString& f, files )
    {
        res.append(dir.absoluteFilePath(f));
    }
    return res;
}

class Lex2 : public Mic::Scanner2
{
public:
    QString sourcePath;
    Mic::PpLexer lex;
    Mic::Token next()
    {
        return lex.nextToken();
    }
    Mic::Token peek(int offset)
    {
        return lex.peekToken(offset);
    }
    QString source() const { return sourcePath; }
};

static QByteArray getModuleName(const QString& file)
{
    Mic::Lexer lex;
    lex.setStream(file);
    Mic::Token t = lex.nextToken();
    while( t.isValid() && t.d_tokenType != Mic::Tok_MODULE )
        t = lex.nextToken();
    if( t.d_tokenType == Mic::Tok_MODULE )
    {
        t = lex.nextToken();
        if( t.d_tokenType == Mic::Tok_ident )
            return t.d_val;
    }
    return QByteArray();
}

#if 0
class Manager : public Mic::Importer {
public:
    typedef QHash<QString,Mic::Declaration*> Modules;

    Manager() {}
    ~Manager() {
        Modules::const_iterator i;
        for( i = modules.begin(); i != modules.end(); ++i )
            delete i.value();
    }

    Mic::MilLoader loader;

    QHash<QByteArray,QString> moduleNameToPath;
    Modules modules;

    Mic::Declaration* loadModule( const Mic::Import& imp )
    {
        // TODO: load and instantiate generic modules
        const QByteArray name = imp.path.join('.');
        const QString file = moduleNameToPath.value(name);
        if( file.isEmpty() )
        {
            qCritical() << "module" << name << "is not included in the files passed to the compiler";
            return 0;
        }else
            return compile(file);
    }

    Mic::Declaration* compile(const QString& file)
    {
        Modules::const_iterator i = modules.find(file);
        if( i != modules.end() )
            return i.value();

//#define _GEN_OUTPUT_
#ifdef _GEN_OUTPUT_
        QFileInfo info(file);
        QFile out(info.dir().absoluteFilePath(info.completeBaseName()+".cod"));
        if( !out.open(QIODevice::WriteOnly) )
        {
            qCritical() << "cannot open file for writing:" << out.fileName();
            return 0;
        }
        qDebug() << "**** generating" << out.fileName().mid(root.size()+1);
        //Mic::EiGen r(&out);
        Mic::IlAsmRenderer r(&out);
#else
        Mic::InMemRenderer r(&loader);
#endif
        Lex2 lex;
        lex.sourcePath = file; // to keep file name if invalid
        lex.lex.setStream(file);
        qDebug() << "**** parsing" << file.mid(root.size()+1);
        Mic::MilEmitter e(&r);
        Mic::AstModel mdl;
        Mic::Parser2 p(&mdl,&lex, &e, this);
        p.RunParser();
        Mic::Declaration* res = 0;
        if( !p.errors.isEmpty() )
        {
            foreach( const Mic::Parser2::Error& e, p.errors )
                qCritical() << e.path.mid(root.size()+1) << e.row << e.col << e.msg;
        }else
        {
            res = p.takeModule();
        }
        modules[file] = res;
        return res;
    }
};

static void compile(const QStringList& files)
{
    int ok = 0;
    QElapsedTimer timer;
    timer.start();
    Manager mgr;
    foreach( const QString& file, files )
    {
        const QByteArray name = getModuleName(file);
        mgr.moduleNameToPath[name] = file;
    }
    foreach( const QString& file, files )
    {
        Mic::Declaration* module = mgr.compile(file);
        if( module )
            ok++;
    }
    foreach( const Mic::MilModule& m, mgr.loader.getModules() )
    {
        QFile out;
        out.open(stdout, QIODevice::WriteOnly);
        Mic::IlAsmRenderer r(&out);
        m.render(&r);
        out.putChar('\n');
    }

    Mic::Expression::killArena();
    Mic::AstModel::cleanupGlobals();
    qDebug() << "#### finished with" << ok << "files ok of total" << files.size() << "files" << "in" << timer.elapsed() << " [ms]";
}
#else

struct ModuleSlot
{
    Mic::Import imp;
    QString file;
    Mic::Declaration* decl;
    ModuleSlot():decl(0) {}
    ModuleSlot( const Mic::Import& i, const QString& f, Mic::Declaration* d):imp(i),file(f),decl(d){}
};

static bool operator==(const Mic::Import& lhs, const Mic::Import& rhs)
{
    if( lhs.path != rhs.path )
        return false;
    if( lhs.metaActuals.size() != rhs.metaActuals.size() )
        return false;
    for( int i = 0; i < lhs.metaActuals.size(); i++ )
    {
        if( lhs.metaActuals[i].mode != rhs.metaActuals[i].mode )
            return false;
        if( lhs.metaActuals[i].type != rhs.metaActuals[i].type )
            return false;
        if( lhs.metaActuals[i].val != rhs.metaActuals[i].val )
            return false;
   }
    return true;
}

class Manager : public Mic::Importer {
public:
    typedef QList<ModuleSlot> Modules;
    Modules modules;
    QList<QDir> searchPath;
    QString rootPath;

    Manager() {}
    ~Manager() {
        Modules::const_iterator i;
        for( i = modules.begin(); i != modules.end(); ++i )
            delete (*i).decl;
    }

    Mic::MilLoader loader;

    ModuleSlot* find(const Mic::Import& imp)
    {
        for(int i = 0; i < modules.size(); i++ )
        {
            if( modules[i].imp == imp )
                return &modules[i];
        }
        return 0;
    }

    QByteArray moduleSuffix( const Mic::Import& imp )
    {
        return "$" + QByteArray::number(modules.size());
    }

    Mic::Declaration* loadModule( const Mic::Import& imp )
    {
        ModuleSlot* ms = find(imp);
        if( ms != 0 )
            return ms->decl;

        QString file = toFile(imp);
        if( file.isEmpty() )
        {
            qCritical() <<  "cannot find source file of module" << imp.path.join('.');
            modules.append(ModuleSlot(imp,QString(),0));
            return 0;
        }

        // immediately add it so that circular module refs lead to an error
        modules.append(ModuleSlot(imp,file,0));
        ms = &modules.back();

//#define _GEN_OUTPUT_
#ifdef _GEN_OUTPUT_
        QFileInfo info(file);
        QFile out(info.dir().absoluteFilePath(info.completeBaseName()+".cod"));
        if( !out.open(QIODevice::WriteOnly) )
        {
            qCritical() << "cannot open file for writing:" << out.fileName();
            return 0;
        }
        qDebug() << "**** generating" << out.fileName().mid(root.size()+1);
        //Mic::EiGen r(&out);
        Mic::IlAsmRenderer r(&out);
#else
        Mic::InMemRenderer r(&loader);
#endif
        Lex2 lex;
        lex.sourcePath = file; // to keep file name if invalid
        lex.lex.setStream(file);
        qDebug() << "**** parsing" << QFileInfo(file).fileName();
        Mic::MilEmitter e(&r);
        Mic::AstModel mdl;
        Mic::Parser2 p(&mdl,&lex, &e, this);
        p.RunParser(imp.metaActuals);
        Mic::Declaration* res = 0;
        if( !p.errors.isEmpty() )
        {
            foreach( const Mic::Parser2::Error& e, p.errors )
                qCritical() << QFileInfo(e.path).fileName() << e.row << e.col << e.msg;
        }else
        {
            res = p.takeModule();
        }
        // TODO: uniquely extend the name of generic module instantiations

        ms->decl = res;
        return res;
    }

    QString toFile(const Mic::Import& imp)
    {
        const QString path = imp.path.join('/') + ".mic";
        foreach( const QDir& dir, searchPath )
        {
            const QString tmp = dir.absoluteFilePath(path);
            if( QFile::exists(tmp) )
                return tmp;
        }
        if( !modules.isEmpty() )
        {
            // if the file is not in the search path, look in the directory of the caller assuming
            // that the required module path is relative to the including moduled
            QFileInfo info( modules.back().file );
            const QString tmp = info.absoluteDir().absoluteFilePath(path);
            if( QFile::exists(tmp) )
                return tmp;
        }
        return QString();
    }
};
#endif

static void compile(const QStringList& files, const QStringList& searchPaths)
{
    int ok = 0;
    int all = 0;
    QElapsedTimer timer;
    timer.start();
    foreach( const QString& file, files )
    {
        Manager mgr;
        QFileInfo info(file);
        mgr.rootPath = info.absolutePath();
        mgr.searchPath.append(info.absoluteDir());
        for( int i = 0; i < searchPaths.size(); i++ )
        {
            const QString path = searchPaths[i];
            mgr.searchPath.append(path);
        }

        Mic::Import imp;
        imp.path.append(info.baseName().toUtf8());
        mgr.loadModule(imp);
#if 1
        foreach( const Mic::MilModule& m, mgr.loader.getModules() )
        {
            QFile out;
            out.open(stdout, QIODevice::WriteOnly);
            Mic::IlAsmRenderer r(&out);
            m.render(&r);
            out.putChar('\n');
        }
#endif
        all += mgr.modules.size();
        foreach( const ModuleSlot& m, mgr.modules )
            ok += m.decl ? 1 : 0;
    }
    Mic::Expression::killArena();
    Mic::AstModel::cleanupGlobals();
    qDebug() << "#### finished with" << ok << "files ok of total" << all << "files" << "in" << timer.elapsed() << " [ms]";
}


int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QCommandLineParser cp;
    cp.setApplicationDescription("Micron compiler");
    cp.addHelpOption();
    cp.addVersionOption();
    cp.addPositionalArgument("main", "the main module of the application");
    QCommandLineOption sp("I", "add a path where to look for modules", "path");
    cp.addOption(sp);

    cp.process(a);
    const QStringList args = cp.positionalArguments();
    if( args.isEmpty() )
        return -1;
    const QStringList searchPaths = cp.values(sp);

    compile(QStringList() << args[0], searchPaths);

    return 0;
}
