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

#ifdef _WINDOWS
#include<windows.h>
#else
#include<ctime>
#endif

char myversion[1024] = "Eval with fixed material Point v1.0, some codes become function";

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

typedef unsigned long long HASHKEY;
#define POS_NUM 32
#define FIN_NUM 16
HASHKEY hashfinpos[FIN_NUM][POS_NUM];
HASHKEY rand64() { // assume RAND_MAX >= 65535
	HASHKEY r = 0;
	for(int i = 0; i < 8; i ++)
		r = r * 0x100 + (rand() % 0xff);
	return r;
}
void hashpoolInit()
{
	for(int i = 0; i < FIN_NUM; i ++)
		for(int j = 0; j < POS_NUM; j ++)
			hashfinpos[i][j] = rand64();
}
HASHKEY hash(const BOARD& B)
{
	HASHKEY h = 0;
	for(int i = 0; i < FIN_NUM; i ++)
		h = h ^ hashfinpos[B.fin[i]][i];
	return h;
}
HASHKEY hashmove(HASHKEY h, const BOARD& B, MOV m, FIN f)
{
	if(m.st!=m.ed) // move or eat
	{
		return h ^ hashfinpos[B.fin[m.st]][m.st] ^ hashfinpos[FIN_E][m.st]
				^ hashfinpos[B.fin[m.ed]][m.ed] ^ hashfinpos[B.fin[m.st]][m.ed];
	}
	else // flip
	{
		return h ^ hashfinpos[FIN_X][m.st] ^ hashfinpos[f][m.st];
	}
}
typedef struct hashentry
{
	HASHKEY key;
	int searchDep;
	char exact;
	SCORE score;
	MOV bestchild;
} HENTRY;
#define hashTblN 0x400000 // about 4.0e+6
HENTRY hashtbl[2][hashTblN] = {{{0}}};

bool TimesUp() {
#ifdef _WINDOWS
	return GetTickCount()-Tick>=TimeOut;
#else
	return clock() - Tick > TimeOut;
#endif
}
/*
// 一個重量不重質的審局函數
SCORE Eval(const BOARD &B) {
	int cnt[2]={0,0};
	for(POS p=0;p<32;p++){const CLR c=GetColor(B.fin[p]);if(c!=-1)cnt[c]++;}
	for(int i=0;i<14;i++)cnt[GetColor(FIN(i))]+=B.cnt[i];
	return cnt[B.who]-cnt[B.who^1];
}
*/
// TODO: LV2 注意循環盤面，若可以勝則避免和棋
SCORE Eval(const BOARD &B) {
	SCORE score[2] = {0,0};
	for(POS p = 0; p < 32; p++)
	{
		if(B.fin[p] < FIN_X)
		{
			const CLR c = GetColor(B.fin[p]);
			const LVL l = GetLevel(B.fin[p]);
			SCORE level_score[7] = {6540,3270,1640,870,390,770,100};
			score[c] += level_score[l];
		}
	}
	for(int i = 0; i < 14; i++)
		score[GetColor(FIN(i))] += (int)(B.cnt[i] * 0.9); // TODO: fix this, it's wrong
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
SCORE NegaScoutSearch(const BOARD &B, SCORE alpha, SCORE beta, int dep, int depLmt, HASHKEY h)
{
fprintf(stderr,"%*s%d ",dep,"",__LINE__);
	if(B.ChkLose())
	{
fprintf(stderr,"%d ",__LINE__);
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
fprintf(stderr,"HASH KEY COLLISION!!!\n");
		if(hashhit && entry->exact && ((depLmt - dep) <= entry->searchDep))
		{
			if(dep == 0)
			{
fprintf(stderr,"%d ",__LINE__);
fprintf(stderr,"%s\n",mov2str(entry->bestchild));
					BestMove = entry->bestchild;
			}
			return entry->score;
		}
	}
	
	// TODO: LV2 使用知識
	// TODO: adjust move order
	if(depLmt == dep 			// remaining search depth
	|| TimesUp()
	|| lst.num == 0)			// a terminal node(regardless flipping chesses)
	{
fprintf(stderr,"%d ",__LINE__);
fprintf(stderr,"score:%d\n",+Eval(B));
		return +Eval(B);
	}

	SCORE m = (hashhit ? entry->score : -INF);	// the current lower bound; fail soft
	SCORE n = beta;								// the current upper bound
	SCORE t;
	for(int i = 0; i < lst.num; i++)
	{
fprintf(stderr,"%*s%d ",dep,"",__LINE__);
fprintf(stderr,"%s ",mov2str(lst.mov[i]));
		BOARD N(B);
		N.Move(lst.mov[i]);		// get the next position, BORAD.Move should not used for FLIP
		HASHKEY nh = hashmove(h,N,lst.mov[i],FIN_X);
fprintf(stderr,"%d\n",__LINE__);
		t = -NegaScoutSearch( N, -n, -max(alpha,m), dep+1, depLmt, nh);// null window search; TEST in SCOUT
fprintf(stderr,"%*s%d ",dep,"",__LINE__);
fprintf(stderr,"tscore:%d ", t);
fprintf(stderr,"lst.num:%d ",lst.num);
		if(t > m)
		{
fprintf(stderr,"%d ",__LINE__);
			if(n == beta || depLmt - dep < 3 || t >= beta)
			{
				m = t;
				bestChildmov = lst.mov[i];
fprintf(stderr,"score:%d ", m);
			}
			else
			{
fprintf(stderr,"%d\n",__LINE__);
				m = -NegaScoutSearch( N, -beta, -t, dep+1, depLmt, nh);	// re-search
fprintf(stderr,"%*s%d ",dep,"",__LINE__);
fprintf(stderr,"score:%d ", m);
				bestChildmov = lst.mov[i];
			}
fprintf(stderr,"%s\n",mov2str(lst.mov[i]));
			if(dep == 0)
				BestMove = lst.mov[i];
		}
fprintf(stderr,"%d ",__LINE__);
		if(m >= beta)
		{
fprintf(stderr,"%d ",__LINE__);
			// update hash table
			// assume key won't == 0
			// TODO: maybe use key == -1 as a mark for an empty hash entry
			if(entry != NULL
			&& (entry->key != h 
				|| (depLmt - dep) > entry->searchDep
				|| ((depLmt - dep) == entry->searchDep && !(entry->exact))))
			{
				entry->key = h;
				entry->searchDep = depLmt - dep;
				entry->exact = 0;
				entry->score = m;
				entry->bestchild = lst.mov[i];
			}
fprintf(stderr,"%d ",__LINE__);
fprintf(stderr,"score:%d ", m);
fprintf(stderr,"%s\n",mov2str(lst.mov[i]));
			return m;			// beta cut off
		}
		n = max(alpha,m) + 1;	// set up a null window
fprintf(stderr,"%d\n",__LINE__);
	}
fprintf(stderr,"%*s%d ",dep,"",__LINE__);
	// update hash table
	// assume key won't == 0
	// TODO: maybe use key == -1 as a mark for an empty hash entry
	if((entry != NULL)
	&& ((entry->key != h )
		|| ((depLmt - dep) >= entry->searchDep)))
	{
fprintf(stderr,"%d\n",__LINE__);
		entry->key = h;
		entry->searchDep = depLmt - dep;
		entry->exact = 1;
		entry->score = m;
		entry->bestchild = bestChildmov;
	}
fprintf(stderr,"%d ",__LINE__);
fprintf(stderr,"score:%d ", m);
fprintf(stderr,"%s\n",mov2str(bestChildmov));
	return m;
}

MOV Play(const BOARD &B) {
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
	// TODO: LV1 detect 目前是第一手或第二手，使用預設的策略
	// TODO: LV1 有策略的翻
	if(B.who==-1)
	{
		p=rand()%32;
		printf("%d\n",p);
		return MOV(p,p);
	}
	
	// TODO: LV5 偵測連續未翻子或吃子的次數
	// TODO: LV4 偵測循環盤面
	// 若搜出來的結果會比現在好就用搜出來的走法
	// TODO: LV4 把HASHKEY打包進BOARD，避免重複計算、傳遞
	
fprintf(stderr,"%d ",__LINE__);
	int depthLimit = 2;
	SCORE scoutscore;
	SCORE nowscore = Eval(B);
	HASHKEY h = hash(B);
	while(depthLimit <= 40)
	{
fprintf(stderr, "DEPLMT: %d:\n",depthLimit);
		scoutscore = NegaScoutSearch(B,-INF,INF,0,depthLimit++,h);
	}
	if(scoutscore > nowscore)
	{
fprintf(stderr,"%d ",__LINE__);
		return BestMove;
	}
	MOV bestMove = BestMove;
	// if(SearchMax(B,0,5)>Eval(B))return BestMove;
	/* 晚點再啟用
	BOARD NB(B);
	NB.who ^= 1;
	SCORE nomove = Eval(NB);
	SCORE nomovenext = NegaScoutSearch(NB,-INF,INF,0,2);// TODO: 注意時間問題
	// 若不動(翻棋)比最佳步更糟，就不翻
	// TODO: 翻棋不等於不動，有可能翻到有嚇阻作用的棋，如何計算？
	// TODO: 找出在危險中的棋子，尋找如何救他
	if(nomovenext < scoutscore)
		return bestMove;
	*/
	
	// 否則看看是否能翻棋
	for(p=0;p<32;p++)if(B.fin[p]==FIN_X)c++;
	// 若沒有地方翻，則用搜到的最好走法(相較目前是變差)
fprintf(stderr,"%d ",__LINE__);
	if(c==0)return bestMove;
	// 否則隨便翻一個地方
	// TODO: LV2 有策略的翻
	c=rand()%c;
	for(p=0;p<32;p++)if(B.fin[p]==FIN_X&&--c<0)break;
fprintf(stderr,"%d ",__LINE__);
	return MOV(p,p);
}

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
			fprintf(stderr, "[%d][%d]%llu ",i,j, hashfinpos[i][j]);
		}
		fprintf(stderr, "\n");
	}

	BOARD B;
	if (argc!=3 && argc!=2) {
		TimeOut=(B.LoadGame("board.txt")-3)*1000;
		if(!B.ChkLose())Output(Play(B));
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

	MOV m;
	if(turn) // 我先
	{
		m = Play(B);
		mov2strs(m, src, dst);

		protocol->send(src, dst);
		protocol->recv(mov, remain_time);
		if( color == PCLR_UNKNOW)
			color = protocol->get_color(mov);
		B.who = color;
		B.DoMove(m, chess2fin(mov[3]));

		protocol->recv(mov, remain_time);
		m = str2mov(mov);
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
		B.DoMove(m, chess2fin(mov[3]));
	}
	B.Display();
	while(1)
	{
		m = Play(B);
		mov2strs(m, src, dst);
fprintf(stderr,"%d'%c' %d'%c' %d'%c'  ",src[0],src[0],src[1],src[1],src[2],src[2]);
fprintf(stderr,"%d'%c' %d'%c' %d'%c' \n",dst[0],dst[0],dst[1],dst[1],dst[2],dst[2]);

		protocol->send(src, dst);
		protocol->recv(mov, remain_time);
		m = str2mov(mov);
		B.DoMove(m, chess2fin(mov[3]));
		B.Display();

		protocol->recv(mov, remain_time);
		m = str2mov(mov);
		B.DoMove(m, chess2fin(mov[3]));
		B.Display();
	}

#ifdef _WINDOWS
	SetConsoleTitle(consoleTitle);
#endif
	return 0;
}
