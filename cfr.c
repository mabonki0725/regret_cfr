/************************
  Counterfactual Regret Minimization
  for porker
*************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#define NUM_ACTION 2
#define NUM_MEMBER 3
#define NUM_CARD   5  //52

#define PASS 0
#define BET  1

typedef 
struct node {
  char infoSet[256];
  int num;
  double regretSum[NUM_ACTION];
  double strategy[NUM_ACTION];
  double strategySum[NUM_ACTION];
  double avgStrategy[NUM_ACTION];
} NODE;

/* node Area in Global */
#define MAX_NODE 1024
#define FATAL     999
int g_nodeNum=0;
NODE *g_pNodes[MAX_NODE];

int g_dbg=0;  /* dbg flag */
char *g_view[2]={"Pose ","Bet  "};

/* PROT TYPE */
NODE *getNode(char *);
NODE *createNode(int *);
int  setNode(NODE*,char *);
int  train(int,FILE *);
double cfr(int *,char *,char *,double*,int *);


/**
Å@á@
**/
int getStrategy(pNode,realizationWeight)
NODE *pNode;
double realizationWeight; 
{
  double normalizingSum=0;
  int a;

  for(a = 0;a < pNode->num;a++) {
    pNode->strategy[a] = pNode->regretSum[a] > 0 ? pNode->regretSum[a] : 0;
    normalizingSum += pNode->strategy[a];
  }
  for(a=0;a < pNode->num; a++) {
    if(normalizingSum > 0) {
      pNode->strategy[a] /= normalizingSum;
    }
    else {
      pNode->strategy[a] = 1.0 / pNode->num;
    }
    pNode->strategySum[a] += realizationWeight * pNode->strategy[a];
  }
  return(0);
}
/***
 áA
***/
int getAverageStrategy(pNode)
NODE *pNode;
{
  double normalizingSum=0;
  int a;

  for(a=0;a<pNode->num;a++) {
    normalizingSum += pNode->strategySum[a];
  }
  for(a=0;a<pNode->num;a++) {
    if(normalizingSum > 0) {
      pNode->avgStrategy[a] = pNode->strategySum[a] / normalizingSum;
    }
    else {
      pNode->avgStrategy[a] = 1.0 / normalizingSum;
    }
  }
  return(0);
}
/***
 áB
****/
int toString(pNode,fw)
NODE *pNode;
FILE *fw;
{
   /* func call */
   int a;

   getAverageStrategy(pNode);
   fprintf(fw,"%4s\n",pNode->infoSet);
   for(a = 0;a < pNode->num;a++) {
     fprintf(fw,"   action=%s avrage=%lf \n",g_view[a],pNode->avgStrategy[a]);
   }
   return(1);
}
/***
  áD
***/
int train(iteration,fw)
int iteration;
FILE *fw;
{
  int i;
  int numA;
  
  int cards[NUM_CARD];
  int c1,c2,itemp,numCards;
  double util;
  char history[256];
  char infoSet[256];
  
  int ret;

  int j;
  double pd[NUM_MEMBER];

  numCards = NUM_CARD;
  numA = NUM_ACTION;

  util=0;
  for(i = 0; i< numCards;i++) cards[i]=i+1;

  for(i =0;i < iteration;i++) {
    /* Shuffle cards áE*/
    for(c1 = numCards-1; c1 > 0; c1--) {
      c2 = (int)((c1+1) * (double)rand()/(double)RAND_MAX);
      itemp = cards[c1];
      cards[c1] = cards[c2];
      cards[c2] = itemp;
    }
    /* call CFR Å@áF*/
    history[0]='\0';
    infoSet[0]='\0';
    for(j=0;j<NUM_MEMBER;j++) pd[j] = 1.0;
    util += cfr(cards,infoSet,history,pd,&ret);
    if(ret == FATAL) return(ret);
  }
  fprintf(fw,"Average game value:%lf \n",util / iteration); 
  /* print out node */
  for(i=0;i<g_nodeNum;i++) {
    toString(g_pNodes[i],fw);
  }

  return(0);
}
/***
  CFRÅ@áF
***/
double cfr(cards,infoSet,history,pd,pRet)
int *cards;
char *infoSet;
char *history;
double *pd; /* NUM_MEMBER */
int *pRet;
{

  int round;
  int player;
  int player1;
  int opponent;

  char s_card[3];
  NODE *pNode;
  double util[NUM_ACTION];
  double nodeUtil=0;
  char nextHistory[256];
  int a,ret;
  char nextInfoSet[256];
  double regret;

  int j;
  double pdd[NUM_MEMBER];
  char s_player[3];
  double pdave;

  double rnd;
  double limit[NUM_MEMBER+1];
  
  round = strlen(history);
#if 0
  player = round % NUM_MEMBER;
#else
  player = 0;
#endif
  opponent = (player == 0 ? 1 : 0);

#if 0
  player1 = round % NUM_MEMBER;
#else
  limit[0]=0.0;
  for(j = 1;j < NUM_MEMBER;j++) {
    limit[j] = limit[j-1] + 1.0/(double)NUM_MEMBER; 
  }
  limit[NUM_MEMBER]=1.0;

  rnd = (double)rand()/(double)RAND_MAX;
  for(j = 0;j < NUM_MEMBER;j++) {
    if(rnd >= limit[j] && rnd < limit[j+1]) break;
  }
  player1 = j;
#endif

  /* Return payoff for terminal state áG */
  if(round > 1) {
    if(history[round-1] == 'p') {
      if(!strncmp(history,"pp",2)) {
        if(cards[player] > cards[opponent]) return( 1);  /* pp */
        else                                return(-1);  /* pp */
      }
      else  return(-1);                                   /* bp */
    }
    else
    if(!strncmp(&history[round-2],"bb",2)) {
        if(cards[player] > cards[opponent]) return( 2);  /* bb */
        else                                return(-2);  /* bb */
    }
  }
  /* round = 0 or 1 */
#if 0
  sprintf(s_card,"%d",cards[0]);s_card[2]='\0';
#else
  sprintf(s_card,"%d",cards[player]);s_card[2]='\0';
#endif
  strcpy(nextInfoSet,s_card);strcat(nextInfoSet,"card ");

#if 1
  sprintf(s_player,"%d",player1+1);s_player[2]='\0';
  if(round > 0) strcat(nextInfoSet,s_player);
#endif

  strcat(nextInfoSet,history);

  /* Get informatin set node or create if if nonexistent áH */
  pNode=getNode(nextInfoSet);
  if(pNode == NULL) {
    pNode = createNode(&ret);
    if(ret == FATAL) {
      *pRet = ret;
      return(0);
    }
    setNode(pNode,nextInfoSet);
  }
  /* for each acton, recursivly call CFR with additional history and probability áI */
  if(!strcmp(nextInfoSet,"1")) {
    if(g_dbg) fprintf(stderr,"3pb");
  }

  for(j = 0;j < NUM_MEMBER;j++) {
    if(j == player) getStrategy(pNode,pd[j]);
  }

  
  nodeUtil = 0;
  for(a = 0;a < pNode->num; a++) {
    strcpy(nextHistory,history);
    if(a == 0) strcat(nextHistory,"p");
    if(a == 1) strcat(nextHistory,"b");

    for(j=0;j<NUM_MEMBER;j++) {
      if(j == player1) pdd[j] = pd[j] * pNode->strategy[a];
      else             pdd[j] = pd[j];
    }
    util[a] = cfr(cards,nextInfoSet,nextHistory,pdd,&ret);

    if(ret == FATAL) {
      *pRet=ret;
      return(0.0);
    }
    nodeUtil += pNode->strategy[a] * util[a];
  }
  /* for each action, compute and accumulate counterfactual regret áJ */
  for(a = 0;a < pNode->num; a++) {
    regret = util[a] - nodeUtil;

#if 0
    if(player1 == 0) pNode->regretSum[a] += pd[1] * regret;
    else             pNode->regretSum[a] += pd[0] * regret;
#else
    pdave=0;
    for(j = 0;j< NUM_MEMBER;j++) {
      if(player1 == j) continue;
      pdave += pd[j];
    }
    pdave /= (NUM_MEMBER-1);  //average except player1
 
    pNode->regretSum[a] += pdave * regret;

#endif
  }

  *pRet=0;
  return(nodeUtil);
}
/***

***/
int main(argc,argv)
int argc;
char *argv[];
{
   int iteration;
   FILE *fw;

   if(argc == 1) {
     fprintf(stderr,"USAGE numOfIteration outFileName\n");
     exit(9);
   }
   else 
   if(argc == 2) {
     iteration = atoi(argv[1]);
     fw=stderr;
   }
   else 
   if(argc == 3) {
     iteration = atoi(argv[1]);
     if(!(fw=fopen(argv[2],"w"))) exit(2);
   }
          
   /* trainning */
   train(iteration,fw);

   if(fw != stderr) fclose(fw);

   /* result */
   return(0);

}
/****

****/
NODE* createNode(pRet)
int *pRet;
{
   int a,maxNum;
   NODE* pNode;

   maxNum = MAX_NODE;
   if(g_nodeNum >= maxNum) {
     fprintf(stderr,"num of Node are exceed limit %d\n",maxNum);
     *pRet = FATAL;
     return(NULL);
   }
   *pRet = 0;

   g_pNodes[g_nodeNum]=(NODE *)malloc(sizeof(NODE));
   g_pNodes[g_nodeNum]->num=NUM_ACTION;
   g_pNodes[g_nodeNum]->infoSet[0]='\0';
   for(a = 0;a < NUM_ACTION;a ++) {
     g_pNodes[g_nodeNum]->strategy[a]=0;
     g_pNodes[g_nodeNum]->strategySum[a]=0;
     g_pNodes[g_nodeNum]->regretSum[a]=0;
     g_pNodes[g_nodeNum]->avgStrategy[a]=0;
   }
   pNode = g_pNodes[g_nodeNum];
   g_nodeNum++;

   return(pNode);
}
/****

*****/
NODE *getNode(infoSet)
char *infoSet;
{
    int i;

    for(i=0; i < g_nodeNum; i++) {
      if(!strcmp(g_pNodes[i]->infoSet,infoSet)) return(g_pNodes[i]);
    }
    return(NULL);
}
/****

*****/
int setNode(pNode,infoSet)
char *infoSet;
NODE *pNode;
{
   strcpy(pNode->infoSet,infoSet);
   return(0);
}