#include <cmath>
#include <limits>
#include <string>
#include <vector>
#include <iomanip>
#include <istream>
#include <fstream>
#include <sstream>
#include <utility>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <omp.h>
#include <sys/time.h>

using namespace std;

typedef pair<double, double> Point;

/******************************************************************************/

double PerpendicularDistance(const Point &pt, const Point &lineStart, const Point &lineEnd)
{
	double dx = lineEnd.first - lineStart.first;
	double dy = lineEnd.second - lineStart.second;

	//Normalise
	double mag = sqrt(dx*dx + dy*dy);
	if (mag > 0.0)
	{
		dx /= mag; dy /= mag;
	}

	double pvx = pt.first - lineStart.first;
	double pvy = pt.second - lineStart.second;

	//Subtract this from pv
	double ax = pvx - (dx * pvx + dy * pvy) * dx;
	double ay = pvy - (dx * pvx + dy * pvy) * dy;

	return sqrt(ax*ax + ay*ay);
}

/******************************************************************************/

void RamerDouglasPeucker(const vector<Point> &pointList, double epsilon,vector<Point> &out)
{
	if (pointList.size() < 2)
	{
		throw invalid_argument("Not enough points to simplify");
	}

	// Find the point with the maximum distance from line between start and end
	double	dmax = 0.0;
	int index = 0;
	int	endRDP = pointList.size() - 1;
	for (int i = 1; i < endRDP; i++)
	{
		double d = PerpendicularDistance(pointList[i], pointList[0], pointList[endRDP]);
		if (d > dmax)
		{
			index = i;
			dmax = d;
		}
	}

	// If max distance is greater than epsilon, recursively simplify
	if (dmax > epsilon)
	{
		vector<Point> recResults1;
		vector<Point> recResults2;
		// Recursive call
		vector<Point> firstLine(pointList.begin(), pointList.begin() + index + 1);
		vector<Point> lastLine(pointList.begin() + index, pointList.end());

	    RamerDouglasPeucker(firstLine, epsilon,recResults1);
		RamerDouglasPeucker(lastLine, epsilon,recResults2);
		// Build the result list
	    out.assign(recResults1.begin(), recResults1.end() - 1);    
		out.insert(out.end(), recResults2.begin(), recResults2.end());
	    if (out.size() < 2)
		{
			throw runtime_error("Problem assembling output");
		}
	}
	else
	{
		//Just return start and end points
		out.clear();
		out.push_back(pointList[0]);
		out.push_back(pointList[endRDP]);
	}
}

/*---------------------------------------------------MAIN FUNCTION-----------------------------------------------------*/

int main(int argc, char *argv[])
{
	int	print_results,numberOfLinesInFile;
	double	epsilon;
	string	line;
	vector<Point>  polyline,simplified_polyline;
	vector< vector<Point> >	AllPolylines, SimplifiedAllPolylines,SimplifiedSubVector;

	if (argc != 5)
	{
		cout << "USAGE: ./RDP <input file name> <epsilon> <print_results>" << endl;
		exit(1);
	}
	int numberOfThreads=stoi(argv[4]);
    if(numberOfThreads != 1 && numberOfThreads != 2 && numberOfThreads != 4 )
    {
      cout << "Thread argument must be 1,2 or 4.No other value is valid" << endl;
      exit(1);
    }
	epsilon = stod(argv[2]);
	print_results = stoi(argv[3]);
	ifstream infile(argv[1]);

/*  Read input file line-by-line. Each line corresponds to a polyline and the format of each line is as follows:
    x_1,y_1 x_2,y_2 x_3,y_3 ... x_n,y_n */

	while (getline(infile, line))
	{

/* Each point in the line read is separated from the next point by a space character.
   The following 4 lines split the line into tokens at each space character and
   create the vector 'vstrings' that contains all pairs x_i,y_i */

		stringstream ss(line);
		istream_iterator<string> begin(ss);
		istream_iterator<string> end;
		vector<string> vstrings(begin, end);

/* Every pair x_i,y_i in 'vstrings' is now split into its constituting coordinates at the comma character and
inserted into the vector of points 'polyline'. */
		polyline.clear();
		for (vector<string>::const_iterator it = vstrings.begin(); it != vstrings.end(); it++)
		{
			Point	p;
			string	token;
			istringstream tokenStream(*it);
			
			getline(tokenStream, token, ',');
			p.first = stod(token);
			getline(tokenStream, token, ',');
			p.second = stod(token);

			polyline.push_back(p);
		}

/* A complete polyline has been created at this point and
   is inserted into the vector 'AllPolylines' */
		AllPolylines.push_back(polyline);
	}
	

/*Calculation of number of files for each thread*/
	
	numberOfLinesInFile=AllPolylines.size();
	int workload_number=numberOfLinesInFile/numberOfThreads;

//Allocates the appropriate memory for better performance
    SimplifiedAllPolylines.reserve(numberOfLinesInFile); 

//Initializing the vector with the starting AllPolylinesVector
    SimplifiedAllPolylines.assign( AllPolylines.begin(), AllPolylines.end() );


/* Start main calculation. */

//Tropos A me #pragma omp for...
/*
	#pragma omp parallel num_threads(numberOfThreads)
	{
		double total_time,s,e;
		int id = omp_get_thread_num();
	    s = omp_get_wtime();

	    #pragma omp for schedule(static, workload_number+1) nowait
	    for(int k=0;k<numberOfLinesInFile;k++)
	    {
		    RamerDouglasPeucker(AllPolylines[k],epsilon,SimplifiedAllPolylines[k]);
		}
		e = omp_get_wtime();
	    total_time = e-s;
	    printf("Time for calculations = %13.6f sec for thread %d\n", total_time,id);
	}
*/

// Tropos B
    printf("executed point1\n");
	#pragma omp parallel num_threads(numberOfThreads)
	{
		
		double s,e,total_time; //Time for each thread to execute
		int id = omp_get_thread_num(); //Unique id to each thread
		int k;
    	s = omp_get_wtime();
		for (k=id*workload_number; k < id*workload_number+workload_number; k++)
	   	{
			RamerDouglasPeucker(AllPolylines[k], epsilon,SimplifiedAllPolylines[k]);
		}

//Code below is used for the case we have some simplifications that haven't executed
//due to the fact that  numberOfLinesInFile/numberOfThreads has modulus not equal to zero

    	if(id==(numberOfThreads-1) && k<numberOfLinesInFile)
		{
			k=4*workload_number;
			while(k<numberOfLinesInFile)
			{
				RamerDouglasPeucker(AllPolylines[k], epsilon,SimplifiedAllPolylines[k]);
       			++k; //simplified polylines counter is incremented
			}
	    }
		e = omp_get_wtime();
	    total_time = e-s;
	    printf("Time for calculations = %13.6f sec for thread %d\n", total_time,id);
	}
    printf("executed point2\n");

/* If requested, print out the initial and the simplified polylines. */
	if (print_results != 0)
	{
		cout << fixed;
		for (vector< vector<Point> >::const_iterator it1 = AllPolylines.begin(), it2 = SimplifiedAllPolylines.begin();
		     it1 != AllPolylines.end(), it2 != SimplifiedAllPolylines.end(); it1++, it2++)
		{
			polyline = (*it1);
			simplified_polyline = (*it2);

			cout << "Polyline:" << endl;
			for (vector<Point>::const_iterator it3 = polyline.begin(); it3 != polyline.end(); it3++)
			{
				cout << setprecision(numeric_limits<double>::digits10 + 1) << "(" << (*it3).first << ", " << (*it3).second << ") ";
			}

			cout << endl << "Simplified:" << endl;
			for (vector<Point>::const_iterator it3 = simplified_polyline.begin(); it3 != simplified_polyline.end(); it3++)
			{
				cout << setprecision(numeric_limits<double>::digits10 + 1) << "(" << (*it3).first << ", " << (*it3).second << ") ";
			}
			cout << endl << endl;
		}
	}
	printf("The number of lines in file are %d\n",numberOfLinesInFile);
	   cout <<"The vector SimplifiedAllPolylines has size "<< SimplifiedAllPolylines.size() << endl;

	return 0;
}