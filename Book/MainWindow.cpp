#include "MainWindow.h"
#include "MoveWindow.h"
#include "OpeningWindow.h"
#include "CommentWindow.h"
#include "PathWindow.h"
#include "EngineWindow.h"
#include "AboutDialog.h"
#include "../Common/BoardWindow.h"
#include "Path.h"
#include <QMenu>
#include <QToolBar>
#include <QAction>
#include <QMenuBar>
#include <QSettings>
#include <QApplication>
#include <QRect>
#include <QDesktopWidget>
#include <QCloseEvent>
#include <QSplitter>
#include <QFileDialog>
#include <QStandardPaths>
#include <QMessageBox>
#include <QSplitter>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
{
	QSettings settings;
	restoreGeometry(settings.value("mainWindowGeometry").toByteArray());

	dataPath = QStandardPaths::locate(QStandardPaths::DocumentsLocation, QCoreApplication::organizationName(), QStandardPaths::LocateDirectory);
	dataPath += "/" + QCoreApplication::applicationName();
	writeTheory = false;
	writeRep = false;
	createMenu();
	readSettings();

	statusBar();

	hSplitter = new QSplitter(Qt::Horizontal);
	v1Splitter = new QSplitter(Qt::Vertical);
	v2Splitter = new QSplitter(Qt::Vertical);

	boardwindow = new BoardWindow;
	openingwindow = new OpeningWindow;
	movewindow = new MoveWindow;
	commentwindow = new CommentWindow;
	pathwindow = new PathWindow;
	enginewindow = new EngineWindow;

	v1Splitter->addWidget(openingwindow);
	v1Splitter->addWidget(boardwindow);
	v1Splitter->addWidget(enginewindow);
	v2Splitter->addWidget(pathwindow);
	v2Splitter->addWidget(movewindow);
	v2Splitter->addWidget(commentwindow);
	hSplitter->addWidget(v1Splitter);
	hSplitter->addWidget(v2Splitter);

	setCentralWidget(hSplitter);

	theoryBase = new Database(QString("theory"));
	repBase = new Database(QString("rep"));
	currentPath = new Path();

	connect(boardwindow, SIGNAL(moveEntered(ChessMove&)), this, SLOT(moveEntered(ChessMove&)));
	connect(pathwindow, SIGNAL(pathSelected(int)), this, SLOT(pathSelected(int)));

//	boardwindow->setVisible(false);
//	enginewindow->setVisible(false);
//	openingwindow->setVisible(false);
//	movewindow->setVisible(false);
//	commentwindow->setVisible(false);

	restoreState(settings.value("mainWindowState").toByteArray());
}

MainWindow::~MainWindow()
{
	delete currentPath;
}

void MainWindow::createMenu()
{
	// File menu
	fileMenu = menuBar()->addMenu("File");
	fileOpenMenu = fileMenu->addMenu("Open book");
	openTheoryAct = fileOpenMenu->addAction("Open theory book", this, &MainWindow::fileOpenTheory);
	openRepAct = fileOpenMenu->addAction("Open repertoire book", this, &MainWindow::fileOpenRep);
	fileNewMenu = fileMenu->addMenu("New book");
	newTheoryAct = fileNewMenu->addAction("New theory book", this, &MainWindow::fileNewTheory);
	newRepAct = fileNewMenu->addAction("New repertoire book", this, &MainWindow::fileNewRep);
	fileCloseMenu = fileMenu->addMenu("Close book");
	closeTheoryAct = fileCloseMenu->addAction("Close theory book", this, &MainWindow::fileCloseTheory);
	closeRepAct = fileCloseMenu->addAction("Close repertoire book", this, &MainWindow::fileCloseRep);
	fileMenu->addSeparator();
	exitAct = fileMenu->addAction("Exit", this, &QWidget::close);

	bookMenu = menuBar()->addMenu("Book");
	bookWriteMenu = bookMenu->addMenu("Write enable");
	writeTheoryAct = bookWriteMenu->addAction("Write to theory book", this, &MainWindow::bookWriteTheory);
	writeRepAct = bookWriteMenu->addAction("Write to repertoire book", this, &MainWindow::bookWriteRep);
	writeTheoryAct->setCheckable(true);
	writeRepAct->setCheckable(true);

	// Setting up the toolbar
	toolbar = addToolBar("Toolbar");
	toolbar->addAction(writeTheoryAct);
	toolbar->addAction(writeRepAct);

	// No database opened as default
	closeTheoryAct->setDisabled(true);
	closeRepAct->setDisabled(true);
	writeTheoryAct->setDisabled(true);
	writeRepAct->setDisabled(true);
}

void MainWindow::writeSettings()
{/*
	QSettings settings;
	settings.setValue("maingeometry", saveGeometry());
	*/
}

void MainWindow::readSettings()
{
	/*
	QSettings settings;
	QByteArray maingeometry = settings.value("maingeometry", QByteArray()).toByteArray();
	if (maingeometry.isEmpty())
	{
		const QRect availableGeometry = QApplication::desktop()->availableGeometry(this);
		resize(availableGeometry.width() / 3, availableGeometry.height() / 2);
		move((availableGeometry.width() - width()) / 2,
			(availableGeometry.height() - height()) / 2);
	}
	else
	{
		restoreGeometry(maingeometry);
	}
	QString version = settings.value("Version", QString()).toString();
	if (version.isEmpty())
		settings.setValue("Version", QCoreApplication::applicationVersion());
		*/
}

void MainWindow::closeEvent(QCloseEvent* event)
{
	QSettings settings;
	settings.setValue("mainWindowGeometry", saveGeometry());
	settings.setValue("mainWindowState", saveState());
//	writeSettings();
//	event->accept();
}

void MainWindow::fileOpenTheory()
{
	QMessageBox msgbox;
	QString path = QFileDialog::getOpenFileName(this, "Open book", dataPath, "Book files (*.book)");
	if (!path.isEmpty())
	{
		if (!theoryBase->open(path))
		{
			msgbox.setText("Can't open book");
			msgbox.exec();
			return;
		}
		bdeTheory = theoryBase->find(currentPath->getPosition());
		movewindow->update(bdeTheory, bdeRep);
		openingwindow->update(bdeTheory, bdeRep);
		commentwindow->update(bdeTheory.comment, bdeRep.comment);
/*
		movewindow->setVisible(true);
		commentwindow->setVisible(true);
		*/

		closeTheoryAct->setDisabled(false);
		writeTheoryAct->setDisabled(false);

		if (writeTheory)
		{
			commentwindow->setWriteTheory(false);
			writeTheory = false;
			writeTheoryAct->setChecked(false);
		}
	}
}

void MainWindow::fileOpenRep()
{
	QMessageBox msgbox;
	QString path = QFileDialog::getOpenFileName(this, "Open book", dataPath, "Book files (*.book)");
	if (!path.isEmpty())
	{
		if (!repBase->open(path))
		{
			msgbox.setText("Can't open book");
			msgbox.exec();
			return;
		}
		bdeRep = repBase->find(currentPath->getPosition());
		movewindow->update(bdeTheory, bdeRep);
		openingwindow->update(bdeTheory, bdeRep);
		commentwindow->update(bdeTheory.comment, bdeRep.comment);
		/*
		movewindow->setVisible(true);
		commentwindow->setVisible(true);
		*/

		closeRepAct->setDisabled(false);
		writeRepAct->setDisabled(false);

		if (writeRep)
		{
			commentwindow->setWriteRep(false);
			writeRep = false;
			writeRepAct->setChecked(false);
		}
	}
}

void MainWindow::fileNewTheory()
{
	QString path = QFileDialog::getSaveFileName(this, "Open book", dataPath, "Book files (*.book)");
	if (!path.isEmpty())
	{
		QFile file(path);
		if (file.exists())
		{
			QMessageBox msgbox;
			msgbox.setText("The book allready exist. Do you want to delete it?");
			msgbox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
			if (msgbox.exec() != QMessageBox::Ok)
				return;
			file.remove();
		}
		theoryBase->create(path);
		bdeTheory.clear();
		bdeTheory.board = currentPath->getStartPosition();
		bdeTheory.eco = "A00";
		theoryBase->add(bdeTheory);

		bdeTheory= theoryBase->find(currentPath->getPosition());
		
		movewindow->update(bdeTheory, bdeRep);
		openingwindow->update(bdeTheory, bdeRep);
		commentwindow->update(bdeTheory.comment, bdeRep.comment);
		/*
		movewindow->setVisible(true);
		commentwindow->setVisible(true);

		closeTheoryAct->setDisabled(false);
		writeTheoryAct->setDisabled(false);
		*/

		if (writeTheory)
		{
			commentwindow->setWriteTheory(false);
			writeTheory = false;
			writeTheoryAct->setChecked(false);
		}
	}
}

void MainWindow::fileNewRep()
{
	QString path = QFileDialog::getSaveFileName(this, "Open book", dataPath, "Book files (*.book)");
	if (!path.isEmpty())
	{
		QFile file(path);
		if (file.exists())
		{
			QMessageBox msgbox;
			msgbox.setText("The book allready exist. Do you want to delete it?");
			msgbox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
			if (msgbox.exec() != QMessageBox::Ok)
				return;
			file.remove();
		}
		repBase->create(path);
		bdeRep.clear();
		bdeRep.board = currentPath->getStartPosition();
		bdeRep.eco = "A00";
		repBase->add(bdeRep);

		bdeRep = repBase->find(currentPath->getPosition());

		movewindow->update(bdeTheory, bdeRep);
		openingwindow->update(bdeTheory, bdeRep);
		commentwindow->update(bdeTheory.comment, bdeRep.comment);
		/*
		movewindow->setVisible(true);
		commentwindow->setVisible(true);

		closeRepAct->setDisabled(false);
		writeRepAct->setDisabled(false);
		*/

		if (writeRep)
		{
			commentwindow->setWriteRep(false);
			writeRep = false;
			writeRepAct->setChecked(false);
		}
	}
}

void MainWindow::fileCloseTheory()
{
	bdeTheory.clear();
	closeTheoryAct->setDisabled(true);
	writeTheoryAct->setDisabled(true);

	if (writeTheory)
	{
		commentwindow->setWriteTheory(false);
		writeTheory = false;
		writeTheoryAct->setChecked(false);
	}
}

void MainWindow::fileCloseRep()
{
	bdeRep.clear();
	closeRepAct->setDisabled(true);
	writeRepAct->setDisabled(true);

	if (writeRep)
	{
		commentwindow->setWriteRep(false);
		writeRep = false;
		writeRepAct->setChecked(false);
	}
}

void MainWindow::bookWriteTheory()
{
	writeTheory = writeTheory ? false : true;
	writeTheoryAct->setChecked(writeTheory);
	writeRep = false;
	writeRepAct->setChecked(false);
	commentwindow->setWriteTheory(writeTheory);
}

void MainWindow::bookWriteRep()
{
	writeRep = writeRep ? false : true;
	writeRepAct->setChecked(writeRep);
	writeTheory = false;
	writeTheoryAct->setChecked(false);
	commentwindow->setWriteRep(writeRep);
}

void MainWindow::flipBoard()
{
	boardwindow->flip();
	boardwindow->update();
}

void MainWindow::aboutDialog()
{
	AboutDialog dialog(this);
	dialog.exec();
}

void MainWindow::moveEntered(ChessMove& move)
{
	ChessBoard board = currentPath->getPosition();
	BookDBMove bm;

	BookDBEntry bde;
	// Do the move if it is legal
	if (!currentPath->add(move))
	{
		boardwindow->setPosition(board);
		return;
	}

	// Save the move if it doesn't exist
	PathEntry pe;
	if (writeTheory)
	{
		for (int i = 0; i < currentPath->size(); i++)
		{
			pe = currentPath->getEntry(i);
			bde = theoryBase->find(pe.board);
			if (!bde.moveExist(move))
			{
				bm.move = move;
				bm.score = 0;
				bm.repertoire = 0;
				bde.movelist.append(bm);
				theoryBase->add(bde);
			}
		}
	} else if (writeRep)
	{
		for (int i = 0; i < currentPath->size(); i++)
		{
			pe = currentPath->getEntry(i);
			bde = repBase->find(pe.board);
			if (!bde.moveExist(move))
			{
				bm.move = move;
				bm.score = 0;
				bm.repertoire = 0;
				bdeRep.movelist.append(bm);
				repBase->add(bdeRep);
			}
		}
	}


	// Change to read from both db
	board = currentPath->getPosition();
	bdeTheory = theoryBase->find(board);
	bdeRep = repBase->find(board);
	boardwindow->setPosition(board);
	movewindow->update(bdeTheory, bdeRep);
	openingwindow->update(bdeTheory, bdeRep);
	commentwindow->update(bdeTheory.comment, bdeRep.comment);
	pathwindow->update(currentPath);
}

void MainWindow::pathSelected(int ply)
{
	if (ply < 1)
		currentPath->clear();
	else
		currentPath->setLength(ply);

	ChessBoard board = currentPath->getPosition();
	bdeTheory = theoryBase->find(board);
	bdeRep = repBase->find(board);
	boardwindow->setPosition(board);
	movewindow->update(bdeTheory, bdeRep);
	openingwindow->update(bdeTheory, bdeRep);
	commentwindow->update(bdeTheory.comment, bdeRep.comment);
	pathwindow->update(currentPath);
}