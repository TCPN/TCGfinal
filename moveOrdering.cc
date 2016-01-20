
#include<cstdio>
#include<cstdlib>
#include"anqi.hh"
#include"Protocol.h"
#include"ClientSocket.h"

#ifdef _WINDOWS
#include<windows.h>
#else
#include<ctime>
#endif

void getMoveScore(BOARD B, MOVLST lst, int mvscore[68], int hashhit)
{
	// adjust move ordering
	// give score
	// TODO: add hash table result
	if(hashhit) mvscore[0] = 10000;
	for(int i = 0; i < lst.num; i ++)
	{
		POS p = lst.mov[i].st, q = lst.mov[i].ed;
		FIN pf = B.fin[p], qf = B.fin[q];
		//assert(pf != FIN_E && pf != FIN_X);
		LVL pl = GetLevel(pf);
		if(qf != FIN_E)
		{
			// capturing
			LVL ql = GetLevel(qf);
			mvscore[i] += (1000 + ((int)(7-ql)) * 100 + pl); // eat big, use small
			if(ql == LVL_C)mvscore[i] += 100;
			if(ql == LVL_N)mvscore[i] -= 100;
			if(pl == LVL_C)mvscore[i] -= 1;
			if(pl == LVL_N)mvscore[i] += 1;
		}
		else
		{
			// move
			mvscore[i] += ((int)(7-pl)); // bigger is better
		}
		if(true)// avoid capture
		{
			;
		}
		
	}
}
void sortMoveByScore(MOVLST &lst, int mvscore[68])
{
	// sort moves: merge sort
	#define DIVIDE 0
	#define MERGE  1
	int sh = 0;							// stack head
	char stk[10][3] = {{DIVIDE,0,lst.num}};  // stack : [0]=DIVIDE/MERGE [1]=arrayhead [2]=length
	int head, len, lh, ll, rh, rl, ml, useleft;
	int scbuf[68];
	MOV movbuf[68];
	while(sh >= 0) {
		head = stk[sh][1];
		len = stk[sh][2];
		if(len <= 1) { sh--; continue; }
		if(stk[sh][0] == DIVIDE) {
			stk[sh][0] = MERGE;
			sh++;
			stk[sh][0] = DIVIDE;
			stk[sh][1] = head;
			stk[sh][2] = len/2;
			sh++;
			stk[sh][0] = DIVIDE;
			stk[sh][1] = head + (len/2);
			stk[sh][2] = len - (len/2);
		}
		else {
			lh = head;
			ll = len/2;
			rh = head + (len/2);
			rl = len - (len/2);
			ml = 0;
			while(ll > 0 || rl > 0) {
				if(ll == 0)							useleft = 0;
				else if (rl == 0)					useleft = 1;
				else if(mvscore[lh] >= mvscore[rh])	useleft = 1;
				else								useleft = 0;
				if(useleft) {
					movbuf[ml] = lst.mov[lh];
					scbuf[ml] = mvscore[lh];
					lh++; ll--; ml++;
				} else {
					movbuf[ml] = lst.mov[rh];
					scbuf[ml] = mvscore[rh];
					rh++; rl--; ml++;
				}
			}
			for(int i = 0; i < len; i ++) {
				lst.mov[head+i] = movbuf[i];
				mvscore[head+i] = scbuf[i];
			}
			sh--;
		}
	}
	#undef DIVIDE
	#undef MERGE
}

