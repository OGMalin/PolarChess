#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QString>
#include "Statistics.h"
#include "Database.h"
#include "Computer.h"
#include "Openings.h"

class DatabaseDialog : public QDialog
{
	Q_OBJECT

public slots:
	void openTheoryDB();
	void newTheoryDB();
	void closeTheoryDB();
	void importPGNTheory();
	void importBookTheory();
	void exportPGNTheory();
	void cleanupTheoryDB();

	void openWhiteDB();
	void newWhiteDB();
	void closeWhiteDB();
	void importPGNWhite();
	void importBookWhite();
	void exportPGNWhite();
	void cleanupWhiteDB();

	void openBlackDB();
	void newBlackDB();
	void closeBlackDB();
	void importPGNBlack();
	void importBookBlack();
	void exportPGNBlack();
	void cleanupBlackDB();

	void openComputerDB();
	void newComputerDB();
	void closeComputerDB();

	void openStatisticDB();
	void newStatisticDB();
	void closeStatisticDB();
	void importStatisticsDB();
	void compactStatisticsDB();

	void openOpeningsDB();
	void newOpeningsDB();
	void closeOpeningsDB();
	void importOpeningsDB();
	void exportOpeningsDB();

public:
	DatabaseDialog(QWidget *parent, Database* theory, Database* white, Database* black, /* Training* ,*/ Computer*, Statistics*, Openings*);

private:
	QLineEdit* theoryFile;
	QLineEdit* whiteFile;
	QLineEdit* blackFile;
	QLineEdit* computerFile;
	QLineEdit* statisticFile;
	QLineEdit* openingsFile;
	Database* theoryDB;
	Database* whiteDB;
	Database* blackDB;
	Computer* computerDB;
	Statistics* statDB;
	Openings* openingsDB;
};

