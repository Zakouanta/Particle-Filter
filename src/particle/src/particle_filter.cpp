#include <random>
#include <algorithm>
#include <iostream>
#include <numeric>
#include <math.h>
#include <iostream>
#include <sstream>
#include <string>
#include <iterator>
#include "particle/particle_filter.h"
using namespace std;

static default_random_engine gen;

/*
 * TODO
 * This function initialize randomly the particles
 * Input:
 *  std - noise that might be added to the position
 *  nParticles - number of particles
 */
void ParticleFilter::init_random(double std[], int nParticles)
{
}

// This function initialize the particles using an initial guess
void ParticleFilter::init(double x, double y, double theta, double std[], int nParticles)
{
    num_particles = nParticles;
    normal_distribution<double> dist_x(-std[0], std[0]); // random value between [-noise.x,+noise.x]
    normal_distribution<double> dist_y(-std[1], std[1]);
    normal_distribution<double> dist_theta(-std[2], std[2]);

    for (int i = 0; i < num_particles; i++)
    {
        Particle p;
        p.id = i;
        p.x = x + dist_x(gen);
        p.y = y + dist_y(gen);
        p.theta = theta + dist_theta(gen);
        p.weight = 1;
        particles.push_back(p);
    }

    is_initialized = true;
}

/*
 * TODO
 * The predict phase uses the state estimate from the previous timestep to produce an estimate of the state at the current timestep
 * Input:
 *  delta_t  - time elapsed beetween measurements
 *  std_pos  - noise that might be added to the position
 *  velocity - velocity of the vehicle
 *  yaw_rate - current orientation
 * Output:
 *  Updated x,y,theta position
 */
void ParticleFilter::prediction(double delta_t, double std_pos[], double velocity, double yaw_rate)
{
    // for each particle
    for (int i = 0; i < num_particles; i++)
    {
        double x, y, theta; // Declare variables here

        if (fabs(yaw_rate) < 0.00001)
        {
            // First use the equations for the predict as seen during the lessons
            x = particles[i].x + velocity * delta_t * cos(particles[i].theta);
            y = particles[i].y + velocity * delta_t * sin(particles[i].theta);
            theta = particles[i].theta;
        }
        else
        {
            x = particles[i].x + (velocity / yaw_rate) * (sin(particles[i].theta + yaw_rate * delta_t) - sin(particles[i].theta));
            y = particles[i].y + (velocity / yaw_rate) * (cos(particles[i].theta) - cos(particles[i].theta + yaw_rate * delta_t));
            theta = particles[i].theta + yaw_rate * delta_t;
        }

        normal_distribution<double> dist_x(0, std_pos[0]);
        normal_distribution<double> dist_y(0, std_pos[1]);
        normal_distribution<double> dist_theta(0, std_pos[2]);

        // Add the computed noise to the current particle's position (x, y, theta)
        particles[i].x = x + dist_x(gen);
        particles[i].y = y + dist_y(gen);
        particles[i].theta = theta + dist_theta(gen);
    }
}

/*
 * TODO
 * This function associates the landmarks from the MAP to the landmarks from the OBSERVATIONS
 * Input:
 *  mapLandmark   - landmarks of the map
 *  observations  - observations of the car
 * Output:
 *  Associated observations to mapLandmarks (perform the association using the ids)
 */
void ParticleFilter::dataAssociation(std::vector<LandmarkObs> mapLandmark, std::vector<LandmarkObs> &observations)
{
    // TODO
    // TIP: Assign to observations[i].id the id of the landmark with the smallest euclidean distance
    for (auto map : mapLandmark)
    {
        double dist_min = std::numeric_limits<double>::max();
        for (auto &observation : observations)
        {
            // Compute the euclidian distance  between observation and landmark
            double distance = dist(observation.x, observation.y, map.x, map.y);
            if (distance < dist_min)
            {
                dist_min = distance;
                observation.id = map.id;
            }
        }
    }
}

/*
 * TODO
 * This function transform a local (vehicle) observation into a global (map) coordinates
 * Input:
 *  observation   - A single landmark observation
 *  p             - A single particle
 * Output:
 *  local         - transformation of the observation from local coordinates to global
 */
LandmarkObs transformation(LandmarkObs observation, Particle p)
{
    LandmarkObs global;

    global.id = observation.id;
    global.x = (observation.x * cos(p.theta)) - (observation.y * sin(p.theta)) + p.x; // TODO
    global.y = (observation.x * sin(p.theta)) + (observation.y * cos(p.theta)) + p.y; // TODO

    return global;
}

/*
 * TODO
 * This function updates the weights of each particle
 * Input:
 *  std_landmark   - Sensor noise
 *  observations   - Sensor measurements
 *  map_landmarks  - Map with the landmarks
 * Output:
 *  Updated particle's weight (particles[i].weight *= w)
 */
void ParticleFilter::updateWeights(double std_landmark[], std::vector<LandmarkObs> observations, Map map_landmarks)
{
    // Creates a vector that stores the map (this part can be improved)
    std::vector<LandmarkObs> mapLandmark;
    for (int j = 0; j < map_landmarks.landmark_list.size(); j++)
    {
        mapLandmark.push_back(LandmarkObs{map_landmarks.landmark_list[j].id_i, map_landmarks.landmark_list[j].x_f, map_landmarks.landmark_list[j].y_f});
    }

    for (int i = 0; i < particles.size(); i++)
    {
        // Before applying the association, we have to transform the observations into global coordinates
        std::vector<LandmarkObs> transformed_observations;

        // TODO: for each observation transform it (transformation function)
        for (int h = 0; h < observations.size(); h++)
        {
            LandmarkObs current_observation = observations[h];
            double transformed_x = (current_observation.x * std::cos(particles[i].theta)) - (current_observation.y * std::sin(particles[i].theta)) + particles[i].x;
            double transformed_y = (current_observation.x * std::sin(particles[i].theta)) + (current_observation.y * std::cos(particles[i].theta)) + particles[i].y;
            transformed_observations.push_back({current_observation.id, transformed_x, transformed_y});
        }

        // TODO: perform the data association (associate the landmarks to the observations)
        dataAssociation(mapLandmark, transformed_observations);

        // Move the weight update loop outside the data association loop
        particles[i].weight = 1.0;
        for (int k = 0; k < transformed_observations.size(); k++)
        {
            double obs_x, obs_y, l_x, l_y;
            obs_x = transformed_observations[k].x;
            obs_y = transformed_observations[k].y;
            // get the associated landmark
            for (int p = 0; p < mapLandmark.size(); p++)
            {
                if (transformed_observations[k].id == mapLandmark[p].id)
                {
                    l_x = mapLandmark[p].x;
                    l_y = mapLandmark[p].y;
                }
            }
            // How likely a set of landmarks measurements are, given a prediction state of the car
            double w = exp(-(pow(l_x - obs_x, 2) / (2 * pow(std_landmark[0], 2)) + pow(l_y - obs_y, 2) / (2 * pow(std_landmark[1], 2)))) / (2 * M_PI * std_landmark[0] * std_landmark[1]);
            particles[i].weight *= w;
        }
    }
}

/*
 * TODO
 * This function resamples the set of particles by repopulating the particles using the weight as metric
 */
void ParticleFilter::resample()
{
    uniform_int_distribution<int> dist_distribution(0, num_particles - 1);
    double beta = 0.0;
    vector<double> weights;
    int index = dist_distribution(gen);
    vector<Particle> new_particles;

    for (int i = 0; i < num_particles; i++)
        weights.push_back(particles[i].weight);

    float max_w = *max_element(weights.begin(), weights.end());
    uniform_real_distribution<double> uni_dist(0.0, max_w);

    for (int i = 0; i < num_particles; i++)
    {
        beta += uni_dist(gen) * 2.0; // Multiply by 2 to ensure beta can be up to 2 times max weight
        while (beta > weights[index])
        {
            beta = beta - weights[index];
            index = (index + 1) % num_particles;
        }
        new_particles.push_back(particles[index]);
    }

    particles = new_particles; // Update particles with the resampled set
}
