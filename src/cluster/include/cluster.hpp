#pragma once
#include <vector>
#include <set>
#include "Vector.hpp"
#include "utils.hpp"
#include "Approximator.hpp"


class Cluster {
	private:
		Vector<double>* center_;
		std::set<DataPoint*> points_;

	public:
		Cluster(uint32_t size) : center_(new Vector<double>(size)) { }
		Cluster(DataPoint* point) : center_(new Vector<double>(point->data())) { }
		~Cluster() { delete center_; }
        void projectToDataset(DataSet& new_dataset);

        double ObjectiveFunctionValue(Distance<uint8_t, double> dist);

		uint32_t size() const { return points_.size(); }
		void add(DataPoint* point);
		void remove(DataPoint* point);
		std::set<DataPoint*>& points() { return points_; }
		Vector<double>& center() { return *center_; }
        void update(uint32_t new_size);
        void clear() { points_.clear(); }
};

class Clusterer {
    protected:
        DataSet* dataset;
        uint32_t k;
		Distance<uint8_t, double> dist;

		std::vector<Cluster*> clusters;

		std::pair<double, Cluster*> closest(DataPoint* point);
    public:
        Clusterer(DataSet& dataset, uint32_t k, Distance<uint8_t, double> dist);
        virtual ~Clusterer();
        void projectToDataset(DataSet& new_dataset);
        void clear();
        std::vector<Cluster*>& get();
        std::pair<std::vector<double>, double> silhouettes(Distance<uint8_t, uint8_t> dist);
        double ObjectiveFunctionValue(Distance<uint8_t,double> dist);
        virtual void apply() = 0;
};

class Lloyd : public Clusterer {
    private:

    public:
        Lloyd(DataSet& dataset, uint32_t k, Distance<uint8_t, double> dist);
        void apply() override;
};

class RAssignment : public Clusterer {
    private:
        Approximator* approx;
		Distance<double, double>  dist;
        double minDistBetweenClusters();
    public:
        RAssignment(DataSet& dataset, uint32_t k, Approximator* approx,
                    Distance<uint8_t, double> dist1, 
		            Distance<double, double>  dist2); 
        ~RAssignment();
        void apply() override;
};