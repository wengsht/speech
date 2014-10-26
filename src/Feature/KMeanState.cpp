// =====================================================================================
// 
//       Filename:  KMeanState.cpp
// 
//    Description:  
// 
//        Version:  0.01
//        Created:  2014/10/21 21时13分32秒
//       Revision:  none
//       Compiler:  clang 3.5
// 
//         Author:  wengsht (SYSU-CMU), wengsht.sysu@gmail.com
//        Company:  
// 
// =====================================================================================

#include "KMeanState.h"
#include <cmath>
#include <cstdlib>
#include "mathtool.h"
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <vector>
using namespace std;

const std::pair<int, int> KMeanState::NullSeg = std::make_pair(0, -1);

KMeanState::KMeanState(std::vector<WaveFeatureOP> * templates) : HMMState(templates) {
    edgePoints.resize(templates->size());
	points = NULL;

    srand(time(0));
}

KMeanState::~KMeanState() {
	clearGaussian();
}
void KMeanState::clearGaussian(){
	for(int i = 0;i<GaussianSet.size();i++){
		if(GaussianSet[i])delete GaussianSet[i];
		if(GaussianModel[i])delete GaussianModel[i];
	}	
	GaussianSet.clear();
	GaussianModel.clear();
	weight.clear();
}
double KMeanState::checkBetter(double best){
	double now_var = 0;
//	printf("check better\n");
	int gn = 0;
	
	if(GaussianSet.size()==0){
		return best;	
	}

	//printf("no zero\n");
	if(best >=0){
		for(int i = 0;i<GaussianSet.size();i++){
			if(GaussianSet[i]->getflag()){
				gn++;
				now_var+=GaussianSet[i]->getCVar();
			}
		}
		//assert(gn!=0);
		now_var/=gn;
		
		if(now_var>=best)return best;
	}
	//printf("clear\n");
	for(int i =0;i<GaussianModel.size();i++){
		if(GaussianModel[i])delete GaussianModel[i];	
		GaussianModel[i] = NULL;
	}
	int mn = GaussianSet.size();

	GaussianModel.clear();
	w.clear();
	GaussianModel.resize(mn);
	w.resize(mn);
	//printf("repalce\n");
	for(int i=0;i<mn;i++){
		GaussianModel[i] = GaussianSet[i];
		GaussianSet[i] = NULL;
		w[i] = weight[i];
	}

	best = now_var;
	//printf("CheckOver\n");
	return best;
}

void KMeanState::gaussianTrain(int gaussianNum) {
	//printf("start train ~~  gaussian num:%d\n",gaussianNum);
	
	// 计算该状态内Feature(Point)总数
	PointNum = 0;
	for(int i = 0;i<edgePoints.size();i++){
		int tmp = edgePoints[i].second - edgePoints[i].first;
		if(tmp>=0){
			PointNum += tmp+1;
		}
	}
	
	//printf("PointNum : %d\n",PointNum);	
	// 重新将feature(point)提取到points,方便访问
	if(points)delete [] points;
	points = new Feature*[PointNum];
	int cnt = 0;
	for(int i = 0;i<edgePoints.size();i++){
		int st = edgePoints[i].first;
		int ed = edgePoints[i].second;
		for(int j = st;j<=ed;j++){
			points[cnt++] = &((*templates)[i][j]);
		}
	}

	int time = 10;
	double best = -1;
	while(time--){
		initTrain(gaussianNum);
		this->KMeanTrain();
		best = checkBetter(best);
	};
	//this->gaussianTrainTest(gaussianNum);
}

double KMeanState::nodeCost(Feature *inputFeature) {
	return this->KMeanNodeCost(inputFeature);
}

void KMeanState::initTrain(int gaussianNum){
//	printf("init\n");
	//初始化高斯模型
	// - 清空高斯集合
	// - 以随机一个point作为第i个高斯的均值（每隔step选一个point）
	// - 所有高斯初始方差相等，权重相等

//	printf("delete\n");
	for(int i = 0;i<GaussianSet.size();i++){
		if(GaussianSet[i])delete GaussianSet[i];
		GaussianSet[i] = NULL;
	}

	GaussianSet.clear();
//	puts("weight");
	//GaussianSet.clear();
	weight.clear();
	
//	puts("checkNum");	
	if(PointNum<gaussianNum){
		gaussianNum = 1;
	}
//	puts("=0");
	if(PointNum == 0){
		gaussianNum = 0;
		return;
	}

//	printf("init Gaussian\n");
	int step = PointNum / gaussianNum;
	int featureSize = points[0]->size();
    Feature initFeature;
	GaussianSet.resize(gaussianNum);
	weight.resize(gaussianNum);
	for(int i = 0;i<gaussianNum;i++){
		Gaussian * g = new Gaussian(featureSize);
        
        generateInitFeature(initFeature);

		g->setMean(points[rand()%step+i*step]);
		
		//g->setMean(&initFeature);
//		g->setRandCVar();
		g->setCVar(rand()%10000);
//		g->setCVar(-1);
		weight[i] = (1.0/gaussianNum);
		GaussianSet[i] = g;
//		printf("G%d ---------\n",i);
//		GaussianSet[i]->print();
	}
}
void KMeanState::generateInitFeature(Feature &initFeature) {
    if(PointNum <= 0) return ;

    int featureSiz = points[0]->size();
    initFeature.resize(featureSiz);

    double *minD = new double[featureSiz];
    double *maxD = new double[featureSiz];

    for(int j = 0;j < featureSiz; j++) {
        minD[j] = maxD[j] = (*(points[0]))[j];
    }
    for(int j = 0;j < initFeature.size(); j++) {
        for(int i = 1;i < PointNum; i++) {
            minD[j] = fmin(minD[j], (*(points[i]))[j]);
            maxD[j] = fmax(maxD[j], (*(points[i]))[j]);
        }
    }

    for(int j = 0;j < initFeature.size(); j++) {
        initFeature[j] = minD[j] + (maxD[j] - minD[j]) * (1.0 *(rand() % 10000) / 10000);
    }

    delete []minD;
    delete []maxD;
}

int KMeanState::Point2Clusters(int pid){
	double mindist = GaussianSet[0]->minuLogP(points[pid]);
	int gid = 0;
	for(int i = 1;i<GaussianSet.size();i++){
		double mlp = GaussianSet[i]->minuLogP(points[pid]);
		if(mlp < mindist ){
			mindist = mlp;
			gid  = i;
		}
	}
	return gid;
}

void KMeanState::KMeanTrain()
{
//	puts("set size==0?");
	if(GaussianSet.size()==0)return;
	bool converge = false;
//	printf("--train\n");
	while(!converge){
		vector<int> count;
	
		for(int i = 0;i<GaussianSet.size();i++) {
			count.push_back(0);
		}

		converge = true;
		// 计算每一点属于哪个gaussian，并统计数据
		for(int i = 0;i<PointNum;i++){
			int gid = Point2Clusters(i);
			GaussianSet[gid]->addFeature(points[i]);
			count[gid] ++;
		}

		// 更新参数并计算是否已经收敛
		for(int i = 0;i<GaussianSet.size();i++){
			double wi = count[i] / (double)PointNum;
			converge &= GaussianSet[i]->done();
			if(fabs(wi-weight[i])>0.0001){
				converge = false;
			}
			weight[i] = wi;
			
		//	printf("G%d ---------\n",i);
		//	printf("wi = %lf\n",wi);
		//	GaussianSet[i]->print();
//			char c;scanf("%c",&c);
		
		}

        /*  
        for(int i = 0;i < 4;i++) {
            printf("%lf ", weight[i]);
        }
        puts("");
        */

	}
/*	printf("\n\nfinally -----------------------\n");
	for(int i = 0;i<GaussianSet.size();i++){
		printf("G %d --------:",i);
		printf("wi = %lf\n",weight[i]);
		GaussianSet[i]->print();
		printf("\n\n");
	}*/
//	printf("trainover\n");
}


double KMeanState::KMeanNodeCost(Feature *f){
//	puts("calc cost");
	if(GaussianModel.size()==0)return Feature::IllegalDist;
//	puts("more");
	double ret = Feature::IllegalDist;
	const double eps = 1e-13;
	for(int i = 0;i<GaussianModel.size();i++){
		if(fabs(w[i])<eps)continue;
//		printf("calc g %d\n",i);
		double t = GaussianModel[i]->minuLogP(f);
		t -= log(w[i]);
		if(ret == Feature::IllegalDist){
			ret = t;
		}
		else{
//			printf("ret : %lf t:%lf\n",ret,t);
			ret = logInsideSum(ret,t);
		}
	}
//	printf("over");
	return ret;
}


/////////////////////////////////////////////////////////////////////


void KMeanState::gaussianTrainTest(int gaussianNum) {
    for(int i = 0;i < 39;i++) {
        u[i] = sigma[i] = 0;
        int cnt = 0;
        for(int j = 0;j < templates->size(); j++) {
            int kst = edgePoints[j].first, ked = edgePoints[j].second;
            for(int k = kst; k <= ked; k++) {

                u[i] += (*templates)[j][k][i];
                sigma[i] += pow((*templates)[j][k][i], 2.0);

                cnt ++;
            }
        }
        if(cnt) {
            u[i] /= cnt;
            sigma[i] /= cnt;
        }
    }

    for(int i = 0;i < 39;i++) {
        sigma[i] -= (u[i] * u[i]);
    }
/*
	for(int i = 0;i<39;i++){
		printf("%lf ",u[i]);
	}puts("");

	for(int i=0;i<39;i++){
		printf("%lf ",sigma[i]);
	}puts("");
	char c;scanf("%c",&c);	
*/
}


double KMeanState::nodeCostTest(Feature *inputFeature) {
    if(u[0] == 0.0)
        return Feature::IllegalDist;

    double a = 0.0, b = 0.0;
    for(int i = 0;i < 39;i++) {
        a += log(2*3.1415926*sigma[i]);

        b += pow((*inputFeature)[i] - u[i], 2.0) / sigma[i];
    }
    a *= 0.5;
    b *= 0.5;

    return a+b; //p2cost(a+b);
}
