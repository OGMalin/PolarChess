#pragma once

#include <QWidget>
#include <QTextEdit>
class Scoresheet :public QTextEdit
{
	Q_OBJECT
public:
	Scoresheet(QWidget* parent = 0);
};