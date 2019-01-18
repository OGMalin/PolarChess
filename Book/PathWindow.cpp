#include "PathWindow.h"
#include "Path.h"
#include <QStringList>

PathWindow::PathWindow(QWidget *parent)
	: QListWidget(parent)
{
	char sz[16];
	setViewMode(QListWidget::IconMode);
	setResizeMode(QListWidget::Adjust);
	setMovement(QListWidget::Static);
	connect(this, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(moveClicked(QListWidgetItem*)));
}

PathWindow::~PathWindow()
{
}

void PathWindow::update(Path* path)
{
	char sz[16];
	QStringList ml;
	QString p = ".";
	clear();
	path->getMoveList(ml);
	addItem("O"); // Start position
	for (int i = 0; i < ml.size(); i++)
	{
		if (!(i % 2))
			addItem(itoa(i / 2 + 1, sz, 10) + p + ml[i]);
		else
			addItem(ml[i]);
	}
}

void PathWindow::moveClicked(QListWidgetItem* item)
{
	emit pathSelected(currentRow());
}