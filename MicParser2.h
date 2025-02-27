#ifndef __MIC_PARSER2__
#define __MIC_PARSER2__

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

#include <Micron/MicToken.h>
#include <Micron/MicAst.h>
#include <QList>
#include <QHash>
#include <QSet>

namespace Mic {
    class MilEmitter;
    class Evaluator;

    class Scanner2 {
	public:
		virtual Token next() = 0;
		virtual Token peek(int offset) = 0;
        virtual QString source() const = 0;
        virtual QByteArrayList path() const { return QByteArrayList(); } // prefix without module name
	};

    class Importer {
    public:
        virtual Declaration* loadModule( const Import& imp ) = 0;
        virtual QByteArray moduleSuffix( const MetaActualList& imp ) = 0;
        virtual QByteArray modulePath( const QByteArrayList& imp ) = 0;
    };

    class Parser2 {
	public:
        Parser2(AstModel* m, Scanner2* s, MilEmitter* out, Importer* = 0);
        ~Parser2();

        void RunParser(const MetaActualList& = MetaActualList());
        Declaration* takeModule(); // get module declaration and take ownership (otherwise deleted by parser)
		struct Error {
		    QString msg;
		    int row, col;
		    QString path;
		    Error( const QString& m, int r, int c, const QString& p):msg(m),row(r),col(c),path(p){}
		};
		QList<Error> errors;

        bool assigCompat(Type* lhs, Type* rhs) const;
    protected:
        Expression* number();
        Expression* integer();

        typedef QPair<QByteArray,QByteArray> Quali;
        Quali qualident();
        struct IdentDef {
            Token name;
            enum Visi { Private, ReadOnly, Public } visi;
            bool isValid() const { return name.d_type == Tok_ident; }
        };
        IdentDef identdef();
		void ConstDeclaration();
        Expression* ConstExpression(Type* hint);
		void TypeDeclaration();
        Type* type(bool deanonymize = true);
        Type* NamedType(Quali* = 0, bool allowUnresovedLocal = false);
        Type* ArrayType();
        void length(quint32& len);
        Type* RecordType();
        Type* ObjectType();
        bool inline_();
        void VariantPart();
        void FixedPart();
        void FieldList();
        typedef QList<IdentDef> IdentDefList;
        IdentDefList IdentList();
        Type* PointerType();
        Type* enumeration();
        DeclList constEnum();
		void VariableDeclaration();
        Expression* designator(bool lvalue);
        Expression* expression(Type* hint, bool lvalue = false);
        quint8 relation();
        Expression* SimpleExpression(Type* hint, bool lvalue = false);
        quint8 AddOperator();
        Expression* term(Type* hint, bool lvalue = false);
        quint8 MulOperator();
        Expression* literal();
        Expression* constructor(Type* hint);
        enum { FirstComponent, Named, Anonymous };
        Expression* component(Type* constrType, int& index);
        Expression* factor(Type* hint, bool lvalue = false);
        Expression* variableOrFunctionCall(bool lvalue = false);
        Expression* element();
		void statement();
        void assignmentOrProcedureCall();
		void StatementSequence();
		void gotoLabel();
		void GotoStatement();
		void IfStatement();
		void CaseStatement();
        typedef QSet<qint64> CaseLabels;
        void Case(Type* t, CaseLabels& ll);
        void CaseLabelList(Type* t, CaseLabels&);
        void LabelRange(Type* t,CaseLabels&);
        Value label(Type* t);
		void WhileStatement();
		void RepeatStatement();
		void ForStatement();
		void LoopStatement();
		void ExitStatement();
		void procedure();
        Type* ProcedureType();
		void ProcedureDeclaration();
        struct NameAndType{
            Token id;
            Type* t;
            NameAndType():t(0){}
        };
        NameAndType Receiver();
		void block();
		void DeclarationSequence();
		void ReturnStatement();
        Type* FormalParameters();
        Type* ReturnType();
		void FPSection();
        Type* FormalType();
		void module();
		void ImportList();
		void import();

        static bool isUnique(const MetaParamList&, const Declaration*);
        MetaParamList MetaParams();
        MetaParamList MetaSection(bool& isType);

    protected:
        void next();
        Token peek(int off);
        void invalid(const char* what);
        bool expect(int tt, bool pkw, const char* where);
        void error( const Token&, const QString& msg);
        void error( int row, int col, const QString& msg );
        void error( const RowCol&, const QString& msg );
        Declaration* findDecl(const Token& id );
        bool assigCompat(Type* lhs, Declaration* rhs) const;
        bool assigCompat(Type* lhs, const Expression* rhs) const;
        bool paramCompat(Declaration* lhs, const Expression* rhs) const;
        bool matchFormals(const QList<Declaration*>& a, const QList<Declaration*>& b) const;
        bool matchResultType(Type* lhs, Type* rhs) const;
        bool sameType(Type* lhs, Type* rhs) const;
        bool equalTypes(Type* lhs, Type* rhs) const;
        void ForwardDeclaration();
        Expression* maybeQualident();
        Declaration* resolveQualident(Quali* = 0, bool allowUnresovedLocal = false);
        static DeclList toList(Declaration*);
        Declaration* addDecl(const Token& id, quint8 visi, quint8 mode, bool* doublette = 0);
        Declaration* addDecl(const IdentDef& id, quint8 mode, bool* doublette = 0);
        void resolveDeferreds();
        Expression* toExpr(Declaration* d, const RowCol&);
        void emitType(Type*, const Quali& = Quali());
        Declaration* addHelper(Type*);
        Declaration* addTemp(Type*);
        typedef QList<QPair<Token,Value> > Args;
        void openArrayError(const Token&, Type*);
        void invalidTypeError(const Token&, Type*);
        void prepareParam(const DeclList& formals, const ExpList& actuals);
        void resolveAndCheckType(Declaration* d);
        Type* resolveAndCheckType(Type*, bool selfRefBroken);
        void checkArithOp(Expression*);
        void checkUnaryOp(Expression*);
        void checkRelOp(Expression*);
        void beginFinallyEnd(bool finally);

    private:
        AstModel* mdl;
        MilEmitter* out;
        Evaluator* ev;
        Importer* imp;
		Token cur;
		Token la;
        Scanner2* scanner;
        Declaration* thisMod, *thisDecl;
        QList<QPair<Type*,Token> > deferred;
        QList<RowCol> loopStack;
        typedef QList<RowCol> Depth;
        Depth blockDepth;
        struct Label {
            Depth depth;
            Token tok;
            bool used;
            Label(const Depth& d, const Token& t):depth(d),tok(t),used(false) {}
        };
        bool inFinally;
        quint8 langLevel;
        bool haveExceptions;

        typedef QHash<const char*,Label> Labels;
        Labels labels;
        typedef QList<QPair<Depth,Token> > Gotos;
        Gotos gotos;
        QSet<Type*> deferDeleteNamedType;
        MetaActualList metaActuals;
	};
}

#endif // include
