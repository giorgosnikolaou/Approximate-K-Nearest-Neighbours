#include "cluster.hpp"
#include <unordered_map>
#include <unordered_set>
#include <cfloat>

using namespace std;

/////////////
// Cluster //
/////////////

// Update cluster center
void Cluster::update(uint32_t new_size = 0) {
	if (points_.size() == 0)
		return ;

	if(new_size == 0)
		new_size = center_->len();

	auto sum = new Vector<uint32_t>(new_size);

	// Compute average vector
	for (auto point : points_)
		*sum += point->data();
	
	delete center_;
	center_ = new Vector<double>(*sum);
	*center_ /= (double)points_.size();

	delete sum;
}

void Cluster::add(DataPoint* point) {

	// Quickly find sum of all vectors
	*center_ *= (double)points_.size();

	points_.insert(point);
	*center_ += point->data();

	// Recompute average
	*center_ /= (double)points_.size();

}

void Cluster::remove(DataPoint* point) {

	// Corner Case
	if (points_.size() == 1) {
		points_.erase(point);
		*center_ *= 0;
		return ;
	}
	
	// Sum of vectors
	*center_ *= (double)points_.size();

	points_.erase(point);
	*center_ -= point->data();

	// Recompute Average
	*center_ /= (double)points_.size();
}

double Cluster::ObjectiveFunctionValue(Distance<uint8_t, double> dist){
	
	double error = 0;
	
	for(auto point : points_)
		error += dist(point->data(), *center_);
	
	error /= points_.size();

	return error;
}

void Cluster::projectToDataset(DataSet& new_dataset){
	set<DataPoint*> new_points;

	for(auto point : points_){
		DataPoint* new_point = new_dataset[point->label()-1]; 
		new_points.insert(new_point);
	}
	points_ = new_points;

	update(new_dataset.dim());
}

///////////////
// Clusterer //
///////////////

Clusterer::Clusterer(DataSet& dataset_, uint32_t k_, Distance<uint8_t, double> dist_) 
: dataset(&dataset_), k(k_), dist(dist_) { 
    
    bool* chosen = new bool[dataset->size()]();

    // Random point as initial center
	uint32_t init_center = Vector<uint32_t>(1, UNIFORM, 0, dataset->size() - 1)[0];

	clusters.push_back(new Cluster((*dataset)[init_center]));
	chosen[init_center] = true;
	
	// Finding rest k - 1 centers
	for (uint32_t j = 1; j < k; j++) {
		
		// Stores (i, D(i) * D(i))
		vector<PAIR> distances;
		
		// Probability distribution of points - candidate centers
		vector<PAIR> probs;

		double sum = 0; // -> sum(D(i) * D(i))

		for (uint32_t i = 0; i < dataset->size(); i++) {
			if (chosen[i])
				continue;
			
			auto p = closest((*dataset)[i]);
			double distance = p.first; // = D(i)

			distances.push_back(pair(i, distance*distance));
			sum += distance*distance;
		}

		// Calculate distribution
		for (auto p : distances) 
			probs.push_back(pair(p.first, p.second / sum));

		// Select new center according to distribution:

		// Random number, Uniform(0, 1)
		double prob = Vector<float>(1, UNIFORM, 0, 1)[0];

		// Probability accumulator
		double accum = 0;

		for (auto p : probs) {
			accum += p.second;

			if (prob > accum)
				continue;
			
			// If prob <= accum, select this point
			clusters.push_back(new Cluster((*dataset)[p.first]));
			chosen[p.first] = true;
			break;
		}
	}

	delete [] chosen;
}

Clusterer::~Clusterer() {
	for (auto cluster : clusters)
		delete cluster;
}

void Clusterer::clear() {
	for (auto cluster : clusters)
		cluster->clear();
}


// Find closest center to point
pair<double, Cluster*> Clusterer::closest(DataPoint* point) {

	if (clusters.size() == 0)
        throw runtime_error("Exception in min_dist: Zero clusters present!\n");

	double min = DBL_MAX;
	Cluster* closest = nullptr;

	for (auto cluster : clusters) {
		double distance = dist(point->data(), cluster->center());

		if (distance < min) {
			min = distance;
			closest = cluster;
		}
	}

	return pair(min, closest);
}

std::vector<Cluster*>& Clusterer::get() { return clusters; }

double average_distance(DataPoint* point, Cluster* cluster, Distance<uint8_t, uint8_t> dist_) {
	double sum = 0;
	size_t count = 0;

	for (auto point2 : cluster->points()) {
		if (point == point2)
			continue;
		
		sum += dist_(point->data(), point2->data());
		count++;
	}

	return sum / count;

}

pair<vector<double>, double> Clusterer::silhouettes(Distance<uint8_t, uint8_t> dist_) {
	vector<double> metr;
	double stotal = 0;

	for (auto cluster : clusters) {
		
		double sum = 0.;

		for (auto point : cluster->points()) {

			double min = DBL_MAX;
			Cluster* closest = nullptr;

			// Find closest cluster, other than the assigned one
			for (auto cluster1 : clusters) {
				if (cluster1 == cluster)
					continue; 

				double distance = dist(point->data(), cluster->center());

				if (distance < min) {
					min = distance;
					closest = cluster1;
				}
			}

			double a = average_distance(point, cluster, dist_);
			double b = average_distance(point, closest, dist_);

			sum += (b - a) / max(a, b);
		}
		
		stotal += sum;

		// Push Silhouette coefficient in vector
		metr.push_back(sum / cluster->size());
	}

	return pair(metr, stotal / dataset->size());
}


double Clusterer::ObjectiveFunctionValue(Distance<uint8_t,double> dist){
	
	double error = 0;

	for(auto cluster : clusters){
		error += cluster->size() * cluster->ObjectiveFunctionValue(dist);
	}

	error /= dataset->size();

	return error;
}


void Clusterer::projectToDataset(DataSet& new_dataset){
	
	if(dataset->size() != new_dataset.size()){
		throw runtime_error("Exception in projectToDataset: DataSet sizes must be equal!\n");
	}

	for(auto cluster : clusters){
		cluster->projectToDataset(new_dataset);
	}
	dataset = &new_dataset;
}

////////////
// Lloyd //
///////////

Lloyd::Lloyd(DataSet& dataset, uint32_t k, Distance<uint8_t, double> dist) : Clusterer(dataset, k, dist) { }

void Lloyd::apply() {
	Cluster** indexes = new Cluster*[dataset->size()]();
	
	while (true) {
		uint32_t changes = 0;

		// For every point
		for (auto point : *dataset) {
			uint32_t index = point->label() - 1;

			auto p = closest(point);

			// Add point to the closest cluster, updating both centers (MacQueen)
			if (p.second != indexes[index]) {
				changes++;
				
				if (indexes[index] != nullptr)
					indexes[index]->remove(point);
					
				p.second->add(point);

				indexes[index] = p.second;
			}
		}

		// Convergence of algorithm
		if (changes == 0)
			break;

	}

	delete [] indexes;
}


////////////////////////
// Reverse Assignment //
////////////////////////

RAssignment::RAssignment(DataSet& dataset, uint32_t k, Approximator* approx_, 
						 Distance<uint8_t, double> dist1, 
						 Distance<double, double>  dist2) : Clusterer(dataset, k, dist1), approx(approx_), dist(dist2) { }

RAssignment::~RAssignment() {
	delete approx;
}

double RAssignment::minDistBetweenClusters() {
	double distance = DBL_MAX;

	for(auto cluster1 : clusters) {
		for(auto cluster2 : clusters) {
			if (cluster1 == cluster2) 
				continue;

			distance = min(distance, dist(cluster1->center(), cluster2->center()));
		}
	}

	return distance;
}

#define MAX_ITERS 15
void RAssignment::apply() {

	// Mapping from datapoints to clusters
	Cluster** indexes = new Cluster*[dataset->size()]();

	double radius = minDistBetweenClusters() / 2;
	uint8_t iters = 0;

	while (iters++ < MAX_ITERS) {

		uint32_t changes = 0;
		
		for (auto cluster : clusters) {
			
			// For each point within radius
			for (auto p : approx->RangeSearch(cluster->center(), radius, Clusterer::dist)) {
				
				uint32_t index = p.first - 1;
				double dist  = p.second;

				auto point = (*dataset)[index];
				auto prev  = indexes[index];
				
				// If new cluster is closer than previous
				if (prev == nullptr || (
						cluster != prev && 
						dist < Clusterer::dist(point->data(), prev->center())
						)
					) {

					changes++;
					
					// Erase point from previous cluster
					if (prev != nullptr)
						prev->points().erase(point);
						
					// Add point to new one
					cluster->points().insert(point);
					indexes[index] = cluster;
				}
			}
		}

		// Update cluster centers
		for (auto cluster : clusters)
			cluster->update();

		if (changes == 0)
			break;
		
		radius *= 2;
	}

	// Unnasigned points are assigned to closest cluster
	for (auto point : *dataset) {
		if (indexes[point->label() - 1] == nullptr) 
			closest(point).second->add(point);
	}

	delete [] indexes;
}