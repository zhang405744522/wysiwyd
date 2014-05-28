// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-

/* 
* Copyright (C) 2012 EFAA Consortium, European Commission FP7 Project IST-270490
* Authors: Gr�goire Pointeau, Maxime Petit
* email:   gregoire.pointeau@inserm.fr, maxime.petit@inserm.fr
* website: http://efaa.upf.edu/ 
* Permission is granted to copy, distribute, and/or modify this program
* under the terms of the GNU General Public License, version 2 or any
* later version published by the Free Software Foundation.
*
* A copy of the license can be found at
* $EFAA_ROOT/license/gpl.txt
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
* Public License for more details

\section tested_os_sec Tested OS
Windows

\author Gr�goire Pointeau, Maxime Petit
*/ 

#include <spatialKnowledge.h>


bool spatialKnowledge::fromBottle(Bottle bInput)
{
	//std::cout << endl << "input : " << endl << bInput.toString() << endl << endl;

	string sError = "in spatialKnowledge : fromBottle\nWrong format of input\n";
	if (bInput.size() < 2)
	{
		std::cout << sError << "  -> wrong number of bottle" << endl;
		return false; 
	}

	if (! (bInput.get(0).isString()) )
	{
		std::cout << sError << "  -> wrong format of action name" << endl;
		return false;
	}
	Bottle bArgument = (*bInput.get(1).asList());
	sName = bInput.get(0).asString().c_str();
	iSize = 0;
	pair<double, double> BEGIN, END, MOVE;
	//std::cout << "Arguments : \t" << bInput.get(1).toString() << endl;
	if (bArgument.size() > 2)
	{
		sArgument = bArgument.get(1).toString().c_str();
	}
	else
	{
		sArgument = abmReasoningFunction::TAG_DB_NONE;
	}
	if (sArgument == "agent1" || sArgument == abmReasoningFunction::TAG_DB_NONE)
	{
		std::cout << sError << "  -> wrong format of data" << endl;
		return false;
	}
	for (int data = 2 ; data < bInput.size() ; data++)
	{
		Bottle bData = *(bInput.get(data)).asList();
		if (bData.size() < 2)
		{
			std::cout << sError << "  -> wrong format of data" << endl;
		}
		else
		{
			BEGIN = abmReasoningFunction::coordFromString(bData.get(0).asString().c_str());
			END = abmReasoningFunction::coordFromString(bData.get(1).asString().c_str());
			MOVE.first = (END.first - BEGIN.first);
			MOVE.second = END.second - BEGIN.second;
			if (!(MOVE.first == 0 && MOVE.second == 0 && END.first == 0 && END.second == 0))
			{
				vX.push_back(END.first);
				vY.push_back(END.second);
				vDX.push_back(MOVE.first);
				vDY.push_back(MOVE.second);
				iSize++;
			//	std::cout << "action added" << endl;
			}
		}
	}
	return true;
}


/**
* Determine if for an action, the final state is or the deplacement is important
* return 1 for the final state
* return 2 for the deplacement
* @param skInput spatialKnowledge to check
*/
void spatialKnowledge::determineInfluence()
{

	// Determine if the influence of an action is in the final state of the move (return 1), or in the Delta (return 2)
	// abs is the final state of a move (coord 0 and 1 of the vector)
	// delta if the deplacement of the move (coord 2 and 3 of the vector)

	// A : depart point
	// B : Final point
	// D : move

	// Get the covariance matrix for abs and delta.
	updateDataFinalDepart();

	vector<double>	covMatrixA = abmReasoningFunction::getCovMatrix(vFromX,vFromY),
		covMatrixB = abmReasoningFunction::getCovMatrix(vX, vY),
		covMatrixD = abmReasoningFunction::getCovMatrix(vDX,vDY);


	double sigmaD;
	double sumdX = 0, sumdY =0;
	for (unsigned int i = 0 ; i < vDX.size() ; i++)
	{
		sumdX += vDX[i];
		sumdY += vDY[i];
	}

	sigmaD = sqrt(sumdX*sumdX + sumdY*sumdY);

	double deter_CovA = (covMatrixA[0] * covMatrixA[3])-(covMatrixA[1] * covMatrixA[2]);
	double deter_CovB = (covMatrixB[0] * covMatrixB[3])-(covMatrixB[1] * covMatrixB[2]);
	double deter_CovD = (covMatrixD[0] * covMatrixD[3])-(covMatrixD[1] * covMatrixD[2]);
/*
	std::cout << sName << "_" << sArgument << " is : \t" << endl;
	std::cout << "\t" << "deter_CovA" << " = " << deter_CovA  << endl;
	std::cout << "\t" << "deter_CovB" << " = " << deter_CovB  << endl;
	std::cout << "\t" << "deter_CovD" << " = " << deter_CovD  << endl;
	std::cout << "\t" << "sigmaD    " << " = " << sigmaD  << endl << endl;;*/


	if (abmReasoningFunction::threshold_is_dispersion > deter_CovD)
	{
		isRelative = true;
	}
	else
	{
		isRelative = false;
	}

	if (abmReasoningFunction::threshold_is_dispersion > deter_CovB)
	{
		isAbsolut = true;
	}
	else
	{
		isAbsolut = false;
	}
}

/**
* Return the distance of a move from a cluster (skInput)
* First determines the influence of the cluster
* @param skInput is the spatialKnoledge to search the distance
* @param XY is the final state of the move
* @param MOVE is the delta of the move done
*/
pair <int, double> spatialKnowledge::distFromMove(pair<double, double> XY, pair<double, double> MOVE)
{
	// return the influence (Abs or Delta) and the Mahalanobis distance
	int Influence =0;
	double Dist;
	if (isAbsolut)
	{
		Dist = abmReasoningFunction::getMahalaDist(vX, vY, XY);
		Influence = 1;
	}
	else if (isRelative)
	{
		Dist = abmReasoningFunction::getMahalaDist(vDX,vDY, MOVE);
		Influence = 2;
	}

	pair <int, double> pOutput;
	pOutput.first = Influence;
	pOutput.second = Dist;

	return pOutput;
}

/*
*  Return a vector with the mean of vX, vY and the variation of vX and vY
*/
vector<double> spatialKnowledge::determineAbsolut()
{
	vector<double>  vOutput;

	double sumX = 0,
		sumY = 0,
		muX,
		muY,
		dX,
		dmaxX = 0.,
		dmaxY = 0.,
		dY;

	for (unsigned int i = 0 ; i < vX.size() ; i++)
	{
		sumX += vX[i];
		sumY += vY[i];
	}

	muX = sumX / (1. * vX.size());
	muY = sumY / (1. * vY.size());

	sumX = 0;
	sumY = 0;

	for (unsigned int i = 0 ; i < vX.size() ; i++)
	{
		sumX += fabs(vX[i] - muX);
		sumY += fabs(vY[i] - muY);

		if (fabs(vX[i] - muX) > dmaxX)
			dmaxX = fabs(vX[i] - muX);
		if (fabs(vY[i] - muY) > dmaxY)
			dmaxY = fabs(vY[i] - muY);
	}

	dX = sumX / (1. * vX.size());
	dY = sumY / (1. * vY.size());

	vOutput.push_back(muX);
	vOutput.push_back(muY);
	vOutput.push_back(dX);
	vOutput.push_back(dY);
	vOutput.push_back(dmaxX);
	vOutput.push_back(dmaxY);

	return vOutput;
}

/*
*	Return the relative coordinates of an object of coordinates Xo Yo on the table, for an agent in Xh Yh, looking to the center of the table
*
*/
pair <double, double> spatialKnowledge::coordRelative(double Xo, double Yo, double Xh, double Yh)
{
	pair<double, double>  pReturn;

	double X,	// X relative Coordinate
		Y,		// Y relative coordinate
		R,		// Distance from agent to object
		A,		// temporaty for atan2 calculation
		B,		// temporaty for atan2 calculation
		Pi = atan(1.)*4,
		Theta;	// angle of the Human relative to the table
	
	R = sqrt ((Xh-Xo)*(Xh-Xo) + (Yh-Yo)*(Yh-Yo));
	
	A = Xh - abmReasoningFunction::X_center;
	B = Yh - abmReasoningFunction::Y_center;

	Theta = atan2( B , A ) + Pi/2.;
	
	X = (Xo - Xh)*cos(Theta) + (Yo-Yh)*sin(Theta);
	Y = -(Xo - Xh)*sin(Theta) + (Yo - Yh)*cos(Theta);

	pReturn.first = X;
	pReturn.second = Y;

	return pReturn;
}


void spatialKnowledge::updateDataFinalDepart()
{
	vFromX.clear();
	vFromY.clear();

	for (unsigned int i = 0 ; i < vX.size() ; i++)
	{
		vFromX.push_back( vX[i] - vDX[i] );
		vFromY.push_back( vY[i] - vDY[i] );
	}
}

