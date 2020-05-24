#pragma once

#pragma once

#include <hpkmediods/initializers/interface.hpp>
#include <hpkmediods/maximizers/interface.hpp>
#include <matrix/matrix.hpp>
#include <memory>
#include <vector>

namespace hpkmediods
{
template <typename T>
class ClusterData
{
public:
    ClusterData();

    ClusterData(const Matrix<T>* data, const int& numClusters);

    bool operator<(const ClusterData<T>& rhs) const;

    bool operator>(const ClusterData<T>& rhs) const;

    void initialize(const IInitializer<T>* const initializer);

    void maximize(const IMaximizer<T>* const maximizer);

    const Matrix<T>* const getCentroids() { return &m_centroids; }

    const std::vector<int32_t>* const getClustering() { return &m_assignments; }

    const std::vector<T>* const getDistances() { return &m_distances; }

    const T getError() { return m_error; }

private:
    const Matrix<T>* p_data;
    Matrix<T> m_centroids;
    std::vector<int32_t> m_assignments;
    std::vector<T> m_distances;
    T m_error;
};
}  // namespace hpkmediods