#pragma once

#include <QString>
#include <QStringList>
#include <string>
#include "../Common/BaseEngine.h"

struct XBoardFeature
{
	bool ping;
	bool setboard;
	bool playother;
	bool san;
	bool usermove;
	bool time;
	bool draw;
	bool sigint;
	bool sigterm;
	bool reuse;
	bool analyze;
	QString myname;
	QStringList variants;
	bool colors;
	bool ics;
	bool name;
	bool pause;
	bool nps;
	bool debug;
	bool memory;
	bool smp;
	QStringList egt;
	QStringList option;
	bool exclude;
	bool setscore;
	bool highlight;
};

class XBoardEngine:public BaseEngine
{
protected:
	XBoardFeature feature;
	virtual void fromEngine(std::string& input);
	void readFeature(std::string& line);
public slots:
	virtual void started();
	void finnishInit();
public:
	XBoardEngine();
};
