//#define _DEBUG_SEARCH

#include <process.h>
#include <string>
#include "Engine.h"
#include "EngineInterface.h"
#include "../Common/utility.h"
#include <assert.h>

#ifdef _DEBUG_SEARCH
#include <iostream>
int highestsearchply;
int highestqsearchply;
#endif


using namespace std;
const int BREAKING = MATE + 400;
const int MAX_DEPTH = 100;

Engine eng;
char sz[256];
ChessMove emptyMove;

// White/black materiale are only used to deside if nullmove should be used.
#define whitemateriale (material[whiteknight]+material[whitebishop]+material[whiterook]+material[whitequeen])
#define blackmateriale (material[blackknight]+material[blackbishop]+material[blackrook]+material[blackqueen])

// Heinz adaptive null-move reduction, p.35 in SSCC
// 2 if (depth<=6) or ((depth<=8)&(max_pieces_per_side<3))
// 3 if (depth>8) or ((depth>6)&(max_pieces_per_side>=3))
#define nullMoveReduction(depth,mat) (2+((depth)>(6+(((mat)<3)?2:0)))) 
//#define nullMoveReduction(depth,material) (2)

void EngineSearchThreadLoop(void* lpv)
{
	ENGINECOMMAND cmd=ENG_none;
	eng.ei = (EngineInterface*)lpv;
	ChessBoard cb;
	EngineGo eg;
	EngineEval ev;
	while (1)
	{
		WaitForSingleObject(eng.ei->hEngine, INFINITE);
		while ((cmd=eng.ei->peekOutQue())!=ENG_none)
		{
			if (cmd == ENG_quit)
				break;
			switch (cmd)
			{
			case ENG_debug:
				eng.debug = true;
				eng.ei->getOutQue();
				break;
			case ENG_nodebug:
				eng.debug = false;
				eng.ei->getOutQue();
				break;
			case ENG_stop: // No need to do anything here because the engine is in waiting state (or should it sent bestmove 0000 ?).
				eng.ei->getOutQue();
				break;
			case ENG_clearhistory:
				eng.ei->getOutQue();
				eng.drawTable.clear();
				break;
			case ENG_history:
				eng.ei->getOutQue(cb);
				eng.drawTable.add(cb);
				break;
			case ENG_ponderhit: // Shouldnt happend here.
				eng.ei->getOutQue();
				break;
			case ENG_clearhash:
				eng.ei->getOutQue();
				break;
			case ENG_go:
				eng.watch.start();
				eng.ei->getOutQue(eg);
				eng.fixedMate = eg.mate;
				eng.fixedNodes = eg.nodes;
				eng.fixedTime = eg.fixedTime*1000;
				eng.maxTime = eg.maxTime*1000;
				eng.fixedDepth = eg.depth;
				eng.searchmoves = eg.searchmoves;
				if (eng.fixedMate)
					eng.searchtype = MATE_SEARCH;
				else if (eng.fixedNodes)
					eng.searchtype = NODES_SEARCH;
				else if (eng.fixedTime)
					eng.searchtype = TIME_SEARCH;
				else if (eng.fixedDepth)
					eng.searchtype = DEPTH_SEARCH;
				else
					eng.searchtype = NORMAL_SEARCH;
				eng.startSearch();
				break;
			case ENG_ponder:
				eng.watch.start();
				eng.ei->getOutQue(eg);
				eng.fixedMate = eg.mate;
				eng.fixedNodes = eg.nodes;
				eng.fixedTime = eg.fixedTime*1000;
				eng.maxTime = eg.maxTime*1000;
				eng.searchmoves = eg.searchmoves;
				if (eng.fixedMate)
					eng.searchtype = MATE_SEARCH;
				else if (eng.fixedNodes)
					eng.searchtype = NODES_SEARCH;
				else if (eng.fixedTime)
					eng.searchtype = TIME_SEARCH;
				else
					eng.searchtype = PONDER_SEARCH;
				eng.startSearch();
				break;
			case ENG_position:
				eng.ei->getOutQue(cb);
				eng.theBoard = cb;
				break;
			case ENG_eval:
				eng.ei->getOutQue(ev);
				if (ev.type == EVAL_contempt)
					eng.contempt = ev.value;
				else if (ev.type == EVAL_pawn)
					eng.eval.pawnValue = ev.value;
				else if (ev.type == EVAL_knight)
					eng.eval.knightValue = ev.value;
				else if (ev.type == EVAL_bishop)
					eng.eval.bishopValue = ev.value;
				else if (ev.type == EVAL_rook)
					eng.eval.rookValue = ev.value;
				else if (ev.type == EVAL_queen)
					eng.eval.queenValue = ev.value;
				else if (ev.type == EVAL_bishoppair)
					eng.eval.bishopValue = ev.value;
				else if (ev.type == EVAL_mobility)
					eng.eval.mobilityScore = ev.value;
				break;
			default:
				// Unknown command, remove it.
				eng.ei->getOutQue();
				break;
			}
		}
		if (cmd == ENG_quit)
			break;
	}
	_endthread();
};


Engine::Engine()
{
	debug = false;
	contempt = 0;
	multiPV = 1;
}

void Engine::startSearch()
{
	int i;
	typeSquare sq;
	bool inCheck;
	HASHKEY hashKey;
	nodes = 0;
	bestMove.clear();
	eval.rootcolor = theBoard.toMove;
	eval.drawscore[eval.rootcolor] = -contempt;
	eval.drawscore[OTHERPLAYER(eval.rootcolor)] = contempt;
	eval.setup(theBoard);

	inCheck = mgen.inCheck(theBoard, theBoard.toMove);
	hashKey = theBoard.hashkey();

	// Clear pv and nullmove
	for (i = 0; i < MAX_PLY; i++)
	{
		pv[i].clear();
		nullmove[i].clear();
		nullmove[i].moveType = NULL_MOVE;
	}

	// Scan pieces
	for (i = 0; i < 12; i++)
		material[i] = 0;
	for (sq = 0; sq < 63; sq++)
		++material[theBoard.board[SQUARE128(i)]];

	if (searchmoves.size())
		ml[0] = searchmoves;
	else
		mgen.makeMoves(theBoard, ml[0]);

	if (!ml[0].size())
	{
		ei->sendInQue(ENG_info, string("string No legal moves, aborting search."));
		return;
	}
	else if ((ml[0].size() == 1) && (searchtype == NORMAL_SEARCH))
	{ // Only one legal move
		bestMove = ml[0][0];
		sendBestMove();
		return;
	}

	iterativeSearch(inCheck, hashKey);
}

void Engine::iterativeSearch(bool inCheck, HASHKEY hashKey)
{
	int depth=1;
	int score=-MATE;
	
	for (depth = 1; depth < MAX_DEPTH; depth++)
	{
		sprintf_s(sz, 256, "depth %i", depth);
		ei->sendInQue(ENG_info, sz);
#ifdef _DEBUG_SEARCH
		highestsearchply = highestqsearchply = 0;
#endif
		score = aspirationSearch(depth, score, inCheck, hashKey);
		if (score == BREAKING)
			return;
#ifdef _DEBUG_SEARCH
		cout << "Max ply: " << highestsearchply << ", Max qply: " << highestqsearchply << endl;
#endif

		if (searchtype == DEPTH_SEARCH)
		{
			if (depth == fixedDepth)
			{
				sendBestMove();
				return;
			}
		}

		// Is there time enough to make a new iteration
		if ((searchtype == NORMAL_SEARCH) && (watch.read(WatchPrecision::Microsecond) > (maxTime / 2)))
		{
			sendBestMove();
			return;
		}
	}
}

int Engine::aspirationSearch(int depth, int bestscore, bool inCheck, HASHKEY hashKey)
{
	int alpha;
	int beta;
	int score;
	if (depth > 1)
	{
		alpha = bestscore - 50;
		beta = bestscore + 50;
	}
	else
	{
		alpha = -MATE;
		beta = MATE;
	}
	score = rootSearch(depth, alpha, beta, inCheck, hashKey);
	if (score == BREAKING)
		return BREAKING;
	if ((score <= alpha) || (score >= beta))
	{
		if (debug || (watch.read(WatchPrecision::Millisecond)>500))
		{
			if (score <= alpha)
				sendPV(pv[0], depth, score, lowerbound);
			else
				sendPV(pv[0], depth, score, upperbound);
		}
		alpha = -MATE;
		beta = MATE;
		score = rootSearch(depth, alpha, beta, inCheck, hashKey);
	}
	return score;
}

int Engine::rootSearch(int depth, int alpha, int beta, bool inCheck, HASHKEY hashKey)
{
	int score;
	int mit;
	HASHKEY newkey;
	bool followPV = true;
	int extention = 0;
	int oldNodes;

	++nodes;

	// Order moves. Do not extend before this
	if (depth == 1)
	{
		orderRootMoves();
		bestMove = ml[0][0];
	}
	else
	{
		mit = ml[0].find(bestMove);
		if (mit < ml[0].size())
			ml[0][mit].score = 0x7fffffff;
		ml[0].sort();
//		orderMoves(ml[0], bestMove.back());
	}

	if (inCheck)
		++depth;

	// Add the root position to the drawtable
	hashDrawTable.add(hashKey, 0);

	bool sendinfo=true;
	for (mit = 0; mit < ml[0].size(); mit++)
	{
//		sendinfo = (watch.read(WatchPrecision::Millisecond) > 999) ? true : false;
		// Send UCI info
		if (debug || sendinfo)
		{
			sprintf_s(sz, 256, "currmove %s currmovenumber %i", theBoard.makeMoveText(ml[0][mit],UCI).c_str(), mit + 1);
			ei->sendInQue(ENG_info, sz);
		}
		newkey = theBoard.newHashkey(ml[0][mit], hashKey);
		mgen.doMove(theBoard, ml[0][mit]);
		--material[ml[0][mit].capturedpiece];

		assert(theBoard.hashkey() == newkey);

		inCheck = mgen.inCheck(theBoard, theBoard.toMove);
		extention = moveExtention(inCheck, ml[0][mit], emptyMove, ml[0].size());
		oldNodes = nodes;
		score = -Search(depth - 1 + extention, -beta, -alpha, inCheck, newkey, 1,followPV, true, ml[0][mit]);
		if (score == -BREAKING)
			return BREAKING;
		mgen.undoMove(theBoard, ml[0][mit]);
		++material[ml[0][mit].capturedpiece];
		ml[0][mit].score = nodes - oldNodes;
		if (score >= beta)
			return beta;
		if (score > alpha)
		{
			copyPV(pv[0], pv[1], ml[0][mit]);
#ifndef _DEBUG_SEARCH
			if (debug || sendinfo)
#endif
				sendPV(pv[0], depth,score);
			alpha = score;

			bestMove = ml[0][mit];
		}
		if (inCheck)
			--extention;
		followPV = false;
	}
	return alpha;
}

int Engine::Search(int depth, int alpha, int beta, bool inCheck, HASHKEY hashKey, int ply, bool followPV, bool doNullmove, ChessMove& lastmove)
{
	int score;
	HASHKEY newkey;
	int extention = 0;

//	pv[ply].clear();

	if (depth == 0)
		return qSearch(alpha, beta, ply);


	if (!(++nodes % 0x400))
		if (abortCheck())
			return BREAKING;

	if (drawTable.exist(theBoard, hashKey) || hashDrawTable.exist(hashKey, ply-1))
		return (eval.drawscore[theBoard.toMove]);

	if (ply >= MAX_PLY)
		return eval.evaluate(theBoard, alpha, beta);

	// Add position to the drawtable
	hashDrawTable.add(hashKey, ply);

	// Null move
	if (!followPV && !inCheck && doNullmove && whitemateriale && blackmateriale)
	{

		newkey = theBoard.newHashkey(nullmove[ply],hashKey);
		mgen.doNullMove(theBoard, nullmove[ply]);
		score = -Search(__max(depth - 1 - nullMoveReduction(depth,__min(whitemateriale,blackmateriale)),0), -beta, -beta+1, false, newkey, ply + 1, false, false,nullmove[ply]);
//		score = -Search(__max(depth - 4, 0), -beta, -beta+1, false, newkey, ply + 1, false, false);
		if (score == -BREAKING)
			return BREAKING;
		mgen.undoNullMove(theBoard, nullmove[ply]);
		if (score >= beta)
			return beta;
	}

	mgen.makeMoves(theBoard, ml[ply]);
	orderMoves(ml[ply], followPV?pv[ply].front():emptyMove);
	int mit;
	for (mit = 0; mit < ml[ply].size(); mit++)
	{
		newkey = theBoard.newHashkey(ml[ply][mit], hashKey);
		mgen.doMove(theBoard, ml[ply][mit]);
		--material[ml[ply][mit].capturedpiece];

		assert(theBoard.hashkey() == newkey);

		inCheck = mgen.inCheck(theBoard, theBoard.toMove);
		extention = moveExtention(inCheck, ml[ply][mit], lastmove, ml[ply].size());
#ifdef _DEBUG_SEARCH
		highestsearchply = __max(ply+1, highestsearchply);
#endif
		score = -Search(depth - 1 + extention, -beta, -alpha, inCheck, newkey, ply + 1, followPV,true, ml[ply][mit]);
		if (score == -BREAKING)
			return BREAKING;
		mgen.undoMove(theBoard, ml[ply][mit]);
		++material[ml[ply][mit].capturedpiece];
		if (score >= beta)
			return beta;
		if (score > alpha)
		{
			alpha = score;
			copyPV(pv[ply], pv[ply + 1], ml[ply][mit]);
		}
		if (inCheck)
			--extention;
		followPV = false;
	}
	if (mit == 0)
	{
		if (!inCheck) // Stalemate
			alpha = 0;
		else
			alpha = -MATE + ply;
	}
	return alpha;
}

int Engine::qSearch(int alpha, int beta, int ply)
{
	int score;

	pv[ply].clear();

	if (!(++nodes % 0x400))
		if (abortCheck())
			return BREAKING;

	score = eval.evaluate(theBoard, alpha, beta);

	if (score >= beta)
		return beta;
	if (ply >= (MAX_PLY-1))
		return score;

	if (score > alpha)
		alpha = score;

	mgen.makeCaptureMoves(theBoard, ml[ply]);
	orderQMoves(ml[ply]);
	int mit;
	for (mit = 0; mit < ml[ply].size(); mit++)
	{
		mgen.doMove(theBoard, ml[ply][mit]);
#ifdef _DEBUG_SEARCH
		highestqsearchply = __max(ply+1, highestqsearchply);
#endif
		score = -qSearch(-beta, -alpha, ply + 1);
		if (score == -BREAKING)
			return BREAKING;
		mgen.undoMove(theBoard, ml[ply][mit]);
		if (score >= beta)
			return beta;
		if (score > alpha)
		{
			alpha = score;
			copyPV(pv[ply], pv[ply + 1], ml[ply][mit]);
		}
	}
	return alpha;
}

bool Engine::abortCheck()
{
	ENGINECOMMAND cmd;
	ChessBoard cb;
	EngineGo eg;
	EngineEval ev;
	switch (searchtype)
	{
	case NODES_SEARCH:
		if (nodes >= fixedNodes)
		{
			sendBestMove();
			return true;
		}
		break;
	case TIME_SEARCH:
		if (watch.read(WatchPrecision::Microsecond) >= fixedTime)
		{
			sendBestMove();
			return true;
		};
		break;
	case NORMAL_SEARCH:
		if (watch.read(WatchPrecision::Microsecond) >= maxTime)
		{
			sendBestMove();
			return true;
		}
		break;
	}
	while ((cmd = ei->peekOutQue()) != ENG_none)
	{
		switch (cmd)
		{
		case ENG_quit: // Let the comman be in the que.
			return true;
		case ENG_debug:
			debug = true;
			ei->getOutQue();
			break;
		case ENG_nodebug:
			debug = false;
			ei->getOutQue();
			break;
		case ENG_stop:
			ei->getOutQue();
			sendBestMove();
			return true;
		case ENG_clearhistory:
			ei->getOutQue();
			drawTable.clear();
			break;
		case ENG_history:
			ei->getOutQue(cb);
			drawTable.add(cb);
			break;
		case ENG_ponderhit:
			ei->getOutQue();
			maxTime+=watch.read(WatchPrecision::Microsecond);
			searchtype=NORMAL_SEARCH;
			break;
		case ENG_clearhash:
			ei->getOutQue();
			break;
		case ENG_go: // Shouldnt happend here
			ei->getOutQue(eg);
			break;
		case ENG_ponder: // Shouldnt happend here
			ei->getOutQue(eg);
			break;
		case ENG_position:
			ei->getOutQue(cb);
			break;
		case ENG_eval:
			ei->getOutQue(ev);
			if (ev.type == EVAL_contempt)
				contempt = ev.value;
			else if (ev.type == EVAL_pawn)
				eval.pawnValue = ev.value;
			else if (ev.type == EVAL_knight)
				eval.knightValue = ev.value;
			else if (ev.type == EVAL_bishop)
				eval.bishopValue = ev.value;
			else if (ev.type == EVAL_rook)
				eval.rookValue = ev.value;
			else if (ev.type == EVAL_queen)
				eval.queenValue = ev.value;
			else if (ev.type == EVAL_bishoppair)
				eval.bishopValue = ev.value;
			else if (ev.type == EVAL_mobility)
				eval.mobilityScore = ev.value;
			break;
		default:
			// Unknown command, remove it.
			ei->getOutQue();
			break;
		}
	}
	return false;
}

void Engine::sendBestMove()
{
	ei->sendInQue(ENG_string, "bestmove " + theBoard.makeMoveText(bestMove,UCI));
}

void Engine::sendPV(const MoveList& pvline, int depth, int score, int type)
{
	string pvstring = "";
	string s;
	int i = 0;
	ULONGLONG t = watch.read(WatchPrecision::Microsecond);
	double ts = t / 1000000.0;
	tempBoard = theBoard;
	ChessMove m;
	while (i < pvline.size())
	{
		m = pvline[i];
		s = tempBoard.makeMoveText(m,UCI);
		if (!tempBoard.doMove(m, true))
			break;
		pvstring += s;
		pvstring += " ";
		++i;
	}
	pvstring = trim(pvstring);
	t /= 1000; //Use milliseconds in pv
	if (type==lowerbound)
		sprintf_s(sz, 256, "depth %u nps %u score lowerbound cp %i nodes %u time %llu pv %s", depth, (DWORD)(nodes / ts), score, nodes, t, pvstring.c_str());
	else if (type==upperbound)
		sprintf_s(sz, 256, "depth %u nps %u score upperbound cb %i nodes %u time %llu pv %s", depth, (DWORD)(nodes / ts), score, nodes, t, pvstring.c_str());
	else if (score > MATE - 200)
		sprintf_s(sz, 256, "depth %u nps %u score mate %i nodes %u time %llu pv %s", depth, (DWORD)(nodes / ts), (MATE - score)/2+1, nodes, t, pvstring.c_str());
	else if (score < -MATE + 200)
		sprintf_s(sz, 256, "depth %u nps %u score mate %i nodes %u time %llu pv %s", depth, (DWORD)(nodes / ts), (MATE + score)/2, nodes, t, pvstring.c_str());
	else
		sprintf_s(sz, 256, "depth %u nps %u score cp %i nodes %u time %llu pv %s", depth, (DWORD)(nodes / ts), score, nodes, t, pvstring.c_str());
	ei->sendInQue(ENG_info, sz);
}

void Engine::orderRootMoves()
{
	int mit;
	for (mit = 0; mit < ml[0].size(); mit++)
	{
		mgen.doMove(theBoard, ml[0][mit]);
		ml[0][mit].score = -eval.evaluate(theBoard, MATE, -MATE);
		mgen.undoMove(theBoard, ml[0][mit]);
	}
	ml[0].sort();

}

void Engine::orderMoves(MoveList& mlist, const ChessMove& first)
{
	static int seevalue[13][13] = { // [victem][attacker]
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // No capture
		 0,  6,  5,  4,  3,  2,  1,  6,  5,  4,  3,  2,  1,
		 0, 12, 11, 10,  9,  8,  7, 12, 11, 10,  9,  8,  7,
		 0, 18, 17, 16, 15, 14, 13, 18, 17, 16, 15, 14, 13,
		 0, 24, 23, 22, 21, 20, 19, 24, 23, 22, 21, 20, 19,
		 0, 30, 29, 28, 27, 26, 25, 30, 29, 28, 27, 26, 25,
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // King can't be a victem
		 0,  6,  5,  4,  3,  2,  1,  6,  5,  4,  3,  2,  1,
		 0, 12, 11, 10,  9,  8,  7, 12, 11, 10,  9,  8,  7,
		 0, 18, 17, 16, 15, 14, 13, 18, 17, 16, 15, 14, 13,
		 0, 24, 23, 22, 21, 20, 19, 24, 23, 22, 21, 20, 19,
		 0, 30, 29, 28, 27, 26, 25, 30, 29, 28, 27, 26, 25,
		 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 };  // King can't be a victem

	static int promotevalue[13] = { 0,0,1,1,5,30,0,0,1,1,5,30,0 };
	int mit,score;

	if (mlist.size() < 2)
		return;
	
	for (mit = 0; mit < mlist.size(); mit++)
	{
		mlist[mit].score = 0;
		if (mlist[mit].moveType & (CAPTURE | PROMOTE))
		{
			score = 0;
			if (mlist[mit].moveType&CAPTURE)
				score += seevalue[mlist[mit].capturedpiece][theBoard.board[mlist[mit].fromSquare]];
			if (mlist[mit].moveType&PROMOTE)
				score += promotevalue[mlist[mit].promotePiece];
			mlist[mit].score = score;
		}
		else if (mlist[mit].moveType & CASTLE)
		{
			mlist[mit].score = 1;
		}
	}

	mit = mlist.find(first);
	if (mit < mlist.size())
		mlist[mit].score = 1000;
	mlist.sort();
}

void Engine::orderQMoves(MoveList& mlist)
{
	static int seevalue[13][13] = { // [victem][attacker]
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // No capture
		0,  6,  5,  4,  3,  2,  1,  6,  5,  4,  3,  2,  1,
		0, 12, 11, 10,  9,  8,  7, 12, 11, 10,  9,  8,  7,
		0, 18, 17, 16, 15, 14, 13, 18, 17, 16, 15, 14, 13,
		0, 24, 23, 22, 21, 20, 19, 24, 23, 22, 21, 20, 19,
		0, 30, 29, 28, 27, 26, 25, 30, 29, 28, 27, 26, 25,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  // King can't be a victem
		0,  6,  5,  4,  3,  2,  1,  6,  5,  4,  3,  2,  1,
		0, 12, 11, 10,  9,  8,  7, 12, 11, 10,  9,  8,  7,
		0, 18, 17, 16, 15, 14, 13, 18, 17, 16, 15, 14, 13,
		0, 24, 23, 22, 21, 20, 19, 24, 23, 22, 21, 20, 19,
		0, 30, 29, 28, 27, 26, 25, 30, 29, 28, 27, 26, 25,
		0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 };  // King can't be a victem

	static int promotevalue[13] = { 0,0,1,1,5,30,0,0,1,1,5,30,0 };
	int mit, score;

	if (mlist.size() < 2)
		return;

	for (mit = 0; mit < mlist.size(); mit++)
	{
		mlist[mit].score = 0;
		score = 0;
		if (mlist[mit].moveType&CAPTURE)
			score += seevalue[mlist[mit].capturedpiece][theBoard.board[mlist[mit].fromSquare]];
		if (mlist[mit].moveType&PROMOTE)
			score += promotevalue[mlist[mit].promotePiece];
		mlist[mit].score = score;
	}

	mlist.sort();
}

void Engine::copyPV(MoveList& m1, MoveList& m2, ChessMove& m)
{
	m1.clear();
	m1.push_back(m);
	for (int i = 0; i<m2.size(); i++)
		m1.push_back(m2[i]);
};

int Engine::moveExtention(bool inCheck, ChessMove& move, ChessMove& lastmove, int moves)
{
	if (move.toSquare == lastmove.toSquare)
		return 1;
	if (inCheck)
		return 1;
	if (move.moveType&PAWNMOVE)
		if ((move.toSquare > h6) || (move.toSquare < a3))
		return 1;
	if (moves == 1)
		return 1;
	return 0;
}
