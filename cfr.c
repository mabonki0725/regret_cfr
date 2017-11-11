/************************
  Counterfactual Regret Minimization
  for porker
*************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#define NUM_ACTION 2
#define NUM_MEMBER 2
#define NUM_CARDS  3

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
int  train(int);
double cfr(int *,char *,double,double,int *);


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
int toString(pNode)
NODE *pNode;
{
   /* func call */
   int a;

   getAverageStrategy(pNode);
   fprintf(stderr,"%4s\n",pNode->infoSet);
   for(a = 0;a < pNode->num;a++) {
     fprintf(stderr,"   action=%s avrage=%lf \n",g_view[a],pNode->avgStrategy[a]);
   }
   return(1);
}
/***
  áD
***/
int train(iteration)
int iteration;
{
  int i;
  int numA;
  
  int cards[3]={1,2,3};
  int c1,c2,itemp,numCards;
  double util;
  char history[256];

  int ret;

  numCards = NUM_CARDS;
  numA = NUM_ACTION;

  util=0;
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
    util += cfr(cards,history,1.0,1.0,&ret);
    if(ret == FATAL) return(ret);
  }
  fprintf(stderr,"Average game value:%lf \n",util / iteration); 
  /* print out node */
  for(i=0;i<g_nodeNum;i++) {
    toString(g_pNodes[i]);
  }

  return(0);
}
/***
  CFRÅ@áF
***/
double cfr(cards,history,p0,p1,pRet)
int *cards;
char *history;
double p0;
double p1;
int *pRet;
{

  int round;
  int player;
  int opponent;

  char s_card[3];
  NODE *pNode;
  double util[NUM_ACTION];
  double nodeUtil=0;
  char nextHistory[256];
  int a,ret;
  char infoSet[256];
  double regret;
  
  round = strlen(history);
  player = round % NUM_MEMBER;
  opponent = (player == 0 ? 1 : 0);

  /* Return payoff for terminal state áG */
  if(round > 1) {
    if(history[round-1] == 'p') {
      if(!strncmp(history,"pp",2)) {
        if(cards[player] > cards[opponent]) return(1);   /* pp */
        else                                return(-1);  /* pp */
      }
      else  return(1);                                   /* bp */
    }
    else
    if(!strncmp(&history[round-2],"bb",2)) {
        if(cards[player] > cards[opponent]) return(2);   /* bb */
        else                                return(-2);  /* bb */
    }
  }
  /* round = 0 or 1 */
  sprintf(s_card,"%2d",cards[player]);s_card[3]='\0';
  strcpy(infoSet,s_card);
  strcat(infoSet,history);
  /* Get informatin set node or create if if nonexistent áH */
  pNode=getNode(infoSet);
  if(pNode == NULL) {
    pNode = createNode(&ret);
    if(ret == FATAL) {
      *pRet = ret;
      return(0);
    }
    setNode(pNode,infoSet);
  }
  /* for each acton, recursivly call CFR with additional history and probability áI */
  if(player == 0) getStrategy(pNode,p0);
  else            getStrategy(pNode,p1);
  
  nodeUtil = 0;
  for(a = 0;a < pNode->num; a++) {
    strcpy(nextHistory,history);
    if(a == 0) strcat(nextHistory,"p");
    if(a == 1) strcat(nextHistory,"b");
    if(player == 0) util[a] = cfr(cards,nextHistory,p0*pNode->strategy[a],p1,&ret);
    else            util[a] = cfr(cards,nextHistory,p0,p1*pNode->strategy[a],&ret);
    if(ret == FATAL) {
      *pRet=ret;
      return(0.0);
    }
    nodeUtil += pNode->strategy[a] * util[a];
  }
  /* for each action, compute and accumulate counterfactual regret áJ */
  for(a = 0;a < pNode->num; a++) {
    regret = util[a] - nodeUtil;
    if(player == 0) pNode->regretSum[a] = p1 * regret;
    else            pNode->regretSum[a] = p0 * regret;
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

   /* trainning */
   train(1000);

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