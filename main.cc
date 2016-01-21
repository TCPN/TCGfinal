/*****************************************************************************\
 * Theory of Computer Games: Fall 2012
 * Chinese Dark Chess Search Engine Template by You-cheng Syu
 *
 * This file may not be used out of the class unless asking
 * for permission first.
 *
 * Modify by Hung-Jui Chang, December 2013
\*****************************************************************************/
#include<cstdio>
#include<cstdlib>
#include"anqi.hh"
#include"Protocol.h"
#include"ClientSocket.h"
#include <cassert>

#ifdef _WINDOWS
#include<windows.h>
#include<ctime>
#else
#include<ctime>
#endif

#include "moveOrdering.cc"

char myversion[1024] = "Eval with fixed material Point v1.0";
FILE* dbgmsgf;

const int DEFAULTTIME = 15;
typedef  int SCORE;
static const SCORE INF=1000001;
static const SCORE WIN=1000000;
SCORE SearchMax(const BOARD&,int,int);
SCORE SearchMin(const BOARD&,int,int);

#ifdef _WINDOWS
DWORD Tick;     // 開始時刻
int   TimeOut;  // 時限
#else
clock_t Tick;     // 開始時刻
clock_t TimeOut;  // 時限
#endif
MOV   BestMove; // 搜出來的最佳著法

#include "hashTable.cc"

bool TimesUp() {
#ifdef _WINDOWS
	return GetTickCount()-Tick>=TimeOut;
#else
	return clock() - Tick > TimeOut;
#endif
}

static const POS ADJ[32][4]={
	{ 1,-1,-1, 4},{ 2,-1, 0, 5},{ 3,-1, 1, 6},{-1,-1, 2, 7},
	{ 5, 0,-1, 8},{ 6, 1, 4, 9},{ 7, 2, 5,10},{-1, 3, 6,11},
	{ 9, 4,-1,12},{10, 5, 8,13},{11, 6, 9,14},{-1, 7,10,15},
	{13, 8,-1,16},{14, 9,12,17},{15,10,13,18},{-1,11,14,19},
	{17,12,-1,20},{18,13,16,21},{19,14,17,22},{-1,15,18,23},
	{21,16,-1,24},{22,17,20,25},{23,18,21,26},{-1,19,22,27},
	{25,20,-1,28},{26,21,24,29},{27,22,25,30},{-1,23,26,31},
	{29,24,-1,-1},{30,25,28,-1},{31,26,29,-1},{-1,27,30,-1}
};

/*
// 一個重量不重質的審局函數
SCORE Eval(const BOARD &B) {
	int cnt[2]={0,0};
	for(POS p=0;p<32;p++){const CLR c=GetColor(B.fin[p]);if(c!=-1)cnt[c]++;}
	for(int i=0;i<14;i++)cnt[GetColor(FIN(i))]+=B.cnt[i];
	return cnt[B.who]-cnt[B.who^1];
}
*/

int MDist(POS a, POS b) {
	if(a < 0 || b < 0)
		return -1;
	return (abs((int)((a/4) - (b/4))) + abs((int)((a%4) - (b%4))));
}
// DONETODO: LV2 注意循環盤面，若可以勝則避免和棋
SCORE Eval(const BOARD &B) {
	SCORE score[2] = {0,0};
	int count[2][7] = {{0}};
	int sum[2] = {0};
	POS pos[2][7][5];
	memset(&pos, -1, sizeof(POS)*2*7*5);
	FIN mf = FIN_X, hf = FIN_X;
	POS mp = -1, hp = -1;
	for(POS p = 0; p < 32; p++)
	{
		if(B.fin[p] < FIN_X)
		{
			const CLR c = GetColor(B.fin[p]);
			const LVL l = GetLevel(B.fin[p]);
	// TODO: try to dynamicly adjust
			SCORE level_score[7] = {6540,3270,1640,870,390,770,100};
			score[c] += level_score[l];
			pos[c][l][count[c][l]++] = p;
			sum[c]++;
			if(c == B.who)
				mp = p;
			else if(c == (B.who^1))
				hp = p;
		}
	}
	for(int i = 0; i < 14; i++)
	{
	// TODO: fix this, it's wrong
		score[GetColor(FIN(i))] += (int)(B.cnt[i] * 0.9);
		//count[GetColor(i)][GetLevel(i)] += (int)B.cnt[i];
		sum[GetColor(FIN(i))] += (int)B.cnt[i];
	}
	
	if(sum[0] == 1 || sum[1] == 1)
	{
		// assume no chess is FIN_X now
		if(sum[B.who] == 1)
			for(int i = 0; i < 7; i++)
				if(count[B.who][i])
					mf = FIN(i + B.who * 7);
		if(sum[B.who^1] == 1)
			for(int i = 0; i < 7; i++)
				if(count[B.who^1][i])
					hf = FIN(i + (B.who^1) * 7);
			
		if(sum[0] == 1 && sum[1] == 1)
		{
			if( GetLevel(mf) != LVL_C
			&& ChkEats(B.fin[mp],B.fin[hp])
			&& (MDist(mp, hp) % 2 == 1))
			{
				SCORE distcost = 0;
				for(int j = 0; j < 4; j ++)
				{
					if(ADJ[hp][j] != -1)
						distcost += MDist(mp, ADJ[hp][j]);
				}
				return (WIN-distcost-100);
			}
			else if(GetLevel(hf) != LVL_C 
			&& ChkEats(B.fin[hp],B.fin[mp])
			&& (MDist(hp, mp) % 2 == 0))
			{
				SCORE distcost = 0;
				for(int j = 0; j < 4; j ++)
				{
					if(ADJ[hp][j] != -1)
						distcost += MDist(mp, ADJ[hp][j]); // ?????
				}
				return (-WIN+distcost+100);
			}
			else
				return 0; //draw
		}
		else if(sum[B.who] == 1)
		{
			int i,j;
			for(i = 0; i < 7; i ++)
				for(j = 0; j < count[B.who^1][i]; j ++)
				{
					hp = pos[B.who^1][i][j];
					if(GetLevel(B.fin[hp]) != LVL_C && ChkEats(B.fin[hp],B.fin[mp]))
						return -WIN+MDist(mp,hp)-1;
				}
			return 0; //draw
		}
		else if(sum[B.who^1] == 1)
		{
			int i,j;
			for(i = 0; i < 7; i ++)
				for(j = 0; j < count[B.who][i]; j ++)
				{
					mp = pos[B.who][i][j];
					if(GetLevel(B.fin[mp]) != LVL_C && ChkEats(B.fin[mp],B.fin[hp]))
						return WIN-MDist(mp,hp)+1;
				}
			return 0; //draw
		}
	}
	return score[B.who]-score[B.who^1];
}

// dep=現在在第幾層
// cut=還要再走幾層
SCORE SearchMax(const BOARD &B,int dep,int cut) {
	if(B.ChkLose())return -WIN;

	MOVLST lst;
	if(cut==0||TimesUp()||B.MoveGen(lst)==0)return +Eval(B);

	SCORE ret=-INF;
	for(int i=0;i<lst.num;i++) {
		BOARD N(B);
		N.Move(lst.mov[i]);
		const SCORE tmp=SearchMin(N,dep+1,cut-1);
		if(tmp>ret){ret=tmp;if(dep==0)BestMove=lst.mov[i];}
	}
	return ret;
}

SCORE SearchMin(const BOARD &B,int dep,int cut) {
	if(B.ChkLose())return +WIN;

	MOVLST lst;
	if(cut==0||TimesUp()||B.MoveGen(lst)==0)return -Eval(B);

	SCORE ret=+INF;
	for(int i=0;i<lst.num;i++) {
		BOARD N(B);
		N.Move(lst.mov[i]);
		const SCORE tmp=SearchMax(N,dep+1,cut-1);
		if(tmp<ret){ret=tmp;}
	}
	return ret;
}

char movstr[6];
char* mov2str(MOV m) {
	
	sprintf(movstr, "%c%c-%c%c",(m.st%4)+'a', m.st/4+'1',(m.ed%4)+'a', m.ed/4+'1');
	return movstr;
}

#define max(a,b) ((a)>(b)?(a):(b))
int ChkCycle(int histLen, HASHKEY history[500])
{
	// TODO: use a string matching algorithm to make this more efficient
	int maxrepeat = 0;
	int now = histLen - 1;
	for(int cyclelen = 4; cyclelen <= 20; cyclelen += 4)
	{
		for(int r = 1; r <= 2; r ++)
		{
			int completeSame = 1;
			for(int i = 0; i < cyclelen; i ++)
			{
				if(history[now - i] != history[now - i - (cyclelen * r)])
				{
					completeSame = 0;
					break;
				}
			}
			if(completeSame)
				maxrepeat = max(maxrepeat,r+1);
			else
				break;
		}
	}
	return maxrepeat;
}
static const char *tbl="KGMRNCPkgmrncpX-";
char bo[64] = {};
char * board2str(const BOARD &B)
{
	for(int i=0;i<32;i++)
		bo[i] = tbl[B.fin[i]];
	return bo;
}

bool completeSearch;
int maxPossibleDepth;

SCORE NegaScoutSearch(const BOARD &B, SCORE alpha, SCORE beta, int dep, int depLmt, HASHKEY h
, int histLen, HASHKEY history[500])
{
	maxPossibleDepth=max(maxPossibleDepth,dep);
	fprintf(dbgmsgf,"%*sD%d %d ",dep,"",dep,__LINE__);
	fprintf(dbgmsgf,"%s ",board2str(B));
//fprintf(stderr,"%*sD%d %d ",dep,"",dep,__LINE__);
//fprintf(dbgmsgf,"%llu ",h);
	if(B.ChkLose())
	{
		fprintf(dbgmsgf,"%d:Lose\n",__LINE__);
		return -WIN;
	}
	
	MOVLST lst;
	B.MoveGen(lst);
	MOV bestChildmov;
	int hashhit = 0;
	HENTRY *entry = NULL;
	if((B.who == 0 || B.who == 1) && ((entry = &(hashtbl[B.who][h % hashTblN]))->key == h))
	{
		for(int i = 0; i < lst.num; i++)
			if(entry->bestchild == lst.mov[i])
			{
				bestChildmov = entry->bestchild;
				lst.mov[i] = lst.mov[0];
				lst.mov[0] = entry->bestchild;
				hashhit = 1;
				break;
			}
		if(!hashhit)
		{
			fprintf(dbgmsgf,"HASH KEY COLLISION!!!\n");
			if(myversion[0] != '!')
			{
				sprintf(myversion,"!HASH KEY COLLISION! %s",myversion);
			}
		}
		fprintf(dbgmsgf,"hashhit sdep:%d %s %d %s ",entry->searchDep,entry->exact?"ex":"bn",entry->score,mov2str(entry->bestchild));
		if(hashhit && entry->exact && ((depLmt - dep) <= entry->searchDep))
		{
			fprintf(dbgmsgf,"%d RECdep:%d, dep%d ",__LINE__,entry->searchDep,depLmt - dep);
			if(dep == 0)
			{
				fprintf(dbgmsgf,"%d ",__LINE__);
				BestMove = entry->bestchild;
			}
			if(ChkCycle(histLen,history) == 2) // or 2
			{
				entry->score = 0;
			}
			else
			{
				fprintf(dbgmsgf,"%d ",__LINE__);
				fprintf(dbgmsgf,"score:%d ",entry->score);
				fprintf(dbgmsgf,"%s /\n",mov2str(entry->bestchild));
				return entry->score;
			}
		}
	}
	
	if(ChkCycle(histLen,history) == 2) // or 2
	{
		fprintf(dbgmsgf,"%d:Cycled ",__LINE__);
		if(hashhit)
			entry->score = 0;
		if(dep != 0)
		{
			fprintf(dbgmsgf,"%d /\n",__LINE__);
			return 0; // draw, because position cycle
		}
	}
	
	// TODO: LV2 使用知識
	// TODO: adjust move order
	if(depLmt == dep 			// remaining search depth
	|| TimesUp()
	|| lst.num == 0)			// a terminal node(regardless flipping chesses)
	{
		fprintf(dbgmsgf,"%d %s%s%s",__LINE__,
		depLmt == dep?"Depth Limit Reach ":"",
		TimesUp()?"Time Up ":"",
		lst.num == 0?"No Move ":"");
		fprintf(dbgmsgf,"eval:%d /\n",+Eval(B));
		
		if(lst.num!=0){
			completeSearch = false;
		}
		
		return +Eval(B);
	}
	
	// adjust move ordering
	// give score
	int mvscore[68] = {0};
	getMoveScore(B, lst, mvscore, hashhit);
//for(int i=0;i<lst.num;i++)fprintf(dbgmsgf, "%d: %s(%d)\n", i, mov2str(lst.mov[i]),mvscore[i]);
	// sort moves: merge sort
	sortMoveByScore(lst, mvscore);
	
	//debug message
	fprintf(dbgmsgf, "Eval(now):%d\n",Eval(B));
	for(int i=0;i<lst.num;i++)
	{
		BOARD T(B);
		fprintf(dbgmsgf, "%*sD%d ", dep,"",dep);
		T.DoMove(lst.mov[i],FIN_X);
		fprintf(dbgmsgf, "%d: %s %d\n", i, mov2str(lst.mov[i]),-Eval(T));
	}
//scanf("%*s");

	SCORE m = (hashhit ? entry->score : -INF);	// the current lower bound; fail soft
	SCORE n = beta;								// the current upper bound
	SCORE t;
	SCORE rec[lst.num];
	fprintf(dbgmsgf,"%*sD%d %sm:%d beta:%d\n",dep,"",dep,(hashhit?"HASHHIT ":""),m,beta);
	for(int i = 0; i < lst.num; i++)
	{
		if(i == 0)
		{
			bestChildmov = lst.mov[i];
			if(dep == 0)
				BestMove = lst.mov[i];
		}
		fprintf(dbgmsgf,"%*sD%d %d ",dep,"",dep,__LINE__);
		fprintf(dbgmsgf,"%s ",mov2str(lst.mov[i]));
		BOARD N(B);
		HASHKEY nh = hashmove(h,N,lst.mov[i],FIN_X);
		N.Move(lst.mov[i]);		// get the next position, BORAD.Move should not used for FLIP
		//assert(N.who != B.who);
if(nh != hash(N))
{
MOV m = lst.mov[i];
fprintf(dbgmsgf, "\n");
fprintf(dbgmsgf, "B  %llu\n", hash(B));
fprintf(dbgmsgf, "h  %llu\n", h);
fprintf(dbgmsgf, "s  %llu\n", hashfinpos[B.fin[m.st]][m.st]);
fprintf(dbgmsgf, "s' %llu\n", hashfinpos[FIN_E][m.st]);
fprintf(dbgmsgf, "e  %llu\n", hashfinpos[B.fin[m.ed]][m.ed]);
fprintf(dbgmsgf, "e' %llu\n", hashfinpos[B.fin[m.st]][m.ed]);
fprintf(dbgmsgf, "N  %llu\n", hash(N));
fprintf(dbgmsgf, "h^ %llu\n", h ^ hashfinpos[B.fin[m.st]][m.st] ^ hashfinpos[FIN_E][m.st]^ hashfinpos[B.fin[m.ed]][m.ed] ^ hashfinpos[B.fin[m.st]][m.ed]);
fprintf(dbgmsgf, "nh %llu\n", nh);
fprintf(dbgmsgf, "%d %d\n", B.fin[m.st], B.fin[m.ed]);
fprintf(dbgmsgf, "%d %d\n", N.fin[m.st], N.fin[m.ed]);
BOARD Z(B);
Z.Move(lst.mov[i]);
fprintf(dbgmsgf, "%d %d\n", Z.fin[m.st], Z.fin[m.ed]);
assert(nh == hash(N));
}
		// null window search; TEST in SCOUT
		history[histLen] = nh;
		fprintf(dbgmsgf,"%d:TEST>deeper\n",__LINE__);
		t = -NegaScoutSearch( N, -n, -max(alpha,m), dep+1, depLmt, nh, histLen+1,history);
#ifdef _TEST
//scanf("%*s");
#else

#endif
		fprintf(dbgmsgf,"%*sD%d %d ",dep,"",dep,__LINE__);
		fprintf(dbgmsgf,"\\back t:%d m:%d ",t,m);
		fprintf(dbgmsgf,"lst.num:%d ",lst.num);
//if(i==0)assert(t>m&&n==beta);
		if(t > m)
		{
			fprintf(dbgmsgf,"%d(t>m) ",__LINE__);
			if(n == beta || depLmt - dep < 3 || t >= beta)
			{
				m = t;
				bestChildmov = lst.mov[i];
				fprintf(dbgmsgf,"score:%d ", m);
			}
			else
			{
				// re-search
				fprintf(dbgmsgf,"%d:re-search>deeper",__LINE__);
				fprintf(dbgmsgf,"%s \\\n",mov2str(lst.mov[i]));
				m = -NegaScoutSearch( N, -beta, -t, dep+1, depLmt, nh, histLen+1,history);
#ifdef _TEST
//scanf("%*s");
#else

#endif
				fprintf(dbgmsgf,"%*sD%d %d ",dep,"",dep,__LINE__);
				fprintf(dbgmsgf,"score:%d ", m);
				bestChildmov = lst.mov[i];
			}
			fprintf(dbgmsgf,"%s ",mov2str(lst.mov[i]));
			if(dep == 0)
				BestMove = lst.mov[i];
		}
		fprintf(dbgmsgf,"%d ",__LINE__);
		rec[i] = m;
		if(m >= beta)
		{
			fprintf(dbgmsgf,"%d(m>=beta) ",__LINE__);
	// update hash table
	// assume key won't == 0
	// TODO: maybe use key == -1 as a mark for an empty hash entry
			if(entry != NULL
			&& (entry->key != h 
				|| (depLmt - dep) > entry->searchDep
				|| ((depLmt - dep) == entry->searchDep && !(entry->exact))))
			{
				fprintf(dbgmsgf,"%d update hash ",__LINE__);
				entry->key = h;
				entry->searchDep = depLmt - dep;
				entry->exact = 0;
				entry->score = m;
				entry->bestchild = lst.mov[i];
			}
//for(int z=0;z<=i;z++)fprintf(stderr, "%*sD%d %d:%s %d\n",dep,"",dep,z,mov2str(lst.mov[z]),m);
//scanf("%*s");
			fprintf(dbgmsgf,"%d ",__LINE__);
			fprintf(dbgmsgf,"score:%d ", m);
			fprintf(dbgmsgf,"%s\n",mov2str(lst.mov[i]));
			return m;			// beta cut off
		}
		n = max(alpha,m) + 1;	// set up a null window
		fprintf(dbgmsgf,"%d next move\n",__LINE__);
	}
	fprintf(dbgmsgf,"%*sD%d %d ",dep,"",dep,__LINE__);
	// update hash table
	// assume key won't == 0
	// TODO: maybe use key == -1 as a mark for an empty hash entry
	if((entry != NULL)
	&& ((entry->key != h )
		|| ((depLmt - dep) >= entry->searchDep)))
	{
		fprintf(dbgmsgf,"%d update hash exact ",__LINE__);
		entry->key = h;
		entry->searchDep = depLmt - dep;
		entry->exact = 1;
		entry->score = m;
		entry->bestchild = bestChildmov;
	}
//for(int z=0;z<lst.num;z++)fprintf(stderr, "%*sD%d %d:%s %d\n",dep,"",dep,z,mov2str(lst.mov[z]),m);
//scanf("%*s");
	fprintf(dbgmsgf,"%d ",__LINE__);
	fprintf(dbgmsgf,"score:%d ", m);
	fprintf(dbgmsgf,"%s\n",mov2str(bestChildmov));
	return m;
}// End of NegaScoutSearch

MOV Play(const BOARD &B, int histLen, HASHKEY history[500]) {
	fprintf(dbgmsgf,"%d Play histlen:%d ",__LINE__, histLen);
#ifdef _WINDOWS
	Tick=GetTickCount();
	// TODO: LV2 決定每一步的時限
	TimeOut = (DEFAULTTIME-3)*1000;
#else
	Tick=clock();
	TimeOut = (DEFAULTTIME-3)*CLOCKS_PER_SEC;
#endif
	POS p; int c=0;

	// 新遊戲？隨機翻子
	// HALFDONETODO: LV1 detect 目前是第一手或第二手，使用預設的策略
	// HALFDONETODO: LV1 有策略的翻
	if(histLen == 1 || B.who==-1)
	{
		fprintf(dbgmsgf,"%d histlen=1 strategy ",__LINE__);
		/*
		p=rand()%32;
		printf("%d\n",p);
		*/
		p=rand()%4;
		p=p+5+(p>=2?2:0);
		return MOV(p,p); // DONETODO: random it
	}
	else if(histLen == 2)
	{
		fprintf(dbgmsgf,"%d histlen=2 strategy ",__LINE__);
		for(p=0;p<32;p++) {
			if(B.fin[p]!=FIN_X) {
				if(GetLevel(B.fin[p])<LVL_C) {
					POS np = p;
					if(p%4<2)	np+=2;
					else		np-=2;
					return MOV(np,np);
				}
				else
				{
					do{
						int i = rand()%4;
						if(ADJ[p][i] != -1)
							return MOV(ADJ[p][i],ADJ[p][i]);
					}while(1);
				}
			}
		}
	}
	
	// TODO: LV5 偵測連續未翻子或吃子的次數
	// HALFDONETODO: LV4 偵測循環盤面
	// 若搜出來的結果會比現在好就用搜出來的走法
	// TODO: LV4 把HASHKEY打包進BOARD，避免重複計算、傳遞
	BestMove = MOV(-1,-1);
	
	fprintf(dbgmsgf,"%d ",__LINE__);
	int depthLimit = 2;
	SCORE scoutscore;
	SCORE nowscore = Eval(B);
	//HASHKEY h = hash(B);
	if(histLen < 1)
	{
		fprintf(dbgmsgf, "HISTORY ERROR!!!\n",depthLimit);
		histLen = 1;
		history[0] = hash(B);
	}
	MOVLST lst;
	B.MoveGen(lst);
	int iterDepthLimit = 20;
	if(lst.num > 0){
		// TODO: quiecent search
		fprintf(dbgmsgf, "DEPLMT: %d:\n",depthLimit);
		completeSearch = true;
		maxPossibleDepth = 0;
		scoutscore = NegaScoutSearch(B,-INF,INF,0,depthLimit,history[histLen-1],histLen,history);
		if(completeSearch)
			iterDepthLimit = maxPossibleDepth;
		depthLimit++;
		
		fprintf(dbgmsgf, "end of DEPLMT: %d BestMove:%s\n",depthLimit-1,mov2str(BestMove));
		SCORE best = scoutscore, thresh = 1000;
		while(depthLimit <= iterDepthLimit)
		{
			completeSearch = true;
			fprintf(dbgmsgf, "DEPLMT: %d: try thresh\n",depthLimit);
			scoutscore = NegaScoutSearch(B,best-thresh,best+thresh,0,depthLimit,history[histLen-1],histLen,history);
			if(scoutscore >= best+thresh) {
				fprintf(dbgmsgf, "DEPLMT: %d: fail high\n",depthLimit);
				scoutscore = NegaScoutSearch(B,scoutscore,+INF,0,depthLimit,history[histLen-1],histLen,history);
			}
			else if(scoutscore <= best-thresh) {
				fprintf(dbgmsgf, "DEPLMT: %d: fail low\n",depthLimit);
				scoutscore = NegaScoutSearch(B,-INF,scoutscore,0,depthLimit,history[histLen-1],histLen,history);
			}
			fprintf(dbgmsgf, "DEPLMT: %d end BestMove:%s\n",depthLimit,mov2str(BestMove));
			if(completeSearch)
				iterDepthLimit = maxPossibleDepth;
			best = scoutscore;
			if(TimesUp())
				break;
			depthLimit++;
		}
		fprintf(dbgmsgf, "\n");
	}
	if(BestMove.st < 0 || BestMove.ed < 0 || BestMove.st >= 32 || BestMove.ed >= 32) // no move
	{
		fprintf(dbgmsgf,"%d ",__LINE__);
		// 翻棋
		for(p=0;p<32;p++){if(B.fin[p]==FIN_X)c++;}
		c=rand()%c;
		for(p=0;p<32;p++){if((B.fin[p]==FIN_X)&&(--c<0))break;}
		fprintf(dbgmsgf,"%d no legal move, flip decide",__LINE__);
		return MOV(p,p);
	}
		fprintf(dbgmsgf,"now:%d moved:%d ",nowscore, scoutscore);
	if(scoutscore > nowscore)
	{
		fprintf(dbgmsgf,"%d better if move, decide ",__LINE__);
		return BestMove;
	}
	fprintf(dbgmsgf,"%d ",__LINE__);
	// if(SearchMax(B,0,5)>Eval(B))return BestMove;
	
	// 否則看看是否能翻棋
	for(p=0;p<32;p++)if(B.fin[p]==FIN_X)c++;
	// 若沒有地方翻，則用搜到的最好走法(相較目前是變差)
	if(c==0)
	{
		fprintf(dbgmsgf,"%d cannot flip, decide ",__LINE__);
		return BestMove;
	}
	
	fprintf(dbgmsgf,"%d ",__LINE__);
	// 若不動(翻棋)比最佳步更糟，就不翻
	BOARD NB(B);
	NB.who = (NB.who^1);
	SCORE nomove = Eval(NB);
	// TODO: 注意時間問題
	TimeOut+=2;
	fprintf(dbgmsgf, "no moves: \n");
	MOV bm = BestMove;
	SCORE nomovenext = -NegaScoutSearch(NB,-INF,INF,0,iterDepthLimit,history[histLen-1],histLen,history);
	BestMove = bm;
	fprintf(dbgmsgf, "end of no moves\n");
	// TODO: 翻棋不等於不動，有可能翻到有嚇阻作用的棋，如何計算？
	// TODO: 找出在危險中的棋子，尋找如何救他
	fprintf(dbgmsgf,"now:%d move:%d stay:%d  ",nowscore,scoutscore,nomovenext);
	if(nomovenext < scoutscore)
	{
		fprintf(dbgmsgf,"%d worse if no move, decide ",__LINE__);
		return BestMove;
	}
	
	// 否則隨便翻一個地方
	// TODO: LV2 有策略的翻
	// 若有子快被吃，
	// 找到要被吃的子
	// 找到造成威脅的子
	// 計算嚇阻的位置
	// 計算嚇阻的機率
	// 計算分數變化的期望值
	// 若單純要翻子
	// 找到最容易被吃的子
	// 計算攻擊的位置
	// 計算攻擊的機率
	// 計算期望值
	c=rand()%c;
	for(p=0;p<32;p++)if(B.fin[p]==FIN_X&&--c<0)break;
	fprintf(dbgmsgf,"%d flip decide ",__LINE__);
	return MOV(p,p);
}// End of Play

FIN type2fin(int type) {
    switch(type) {
	case  1: return FIN_K;
	case  2: return FIN_G;
	case  3: return FIN_M;
	case  4: return FIN_R;
	case  5: return FIN_N;
	case  6: return FIN_C;
	case  7: return FIN_P;
	case  9: return FIN_k;
	case 10: return FIN_g;
	case 11: return FIN_m;
	case 12: return FIN_r;
	case 13: return FIN_n;
	case 14: return FIN_c;
	case 15: return FIN_p;
	default: return FIN_E;
    }
}
FIN chess2fin(char chess) {
    switch (chess) {
	case 'K': return FIN_K;
	case 'G': return FIN_G;
	case 'M': return FIN_M;
	case 'R': return FIN_R;
	case 'N': return FIN_N;
	case 'C': return FIN_C;
	case 'P': return FIN_P;
	case 'k': return FIN_k;
	case 'g': return FIN_g;
	case 'm': return FIN_m;
	case 'r': return FIN_r;
	case 'n': return FIN_n;
	case 'c': return FIN_c;
	case 'p': return FIN_p;
	default: return FIN_E;
    }
}
MOV str2mov(char mov[6]) {
	MOV m;
	m.st = mov[0] - 'a' + (mov[1] - '1')*4;
	m.ed = (mov[2]=='(')?m.st:(mov[3] - 'a' + (mov[4] - '1')*4);
	return m;
}
void mov2strs(MOV m, char src[3], char dst[3]) {
	sprintf(src, "%c%c",(m.st%4)+'a', m.st/4+'1');
	sprintf(dst, "%c%c",(m.ed%4)+'a', m.ed/4+'1');
}

int main(int argc, char* argv[]) {
#ifdef _TEST
	dbgmsgf = stderr;
#else
	char filen[1024];
	sprintf(filen, "DEBUG-%d.txt", time(NULL));
	dbgmsgf = fopen(filen,"w");
#endif
#ifdef _WINDOWS
	srand(Tick=GetTickCount());
	char consoleTitle[1024];
	GetConsoleTitle(consoleTitle, 1024);
	SetConsoleTitle(myversion);
#else
	srand(Tick=time(NULL));
#endif
	hashpoolInit();
	for(int i = 0; i < FIN_NUM; i ++)
	{
		for(int j = 0; j < POS_NUM; j ++)
		{
			fprintf(dbgmsgf, "[%d][%d]%llu ",i,j, hashfinpos[i][j]);
		}
		fprintf(dbgmsgf, "\n");
	}

	BOARD B;
	if (argc!=3 && argc!=2) {
		TimeOut=(B.LoadGame("board.txt")-3)*1000;
		HASHKEY h = hash(B);
		HASHKEY history[500] = {};
		int histLen = 0;
		history[histLen++] = h;
		if(!B.ChkLose())Output(Play(B, histLen, history));
		return 0;
	}
	Protocol *protocol;
	protocol = new Protocol();
	if (argc == 2)
		protocol->init_protocol(argv[0],atoi(argv[1]));
	if (argc == 3)
		protocol->init_protocol(argv[1],atoi(argv[2]));
	int iPieceCount[14];
	char iCurrentPosition[32];
	int type, remain_time;
	bool turn;
	PROTO_CLR color; // 先手的顏色

	char src[3], dst[3], mov[6];
	History moveRecord;
	protocol->init_board(iPieceCount, iCurrentPosition, moveRecord, remain_time);
	protocol->get_turn(turn,color);
	

	TimeOut = (DEFAULTTIME-3)*1000;
	

	B.Init(iCurrentPosition, iPieceCount, (color==PCLR_UNKNOW)?(-1):(int)color);
	HASHKEY h = hash(B);
	HASHKEY history[500] = {};
	int histLen = 0;
	history[histLen++] = h;
	
fprintf(dbgmsgf, "History move number: %d\n", moveRecord.number_of_moves);
	if(moveRecord.number_of_moves>0) {
		BOARD H; H.NewGame();
		histLen = 0;
		history[histLen] = hash(H);
		histLen++;
		MOV mh;
		for(int i=0;i<moveRecord.number_of_moves;i++) {
			mh = str2mov(moveRecord.move[i]);
			history[histLen] = hashmove(history[histLen-1],H,mh,chess2fin(moveRecord.move[i][3]));
			histLen++;
			H.DoMove(mh, chess2fin(moveRecord.move[i][3]));
		}
		if(h != hash(H))
		{
			fprintf(dbgmsgf, "HASH DOESN'T MEET.\n");
			histLen = 0;
			history[histLen++] = h;
		}
	}
fprintf(dbgmsgf, "histLen:%d\n",histLen);

	MOV m;
	if(turn) // 我先
	{
		m = Play(B, histLen, history);
		mov2strs(m, src, dst);

		protocol->send(src, dst);
		protocol->recv(mov, remain_time);
		if( color == PCLR_UNKNOW)
			color = protocol->get_color(mov);
		B.who = color;
		m = str2mov(mov);
		history[histLen] = hashmove(history[histLen-1],B,m,chess2fin(mov[3]));
		histLen++;
		B.DoMove(m, chess2fin(mov[3]));

		protocol->recv(mov, remain_time);
		m = str2mov(mov);
		history[histLen] = hashmove(history[histLen-1],B,m,chess2fin(mov[3]));
		histLen++;
		B.DoMove(m, chess2fin(mov[3]));
	}
	else // 對方先
	{
		protocol->recv(mov, remain_time);
		if( color == PCLR_UNKNOW)
		{
			color = protocol->get_color(mov);
			B.who = color;
		}
		else {
			B.who = color;
			B.who^=1;
		}
		m = str2mov(mov);
		history[histLen] = hashmove(history[histLen-1],B,m,chess2fin(mov[3]));
		histLen++;
		B.DoMove(m, chess2fin(mov[3]));
	}
	B.Display();
	while(1)
	{
#ifdef _TEST
		char cmd[1024]={};
		scanf("%s",cmd);
		if(cmd[0]=='p')
			m = Play(B, histLen, history);
		else
			m = str2mov(cmd);
#else
		m = Play(B, histLen, history);
#endif
		mov2strs(m, src, dst);
fprintf(dbgmsgf,"%d'%c' %d'%c' %d'%c'  ",src[0],src[0],src[1],src[1],src[2],src[2]);
fprintf(dbgmsgf,"%d'%c' %d'%c' %d'%c' \n",dst[0],dst[0],dst[1],dst[1],dst[2],dst[2]);
fprintf(dbgmsgf,"#####%d:%s-%s#####\n",histLen,dst,src);
		
#ifdef _WINDOWS
		char titleMessage[1024] = {0};
		sprintf(titleMessage, "%s Eval: %d  BestMove: %s", myversion, Eval(B), mov2str(m));
		SetConsoleTitle(titleMessage);
#endif

		protocol->send(src, dst);
		protocol->recv(mov, remain_time);
		m = str2mov(mov);
		history[histLen] = hashmove(history[histLen-1],B,m,chess2fin(mov[3]));
		histLen++;
		B.DoMove(m, chess2fin(mov[3]));
		B.Display();

		protocol->recv(mov, remain_time);
		m = str2mov(mov);
		history[histLen] = hashmove(history[histLen-1],B,m,chess2fin(mov[3]));
		histLen++;
		B.DoMove(m, chess2fin(mov[3]));
		B.Display();
	}

#ifdef _WINDOWS
	SetConsoleTitle(consoleTitle);
#endif
	return 0;
}
