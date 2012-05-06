#include "pch.h"
#include "common/Defines.h"
#include "OgreGame.h"
#include "../vdrift/game.h"
#include "../vdrift/car.h"

#include "common/GraphView.h"
#include "SplitScreen.h"
#include <OgreSceneManager.h>
using namespace Ogre;
using namespace MyGUI;



//  Update
//-----------------------------------------
void App::UpdateGraphs()
{
	for (int i=0; i < graphs.size(); ++i)
		graphs[i]->Update();
}

void App::DestroyGraphs()
{
	for (int i=0; i < graphs.size(); ++i)
	{
		graphs[i]->Destroy();
		delete graphs[i];
	}
	graphs.clear();
}

//  util
inline double negPow(double x, double y)
{
	return (x >= 0.0) ? pow(x, y) : -pow(-x, y);
}


///  Create Graphs  .-_/\._-
//-----------------------------------------------------------------------------------
void App::CreateGraphs()
{
	if (!graphs.empty())  return;
	SceneManager* scm = mSplitMgr->mGuiSceneMgr;

	switch (pSet->graphs_type)
	{
	case 0:  /// bullet hit
		for (int i=0; i < 5; ++i)
		{
			GraphView* gv = new GraphView(scm,mWindow,mGUI);
			int c = i%5;  /*clr*/
			gv->Create(512/*len*/, "graph"+toStr(c+1), i==0||i==2 ? 0.52f : 0.f/*alpha*/);
			switch (i)
			{
				case 0:  gv->CreateTitle("norm vel",	c, 0.0f, -2, 24);  break;
				case 1:  gv->CreateTitle("hit force",	c, 0.15f,-2, 24);  break;
				case 2:  gv->CreateTitle("N snd",		c, 0.0f, -2, 24);  break;
				case 3:  gv->CreateTitle("scrap",		c, 0.1f, -2, 24);  break;
				case 4:  gv->CreateTitle("screech",		c, 0.2f, -2, 24);  break;
			}
			if (i < 2)	gv->SetSize(0.f, 0.5f, 0.5f, 0.15f);
			else		gv->SetSize(0.f, 0.35f, 0.5f, 0.15f);

			gv->SetVisible(pSet->show_graphs);
			graphs.push_back(gv);
		}	break;

	case 1:  /// sound
		for (int i=0; i < 4; ++i)
		{
			GraphView* gv = new GraphView(scm,mWindow,mGUI);
			int c = i%2*2;
			gv->Create(2*512, "graph"+toStr(c+1), c>0 ? 0.f : 0.4f);
			if (c == 0)
				gv->CreateGrid(2,1, 0.4f, 0.5f);  //64,256
			switch (i)
			{
				case 0:  gv->CreateTitle("vol ampl.",		c, 0.0f,-2, 24);  break;
				case 1:  gv->CreateTitle("pan: L up R dn",	c, 0.0f, 2, 24);  break;
				case 2:  gv->CreateTitle("wave L",			c, 0.0f,-2, 24);  break;
				case 3:  gv->CreateTitle("wave R",			c, 0.0f, 2, 24);  break;
			}
			if (i < 2)	gv->SetSize(0.00f, 0.24f, 0.40f, 0.25f);
			else		gv->SetSize(0.60f, 0.24f, 0.40f, 0.25f);

			gv->SetVisible(pSet->show_graphs);
			graphs.push_back(gv);
		}	break;

	case 2:  /// tire
	case 3:	 // susp
		for (int i=0; i < 8; ++i)
		{
			GraphView* gv = new GraphView(scm,mWindow,mGUI);
			int c = i%4;
			gv->Create(512, "graph"+toStr(c+1), c>0 ? 0.f : (i < 14 ? 0.44f : 0.62f));
			if (c == 0)
				gv->CreateGrid(10,1, 0.2f, 0.4f);

			const static float x0 = 0.0f, x1 = 0.07f, x2 = 0.08f;
			const static char* cgt[8][2] = {
				// Front ^ Back/Rear v  Left L< Right R>
				 "FL [^"			,"FL <^"
				,"^] FR   long |"	,"^> FR susp pos"
				,"BL [_"			,"BL <v"
				,"_] BR   slip"		,"v> BR"
				,"FL [^"			,"FL <^"
				,"^] FR   lat --"	,"^> FR susp vel"
				,"BL [_"			,"BL <v"
				,"_] BR   slide"	,"v> BR"	};

			int t = pSet->graphs_type == 2 ? 0 : 1;
			float x = i%2==0 ? x0 : (t ? x2 : x1);  char y = i/2%2==0 ? -2 : -3;
			gv->CreateTitle(cgt[i][t], c, x, y, 24);

			if (i < 4)	gv->SetSize(0.00f, 0.24f, 0.40f, 0.25f);
			//else		gv->SetSize(0.60f, 0.24f, 0.40f, 0.25f);  // right
			else		gv->SetSize(0.00f, 0.50f, 0.40f, 0.25f);  // top
			
			gv->SetVisible(pSet->show_graphs);
			graphs.push_back(gv);
		}	break;

	case 4:  /// tire pacejka
		const int NG = 6;
		for (int i=0; i < NG*2; ++i)
		{
			GraphView* gv = new GraphView(scm,mWindow,mGUI);
			int c = i%NG;  bool b = i >= NG;
			gv->Create(512, String("graph")+(b?"B":"A")+toStr(c), i>0 ? 0.f : 0.5f);
			if (c == 0)
			{	gv->CreateGrid(10,10, 0.2f, 0.4f);
				if (b)	gv->CreateTitle("Tire Fx", 0, 0.f, -2, 24);
				else	gv->CreateTitle("Tire Fy", 2, 0.3f, 3, 24);
			}
			gv->SetSize(0.00f, 0.40f, 0.50f, 0.50f);

			gv->SetVisible(pSet->show_graphs);
			graphs.push_back(gv);
		}	break;
	}
}

///  add new Values to graphs (each frame)
//-----------------------------------------------------------------------------------
void App::GraphsNewVals()				// Game
{
	size_t gsi = graphs.size();
	switch (pSet->graphs_type)
	{
	case 0:  /// bullet hit  force,normvel, sndnum,scrap,screech
		if (gsi >= 5)
		if (carModels.size() > 0)
		{
			const CARDYNAMICS& cd = carModels[0]->pCar->dynamics;
			graphs[0]->AddVal(std::min(1.f, cd.fHitForce * 2.f));
			graphs[1]->AddVal(std::min(1.f, cd.fHitForce2));
			graphs[2]->AddVal(std::min(1.f, cd.fHitForce3));
			graphs[3]->AddVal(std::min(1.f, cd.fCarScrap));
			graphs[4]->AddVal(std::min(1.f, cd.fCarScreech));
		}
		break;

	case 1:  /// sound  vol,pan, wave L,R
	if (gsi >= 4)
	{	float minL=1.f,maxL=-1.f,minR=1.f,maxR=-1.f;
		//for (int i=0; i < 4*512; i+=4)  if (sound.Init(/*2048*/512
		for (int i=0; i < 2*512; ++i)
		{
			//  wave osc
			float l = pGame->sound.waveL[i] / 32767.f * 0.5f + 0.5f;
			float r = pGame->sound.waveR[i] / 32767.f * 0.5f + 0.5f;
			//if (i%4==0)
			{
				graphs[2]->AddVal(l);  // L cyan  R yellow
				graphs[3]->AddVal(r);
			}
			//  amplutude
			if (l > maxL)  maxL = l;  if (l < minL)  minL = l;
			if (r > maxR)  maxR = r;  if (r < minR)  minR = r;
		}
		float al = (maxL-minL), ar = (maxR-minR);
		graphs[0]->AddVal((al+ar)*0.5f);       // vol ampl  cyan
		graphs[1]->AddVal((al-ar)*0.5f+0.5f);  // pan  yellow  ^L 1  _R 0
	}	break;
	}
}

//-----------------------------------------------------------------------------------
void CAR::GraphsNewVals(double dt)		 // CAR
{	
	size_t gsi = pApp->graphs.size();
	switch (pApp->pSet->graphs_type)
	{
	case 2:  /// tire slide,slip
		if (gsi >= 8)
		for (int i=0; i < 4; ++i)
		{
			pApp->graphs[i]->AddVal(negPow(dynamics.tire[i].slideratio, 0.2) * 0.12f +0.5f);
			pApp->graphs[i+4]->AddVal(dynamics.tire[i].slipratio * 0.1f +0.5f);
		}	break;
		
	case 3:  /// suspension
		if (gsi >= 8)
		for (int i=0; i < 4; ++i)
		{
			const CARSUSPENSION <CARDYNAMICS::T> & susp = dynamics.GetSuspension((WHEEL_POSITION)i);
			pApp->graphs[i+4]->AddVal(negPow(susp.GetVelocity(), 0.5) * 0.2f +0.5f);
			pApp->graphs[i]->AddVal(susp.GetDisplacementPercent());
		}	break;
		
	case 4:  /// tire pacejka
		if (gsi >= 12)
		{
			typedef CARDYNAMICS::T T;
			CARTIRE <T> & tire = dynamics.tire[0];
			T* ft = new T[512];

			T fmin, fmax, frng, maxF;
			const bool common = 1;  // common range for all

			const int NG = 6, LEN = 512;
			for (int i=0; i < NG; ++i)  /// Fy lateral --
			{
				bool comi = common || i == 0;
				if (comi)
				{	fmin = FLT_MAX;  fmax = FLT_MIN;  frng = 0.0;  }
				
				for (int x=0; x < LEN; ++x)
				{
					//T alpha = 360.0 * 2.0 * (x-LEN*0.5) / LEN;
					T alpha = 360.0 * x / LEN;
					T n = (NG-1-i) * 0.5 + 0.1;
					T fy = tire.Pacejka_Fy(alpha, n, 0, 1.0, maxF); // normF
					//T fy = tire.Pacejka_Fy(alpha, 3, n-2, 1.0, maxF); // gamma
					//T fy = tire.Pacejka_Fy(alpha, 3, 0.4, i / 8.0, maxF); // frict
					ft[x] = fy;

					if (comi)  // get min, max
					{	if (fy < fmin)  fmin = fy;
						if (fy > fmax)  fmax = fy;  }
				}
				if (comi)  // get range
					frng = 1.0 / (fmax - fmin);
				
				for (int x = 0; x < 512; ++x)
					pApp->graphs[i]->AddVal( (ft[x] - fmin) * frng );

				if (i==0)
					pApp->graphs[i]->UpdTitle("Tire Fy--\n"
						"min: "+fToStr((float)fmin,3,6)+"\n"+
						"max: "+fToStr((float)fmax,3,6)+"\n");
			}
			for (int i=0; i < NG; ++i)  /// Fx long |
			{
				bool comi = common || i == 0;
				if (comi)
				{	fmin = FLT_MAX;  fmax = FLT_MIN;  frng = 0.0;  }
				
				for (int x=0; x < LEN; ++x)
				{
					//T sigma = 360.0 * 2.0 * (x-LEN*0.5) / LEN;
					T sigma = 15.0 * x / LEN;
					T n = (NG-1-i) * 0.5 + 0.1;
					T fx = tire.Pacejka_Fx(sigma, n, 1.0, maxF); // normF
					ft[x] = fx;

					if (comi)  // get min, max
					{	if (fx < fmin)  fmin = fx;
						if (fx > fmax)  fmax = fx;  }
				}
				if (comi)  // get range
					frng = 1.0 / (fmax - fmin);
				
				for (int x = 0; x < 512; ++x)
					pApp->graphs[i+NG]->AddVal( (ft[x] - fmin) * frng );

				if (i==0)
					pApp->graphs[i+NG]->UpdTitle("Tire Fx |\n"
						"min: "+fToStr((float)fmin,3,6)+"\n"+
						"max: "+fToStr((float)fmax,3,6)+"\n");
			}
			delete[]ft;
		}	break;
		
	}
}
