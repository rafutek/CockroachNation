#include <stdlib.h>		// For exit()
#include <stdio.h>		// For printf()
#include <math.h>		// For sin() and cos()
#include <strings.h>	// For bzero()
#include <stdbool.h>
#include <time.h>

#include "GfxLib.h"		// To do simple graphics
#include "ESLib.h"		// For valeurAleatoire()

#include "definitions.h"
#include "positionFood.h"

#ifndef M_PI
#define M_PI 3.141592654
#endif

#define NumberOfCockroachs 30
const double PowerForWeights = 3;	// To decrease or increase the influence of the distance
const double Horizon = 300.;		// Units to look for neighbors
const double WeightOfNeighbors = .1;
const double Bubble = 5.;			// A minimal distance between cockroachs
const double WeightOfEscape = .8;
const double WeightOfMimic = .3;
const double MimicHorizon = 30.;		// Units to look for neighbors to mimic
const double lightBubble = 100.;		// A minimal distance with the light
const double foodBubble = 50.;		// food area distance
const int nb_max_foodPoints = 4;
const double WeightOflightEscape = .8;
const double MinDistanceFromBoxEdges = 10.;// A minimal distance with the edges of the box

enum {RandomWalker, SimpleCockroach} mode = SimpleCockroach /*RandomWalker*/;

typedef struct {
	float x;
	float y;
	float speedRho;
	float speedTheta;
} Cockroach;


// To draw a circle
void circle(float xCenter, float yCenter, float radius)
{
	const int Steps = 16; // Number of sectors to draw a circle
	const double AngularSteps = 2.*M_PI/Steps;
	int index;

	for (index = 0; index < Steps; ++index) // For each sector
	{
		const double angle = 2.*M_PI*index/Steps; // Compute the starting angle
		triangle(xCenter, yCenter,
                xCenter+radius*cos(angle), yCenter+radius*sin(angle),
			     xCenter+radius*cos(angle+AngularSteps), yCenter+radius*sin(angle+AngularSteps));
	}
}

void displayFood(POINT* foodPoints, int nb_foodPoints)
{
	couleurCourante(51,102,0);
    for(int i=0; i<nb_foodPoints; ++i){
        circle(foodPoints[i].x,foodPoints[i].y,foodBubble);
    }
}


Cockroach *initializeSwarm(const int swarmSize) {
	Cockroach *swarm = (Cockroach*)calloc(swarmSize, sizeof(Cockroach));
	const double GroupSpeedTheta = 0.;//valeurAleatoire()*2.*M_PI;
	for (int i = 0; i < swarmSize; ++i) {
		const double theta = valeurAleatoire()*2.*M_PI;
		const double rho = sqrt(valeurAleatoire()*pow(fmin(WindowWidth/2., WindowHeight/2.), 2));
		const double x = WindowWidth*.5+cos(theta)*rho;
		const double y = WindowHeight*.5+sin(theta)*rho;
		swarm[i] = (Cockroach){
			.x = x,
			.y = y,
			.speedRho = 1.,
			.speedTheta = valeurAleatoire()*2.*M_PI,	// GroupSpeedTheta
		};
	}
	return swarm;
}

void displaySwarm(const Cockroach *swarm, const int swarmSize) {
	epaisseurDeTrait(2);
	couleurCourante(0, 0, 0);
	for (int i = 0; i < swarmSize; ++i)
		point(swarm[i].x, swarm[i].y);//, 1);
}

void updateSwarm(Cockroach *swarm, const int swarmSize, int lightAbscissa, int lightOrdinate) {
	for (int i = 0; i < swarmSize; ++i) {	// All the individuals
		switch (mode) {
			case RandomWalker:
				swarm[i].speedTheta = valeurAleatoire()*2.*M_PI;
				break;
			case SimpleCockroach:
				{
					// Rule 1: avoid being alone
					double sumX = -0.;
					double sumY = -0.;
					int neighbors = 0;
					for (int n = 0; n < swarmSize; ++n) {	// All the neighbors
						if (i != n) {
							const double deltaX = swarm[n].x-swarm[i].x;
							const double deltaY = swarm[n].y-swarm[i].y;
							const double hypotenuse = hypot(deltaX, deltaY);
							if (hypotenuse < Horizon) {
								const double weight = pow(hypotenuse, -1./PowerForWeights);
								sumX += weight*deltaX;
								sumY += weight*deltaY;
								++neighbors;
							}
						}
					}
					if (neighbors != 0) {
						double normalization = hypot(sumX, sumY);
						sumX = sumX/normalization*WeightOfNeighbors+(1.-WeightOfNeighbors)*cos(swarm[i].speedTheta)*swarm[i].speedRho;
						sumY = sumY/normalization*WeightOfNeighbors+(1.-WeightOfNeighbors)*sin(swarm[i].speedTheta)*swarm[i].speedRho;
						normalization = hypot(sumX, sumY);
						//swarm[i].speedTheta = atan2(sumY/normalization, sumX/normalization);
					}
				}
				{
					// Rule 3: mimic the neighbors
					double sumX = -0.;
					double sumY = -0.;
					int models = 0;
					for (int n = 0; n < swarmSize; ++n) {	// All the neighbors
						if (i != n) {
							const double deltaX = swarm[n].x-swarm[i].x;
							const double deltaY = swarm[n].y-swarm[i].y;
							const double hypotenuse = hypot(deltaX, deltaY);
							if (hypotenuse < MimicHorizon) {
								const double weight = pow(hypotenuse, -1./PowerForWeights);
								sumX += weight*cos(swarm[n].speedTheta)*swarm[n].speedRho;
								sumY += weight*sin(swarm[n].speedTheta)*swarm[n].speedRho;
								++models;
							}
						}
					}
					if (models != 0) {
						const double normalization = hypot(sumX, sumY);
						sumX = sumX/normalization*WeightOfMimic+(1.-WeightOfMimic)*cos(swarm[i].speedTheta)*swarm[i].speedRho;
						sumY = sumY/normalization*WeightOfMimic+(1.-WeightOfMimic)*sin(swarm[i].speedTheta)*swarm[i].speedRho;
						swarm[i].speedTheta = atan2(sumY, sumX);
					}
				}
				{
					// Rule 2: don't move too close
					double closerDistance = INFINITY;
					int closerIndex = i;
					for (int n = 0; n < swarmSize; ++n) {	// All the neighbors
						if (i != n) {
							const double deltaX = swarm[n].x-swarm[i].x;
							const double deltaY = swarm[n].y-swarm[i].y;
							const double hypotenuse = hypot(deltaX, deltaY);
							if (hypotenuse < Bubble && hypotenuse < closerDistance) {
								closerDistance = hypotenuse;
								closerIndex = n;
							}
						}
					}
					if (closerIndex != i) {
						const double sumX = -(swarm[closerIndex].x-swarm[i].x)/closerDistance*WeightOfEscape+(1.-WeightOfEscape)*cos(swarm[i].speedTheta)*swarm[i].speedRho;
						const double sumY = -(swarm[closerIndex].y-swarm[i].y)/closerDistance*WeightOfEscape+(1.-WeightOfEscape)*sin(swarm[i].speedTheta)*swarm[i].speedRho;
						swarm[i].speedTheta = atan2(sumY, sumX);
					}
				}
				{
					// Rule 4: avoid the light
					if (lightAbscissa >= 0 && lightOrdinate >= 0) {
						const double deltaX = lightAbscissa-swarm[i].x;
						const double deltaY = lightOrdinate-swarm[i].y;
						const double hypotenuse = hypot(deltaX, deltaY);
						if (hypotenuse < lightBubble) {
							const double sumX = -(lightAbscissa-swarm[i].x)/hypotenuse*WeightOflightEscape+(1.-WeightOflightEscape)*cos(swarm[i].speedTheta)*swarm[i].speedRho;
							const double sumY = -(lightOrdinate-swarm[i].y)/hypotenuse*WeightOflightEscape+(1.-WeightOflightEscape)*sin(swarm[i].speedTheta)*swarm[i].speedRho;
							swarm[i].speedTheta = atan2(sumY, sumX);
						}
					}
				}
				{
					// Rule 5: stay in the box
					bool tooClose = false;
					double sumX = -0.;
					double sumY = -0.;
					const double deltaX = WindowWidth-swarm[i].x;
					const double deltaY = WindowHeight-swarm[i].y;
					if (deltaX < MinDistanceFromBoxEdges) 
					{
						tooClose = true;
						sumX = (MinDistanceFromBoxEdges-swarm[i].x)/swarm[i].x*WeightOfEscape+(1.-WeightOfEscape)*cos(swarm[i].speedTheta)*swarm[i].speedRho;

					}
					else if (deltaX > WindowWidth-MinDistanceFromBoxEdges) 
					{
						tooClose = true;
						sumX = (( WindowWidth-MinDistanceFromBoxEdges)-swarm[i].x)/swarm[i].x*WeightOfEscape+(1.-WeightOfEscape)*cos(swarm[i].speedTheta)*swarm[i].speedRho;

					}
					if (deltaY < MinDistanceFromBoxEdges) 
					{
						tooClose = true;
						sumY = (MinDistanceFromBoxEdges-swarm[i].y)/swarm[i].y*WeightOfEscape+(1.-WeightOfEscape)*sin(swarm[i].speedTheta)*swarm[i].speedRho;
					}
					else if (deltaY > WindowHeight-MinDistanceFromBoxEdges) 
					{
						tooClose = true;
						sumY = ((WindowHeight-MinDistanceFromBoxEdges)-swarm[i].y)/swarm[i].y*WeightOfEscape+(1.-WeightOfEscape)*sin(swarm[i].speedTheta)*swarm[i].speedRho;
					}
					if(tooClose)
						swarm[i].speedTheta = atan2(sumY, sumX);
				}
				break;
		}
	}
	// Update position
	for (int i = 0; i < swarmSize; ++i) {
		swarm[i].x += swarm[i].speedRho*cos(swarm[i].speedTheta);
		swarm[i].y += swarm[i].speedRho*sin(swarm[i].speedTheta);
	}
}

int main(int argc, char *argv[]) {
	initialiseGfx(argc, argv);

	prepareFenetreGraphique(argv[argc-1], WindowWidth, WindowHeight);
	lanceBoucleEvenements();

	return 0;
}


void gestionEvenement(EvenementGfx event) {
	static Cockroach *cockroach = NULL;
	static bool displaylight = false;
	static int lightAbscissa = -1;
	static int lightOrdinate = -1;

	static int nb_foodPoints;
	static POINT *foodPoints = NULL; 
	
	switch (event) {
		case Initialisation:
			srand(time(NULL)); //seed initialization for random values
			demandeTemporisation(20);
			cockroach = initializeSwarm(NumberOfCockroachs);
			nb_foodPoints = rand_a_b(1,nb_max_foodPoints+1);
			foodPoints = positionsFoodAreas(nb_foodPoints);
			break;

		case Temporisation:
			updateSwarm(cockroach, NumberOfCockroachs, lightAbscissa, lightOrdinate);
			rafraichisFenetre();
			break;

		case Affichage:
			effaceFenetre (255, 255, 255);
			displayFood(foodPoints,nb_foodPoints);
			if (displaylight)
			{
				couleurCourante(255, 235, 0);
				circle(abscisseSouris(), ordonneeSouris(), lightBubble);
			}
			displaySwarm(cockroach, NumberOfCockroachs);
			break;

		case Clavier:
			switch (caractereClavier()) {
				case 'Q':
				case 'q':
					exit(0);
					break;
				case 'R':
				case 'r':
					free(cockroach);
					cockroach = initializeSwarm(NumberOfCockroachs);
					break;
			}
			break;

		case BoutonSouris:
			if (etatBoutonSouris() == GaucheAppuye)
				displaylight = true;
			else if (etatBoutonSouris() == GaucheRelache)
				displaylight = false;
		case Souris:
			if (displaylight) {
				lightAbscissa = abscisseSouris();
				lightOrdinate = ordonneeSouris();   
			}
			else {
				lightAbscissa = -1;
				lightOrdinate = -1;
			}
			break;

		case ClavierSpecial: 
		case Inactivite:
		case Redimensionnement: 
			break;
	}
}
