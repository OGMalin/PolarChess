#pragma once

#include <QWidget>
#include <QListWidget>
#include <QPoint>
#include <QFont>
#include <QBrush>

class Path;

class PathWindow : public QListWidget
{
	Q_OBJECT

public:
	PathWindow(QWidget *parent = 0);
	~PathWindow();
	void refresh(Path* path);
	void addPath(int);
	QString fontToString();
	void fontFromString(const QString&);

signals:
	void pathSelected(int);
	void pathToDB(int);
	void pathCopy();
	void pathPaste();

public slots:
	void moveClicked(QListWidgetItem*);
	void showContextMenu(const QPoint& pos);
	void selectFont();
	void addPathT() { addPath(0); };
	void addPathW() { addPath(1); };
	void addPathB() { addPath(2); };
	void copy();
	void paste();

private:
	QBrush normalBrush;
	QBrush grayedBrush;
};

