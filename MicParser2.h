#ifndef __MIC_PARSER2__
#define __MIC_PARSER2__
// This file was automatically generated by EbnfStudio; don't modify it!

#include <Micron/MicToken.h>
#include <QList>

namespace Mic {

	class Scanner2 {
	public:
		virtual Token next() = 0;
		virtual Token peek(int offset) = 0;
	};

	class Parser2 {
	public:
        Parser2(Scanner2* s):scanner(s) {}
		void RunParser();
		struct Error {
		    QString msg;
		    int row, col;
		    QString path;
		    Error( const QString& m, int r, int c, const QString& p):msg(m),row(r),col(c),path(p){}
		};
		QList<Error> errors;
	protected:
		void Micron();
		void number();
		void qualident();
		void identdef();
		void ConstDeclaration();
		void ConstExpression();
		void TypeDeclaration();
		void type();
		void NamedType();
		void ArrayType();
		void length();
		void RecordType();
		void inline_();
		void VariantPart();
		void FixedPart();
		void FieldList();
		void IdentList();
		void PointerType();
		void enumeration();
		void constEnum();
		void symbolEnum();
		void VariableDeclaration();
		void designator();
		void selector();
		void ExpList();
		void expression();
		void relation();
		void SimpleExpression();
		void AddOperator();
		void term();
		void MulOperator();
		void literal();
		void constructor();
		void component();
		void factor();
		void variableOrFunctionCall();
		void set();
		void element();
		void statement();
		void assignmentOrProcedureCall();
		void StatementSequence();
		void gotoLabel();
		void GotoStatement();
		void IfStatement();
		void ElsifStatement();
		void ElseStatement();
		void CaseStatement();
		void Case();
		void CaseLabelList();
		void LabelRange();
		void label();
		void WhileStatement();
		void ElsifStatement2();
		void RepeatStatement();
		void ForStatement();
		void LoopStatement();
		void ExitStatement();
		void procedure();
		void ProcedureType();
		void ProcedureDeclaration();
		void ProcedureHeading();
		void block();
		void ProcedureBody();
		void DeclarationSequence();
		void ReturnStatement();
		void FormalParameters();
		void ReturnType();
		void FPSection();
		void FormalType();
		void module();
		void ImportList();
		void import();
		void MetaActuals();
		void MetaParams();
		void MetaSection();
	protected:
		Token cur;
		Token la;
		Scanner2* scanner;
		void next();
		Token peek(int off);
		void invalid(const char* what);
		bool expect(int tt, bool pkw, const char* where);
	};
}
#endif // include