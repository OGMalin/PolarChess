#include <Windows.h>
#include <string>
#include <fstream>
#include "frontend.h"
#include "engine.h"
#include "../Common/Utility.h"
#include "../Common/ChessBoard.h"
#include "../Common/MoveList.h"
#include "../Common/MoveGenerator.h"
#include "../Common/StopWatch.h"

using namespace std;

const char* ENGINENAME = "PolarChess 2.0 B6";

// To calculate rating
// Testmachine speed in MHz
const DWORD testFrequence = 2100;
// Full strength rating on test machine
const DWORD testElo = 2000;
// Rating difference when doubling speed
const double halfSpeed = 100.0;
// Nodes for a 1 sec. search (nps)
const DWORD testNodes = 140000;

FrontEnd::FrontEnd()
{
	// Default values
	debug = false;
	maxElo = 2000;
	currentElo = maxElo;
	minElo = 600;
	limitStrength = false;
	contempt = 0;
	currentBoard.setStartposition();
}

FrontEnd::~FrontEnd()
{

}


// The main loop of the engine.
int FrontEnd::run()
{
	HANDLE hs[2];
	DWORD dw;
	hs[0] = uci.hEvent;
	hs[1] = engine.hEvent;

#ifdef _DEBUG
	debug = true;
	engine.sendOutQue(ENG_debug);
#endif

	findMaxElo();
	currentElo = maxElo;
	readIniFiles();

	engine.sendOutQue(ENG_clearhistory);
	engine.sendOutQue(ENG_eval, EngineEval(EVAL_contempt, contempt));

	while (1)
	{
		dw = WaitForMultipleObjects(2, hs, FALSE, INFINITE);

		switch (dw)
		{
		case WAIT_OBJECT_0:
			if (uciInput())
			{
				engine.sendOutQue(ENG_quit);
				WaitForSingleObject(engine.hThread, 5000);
				return 0;
			}
			break;
		case WAIT_ABANDONED_0:
			break;
		case WAIT_OBJECT_0+1:
			engineInput();
			break;
		case WAIT_ABANDONED_0+1:
			break;
		case WAIT_TIMEOUT:
			break;
		default:
			break;
		}
	}
	return 0;
};

int FrontEnd::uciInput()
{
	string input;
	int uciCmd;
	while ((uciCmd = uci.get(input)) != UCI_none)
	{
		switch (uciCmd)
		{
		case UCI_uci:
			uciUci();
			break;
		case UCI_debug:
			uciDebug(input);
			break;
		case UCI_isready:
			uciIsready();
			break;
		case UCI_setoption:
			uciSetoption(input);
			break;
		case UCI_register:
			uciRegister(input);
			break;
		case UCI_ucinewgame:
			uciUcinewgame();
			break;
		case UCI_position:
			uciPosition(input);
			break;
		case UCI_go:
			uciGo(input);
			break;
		case UCI_stop:
			uciStop();
			break;
		case UCI_ponderhit:
			uciPonderhit();
			break;
		case UCI_quit:
			return true;
		case UCI_movegen:
			uciMovegen(input);
			break;
		case UCI_readfile:
			uciReadFile(input);
			break;
		case UCI_eval:
			uciEval(input);
			break;
		case UCI_unknown:
			uci.write(string("info string Unknown command: " + input));
			break;
		}
	}
	return false;
}

/* uci
*
* tell engine to use the uci (universal chess interface), this will be sent
* once as a first command after program boot to tell the engine to switch to
* uci mode.
* After receiving the uci command the engine must identify itself with the "id"
* command and send the "option" commands to tell the GUI which engine settings
* the engine supports if any.
* After that the engine should send "uciok" to acknowledge the uci mode.
* If no uciok is sent within a certain time period, the engine task will be killed
* by the GUI.
*/
void FrontEnd::uciUci()
{
	char sz[256];
	string s;
	list<string>::iterator it;

	uci.write("id name " + string(ENGINENAME));
	uci.write("id author Odd Gunnar Malin");
	uci.write("option name UCI_EngineAbout type string default http://polarchess.net");
	uci.write("option name Ponder type check default false");
	sprintf_s(sz, 256, "option name Contempt type spin default %i min -500 max 500", contempt);
	uci.write(sz);
	if (personalities.size())
	{
		s = "option name Personality type combo default ";
		if (find(personalities.begin(), personalities.end(), "Normal") != personalities.end())
			s += "Normal";
		else
			s += "<empty>";
		for (it = personalities.begin(); it != personalities.end(); it++)
			s += " var " + *it;
		uci.write(s);
	}
//	uci.write("option name MultiPV type spin default 1 min 1 max 100");
//	uci.write(sz);
	uci.write("option name UCI_AnalyseMode type check default false");
	s = "option name UCI_LimitStrength type check default ";
	if (limitStrength)
		s += "true";
	else
		s += "false";
	uci.write(s);
	sprintf_s(sz, 256, "option name UCI_Elo type spin default %u min %u max %u", currentElo, minElo, maxElo);
	uci.write(sz);

	uci.write("uciok");
}

/* debug [ on | off ]
*
* switch the debug mode of the engine on and off.
* In debug mode the engine should send additional infos to the GUI, e.g. with the "info string" command,
* to help debugging, e.g. the commands that the engine has received etc.
* This mode should be switched off by default and this command can be sent
* any time, also when the engine is thinking.
*/
void FrontEnd::uciDebug(const std::string& s)
{
	if (s.length())
		debug = booleanString(s);
	engine.sendOutQue(debug ? ENG_debug : ENG_nodebug);
}

/* isready
*
* this is used to synchronize the engine with the GUI. When the GUI has sent a
* command or multiple commands that can take some time to complete, this command
* can be used to wait for the engine to be ready again or to ping the engine to
* find out if it is still alive.
* E.g. this should be sent after setting the path to the tablebases as this can
* take some time.
* This command is also required once before the engine is asked to do any search
* to wait for the engine to finish initializing.
* This command must always be answered with "readyok" and can be sent also when
* the engine is calculating in which case the engine should also immediately answer
* with "readyok" without stopping the search.
*/
void FrontEnd::uciIsready()
{
	uci.write("readyok");
}

/* setoption name <id> [value <x>]
*
* this is sent to the engine when the user wants to change the internal parameters
* of the engine. For the "button" type no value is needed.
* One string will be sent for each parameter and this will only be sent when the
* engine is waiting.
* The name and value of the option in <id> should not be case sensitive and can
* inlude spaces.
* The substrings "value" and "name" should be avoided in <id> and <x> to allow
* unambiguous parsing, for example do not use <name> = "draw value".
* Here are some strings for the example below:
*    "setoption name Nullmove value true\n"
*    "setoption name Selectivity value 3\n"
*    "setoption name Style value Risky\n"
*    "setoption name Clear Hash\n"
*    "setoption name NalimovPath value c:\chess\tb\4;c:\chess\tb\5\n"
*/
void FrontEnd::uciSetoption(const std::string& s)
{
	string name, value, ss;
	int i;
	bool b;
	EngineEval ev;

	i = 2;
	while ((ss = getWord(s, i++)).length())
	{
		if (ss == "value")
			break;
		name += ss;
		name += ' ';
	}

	while ((ss = getWord(s, i++)).length())
	{
		value += ss;
		value += ' ';
	}

	name = trim(name);
	value = trim(value);

	if (name == "Contempt")
	{
		contempt = atoi(value.c_str());
		engine.sendOutQue(ENG_eval, EngineEval(EVAL_contempt, contempt));
	}
	else if (name == "MultiPV")
	{
		engine.sendOutQue(ENG_eval, EngineEval(EVAL_multipv, atoi(value.c_str())));
	}
	else if (name == "Personality")
	{
		string path = getProgramPath();
		path += "personalities\\";
		path += value;
		path += ".per";
		uciReadFile(path);
	}
	else if (name == "UCI_AnalyseMode")
	{
		b = booleanString(value);
		ev.type = EVAL_contempt;
		if (b)
		{
			ev.value = 0;
			engine.sendOutQue(ENG_eval, ev);
		}
		else
		{
			ev.value = contempt;
			engine.sendOutQue(ENG_eval, ev);
		}
	}
	else if ("UCI_LimitStrength")
	{
		limitStrength = booleanString(value);
//		if (limitStrength)
//			engine.sendOutQue(ENG_eval, EngineEval(EVAL_strength, calculateStrength(currentElo)));

	}
	else if ("UCI_Elo")
	{
		currentElo = atoi(value.c_str());
//		if (limitStrength)
//			engine.sendOutQue(ENG_eval,EngineEval(EVAL_strength, calculateStrength(currentElo)));
	}
}

/* register
*
* this is the command to try to register an engine or to tell the engine that
* registration will be done later. This command should always be sent if the engine
* has sent "registration error" at program startup.
* The following tokens are allowed:
*   * later
*     the user doesn't want to register the engine now.
*   * name <x>
*     the engine should be registered with the name <x>
*   * code <y>
*     the engine should be registered with the code <y>
* Example:
*   "register later"
*   "register name Stefan MK code 4359874324"
*/
void FrontEnd::uciRegister(const std::string& s)
{

}

/* ucinewgame
*
* this is sent to the engine when the next search (started with "position" and "go")
* will be from a different game. This can be a new game the engine should play or a
* new game it should analyse but also the next position from a testsuite with
* positions only.
* If the GUI hasn't sent a "ucinewgame" before the first "position" command, the
* engine shouldn't expect any further ucinewgame commands as the GUI is probably not
* supporting the ucinewgame command.
* So the engine should not rely on this command even though all new GUIs should
* support it.
* As the engine's reaction to "ucinewgame" can take some time the GUI should always
* send "isready" after "ucinewgame" to wait for the engine to finish its operation.
*/
void FrontEnd::uciUcinewgame()
{
	currentBoard.setFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	engine.sendOutQue(ENG_clearhistory);
	engine.sendOutQue(ENG_clearhash);
}

/* position [fen <fenstring> | startpos ]  moves <move1> .... <movei>
*
* set up the position described in fenstring on the internal board and play the
* moves on the internal chess board.
* If the game was played  from the start position the string "startpos" will be sent
* Note: no "new" command is needed. However, if this position is from a different
* game than the last position sent to the engine, the GUI should have sent a
* "ucinewgame" inbetween.
*/
void FrontEnd::uciPosition(const std::string& input)
{
	string cmd;
	ChessMove m;
	int i = 1, n;

	// Clear 3-rep table (m=empty move).
	engine.sendOutQue(ENG_clearhistory);

	// Read the inputline
	cmd = getWord(input, i++);
	while (cmd.length()>0)
	{
		// Startposition as fen-string
		if (cmd == "fen")
		{
			cmd = getWord(input, i++);
			if ((getWord(input, i) == "w") || (getWord(input, i) == "b"))
			{
				cmd += ' ';
				cmd += getWord(input, i++);
				if (existIn(getWord(input, i).at(0), "KQkq-"))
				{
					cmd += ' ';
					cmd += getWord(input, i++);
					if (existIn(getWord(input, i).at(0), "abcdefgh"))
					{
						if (existIn(getWord(input, i).at(1), "36"))
						{
							cmd += ' ';
							cmd += getWord(input, i++);
						}
					}
				}
			}
			currentBoard.setFen(cmd.c_str());
		}
		else if (cmd == "startpos")
		{
			// Startposition as a normal gamestart.
			currentBoard.setFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
		}
		else if (cmd == "moves")
		{
			// Read the moves and update the position and drawtable.
			n = 0;
			cmd = getWord(input, i++);
			while (cmd.length())
			{
				engine.sendOutQue(ENG_history, currentBoard);
				m = currentBoard.getMoveFromText(cmd);
				currentBoard.doMove(m, false);
				if (currentBoard.move50draw == 0)
					engine.sendOutQue(ENG_clearhistory);
				cmd = getWord(input, i++);
			}

		}
		cmd = getWord(input, i++);
	}
}

/* go
*
* start calculating on the current position set up with the "position" command.
* There are a number of commands that can follow this command, all will be sent in
* the same string.
* If one command is not sent its value should be interpreted as it would not
* influence the search.
*/
void FrontEnd::uciGo(const std::string& input)
{
	int i = 1;
	int wtime = 0, btime = 0, winc = 0, binc = 0, movestogo = 0, depth = 0, nodes = 0, mate = 0, movetime = 0;
	string cmd;
	std::list<std::string> searchmoves;
	bool ponder=false, infinite=false;
	cmd = getWord(input, i++);
	while (cmd.length()>0)
	{
		/* searchmoves <move1> .... <movei>
		*
		* restrict search to this moves only
		* Example : After "position startpos" and "go infinite searchmoves e2e4 d2d4"
		* the engine should only search the two moves e2e4 and d2d4 in the initial position.
		*/
		if (cmd == "searchmoves")
		{
			cmd = getWord(input, i++);
			while (isMoveText(cmd))
			{
				searchmoves.push_back(cmd);
				cmd = getWord(input, i++);
			}
		}
		/* ponder
		*
		* start searching in pondering mode.
		* Do not exit the search in ponder mode, even if it's mate!
		* This means that the last move sent in in the position string is the ponder move.
		* The engine can do what it wants to do, but after a "ponderhit" command
		* it should execute the suggested move to ponder on.This means that the ponder move sent by
		* the GUI can be interpreted as a recommendation about which move to ponder.However, if the
		* engine decides to ponder on a different move, it should not display any mainlines as they are
		* likely to be misinterpreted by the GUI because the GUI expects the engine to ponder
		* on the suggested move.
		*/
		else if (cmd == "ponder")
		{
			ponder = true;
		}
		/* wtime <x>
		*
		* white has x msec left on the clock
		*/
		else if (cmd == "wtime")
		{
			wtime = atoi(getWord(input, i++).c_str());
		}
		/* btime <x>
		*
		* black has x msec left on the clock
		*/
		else if (cmd == "btime")
		{
			btime = atoi(getWord(input, i++).c_str());
		}
		/* winc <x>
		*
		* white increment per move in mseconds if x > 0
		*/
		else if (cmd == "winc")
		{
			winc = atoi(getWord(input, i++).c_str());
		}
		/* binc <x>
		*
		* black increment per move in mseconds if x > 0
		*/
		else if (cmd == "binc")
		{
			binc = atoi(getWord(input, i++).c_str());
		}
		/* movestogo <x>
		*
		* there are x moves to the next time control,
		* this will only be sent if x > 0,
		* if you don't get this and get the wtime and btime it's sudden death
		*/
		else if (cmd == "movestogo")
		{
			movestogo = atoi(getWord(input, i++).c_str());
		}
		/* depth <x>
		*
		* search x plies only.
		*/
		else if (cmd == "depth")
		{
			depth = atoi(getWord(input, i++).c_str());
		}
		/* nodes <x>
		*
		* search x nodes only,
		*/
		else if (cmd == "nodes")
		{
			nodes = atoi(getWord(input, i++).c_str());
		}
		/* mate <x>
		*
		* search for a mate in x moves
		*/
		else if (cmd == "mate")
		{
			mate = atoi(getWord(input, i++).c_str());
		}
		/* movetime <x>
		*
		* search exactly x mseconds
		*/
		else if (cmd == "movetime")
		{
			movetime = atoi(getWord(input, i++).c_str());
		}
		/* infinite
		*
		* search until the "stop" command. Do not exit the search without being told so in this mode!
		*/
		else if (cmd == "infinite")
		{
			infinite = true;
		}
		cmd = getWord(input, i++);
	}

	EngineGo eg;
	if (!movestogo)
		movestogo = 30;
	// Find time to use for the next move.
	wtime = (currentBoard.toMove == WHITE) ? wtime : btime;
	winc = (currentBoard.toMove == WHITE) ? winc : binc;
	eg.maxTime = (wtime + (movestogo*winc)) / movestogo;
	if (eg.maxTime > (DWORD)(wtime / 2))
		eg.maxTime = wtime/2;

	eg.fixedTime = movetime;
	eg.nodes=nodes;
	eg.depth = depth;
	eg.mate = mate;
	if (searchmoves.size())
	{
		std::list<std::string>::iterator it= searchmoves.begin();
		ChessMove m;
		while (it != searchmoves.end())
		{
			m = currentBoard.getMoveFromText(*it);
			if (!m.empty())
				eg.searchmoves.push_back(m);
			++it;
		}
	}

	engine.sendOutQue(ENG_position, currentBoard);
	if (ponder||infinite)
	{
		engine.sendOutQue(ENG_ponder, eg);
	}
	else
	{
		if (limitStrength)
		{
			eg.nodes=calculateStrength(currentElo, eg.maxTime);
		}
		engine.sendOutQue(ENG_go, eg);
	}
}

/* stop
*
* stop calculating as soon as possible,
* don't forget the "bestmove" and possibly the "ponder" token when finishing
* the search
*/
void FrontEnd::uciStop()
{
	engine.sendOutQue(ENG_stop);
}

/* ponderhit (UCI)
*
* the user has played the expected move. This will be sent if the engine was told
* to ponder on the same move the user has played. The engine should continue
* searching but switch from pondering to normal search.
*/
void FrontEnd::uciPonderhit()
{
	engine.sendOutQue(ENG_ponderhit);
}

void FrontEnd::uciMovegen(const std::string& s)
{
	DWORD i;
	ULONGLONG t;
	char sz[256];
	StopWatch st;
	int depth = atoi(getWord(s, 1).c_str());
	st.start();
	i = movegenTest(depth);
	t = st.read(WatchPrecision::Microsecond);
	sprintf_s(sz, 256, "%u nodes in %llu ms", i, t/1000);
	uci.write(string(sz));
}

void FrontEnd::engineInput()
{
	int engCmd;
	string s;
	while ((engCmd = engine.peekInQue()) != ENG_none)
	{
		switch (engCmd)
		{
		case ENG_debug:
			engine.getInQue(s);
			if (debug)
				uci.write("info string " + s);
			break;
		case ENG_info:
			engine.getInQue(s);
			uci.write("info " + s);
			break;
		case ENG_string:
			engine.getInQue(s);
			uci.write(s);
			break;
		default:
			engine.getInQue(s);
			uci.write(s);
			break;
		}
	}
}

DWORD FrontEnd::movegenTest(int depth, bool init, int ply)
{
	int moveit;
	static DWORD testNodes;
	static MoveList testList[30];
	static MoveGenerator testGen;
	static ChessBoard b;
	if (init)
	{
		testList->clear();
		testNodes=0;
		b.copy(currentBoard);
	}
	if (depth==0)
		return ++testNodes;
	testGen.makeMoves(b,testList[ply]);
	moveit=0;
	while (moveit!=testList[ply].end())
	{
		testGen.doMove(b,testList[ply][moveit]);
		movegenTest(depth-1,false,ply+1);
		testGen.undoMove(b,testList[ply][moveit]);
		++moveit;
	};
	return testNodes;
}

bool FrontEnd::isMoveText(const std::string& input)
{
	//  e2e4, e2-e4, e2xe4
	//  e4, ed, exd, exd4
	//  Ne4,Nxe4, N3e4, N3xe4, Nce4, Ncxe4
	//  O-O, o-o, 0-0, 
	if (input.length()<2) return false;
	if (strchr("NBRQKabcdefghoO0", input.at(0)) == NULL) return false;
	if (strchr("abcdefgh12345678-xX", input.at(1)) == NULL) return false;
	if (input.length()>2)
		if (strchr("abcdefgh12345678-xX!?#+", input.at(1)) == NULL) return false;
	if (input.length()>3)
		if (strchr("abcdefgh12345678-xX!?#+", input.at(1)) == NULL) return false;
	if (input.length()>4)
		if (strchr("abcdefgh12345678-xX!?#+", input.at(1)) == NULL) return false;
	return true;
}

void FrontEnd::findMaxElo()
{
	LARGE_INTEGER pf;
	if (!QueryPerformanceFrequency(&pf))
		return;
	freqMz = pf.QuadPart / 1000;
	double fact = (double)freqMz / testFrequence;


	int elodiff=(int)(halfSpeed*log(1 / fact) / log(2));
	maxElo = testElo-elodiff;
	// Test machine

#ifdef _DEBUG
	char sz[256];
	sprintf_s(sz, 256, "info string test fq: %u, this fq: %llu", testFrequence, freqMz);
	uci.write(sz);
#endif
}  

void FrontEnd::uciReadFile(const std::string& filename)
{
	if (filename.length() < 1)
	{
		if (debug)
			uci.write("info string No filename");
		return;
	}
	string line;

	ifstream file(filename);
	if (file.is_open())
	{
		while (getline(file, line))
		{
			trim(line);
			if (line.length())
				if (line.at(0)!=';')
					uci.input(line);
		}
		file.close();
	}
	else
	{
		if (debug)
			uci.write("info string Unable to open file: " + filename);
	}
}

void FrontEnd::uciEval(const std::string& input)
{
	string para, value;
	para = getWord(input, 1);
	if (para.length() > 0)
	{
		value = getWord(input, 2);
		if (value.length() > 0)
		{
			if (para == "contempt")
			{
				contempt = atoi(value.c_str());
				engine.sendOutQue(ENG_eval, EngineEval(EVAL_contempt, contempt));
				return;
			}
			if (para == "pawn")
			{
				engine.sendOutQue(ENG_eval, EngineEval(EVAL_pawn, atoi(value.c_str())));
				return;
			}
			if (para == "knight")
			{
				engine.sendOutQue(ENG_eval, EngineEval(EVAL_knight, atoi(value.c_str())));
				return;
			}
			if (para == "bishop")
			{
				engine.sendOutQue(ENG_eval, EngineEval(EVAL_bishop, atoi(value.c_str())));
				return;
			}
			if (para == "rook")
			{
				engine.sendOutQue(ENG_eval, EngineEval(EVAL_rook, atoi(value.c_str())));
				return;
			}
			if (para == "queen")
			{
				engine.sendOutQue(ENG_eval, EngineEval(EVAL_queen, atoi(value.c_str())));
				return;
			}
			if (para == "bishoppair")
			{
				engine.sendOutQue(ENG_eval, EngineEval(EVAL_bishoppair, atoi(value.c_str())));
				return;
			}
			if (para == "elo")
			{
				int elo = atoi(value.c_str());
				if (elo > (int)maxElo)
					elo = maxElo;
				limitStrength = true;
//				engine.sendOutQue(ENG_eval, EngineEval(EVAL_strength, calculateStrength(elo)));
				return;
			}
			if (para == "mobility")
			{
				engine.sendOutQue(ENG_eval, EngineEval(EVAL_mobility, atoi(value.c_str())));
				return;
			}
			uci.write("info string Unknown Eval parameter.");
			return;
		}
	}
	uci.write("info string wrong eval syntax.");
}

void FrontEnd::readIniFiles()
{
	WIN32_FIND_DATA wfd;
	HANDLE hFind;
	string s;
	//directory_iterator();
	string path = getProgramPath();
	path += "personalities\\";
	s = path + "*.per";
	hFind=FindFirstFile(s.c_str(),&wfd);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		while (1)
		{
			wfd.cFileName[strlen(wfd.cFileName) - 4] = '\0';
			s = wfd.cFileName;
			personalities.push_back(string(wfd.cFileName));
			if (!FindNextFile(hFind, &wfd))
				break;
		}
		FindClose(hFind);
	}
	if (find(personalities.begin(), personalities.end(), string("Normal")) != personalities.end())
		uciReadFile(path + "Normal.per");
	
}

const std::string FrontEnd::getProgramPath()
{
	string s;
	char sz[MAX_PATH];
	if (!GetModuleFileName(NULL, sz, MAX_PATH))
		return string("");
	string::size_type size;
	s = sz;
	size = s.find_last_of('\\');
	if (size != string::npos)
		return s.substr(0, size + 1);
	return string("");
}

DWORD FrontEnd::calculateStrength(int elo, int tm)
{
	int diff = maxElo - elo;
	DWORD nodes=0;
	double dnodes = 0.0;
	if (diff <= 0)
		return 0;
	double f1 = diff / halfSpeed;
	double f2 = 1 / (pow(2,f1));
	dnodes = testNodes * (double)(freqMz / testFrequence);
	nodes = (DWORD)(f2*dnodes*tm/1000);
#ifdef _DEBUG
	char sz[256];
	sprintf_s(sz, 256, "info string Elo: %u, nodes: %u", elo, nodes);
	uci.write(sz);
#endif
	return nodes;
}
