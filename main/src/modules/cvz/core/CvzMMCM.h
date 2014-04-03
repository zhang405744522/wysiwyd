#ifndef __CVZ_CVZMMCM_H__
#define __CVZ_CVZMMCM_H__

#include <map>
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include "ICvz.h"

#include "helpers.h"
#include <queue>
using namespace std;

class CvzMMCM : public IConvergenceZone
{
	int height, width, layers;
	int xWin, yWin, zWin, xLoose, yLoose, zLoose;
	Cube activity;
	map<IModality*, vector< Cube > > weights;
	yarp::os::BufferedPort<yarp::os::Bottle> portActivity;
	queue< vector< int > > winnersBuffer;
	int recurrenceDelay;
	int xBuff, yBuff, zBuff;
	std::string weightFilePath;
	yarp::os::Semaphore mutex;

public:

	double lRate, sigmaH, sigmaV;
	int H() { return height; }
	int W() { return width; }
	int L() { return layers; }

	map<IModality*, double > influence;
	IModality* recurrentModality;

	virtual bool interpretRpcCommand(const yarp::os::Bottle &cmd, yarp::os::Bottle &reply)
	{
		std::string kWord1 = cmd.get(0).asString();
		if (kWord1 == "save")
		{
			weightFilePath = cmd.get(1).asString();
			saveWeights(weightFilePath);
			reply.addString("ACK");
			return true;
		}		
		else if (kWord1 == "load")
		{
			weightFilePath = cmd.get(1).asString();
			yarp::os::ResourceFinder rf;
			weightFilePath = rf.findFileByName(weightFilePath);
			if (loadWeights(weightFilePath))
				reply.addString("ACK");
			else
				reply.addString("NACK");
			return true;
		}
		return false;
	}

	virtual bool close()
	{
		bool ok = this->IConvergenceZone::close();

		portActivity.interrupt();
		portActivity.close();
		return ok;
	}

	bool configure(yarp::os::ResourceFinder &rf)
	{
		//Call the base class configure
		this->IConvergenceZone::configure(rf);

		//Get additional parameters
		height = rf.check("height", yarp::os::Value(10)).asInt();
		width = rf.check("width", yarp::os::Value(10)).asInt();
		layers = rf.check("layers", yarp::os::Value(1)).asInt();
		lRate = rf.check("learningRate", yarp::os::Value(0.05)).asDouble();
		weightFilePath = rf.check("weightFilePath", yarp::os::Value(getName()+".mmw")).asString();
		int recModalitySize = rf.check("recurrentModality", yarp::os::Value(0)).asInt();
		recurrenceDelay = rf.check("recurrentDelay", yarp::os::Value(10)).asInt();
		if (recModalitySize > 0)
		{
			std::vector<double> maxBounds;
			maxBounds.resize(recModalitySize, 1.0);
			std::vector<double> minBounds;
			minBounds.resize(recModalitySize, 1.0);
			std::string recModName = "/";
			recModName += getName();
			recModName += "/recurrent";
			recurrentModality = new IModality(recModName, recModalitySize, minBounds, maxBounds);
			influence[recurrentModality] = rf.check("recurrentInfluence", yarp::os::Value(1.0)).asDouble();
		}
		else
			recurrentModality = NULL;

		sigmaH = (1.0 / 4.0) * (height + width) / 2.0;
		sigmaV = (1.0 / 4.0) * layers;

		//Allocate the map
		activity.allocate(width, height, layers);

		//Allocate the weights
		for (map<string, IModality*>::iterator it = modalitiesBottomUp.begin(); it != modalitiesBottomUp.end(); it++)
		{
			vector< Cube > w;
			w.resize(it->second->Size());
			for (int i = 0; i < it->second->Size(); i++)
			{
				w[i].allocate(width, height, layers);
				w[i].randomize(0.0, 1.0);
			}
			weights[it->second] = w;
		}

		for (map<string, IModality*>::iterator it = modalitiesTopDown.begin(); it != modalitiesTopDown.end(); it++)
		{
			vector< Cube > w;
			w.resize(it->second->Size());
			for (int i = 0; i < it->second->Size(); i++)
			{
				w[i].allocate(width, height, layers);
				w[i].randomize(0.0, 1.0);
			}
			weights[it->second] = w;
		}

		if (recurrentModality != NULL)
		{
			vector< Cube > w;
			w.resize(recurrentModality->Size());
			for (int i = 0; i < recurrentModality->Size(); i++)
			{
				w[i].allocate(width, height, layers);
				w[i].randomize(0.0, 1.0);
			}
			weights[recurrentModality] = w;
		}

		//Set influence of modalities to 1.0 by default
		for (map<string, IModality*>::iterator it = modalitiesBottomUp.begin(); it != modalitiesBottomUp.end(); it++)
			influence[it->second] = 1.0;
		for (map<string, IModality*>::iterator it = modalitiesTopDown.begin(); it != modalitiesTopDown.end(); it++)
			influence[it->second] = 1.0;

		//Zero everything
		activity = 0.0;
		xBuff = yBuff = zBuff = xWin = yWin = zWin = xLoose = yLoose = zLoose = 0;
		std::string actPortName = "/";
		actPortName += getName();
		actPortName += "/activity:o";
		portActivity.open(actPortName);

		//Good to go!
		std::cout << std::endl<< "Multi Modal Convergence Map configured:" << endl
			<< "\t Width :  " << width << endl
			<< "\t Height : " << height << endl
			<< "\t Layers : " << layers << endl;

		return true;
	}

	virtual void ComputePrediction()
	{
		mutex.wait();
		//Reset the cube activity
		activity = 0.0;

		xWin = yWin = zWin = xLoose = yLoose = zLoose = 0;

		double influenceTotal = 0;
		for (std::map< IModality*, double>::iterator it = influence.begin(); it != influence.end(); it++)
			influenceTotal += it->second;
		
		yarp::os::Bottle &botActivity = portActivity.prepare();
		botActivity.clear();

		//Compute map activity 
		for (int x = 0; x < width; x++)
		{
			for (int y = 0; y < height; y++)
			{
				for (int z = 0; z < layers; z++)
				{
					//from bottom up input
					for (std::map<std::string, IModality*>::iterator it = modalitiesBottomUp.begin(); it != modalitiesBottomUp.end(); it++)
					{
						std::vector<double> valueReal = it->second->GetValueReal();
						double modalityMeanError = 0.0;
						for (int i = 0; i < it->second->Size(); i++)
						{
							modalityMeanError += fabs(weights[it->second][i][x][y][z] - valueReal[i]);
						}
						modalityMeanError /= it->second->Size();
						activity[x][y][z] += 1.0 - (modalityMeanError * influence[it->second]);
					}

					//from top down feedback
					for (std::map<std::string, IModality*>::iterator it = modalitiesTopDown.begin(); it != modalitiesTopDown.end(); it++)
					{
						std::vector<double> valueReal = it->second->GetValueReal();
						double modalityMeanError = 0.0;
						for (int i = 0; i < it->second->Size(); i++)
						{
							modalityMeanError += fabs(weights[it->second][i][x][y][z] - valueReal[i]);
						}
						modalityMeanError /= it->second->Size();
						activity[x][y][z] += 1.0 - (modalityMeanError * influence[it->second]);
					}

					//Reccurent modality
					if (recurrentModality != NULL)
					{
						std::vector<double> recVReal = recurrentModality->GetValueReal();
						double modalityMeanError = 0.0;
						for (int i = 0; i < recurrentModality->Size(); i++)
						{
							modalityMeanError += fabs(weights[recurrentModality][i][x][y][z] - recVReal[i]);
						}
						modalityMeanError /= recurrentModality->Size();
						activity[x][y][z] += 1.0 - (modalityMeanError * influence[recurrentModality]);
					}

					//Get the whole activity in [0,1]
					activity[x][y][z] /= influenceTotal;// modalitiesBottomUp.size();

					botActivity.addDouble(activity[x][y][z]);

					//keep track of winner neuron & error
					if (activity[x][y][z] > activity[xWin][yWin][zWin])
					{
						xWin = x;
						yWin = y;
						zWin = z;
					}
					if (activity[x][y][z] < activity[xLoose][yLoose][zLoose])
					{
						xLoose = x;
						yLoose = y;
						zLoose = z;
					}
				}
			}
		}

		//Send the output activity
		portActivity.write();

		if (winnersBuffer.size() == recurrenceDelay)
			winnersBuffer.pop();
		vector<int> currentWinner(3);
		currentWinner[0] = xWin;
		currentWinner[1] = yWin;
		currentWinner[2] = zWin;
		winnersBuffer.push(currentWinner);

		if (cyclesElapsed % recurrenceDelay == 0)
		{
			xBuff = xWin;
			yBuff = yBuff;
			zBuff = zBuff;
		}

		//Learning
		if (lRate > 0.0)
			adaptWeights();

		//Set the predicted values 
		//feedback
		for (std::map<std::string, IModality*>::iterator it = modalitiesBottomUp.begin(); it != modalitiesBottomUp.end(); it++)
		{
			std::vector<double> valuePrediction;
			valuePrediction.resize(it->second->Size());
			for (int i = 0; i < it->second->Size(); i++)
			{
				valuePrediction[i] = weights[it->second][i][xWin][yWin][zWin];
			}
			it->second->SetValuePrediction(valuePrediction);
		}	

		//feedforward
		for (std::map<std::string, IModality*>::iterator it = modalitiesTopDown.begin(); it != modalitiesTopDown.end(); it++)
		{
			std::vector<double> valuePrediction;
			valuePrediction.resize(it->second->Size());
			for (int i = 0; i < it->second->Size(); i++)
			{
				valuePrediction[i] = weights[it->second][i][xWin][yWin][zWin];
			}
			it->second->SetValuePrediction(valuePrediction);
		}

		//Handle the reccurent modality I/O
		if (recurrentModality != NULL)
		{
			std::vector<double> valuePrediction;
			valuePrediction.resize(recurrentModality->Size());
			for (int i = 0; i < recurrentModality->Size(); i++)
			{
				valuePrediction[i] = weights[recurrentModality][i][xWin][yWin][zWin];
			}
			recurrentModality->SetValuePrediction(valuePrediction);
			recurrentModality->SetValueReal(valuePrediction); //we set the next input of this modality to be the last prediction
		}
		mutex.post();
	}

	void adaptWeights()
	{
		for (int x = 0; x < width; x++)
		{
			for (int y = 0; y < height; y++)
			{
				for (int z = 0; z < layers; z++)
				{
					float distanceH = sqrt(pow(x - xWin, 2.0) + pow(y - yWin, 2.0));
					float distanceV = sqrt(pow(z - zWin, 2.0));
					float dHCoef = GaussianBell(distanceH, sigmaH);
					float dVCoef = GaussianBell(distanceV, sigmaV);
					//float dHCoef = MexicanHat(distanceH, sigmaH);
					//float dVCoef = MexicanHat(distanceV, sigmaV);

					//from bottom up input
					for (std::map<std::string, IModality*>::iterator it = modalitiesBottomUp.begin(); it != modalitiesBottomUp.end(); it++)
					{
						std::vector<double> valueReal = it->second->GetValueReal();
						for (int i = 0; i < it->second->Size(); i++)
						{
							double currentW = weights[it->second][i][x][y][z];
							double desiredW = valueReal[i];
							double dW = (valueReal[i] - weights[it->second][i][x][y][z]);
							weights[it->second][i][x][y][z] +=
								lRate *
								dHCoef *
								dVCoef *
								//winnerError *
								dW;
							Clamp(weights[it->second][i][x][y][z], 0.0, 1.0);
						}
					}

					//from topdown input
					for (std::map<std::string, IModality*>::iterator it = modalitiesTopDown.begin(); it != modalitiesTopDown.end(); it++)
					{
						std::vector<double> valueReal = it->second->GetValueReal();
						for (int i = 0; i < it->second->Size(); i++)
						{
							weights[it->second][i][x][y][z] +=
								lRate *
								dHCoef *
								dVCoef *
								//winnerError *
								(valueReal[i] - weights[it->second][i][x][y][z]);

							Clamp(weights[it->second][i][x][y][z], 0.0, 1.0);
						}
					}

					//recurrent connection is buffered
					if (recurrentModality)
					{

						float pastDistanceH = sqrt(pow(x - winnersBuffer.back()[0], 2.0) + pow(y - winnersBuffer.back()[1], 2.0));
						float pastDistanceV = sqrt(pow(z - winnersBuffer.back()[2], 2.0));
						float pdHCoef = GaussianBell(pastDistanceH, sigmaH);
						float pdVCoef = GaussianBell(pastDistanceV, sigmaV);
						std::vector<double> valueReal = recurrentModality->GetValueReal();
						for (int i = 0; i < recurrentModality->Size(); i++)
						{
							double currentW = weights[recurrentModality][i][x][y][z];
							double desiredW = valueReal[i];
							double dW = (weights[recurrentModality][i][xWin][yWin][zWin] - weights[recurrentModality][i][x][y][z]);
							weights[recurrentModality][i][x][y][z] +=
								lRate *
								dHCoef *
								dVCoef *
								//winnerError *
								dW;
							Clamp(weights[recurrentModality][i][x][y][z], 0.0, 1.0);
						}
					}
				}
			}
		}
	}

	bool saveWeights(std::string path)
	{
		ofstream file;
		file.open(path);
		file << "width" << '\t' << width << std::endl;
		file << "height" << '\t' << height << std::endl;
		file << "layers" << '\t' << layers << std::endl;

		file << std::endl;

		int modCount = 0;
		for (map<IModality*, vector<Cube> >::iterator wModIt = weights.begin(); wModIt != weights.end(); wModIt++)
		{
			file << "[modality_" << modCount << "]" << std::endl;
			file << "name" << '\t' << wModIt->first->Name() << std::endl;
			file << "size" << '\t' << wModIt->first->Size() << std::endl;
			yarp::os::Bottle b;
			for (int comp = 0; comp < wModIt->second.size(); comp++)
			{
				for (int x = 0; x < wModIt->second[comp].size(); x++)
				{
					for (int y = 0; y < wModIt->second[comp][x].size(); y++)
					{
						for (int z = 0; z < wModIt->second[comp][x][y].size(); z++)
						{
							b.addDouble(wModIt->second[comp][x][y][z]);
						}
					}
				}
			}
			file << "weights (" << '\t' << b.toString() <<")"<< std::endl;
			file << std::endl;
			modCount++;
		}
		file.close();
		return true;
	}

	bool loadWeights(std::string path)
	{
		yarp::os::Property file; file.fromConfigFile(path.c_str());
		int tmpW = file.find("width").asInt();
		int tmpH = file.find("height").asInt();
		int tmpL = file.find("layers").asInt();

		if (tmpW != width || tmpH != height || tmpL != layers)
		{
			std::cerr << "Error while loading the weights. The map size is not matching." << std::endl;
			return false;
		}

		int modCount = 0;
		yarp::os::Bottle bGroup = file.findGroup("modality_0");
		while (!bGroup.isNull())
		{

			std::string mName = bGroup.find("name").asString();
			int mSize = bGroup.find("size").asInt();

			//Get the right modality
			IModality* m = NULL;
			for (map<IModality*, vector<Cube> >::iterator wModIt = weights.begin(); wModIt != weights.end(); wModIt++)
			{
				if (wModIt->first->Name() == mName)
				{
					m = wModIt->first;
					break;
				}
			}

			if (m == NULL)
			{
				std::cerr << "Error while loading the weights. The modality " <<mName<<" was not found in the current map"<< std::endl;
				return false;
			}

			yarp::os::Bottle *bWeights = bGroup.find("weights").asList();
			int wCtr = 0;
			for (int comp = 0; comp < mSize; comp++)
			{
				for (int x = 0; x < weights[m][comp].size(); x++)
				{
					for (int y = 0; y < weights[m][comp][x].size(); y++)
					{
						for (int z = 0; z < weights[m][comp][x][y].size(); z++)
						{
							weights[m][comp][x][y][z] = bWeights->get(wCtr).asDouble();
							wCtr++;
						}
					}
				}
			}

			modCount++;
			std::stringstream ss;
			ss << "modality_" << modCount;
			bGroup = file.findGroup(ss.str());
		}
		return true;
	}

	yarp::sig::ImageOf<yarp::sig::PixelRgb> getLayerActivity(int i)
	{
		mutex.wait();
		yarp::sig::ImageOf<yarp::sig::PixelRgb> img;
		double maxActivity = activity[xWin][yWin][zWin];
		double minActivity = activity[xLoose][yLoose][zLoose];
		img.resize(width, height);
		for (int x = 0; x < width; x++)
		for (int y = 0; y < height; y++)
		{
			double normalisedValue = activity[x][y][i];
			if (maxActivity - minActivity != 0)
				normalisedValue = (normalisedValue - minActivity) / (maxActivity - minActivity);
			img.pixel(x, y) = double2RGB(normalisedValue);

			//Paint the whole collection of neurons equally activated as the winner
			if (activity[x][y][i] == maxActivity)
			{
				img.pixel(x, y).r = 255;
				img.pixel(x, y).g = 0;
				img.pixel(x, y).b = 255;
			}
			if (x == xWin && y == yWin && i == zWin)
			{
				img.pixel(x, y).r = 0;
				img.pixel(x, y).g = 0;
				img.pixel(x, y).b = 0;
			}
		}
		mutex.post();

		//Console.WriteLine("Visualization computed in " + (time2 - time1).ToString());
		return img;
	}
};

#endif