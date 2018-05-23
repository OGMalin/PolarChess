#pragma once

#include "ChessMove.h"

const int MOVELISTSIZE=1024;

class MoveList
{
public:
  ChessMove nomove;
  static int compareMove(const void* m1, const void* m2);
  int compareMove(int m1, int m2);
  ChessMove list[MOVELISTSIZE];
  int size; // pointer to next avaiable element
  MoveList();
  virtual ~MoveList();
  void clear();
  int begin();
  int end();
  bool empty();
  void copy(MoveList& ml);
  ChessMove& at(int n);
  void push_back(const ChessMove& m);
  void push_front(const ChessMove& m);
  // Find the index to a move in the list. Returns size if the move isn't in the list.
  int find(const ChessMove& m);
  void erase(int n, int count = 1);
  MoveList& operator=(const MoveList& ml);
  void swap(int m1, int m2);
  void sort(int start = 0, int end = -1);
  void quicksort(int start, int end);
  void bubblesort(int start, int end);
  const ChessMove& front();
  const ChessMove& back();
  void trunc();
  bool exist(const ChessMove& m);
  ChessMove& next(int movenr);
};
