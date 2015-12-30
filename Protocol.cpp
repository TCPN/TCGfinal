#include "Protocol.h"

Protocol::Protocol()
{ 
} // end Protocol()

bool Protocol::init_protocol(const char* ip,const int port)
{
	if( !this->m_csock.InitSocket(ip,port) )// 初始socket 
		return false;
	return true;
} // end init_protocol(const char* ip,const int port)

Protocol::~Protocol(void)
{
} // end ~Protocol(void)
 
void Protocol::init_board(int piece_count[14], char current_position[32], struct History& history,int &time)
{ 
	char *recvbuf= NULL; 
	if(this->m_csock.Recieve(&recvbuf)){
		puts(recvbuf);
		if( strstr(recvbuf, "/start") !=NULL ){
			Start( &recvbuf[7],piece_count,current_position, history,time);	
		}
	}
	this->m_csock.Send("/start");
	if(recvbuf != NULL) free(recvbuf);recvbuf = NULL;
} // end init_board(int iPieceCount[14], int iCurrentPosition[32], History& history,int &time)


void Protocol::Start(const char* state, int piece_count[], char current_position[], History& history,int &time)
{ 
	static const char skind[]={'-','K','G','M','R','N','C','P','X','k','g','m','r','n','c','p'};
	char *sTmp = NULL, *token = NULL;  
	int  iPieceDark[14]={1,2,2,2,2,2,5,1,2,2,2,2,2,5}; // init dark chess num
	if(state != NULL){
		sTmp = (char*)malloc(sizeof(char)*strlen(state)+1);
		strcpy(sTmp,state);

		// init board
		token = strtok( sTmp, "," ) ;
		current_position[28] = skind[atoi(token)];  
		for(int i=1;i<32;i++){
			token = strtok( NULL, "," );
			current_position[(7-i/4)*4+i%4] = skind[atoi(token)]; 
		}	 
		// 初始盤面尚存兵種數
		for(int i=0;i<14;i++){
			token = strtok( NULL, "," );
			piece_count[i] = atoi(token); 
		}	 
		  
	}else{
		for( int i=0; i<32; i++ ) current_position[i] = 8; // DarkChess
		for( int i=0; i<14; i++) piece_count[i] = iPieceDark[i];
	} 
	// moveRecord
	token = strtok( NULL, "," );
	int steps = atoi(token); 
	history.number_of_moves=steps;
	if(history.number_of_moves){
		history.move = (char**)malloc(sizeof(char*)*steps);
		for (int i = 0; i < steps; i++) history.move[i] = (char*)malloc(sizeof(char)*6);
		for(int i=0; i<steps;i++){
			char tmp[3]={0};
			int fin, src, dst;
			token = strtok( NULL, "," );
			if(token[2]=='-'){ // move
				tmp[0]=token[0],tmp[1]=token[1];
				src=atoi(tmp);
				tmp[0]=token[3],tmp[1]=token[4];
				dst=atoi(tmp);
				sprintf(history.move[i], "%c%c-%c%c",'a'+(src%4), '1'+(src/4),'a'+(dst%4), '1'+(dst/4));
			}
			else{
				tmp[0]=token[0],tmp[1]=token[1];
				src=atoi(tmp);
				tmp[0]=token[3],tmp[1]=token[4];
				fin=atoi(tmp);
				sprintf(history.move[i], "%c%c(%c)",'a'+(src%4), '1'+(src/4), skind[fin]);
			}
			printf("%s\n",history.move[i]);
		} 
	} 
	//回合制時間
	token = strtok( NULL, "," );
	if(token != NULL)
		time = atoi(token);
	printf("time: %d (ms)\n",time);

} // end Start(const char* sState, int iPieceCount[], int iCurrentPosition[],int time)

void Protocol::get_turn(bool &turn, PROTO_CLR &color)
{
	char *recvbuf = NULL;
	if(this->m_csock.Recieve(&recvbuf)){
		if( strstr(recvbuf, "/turn") !=NULL ){
			turn = (recvbuf[6]=='0')?false:true;
			color = (recvbuf[8]=='0')?PCLR_RED:((recvbuf[8]=='1')?PCLR_BLACK:PCLR_UNKNOW);
		}
	} 
	this->m_csock.Send("/turn");
	if(recvbuf != NULL) free(recvbuf);recvbuf = NULL;
} // end get_turn(int &turn, int &color)
 
void Protocol::send(const char src[3], const char dst[3])
{ 
	char sRetCmd[BUFFER_SIZE] = {0}; 
	if( strcmp(src, dst) ) 
		sprintf(sRetCmd,"/move %d %d %d %d", src[0]-'a', src[1]-'1', dst[0]-'a', dst[1]-'1');
	else
		sprintf(sRetCmd,"/flip %d %d", src[0]-'a', src[1]-'1'); 
	puts(sRetCmd);
	this->m_csock.Send(sRetCmd);
} // end send(const char src[3], const char dst[3])

void Protocol::send(const char move[6])
{ 
	char sRetCmd[BUFFER_SIZE] = {0};  
	if( move[0]==move[3] && move[1]==move[4] ) // Flip src = dst
		sprintf(sRetCmd,"/flip %d %d", move[0]-'a', move[1]-'1');
	else
		sprintf(sRetCmd,"/move %d %d %d %d", move[0]-'a', move[1]-'1', move[3]-'a', move[4]-'1');
	this->m_csock.Send(sRetCmd);
} // end send(const char move[6])

void Protocol::recv(char mov[6],int &time)
{
	char *recvbuf = NULL;
	static const char skind[]={'-','K','G','M','R','N','C','P','O','k','g','m','r','n','c','p'};
	int srcX,srcY,dstX,dstY,fin;
	if(this->m_csock.Recieve(&recvbuf)){
		printf("recv:%s\n",recvbuf);
		if( strstr(recvbuf, "/move") !=NULL ){
			sscanf(recvbuf,"%*s %d %d %d %d %d",&srcX,&srcY,&dstX,&dstY,&time); 
			sprintf(mov,"%c%c-%c%c", 'a'+srcX, '1'+srcY, 'a'+dstX, '1'+dstY );
			printf("time left over %d ms\n",time);
		}else if( strstr(recvbuf, "/flip") !=NULL ){
			sscanf(recvbuf,"%*s %d %d %d %d",&srcX,&srcY,&fin,&time); 
			sprintf(mov,"%c%c(%c)", 'a'+srcX, '1'+srcY, skind[fin] );
			printf("time left over %d ms\n",time);
		}
	}  
	if(recvbuf != NULL) free(recvbuf);recvbuf = NULL;
} // end recv(char mov[6])

PROTO_CLR Protocol::get_color(const char move[6])
{
	static const char skind[]={'K','G','M','R','N','C','P','k','g','m','r','n','c','p'};
	if (move[2]!='(') puts("error"); 
	for (int i = 0; i < 14; i ++) {
		if (move[3]==skind[i])
			return (i<7)?PCLR_RED:PCLR_BLACK;
	}
	return PCLR_UNKNOW;
} // end get_color(char move[6])
