#include "EngineWindow.h"
#include <QCoreApplication>
#include <QStandardPaths>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableView>

EngineWindow::EngineWindow(QWidget *parent)
	: QWidget(parent)
{
	char sz[16];
	multipv = 1;
	analyzing = false;
	freezing = false;
	engine = new Engine;
	currentBoard.startposition();
	engineName = "Engine.eng"; 
	iniPath = QStandardPaths::locate(QStandardPaths::DocumentsLocation, QCoreApplication::organizationName(), QStandardPaths::LocateDirectory);
	iniPath += "/" + QCoreApplication::applicationName() + "/Engines";

	QGridLayout* grid = new QGridLayout;

	QHBoxLayout* hbox = new QHBoxLayout;
	lines = new QLabel;
	lines->setAlignment(Qt::AlignCenter);
	lines->setText(itoa(multipv, sz, 10));
	decline = new QPushButton("-");
	incline = new QPushButton("+");
	analyze = new QPushButton("Analyze");
	analyze->setCheckable(true);
	freeze = new QPushButton("Freeze");
	freeze->setCheckable(true);
	hbox->addWidget(decline);
	hbox->addWidget(lines);
	hbox->addWidget(incline);
	hbox->addWidget(analyze);
	hbox->addWidget(freeze);
	grid->addLayout(hbox, 0, 0);

	model = new QStandardItemModel(multipv, 5);
	QStringList header;
	header << "Score" << "Depth" << "Time" << "Nodes" << "PV";
	model->setHorizontalHeaderLabels(header);
	QTableView* output = new QTableView;
	output->setModel(model);

	grid->addWidget(output, 1, 0);// , Qt::AlignLeft);

	setLayout(grid);

	if (multipv == 1)
		decline->setEnabled(false);

	connect(decline, SIGNAL(clicked(bool)), this, SLOT(declineClicked(bool)));
	connect(incline, SIGNAL(clicked(bool)), this, SLOT(inclineClicked(bool)));
	connect(freeze, SIGNAL(clicked(bool)), this, SLOT(freezeClicked(bool)));
	connect(analyze, SIGNAL(clicked(bool)), this, SLOT(analyzeClicked(bool)));
	connect(engine, SIGNAL(engineReady()), this, SLOT(engineReady()));
	connect(engine, SIGNAL(engineMove(const QString&, const QString&)), this, SLOT(engineStoped(const QString&, const QString&)));
	connect(engine, SIGNAL(engineInfo(const EngineInfo&)), this, SLOT(engineInfo(const EngineInfo&)));
}

EngineWindow::~EngineWindow()
{
	delete engine;
}

void EngineWindow::update(ChessBoard& board)
{
	currentBoard = board;
	if (analyzing)
		if (!freezing)
			engine->stop();
}

void EngineWindow::analyzeClicked(bool)
{
	if (analyze->isChecked())
	{
		if (analyzing)
			return;
		QString enginePath = iniPath + "/" + engineName;
		if (engine->load(enginePath))
		{
			analyzing = true;
		}
	}
	else
	{
		if (analyzing)
		{
			engine->unload();
			analyzing = false;
		}
	}
}

void EngineWindow::freezeClicked(bool)
{
	if (freeze->isChecked())
	{
		freezing = true;
		freezeBoard = currentBoard;
	}
	else
	{
		freezing = false;
	}
}

void EngineWindow::inclineClicked(bool)
{
	char sz[16];
	if (multipv == 1)
		decline->setEnabled(true);
	++multipv;
	lines->setText(itoa(multipv, sz, 10));
	engine->setMultiPV(multipv);
	model->setRowCount(multipv);
}

void EngineWindow::declineClicked(bool)
{
	char sz[16];
	if (multipv > 1)
	{
		--multipv;
		lines->setText(itoa(multipv, sz, 10));
		if (multipv == 1)
			decline->setEnabled(false);
		engine->setMultiPV(multipv);
		model->setRowCount(multipv);
	}
}

void EngineWindow::engineReady()
{
	engine->setMultiPV(multipv);
	if (analyzing)
		engine->search(currentBoard, movelist, INFINITE_SEARCH);
}

void EngineWindow::engineStoped(const QString& move, const QString& ponder)
{
	if (analyzing)
		engine->search(currentBoard, movelist, INFINITE_SEARCH);
}

void EngineWindow::engineInfo(const EngineInfo& info)
{
	char sz[16];
	int line = info.multipv;
	if (line < 1)
		line = 1;
	if (info.pv.size)
	{
		QStandardItem* item;

		if (info.lowerbound)
			item = new QStandardItem("--");
		else if (info.upperbound)
			item = new QStandardItem("++");
		else if (info.mate)
			item = new QStandardItem(QString("M") + itoa(info.mate,sz,10));
		else
			item = new QStandardItem(itoa(info.mate, sz, 10));
		item->setEditable(false);
		item->setTextAlignment(Qt::AlignCenter);
		model->setItem(line-1, 0, item);

		item = new QStandardItem(itoa(info.depth, sz, 10));
		item->setEditable(false);
		item->setTextAlignment(Qt::AlignCenter);
		model->setItem(line - 1, 1, item);

		item = new QStandardItem(itoa(info.time/1000, sz, 10));
		item->setEditable(false);
		item->setTextAlignment(Qt::AlignCenter);
		model->setItem(line - 1, 2, item);

		item = new QStandardItem(itoa(info.nodes, sz, 10));
		item->setEditable(false);
		item->setTextAlignment(Qt::AlignCenter);
		model->setItem(line - 1, 3, item);

		QString s;
		ChessBoard b;
		ChessMove m;
		if (freezing)
			b = freezeBoard;
		else
			b = currentBoard;
		for (int i = 0; i < info.pv.size; i++)
		{
			m = info.pv.list[i];

			s += QString(b.makeMoveText(m,FIDE).c_str());
			if (i<(info.pv.size-1))
				s+=" ";
			b.doMove(m,false);
		}
		item = new QStandardItem(s);
		item->setEditable(false);
		item->setTextAlignment(Qt::AlignCenter);
		model->setItem(line - 1, 4, item);
	}
}