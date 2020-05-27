#pragma once

#include <matrix/matrix.hpp>
#include <set>
#include <vector>

namespace hpkmediods
{
template <typename T>
class IInitializer
{
public:
    virtual ~IInitializer() = default;

    virtual void initialize(const Matrix<T>* const data, Matrix<T>* const centroids,
                            std::vector<int32_t>* const assignments, Matrix<T>* const dataDistMat,
                            Matrix<T>* const centroidDistMat, std::set<int32_t>* const unselected,
                            std::vector<int32_t>* const selected) const = 0;
};
}  // namespace hpkmediods