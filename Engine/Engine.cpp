#include <process.h>
#include <string>
#include "Engine.h"
#include "EngineInterface.h"

using namespace std;
const int BREAKING = MATE + 400;
const int MAX_DEPTH = 100;

Engine eng;
char sz[256];
ChessMove emptyMove;

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
				eng.fixedTime = eg.fixedTime;
				eng.maxTime = eg.maxTime;
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
				eng.fixedTime = eg.fixedTime;
				eng.maxTime = eg.maxTime;
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
	strength = 10000; // Strength defaults to 100%
	debug = false;
	contempt = 0;
}

void Engine::startSearch()
{
	int i;
	bool inCheck;
	HASHKEY hashKey;
	nodes = 0;
	bestMove.clear();
	eval.rootcolor = theBoard.toMove;
	eval.drawscore[eval.rootcolor] = -contempt;
	eval.drawscore[OTHERPLAYER(eval.rootcolor)] = contempt;

	inCheck = mgen.inCheck(theBoard, theBoard.toMove);
	hashKey = theBoard.hashkey();

	// Clear pv
	for (i = 0; i<MAX_PLY; i++)
		pv[i].clear();

	interativeSearch(inCheck, hashKey);
}

void Engine::interativeSearch(bool inCheck, HASHKEY hashKey)
{
	int depth=1;
	int score=-MATE;
	for (depth = 1; depth < MAX_DEPTH; depth++)
	{
		sprintf_s(sz, 256, "depth %i", depth);
		ei->sendInQue(ENG_info, sz);
		score = aspirationSearch(depth, score, inCheck, hashKey);
		if (score == BREAKING)
			return;
		if (searchtype == DEPTH_SEARCH)
		{
			if (depth == fixedDepth)
			{
				sendBestMove();
				return;
			}
		}
	}
}

int Engine::aspirationSearch(int depth, int bestscore, bool inCheck, HASHKEY hashKey)
{
	int alpha=-MATE;
	int beta=MATE;
	return rootSearch(depth, alpha, beta, inCheck, hashKey);
}

int Engine::rootSearch(int depth, int alpha, int beta, bool inCheck, HASHKEY hashKey)
{
	int score;
	HASHKEY newkey;
	++nodes;
	mgen.makeMoves(theBoard, ml[0]);
	if (!ml[0].size)
	{
		if (!inCheck) // Stalemate
			alpha = 0;
		else
			alpha = -MATE;
		ei->sendInQue(ENG_info, string("string No legal moves, aborting search."));
		return alpha;
	}

	// Order moves
	if (depth == 1)
	{
		orderMoves(ml[0],emptyMove);
		ml[0].list[0].score = watch.read();
		bestMove.push_back(ml[0].list[0]);
	}
	else
	{
		orderMoves(ml[0],bestMove.back());
	}

	// Add the root position to the drawtable
	hashDrawTable.add(hashKey, 0);

	int mit;
	for (mit = 0; mit < ml[0].size; mit++)
	{
		// Send UCI info
		sprintf_s(sz, 256, "currmove %s currmovenumber %i", theBoard.uciMoveText(ml[0].list[mit]).c_str(), mit+1);
		ei->sendInQue(ENG_info, sz);
		newkey = theBoard.newHashkey(ml[0].list[mit], hashKey);
		mgen.doMove(theBoard, ml[0].list[mit]);
		inCheck = mgen.inCheck(theBoard, theBoard.toMove);
		score = -Search(depth - 1, -beta, -alpha, inCheck, newkey, 1, true);
		if (score == -BREAKING)
			return BREAKING;
		mgen.undoMove(theBoard, ml[0].list[mit]);
		if (score >= beta)
			return beta;
		if (score > alpha)
		{
			ml[0].list[mit].score = score;
			copyPV(pv[0], pv[1], ml[0].list[mit]);
			sendPV(pv[0]);

			alpha = score;

			ml[0].list[mit].score = watch.read();
			bestMove.push_back(ml[0].list[mit]);
		}
	}
	return alpha;
}

int Engine::Search(int depth, int alpha, int beta, bool inCheck, HASHKEY hashKey, int ply, bool followPV)
{
	int score;
	HASHKEY newkey;
	if (depth == 0)
		return qSearch(alpha, beta, ply);

	pv[ply].clear();

	if (!(++nodes % 0x400))
		if (abortCheck())
			return BREAKING;

	if (drawTable.exist(theBoard, hashKey) || hashDrawTable.exist(hashKey, ply))
		return (eval.drawscore[theBoard.toMove]);

	if (ply >= MAX_PLY)
		return eval.evaluate(theBoard, alpha, beta);

	// Add position to the drawtable
	hashDrawTable.add(hashKey, ply);

	mgen.makeMoves(theBoard, ml[ply]);
	orderMoves(ml[ply], followPV?pv[ply].front():emptyMove);
	int mit;
	for (mit = 0; mit < ml[ply].size; mit++)
	{
		newkey = theBoard.newHashkey(ml[0].list[mit], hashKey);
		mgen.doMove(theBoard, ml[ply].list[mit]);
		inCheck = mgen.inCheck(theBoard, theBoard.toMove);
		score = -Search(depth - 1, -beta, -alpha, inCheck, newkey, ply+1, followPV);
		if (score == -BREAKING)
			return BREAKING;
		mgen.undoMove(theBoard, ml[ply].list[mit]);
		if (score >= beta)
			return beta;
		if (score > alpha)
		{
			alpha = score;
			copyPV(pv[ply], pv[ply + 1], ml[ply].list[mit]);
		}
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

	if (!(++nodes % 0x400))
		if (abortCheck())
			return BREAKING;

	score = eval.evaluate(theBoard, alpha, beta);
	if (score >= beta)
		return beta;
	if (score > alpha)
		alpha = score;

	mgen.makeCaptureMoves(theBoard, ml[ply]);
	int mit;
	for (mit = 0; mit < ml[ply].size; mit++)
	{
		mgen.doMove(theBoard, ml[ply].list[mit]);
		score = -qSearch(-beta, -alpha, ply+1);
		if (score == -BREAKING)
			return BREAKING;
		mgen.undoMove(theBoard, ml[ply].list[mit]);
		if (score >= beta)
			return beta;
		if (score > alpha)
			alpha = score;
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
		if (watch.read() >= fixedTime)
		{
			sendBestMove();
			return true;
		};
		break;
	case NORMAL_SEARCH:
		if (watch.read() >= maxTime)
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
			maxTime+=watch.read();
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
	string s;

	if (searchtype != NORMAL_SEARCH)
	{
		ei->sendInQue(ENG_string, "bestmove " + theBoard.uciMoveText(bestMove.back()));
		return;
	}
	// Full strength
	if (strength == 10000)
	{
		ei->sendInQue(ENG_string, "bestmove " + theBoard.uciMoveText(bestMove.back()));
		return;
	}
	DWORD dw = (DWORD)((DOUBLE)watch.read()*((double)strength/10000));
	int mit;
	for (mit = 0; mit < bestMove.size; mit++)
	{
		if ((DWORD)bestMove.list[mit].score > dw)
			break;
	}
	--mit;
	if (mit < 0)
		mit = 0;
	ei->sendInQue(ENG_string, "bestmove " + theBoard.uciMoveText(bestMove.list[mit]));
}

void Engine::sendPV(const MoveList& pvline)
{
	string s="";
	int i=0;
	while (i < pvline.size)
	{ 
		s+=theBoard.uciMoveText(pvline.list[i]);
		++i;
		if (i < pvline.size)
			s += " ";
	}
	sprintf_s(sz, 256, "score cp %i nodes %u time %u pv %s", pvline.list[0].score, nodes, watch.read(), s.c_str());
	ei->sendInQue(ENG_info, sz);
}

void Engine::orderMoves(MoveList& mlist, const ChessMove& first)
{
	int victem[] = { 0,100,300,300,500,9000,0,100,300,300,500,9000,0 };
	int attacker[] = { 0,1,2,3,4,5,6,1,2,3,4,5,6 };
	int promotion[] = { 0,0,1,1,2,3,0,0,1,1,2,3,0 };
	int seevalue[] = { 0,1,3,3,5,9,0,1,3,3,5,9,0 };
	int i,j;

	if (mlist.size < 2)
		return;
	
	for (i = 0; i < mlist.size; i++)
	{
		mlist.list[i].score = 0;
		if (mlist.list[i].moveType & (CAPTURE | PROMOTE))
		{
			j = seevalue[theBoard.board[mlist.list[i].fromSquare]];
			if (mlist.list[i].moveType&CAPTURE)
				j += seevalue[mlist.list[i].capturedpiece];
			if (mlist.list[i].moveType&PROMOTE)
				j += seevalue[mlist.list[i].promotePiece];
			j += 2000000000;
			mlist.list[i].score = j;
		}
		else if (mlist.list[i].moveType & CASTLE)
		{
			mlist.list[i].score = 1999999999;
		}
	}

	i=mlist.find(first);
	if (i < mlist.size)
		mlist.list[i].score = 0x7fffffff;

	mlist.sort();
}