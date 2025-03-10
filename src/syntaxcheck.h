#ifndef SYNTAXCHECK_H
#define SYNTAXCHECK_H

#include "mostQtHeaders.h"
#include "smallUsefulFunctions.h"
#include "latexparser/latexparser.h"
#include "qdocumentline_p.h"
#include <QThread>
#include <QSemaphore>
#include <QMutex>
#include <QQueue>

class LatexDocument;
/*!
 * \brief store information on open environments
 */
class Environment
{
public:
    Environment(): id(-1), excessCol(0), dlh(nullptr), ticket(0), level(0) {} ///< constructor

	QString name; ///< name of environment, partially an alias is used, e.g. math instead of '$'
    QString origName; ///< original name of environment if alias is used, otherwise empty
	int id; ///< mostly unused, contains the number of columns for tabular-environments
	int excessCol; ///< number of unused tabular-columns if columns are strechted over several text lines
	QDocumentLineHandle *dlh; ///< linehandle of starting line
	int ticket;
    int level; ///< command level (see tokens) in order to handle nested commands like \shortstack

	bool operator ==(const Environment &env) const
	{
        return (name == env.name) && (id == env.id) && (excessCol == env.excessCol) && (origName == env.origName) && (level == env.level);
	}
	bool operator !=(const Environment &env) const
	{
        return (name != env.name) || (id != env.id) || (excessCol != env.excessCol) || (origName != env.origName) || (level != env.level);
	}
};

typedef QStack<Environment> StackEnvironment;

Q_DECLARE_METATYPE(StackEnvironment)

class SyntaxCheck : public SafeThread
{
	Q_OBJECT

public:
    /*!
     * \brief type of error
     */
	enum ErrorType {
		ERR_none, ///< no error
		ERR_unrecognizedEnvironment, ///< environment unknown
		ERR_unrecognizedCommand, ///< command unknown
		ERR_unrecognizedMathCommand, ///< unknown command for math environment
		ERR_unrecognizedTabularCommand, ///< unknown command for tabular environment
		ERR_TabularCommandOutsideTab, ///< tabular command outside tabular (e.g. \hline)
		ERR_MathCommandOutsideMath, ///< math command outside of math env
		ERR_TabbingCommandOutside, ///< tabbing command outside of tabbing env
		ERR_tooManyCols, ///< tabular has more columns in line than in definition
		ERR_tooLittleCols, ///< tabular has fewer columns in line than in definition
		ERR_missingEndOfLine, ///< unused
		ERR_closingUnopendEnv, ///< end{env} without corrresponding begin{env}
		ERR_EnvNotClosed, ///< end{env} missing
		ERR_unrecognizedKey, ///< in key/value argument, an unknown key is used
		ERR_unrecognizedKeyValues, ///< in key/value argument, an unknown value is used for a key
		ERR_commandOutsideEnv, ///< command used outside of designated environment (similar math command outside math)
		ERR_MAX  // always last
	};
    /*!
     * \brief info which is queued for syntaxchecking
     */
	struct SyntaxLine {
		StackEnvironment prevEnv; ///< environmentstack at start of line
		TokenStack stack; ///< tokenstack at start of line (open arguments)
		int ticket; ///< ticket number
		bool clearOverlay; ///< clear syntax overlay, sometimes not necessary as it was done somewhere else
		QDocumentLineHandle *dlh; ///< linehandle
	};

    /*!
     * \brief structure to describe an syntax error
     */
	struct Error {
		QPair<int, int> range; ///<  start,stop of error marker
		ErrorType type; ///< type of error
	};

	typedef QList<Error > Ranges;

    explicit SyntaxCheck(QObject *parent = nullptr);

	void putLine(QDocumentLineHandle *dlh, StackEnvironment previous, TokenStack stack, bool clearOverlay = false);
	void stop();
	void setErrFormat(int errFormat);
	QString getErrorAt(QDocumentLineHandle *dlh, int pos, StackEnvironment previous, TokenStack stack);
	int verbatimFormat; ///< format number for verbatim text (LaTeX)
	void waitForQueueProcess();
	static int containsEnv(const LatexParser &parser, const QString &name, const StackEnvironment &envs, const int id = -1);
	int topEnv(const QString &name, const StackEnvironment &envs, const int id = -1);
	bool checkCommand(const QString &cmd, const StackEnvironment &envs);
	static bool equalEnvStack(StackEnvironment env1, StackEnvironment env2);
	bool queuedLines();
	void setLtxCommands(const LatexParser &cmds);
	void markUnclosedEnv(Environment env);

signals:
	void checkNextLine(QDocumentLineHandle *dlh, bool clearOverlay, int ticket); ///< enqueue next line for syntax checking as context has changed

protected:
	void run();
	void checkLine(const QString &line, Ranges &newRanges, StackEnvironment &activeEnv, QDocumentLineHandle *dlh, int ticket);
	void checkLine(const QString &line, Ranges &newRanges, StackEnvironment &activeEnv, QDocumentLineHandle *dlh, TokenList tl, TokenStack stack, int ticket);

private:
	QQueue<SyntaxLine> mLines;
	QSemaphore mLinesAvailable;
	QMutex mLinesLock;
	bool stopped;
	int syntaxErrorFormat;
	LatexParser *ltxCommands;

	LatexParser newLtxCommands;
	bool newLtxCommandsAvailable;
	QMutex mLtxCommandLock;
	bool stackContainsDefinition(const TokenStack &stack) const;
};

#endif // SYNTAXCHECK_H
