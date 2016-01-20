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
	for(int i = 0; i < POS_NUM; i ++)
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
// TODO: 增加hash table大小
